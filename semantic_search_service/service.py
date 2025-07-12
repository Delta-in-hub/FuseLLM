import logging
import os
import shutil
import json
import zmq

from llama_index.core import VectorStoreIndex, SimpleDirectoryReader, Document, StorageContext, load_index_from_storage
from llama_index.core.node_parser import SentenceSplitter
from llama_index.core.query_engine import BaseQueryEngine

# --- Configuration ---
LOG_LEVEL = logging.DEBUG
ZMQ_ENDPOINT = "ipc:///tmp/fusellm-semantic.ipc"
PERSIST_DIR_BASE = "./index_storage"
DEFAULT_SIMILARITY_TOP_K = 3

# --- Setup Logging ---
logging.basicConfig(level=LOG_LEVEL,
                    format='%(asctime)s - %(levelname)s - %(message)s')


class SemanticSearchService:
    """
    A ZeroMQ-based service providing semantic search capabilities using LlamaIndex.
    """

    def __init__(self, endpoint: str, persist_dir: str):
        """
        Initializes the service, ZMQ socket, and storage directory.

        Args:
            endpoint: The ZMQ endpoint string to bind to.
            persist_dir: The base directory to store all search indexes.
        """
        self.persist_dir = persist_dir
        self.context = zmq.Context(io_threads=8)
        self.socket = self.context.socket(zmq.REP)
        self.socket.bind(endpoint)

        if not os.path.exists(self.persist_dir):
            os.makedirs(self.persist_dir)
            logging.info(f"Created storage directory: {self.persist_dir}")
        
        # In-memory cache for loaded indexes and query engines to improve performance
        self._index_cache: dict[str, VectorStoreIndex] = {}
        self._query_engine_cache: dict[str, BaseQueryEngine] = {}


    def _get_index_path(self, index_name: str) -> str:
        """Constructs the full path for a given index name."""
        # Basic security check to prevent path traversal
        if ".." in index_name or "/" in index_name or "\\" in index_name:
            raise ValueError(f"Invalid characters in index name: {index_name}")
        return os.path.join(self.persist_dir, index_name)
    
    def _load_or_create_index(self, index_name: str) -> VectorStoreIndex:
        """
        Loads an index from disk if it exists, otherwise creates a new empty index.
        Uses an in-memory cache to avoid redundant disk I/O.

        Args:
            index_name: The name of the index.

        Returns:
            A LlamaIndex VectorStoreIndex object.
        """
        if index_name in self._index_cache:
            return self._index_cache[index_name]

        index_path = self._get_index_path(index_name)
        if os.path.exists(index_path):
            logging.debug(f"Loading index '{index_name}' from disk.")
            storage_context = StorageContext.from_defaults(persist_dir=index_path)
            index = load_index_from_storage(storage_context)
        else:
            # This path is mainly for operations like add_document on an index
            # that was just created via mkdir and doesn't exist on disk yet.
            logging.debug(f"Creating new in-memory index for '{index_name}'.")
            index = VectorStoreIndex.from_documents([])

        self._index_cache[index_name] = index
        return index

    def _get_query_engine(self, index_name: str) -> BaseQueryEngine:
        """
        Retrieves or creates a query engine for a given index.
        Uses an in-memory cache.
        """
        if index_name in self._query_engine_cache:
            return self._query_engine_cache[index_name]

        index = self._load_or_create_index(index_name)
        logging.debug(f"Creating new query engine for index '{index_name}'.")
        engine = index.as_query_engine(similarity_top_k=DEFAULT_SIMILARITY_TOP_K)
        self._query_engine_cache[index_name] = engine
        return engine
    
    def _clear_cache(self, index_name: str):
        """Clears the cache for a specific index when it's modified."""
        self._index_cache.pop(index_name, None)
        self._query_engine_cache.pop(index_name, None)
        logging.debug(f"Cleared cache for index '{index_name}'.")

    # --- Handler Methods ---

    def handle_create_index(self, payload: dict) -> dict:
        index_name = payload.get("index_name")
        if not index_name:
            return {"error": "Missing 'index_name' in payload."}

        index_path = self._get_index_path(index_name)
        if os.path.exists(index_path):
            return {"error": f"Index '{index_name}' already exists."}
        
        # Create an empty index on disk
        empty_index = VectorStoreIndex.from_documents([])
        empty_index.storage_context.persist(persist_dir=index_path)
        
        logging.info(f"Created index '{index_name}' at {index_path}")
        return {"status": "ok"}

    def handle_delete_index(self, payload: dict) -> dict:
        index_name = payload.get("index_name")
        if not index_name:
            return {"error": "Missing 'index_name' in payload."}

        self._clear_cache(index_name)
        index_path = self._get_index_path(index_name)

        if os.path.exists(index_path):
            shutil.rmtree(index_path)
            logging.info(f"Deleted index '{index_name}' from {index_path}")
            return {"status": "ok"}
        else:
            return {"error": f"Index '{index_name}' not found."}

    def handle_list_indexes(self, payload: dict) -> list:
        try:
            entries = os.listdir(self.persist_dir)
            # Filter for directories only, which represent our indexes
            index_names = [name for name in entries if os.path.isdir(os.path.join(self.persist_dir, name))]
            return index_names
        except Exception as e:
            logging.error(f"Failed to list indexes: {e}")
            return {"error": str(e)}

    def handle_add_document(self, payload: dict) -> dict:
        index_name = payload.get("index_name")
        doc_id = payload.get("document_id")
        text = payload.get("text")

        if not all([index_name, doc_id, text is not None]):
            return {"error": "Missing 'index_name', 'document_id', or 'text' in payload."}
        
        self._clear_cache(index_name)
        index = self._load_or_create_index(index_name)
        
        # We must delete any existing document with the same ID before inserting
        # to prevent duplicates.
        index.delete_ref_doc(doc_id, delete_from_docstore=True)

        llama_doc = Document(text=text, doc_id=doc_id)
        index.insert_nodes([llama_doc])
        
        index.storage_context.persist(persist_dir=self._get_index_path(index_name))
        logging.info(f"Added/updated document '{doc_id}' in index '{index_name}'.")
        return {"status": "ok"}

    def handle_remove_document(self, payload: dict) -> dict:
        index_name = payload.get("index_name")
        doc_id = payload.get("document_id")

        if not all([index_name, doc_id]):
            return {"error": "Missing 'index_name' or 'document_id' in payload."}

        self._clear_cache(index_name)
        index = self._load_or_create_index(index_name)
        index.delete_ref_doc(doc_id, delete_from_docstore=True)
        index.storage_context.persist(persist_dir=self._get_index_path(index_name))
        
        logging.info(f"Removed document '{doc_id}' from index '{index_name}'.")
        return {"status": "ok"}
    
    def handle_list_documents(self, payload: dict) -> dict | list:
        index_name = payload.get("index_name")
        if not index_name:
            return {"error": "Missing 'index_name' in payload."}

        index = self._load_or_create_index(index_name)
        if not hasattr(index, 'docstore'):
             return {"error": f"Index '{index_name}' does not have a document store."}
        
        doc_infos = index.docstore.get_all_ref_doc_info()
        return list(doc_infos.keys()) if doc_infos else []

    def handle_query(self, payload: dict) -> str:
        index_name = payload.get("index_name")
        query_text = payload.get("query")

        if not all([index_name, query_text]):
            return json.dumps({"error": "Missing 'index_name' or 'query' in payload."})

        try:
            query_engine = self._get_query_engine(index_name)
            response = query_engine.query(query_text)
            
            # Format the response as specified in Goal.md
            if not response.source_nodes:
                return "No relevant documents found."

            output = []
            total_results = len(response.source_nodes)
            for i, node_with_score in enumerate(response.source_nodes):
                node = node_with_score.node
                score = node_with_score.score
                source = node.metadata.get("doc_id", "N/A")
                
                # The design doc specifies source as `/corpus/doc.md`
                # LlamaIndex provides the `document_id` we set, so we format it.
                formatted_source = f"/corpus/{source}"

                output.append(
                    f"--- Result {i+1}/{total_results} (Score: {score:.2f}) ---\n"
                    f"Source: {formatted_source}\n"
                    f"Content: {node.get_content().strip()}"
                )
            
            return "\n\n".join(output)

        except Exception as e:
            logging.error(f"Query failed for index '{index_name}': {e}", exc_info=True)
            return json.dumps({"error": f"An internal error occurred during the query: {e}"})

    def run(self):
        """
        The main service loop that listens for requests and dispatches them.
        """
        logging.info(f"Semantic Search Service started. Listening on {ZMQ_ENDPOINT}")
        op_map = {
            "create_index": self.handle_create_index,
            "delete_index": self.handle_delete_index,
            "list_indexes": self.handle_list_indexes,
            "add_document": self.handle_add_document,
            "remove_document": self.handle_remove_document,
            "list_documents": self.handle_list_documents,
            "query": self.handle_query,
        }

        while True:
            try:
                # Wait for next request from client, received as a multipart message
                op_code_bytes, payload_bytes = self.socket.recv_multipart()
                op_code = op_code_bytes.decode('utf-8')
                payload_str = payload_bytes.decode('utf-8')
                
                logging.debug(f"Received request -> OP: {op_code}, Payload: {payload_str}")

                payload = json.loads(payload_str)
                handler_func = op_map.get(op_code)

                if handler_func:
                    result = handler_func(payload)
                else:
                    result = {"error": f"Unknown operation code: {op_code}"}
                
                # The query handler returns a pre-formatted string. Other handlers return dict/list.
                if isinstance(result, str):
                    response_str = result
                else:
                    response_str = json.dumps(result)
                
                self.socket.send_string(response_str)

            except zmq.ZMQError as e:
                logging.error(f"ZMQ Error: {e}")
                break # Exit on critical ZMQ errors
            except json.JSONDecodeError as e:
                logging.error(f"Invalid JSON payload received: {e}")
                self.socket.send_string(json.dumps({"error": "Malformed JSON payload."}))
            except Exception as e:
                logging.error(f"An unhandled exception occurred: {e}", exc_info=True)
                self.socket.send_string(json.dumps({"error": f"An internal server error occurred: {e}"}))


if __name__ == "__main__":
    service = SemanticSearchService(endpoint=ZMQ_ENDPOINT, persist_dir=PERSIST_DIR_BASE)
    try:
        service.run()
    except KeyboardInterrupt:
        logging.info("Service shutting down.")