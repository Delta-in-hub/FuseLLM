import zmq
import json
import logging
from llama_index.core import VectorStoreIndex, SimpleDirectoryReader, Document

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# In-memory storage for LlamaIndex objects
# Structure: {"index_id": {"index": VectorStoreIndex, "documents": {doc_id: Document}}}
indexes = {}

def handle_request(req_data):
    action = req_data.get('action')
    index_id = req_data.get('index_id')

    if not action or not index_id:
        return {"status": "error", "message": "'action' and 'index_id' are required."}

    # Create index if it doesn't exist for most actions
    if index_id not in indexes and action in ["index", "query"]:
        logging.info(f"Creating new in-memory index for id: {index_id}")
        # Create an index with a dummy document, it can be extended later
        dummy_doc = Document(text="Initial document", doc_id="_dummy")
        indexes[index_id] = {
            "index": VectorStoreIndex.from_documents([dummy_doc]),
            "documents": {"_dummy": dummy_doc}
        }

    if action == 'index':
        doc_id = req_data.get('document_id')
        content = req_data.get('content')
        if not doc_id or content is None:
            return {"status": "error", "message": "'document_id' and 'content' are required for indexing."}
        
        logging.info(f"Indexing document '{doc_id}' in index '{index_id}'")
        index_obj = indexes[index_id]["index"]
        doc = Document(text=content, doc_id=doc_id)
        
        # If document exists, update it. Otherwise, insert.
        if doc_id in indexes[index_id]["documents"]:
            index_obj.update_ref_doc(doc)
        else:
            index_obj.insert(doc)
        
        indexes[index_id]["documents"][doc_id] = doc
        return {"status": "ok"}

    elif action == 'delete':
        doc_id = req_data.get('document_id')
        if not doc_id:
            return {"status": "error", "message": "'document_id' is required for deletion."}
        
        if index_id not in indexes or doc_id not in indexes[index_id]["documents"]:
            return {"status": "error", "message": f"Document '{doc_id}' not found in index '{index_id}'."}

        logging.info(f"Deleting document '{doc_id}' from index '{index_id}'")
        indexes[index_id]["index"].delete_ref_doc(doc_id, delete_from_docstore=True)
        del indexes[index_id]["documents"][doc_id]
        return {"status": "ok"}

    elif action == 'query':
        query_text = req_data.get('content')
        if not query_text:
            return {"status": "error", "message": "'content' (the query text) is required for query."}

        logging.info(f"Querying index '{index_id}' with: '{query_text[:50]}...'" )
        index_obj = indexes[index_id]["index"]
        query_engine = index_obj.as_query_engine()
        response = query_engine.query(query_text)

        results = [
            {
                "score": node.score,
                "source": node.node.ref_doc_id or "N/A",
                "content": node.get_content(),
            }
            for node in response.source_nodes
        ]
        return {"status": "ok", "results": results}

    else:
        return {"status": "error", "message": f"Unknown action: {action}"}

def main():
    service_url = "ipc:///tmp/fusellm-semantic.ipc"
    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind(service_url)
    logging.info(f"ZMQ service listening on {service_url}")

    try:
        while True:
            message = socket.recv_string()
            logging.info(f"Received request: {message}")
            try:
                req_data = json.loads(message)
                response = handle_request(req_data)
            except Exception as e:
                logging.error(f"Error processing request: {e}", exc_info=True)
                response = {"status": "error", "message": str(e)}
            
            socket.send_string(json.dumps(response))
    except KeyboardInterrupt:
        logging.info("Shutting down service.")
    finally:
        socket.close()
        context.term()

if __name__ == '__main__':
    main()


    def query_index(self, index_id: str, query_text: str, top_k: int = 5) -> dict:
        """Performs a semantic search query against an index."""
        try:
            index = search_indexes.get(index_id)
            if not index or not index["documents"]:
                return {"status": "Result", "data": {"content": "No documents in index to search."}}

            model = self._get_or_load_model(index_id)
            
            filepaths = list(index["embeddings"].keys())
            corpus_embeddings = np.array(list(index["embeddings"].values()))
            
            query_embedding = model.encode([query_text])[0]
            
            similarities = cosine_similarity([query_embedding], corpus_embeddings)[0]
            
            top_indices = np.argsort(similarities)[-top_k:][::-1]
            
            results = []
            for i in top_indices:
                filepath = filepaths[i]
                score = similarities[i]
                content = index["documents"][filepath]
                # Simple formatting for the result
                formatted_result = f"---\nFile: {filepath}\nScore: {score:.4f}\n\n{content[:500]}...\n"
                results.append(formatted_result)
            
            return {"status": "Result", "data": {"content": "".join(results)}}
        except Exception as e:
            return {"status": "Error", "data": {"message": str(e)}}

    def dispatch(self, request: dict) -> dict:
        """Dispatches a request to the appropriate handler."""
        command = request.get("command")
        payload = request.get("payload", {})

        if command == "CreateIndex":
            return self.create_index(payload.get("index_id"))
        elif command == "DeleteIndex":
            return self.delete_index(payload.get("index_id"))
        elif command == "Add":
            return self.add_document(payload.get("index_id"), payload.get("file_path"), payload.get("content"))
        elif command == "Remove":
            return self.remove_document(payload.get("index_id"), payload.get("file_path"))
        elif command == "Query":
            return self.query_index(payload.get("index_id"), payload.get("query_text"))
        else:
            return {"status": "Error", "data": {"message": f"Unknown command: {command}"}}

def main():
    """
    Main function to run the ZMQ server.
    It listens for requests, dispatches them, and sends back responses.
    """
    # This would typically come from the config file passed to the Rust app
    ZMQ_ADDRESS = "tcp://*:5555" 
    EMBEDDING_MODEL = "all-MiniLM-L6-v2"

    context = zmq.Context()
    socket = context.socket(zmq.REP)
    socket.bind(ZMQ_ADDRESS)
    logging.info(f"ZMQ server listening on {ZMQ_ADDRESS}")

    service = SearchService(embedding_model_name=EMBEDDING_MODEL)

    while True:
        try:
            message = socket.recv_json()
            logging.info(f"Received request: {message}")
            
            response = service.dispatch(message)
            
            socket.send_json(response)
            logging.info(f"Sent response: {response}")

        except json.JSONDecodeError:
            logging.error("Received invalid JSON")
            socket.send_json({"status": "Error", "data": {"message": "Invalid JSON format"}})
        except Exception as e:
            logging.error(f"An unexpected error occurred: {e}", exc_info=True)
            socket.send_json({"status": "Error", "data": {"message": str(e)}})

if __name__ == "__main__":
    main()
