import os
os.environ['HF_ENDPOINT'] = 'https://hf-mirror.com'

import argparse  # 引入命令行参数解析库
import zmq
import json
import shutil
import logging
from llama_index.core import (
    VectorStoreIndex, Document, StorageContext,
    load_index_from_storage, Settings
)
from llama_index.core.node_parser import SentenceSplitter
from llama_index.core.query_engine import BaseQueryEngine
from llama_index.embeddings.huggingface import HuggingFaceEmbedding


# ==============================================================================
# 1. 早期环境和日志设置
# ==============================================================================

# 在导入任何可能触发模型下载的库（如 llama_index）之前设置环境变量


# 配置日志记录
logging.basicConfig(level=logging.DEBUG,
                    format='%(asctime)s - %(levelname)s - %(message)s')

logging.info(f"Hugging Face endpoint set to: {os.getenv('HF_ENDPOINT')}")


# ==============================================================================
# 2. LlamaIndex 和模型相关的导入与全局配置
# ==============================================================================


# --- 全局模型配置 ---
# 配置 LlamaIndex 使用本地嵌入模型而不是依赖 OpenAI。
# 第一次运行时，LlamaIndex 会通过上面设置的镜像下载并缓存这个模型。
try:
    logging.info("Initializing embedding model (BAAI/bge-small-en-v1.5)...")
    # 这会从 Hugging Face Hub 下载模型（如果本地没有缓存的话）
    embed_model = HuggingFaceEmbedding(model_name="BAAI/bge-small-en-v1.5")
    Settings.embed_model = embed_model
    Settings.llm = None
    logging.info("Embedding model initialized successfully.")
except Exception as e:
    logging.error(f"Failed to initialize embedding model: {e}", exc_info=True)
    logging.error("Please ensure you have an internet connection to download the model on first run, and that 'pip install torch sentence-transformers' is complete.")
    exit(1)


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
        if not os.path.exists(self.persist_dir):
            os.makedirs(self.persist_dir)
            logging.info(f"Created storage directory: {self.persist_dir}")

        # In-memory cache for loaded indexes and query engines to improve performance
        self._index_cache: dict[str, VectorStoreIndex] = {}
        self._query_engine_cache: dict[str, BaseQueryEngine] = {}

        # --- ZMQ Socket 初始化与清理 ---
        self.context = zmq.Context(io_threads=4)

        # 如果使用 IPC，在绑定前检查并清理旧的 socket 文件，防止启动失败
        if endpoint.startswith("ipc://"):
            ipc_path = endpoint.replace("ipc://", "")
            if os.path.exists(ipc_path):
                logging.warning(f"Removing stale IPC socket file: {ipc_path}")
                try:
                    os.remove(ipc_path)
                except OSError as e:
                    logging.error(
                        f"Error removing socket file {ipc_path}: {e}")
                    exit(1)

        self.socket = self.context.socket(zmq.REP)
        self.socket.bind(endpoint)

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
        """
        if index_name in self._index_cache:
            return self._index_cache[index_name]

        index_path = self._get_index_path(index_name)
        if os.path.exists(index_path):
            logging.debug(f"Loading index '{index_name}' from disk.")
            # LlamaIndex 会自动使用全局 Settings 中配置的 embed_model
            storage_context = StorageContext.from_defaults(
                persist_dir=index_path)
            index = load_index_from_storage(storage_context)
        else:
            logging.debug(f"Creating new in-memory index for '{index_name}'.")
            # 同样，这里也会自动使用全局配置的 embed_model
            index = VectorStoreIndex.from_documents([])

        self._index_cache[index_name] = index
        return index

    def _get_query_engine(self, index_name: str) -> BaseQueryEngine:
        """
        Retrieves or creates a query engine for a given index.
        Uses an in-memory cache.
        Only uses vector similarity search, no LLM.
        """
        if index_name in self._query_engine_cache:
            return self._query_engine_cache[index_name]

        index = self._load_or_create_index(index_name)
        logging.debug(f"Creating new query engine for index '{index_name}'.")
        # 配置查询引擎仅使用向量相似度，不使用LLM生成回答
        # response_mode="no_text"确保不生成文本回答，仅返回相似节点
        engine = index.as_query_engine(
            similarity_top_k=3,
            response_mode="no_text",  # 不使用LLM生成文本
            llm=None,
        )
        self._query_engine_cache[index_name] = engine
        return engine

    def _clear_cache(self, index_name: str):
        """Clears the cache for a specific index when it's modified."""
        self._index_cache.pop(index_name, None)
        self._query_engine_cache.pop(index_name, None)
        logging.debug(f"Cleared cache for index '{index_name}'.")

    # --- Handler Methods (此部分逻辑与原来保持一致，无需修改) ---

    def handle_create_index(self, payload: dict) -> dict:
        index_name = payload.get("index_name")
        if not index_name:
            return {"error": "Missing 'index_name' in payload."}

        index_path = self._get_index_path(index_name)
        if os.path.exists(index_path):
            return {"error": f"Index '{index_name}' already exists."}

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
            index_names = [name for name in entries if os.path.isdir(
                os.path.join(self.persist_dir, name))]
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

        index.delete_ref_doc(doc_id, delete_from_docstore=True)
        llama_doc = Document(text=text, doc_id=doc_id)
        # insert_nodes 在新版 llama-index 中被推荐
        index.insert_nodes([llama_doc])

        index.storage_context.persist(
            persist_dir=self._get_index_path(index_name))
        logging.info(
            f"Added/updated document '{doc_id}' in index '{index_name}'.")
        return {"status": "ok"}

    def handle_remove_document(self, payload: dict) -> dict:
        index_name = payload.get("index_name")
        doc_id = payload.get("document_id")

        if not all([index_name, doc_id]):
            return {"error": "Missing 'index_name' or 'document_id' in payload."}

        self._clear_cache(index_name)
        index = self._load_or_create_index(index_name)
        index.delete_ref_doc(doc_id, delete_from_docstore=True)
        index.delete_nodes([doc_id], delete_from_docstore=True)

        index.storage_context.persist(
            persist_dir=self._get_index_path(index_name))

        logging.info(
            f"Removed document/node '{doc_id}' from index '{index_name}'.")
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
        """
        执行纯向量相似度搜索，不使用LLM处理结果
        """
        index_name = payload.get("index_name")
        query_text = payload.get("query")

        if not all([index_name, query_text]):
            return json.dumps({"error": "Missing 'index_name' or 'query' in payload."})

        try:
            query_engine = self._get_query_engine(index_name)
            # 使用查询引擎执行查询，由于配置了response_mode="no_text"，只返回相似节点
            response = query_engine.query(query_text)

            # 检查是否有搜索结果
            if not hasattr(response, 'source_nodes') or not response.source_nodes:
                return "No relevant documents found."

            output = []
            total_results = len(response.source_nodes)
            logging.info(
                f"Found {total_results} results for query: {query_text}")

            # 处理每个相似节点，按相似度分数排序
            for i, node_with_score in enumerate(response.source_nodes):
                node = node_with_score.node
                score = node_with_score.score if hasattr(
                    node_with_score, 'score') else 0.0
                source = node.id_ or node.ref_doc_id or "NA"
                formatted_source = f"/corpus/{source}"

                # 格式化输出每个搜索结果
                output.append(
                    f"--- Result {i+1}/{total_results} (Similarity Score: {score:.4f}) ---\n"
                    f"Source: {formatted_source}\n"
                    f"Content: {node.get_content().strip()}"
                )

            return "\n\n".join(output)

        except Exception as e:
            logging.error(
                f"Vector search failed for index '{index_name}': {e}", exc_info=True)
            return json.dumps({"error": f"An error occurred during vector similarity search: {e}"})

    def run(self):
        """
        The main service loop that listens for requests and dispatches them.
        """
        endpoint = self.socket.getsockopt_string(zmq.LAST_ENDPOINT)
        logging.info(
            f"Semantic Search Service started. Listening on {endpoint}")
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
                op_code_bytes, payload_bytes = self.socket.recv_multipart()
                op_code = op_code_bytes.decode('utf-8')
                payload_str = payload_bytes.decode('utf-8')

                logging.debug(
                    f"Received request -> OP: {op_code}, Payload: {payload_str}")
                payload = json.loads(payload_str)
                handler_func = op_map.get(op_code)

                if handler_func:
                    result = handler_func(payload)
                else:
                    result = {"error": f"Unknown operation code: {op_code}"}

                response_str = result if isinstance(
                    result, str) else json.dumps(result)
                self.socket.send_string(response_str)

            except zmq.ZMQError as e:
                logging.error(f"ZMQ Error: {e}")
                break
            except json.JSONDecodeError as e:
                logging.error(f"Invalid JSON payload received: {e}")
                self.socket.send_string(json.dumps(
                    {"error": "Malformed JSON payload."}))
            except Exception as e:
                logging.error(
                    f"An unhandled exception occurred: {e}", exc_info=True)
                self.socket.send_string(json.dumps(
                    {"error": f"An internal server error occurred: {e}"}))


if __name__ == "__main__":
    # ==============================================================================
    # 3. 使用命令行参数来运行服务
    # ==============================================================================
    parser = argparse.ArgumentParser(
        description="FuseLLM Semantic Search Service")
    parser.add_argument(
        "--endpoint",
        default="ipc:///tmp/fusellm-semantic.ipc",
        help="The ZMQ endpoint for the service to bind to (e.g., 'ipc:///tmp/fusellm.ipc' or 'tcp://*:5555')."
    )
    parser.add_argument(
        "--storage-dir",
        default="./index_storage",
        help="The directory to persist semantic search indexes."
    )
    args = parser.parse_args()

    service = SemanticSearchService(
        endpoint=args.endpoint, persist_dir=args.storage_dir)
    try:
        service.run()
    except KeyboardInterrupt:
        logging.info("Service shutting down gracefully.")
