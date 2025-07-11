import zmq
import json
import logging
from pathlib import Path
from sentence_transformers import SentenceTransformer
import numpy as np
from sklearn.metrics.pairwise import cosine_similarity

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# In-memory storage for search indexes
# Structure:
# {
#   "index_id_1": {
#     "model": SentenceTransformer,
#     "documents": {
#       "file_path_1": "content_1",
#       ...
#     },
#     "embeddings": {
#       "file_path_1": np.array([...]),
#       ...
#     }
#   },
#   ...
# }
search_indexes = {}

class SearchService:
    """Handles all semantic search operations."""

    def __init__(self, embedding_model_name: str):
        """
        Initializes the search service.

        Args:
            embedding_model_name (str): The name of the sentence-transformer model to use.
        """
        self.default_model_name = embedding_model_name

    def _get_or_load_model(self, index_id: str) -> SentenceTransformer:
        """Loads a model for an index if it's not already loaded."""
        if index_id not in search_indexes:
            raise ValueError(f"Index '{index_id}' does not exist.")
        
        index = search_indexes[index_id]
        if "model" not in index or index["model"] is None:
            logging.info(f"Loading model '{self.default_model_name}' for index '{index_id}'...")
            index["model"] = SentenceTransformer(self.default_model_name)
            logging.info(f"Model for index '{index_id}' loaded.")
        return index["model"]

    def create_index(self, index_id: str) -> dict:
        """Creates a new, empty search index."""
        if index_id in search_indexes:
            return {"status": "Error", "data": {"message": f"Index '{index_id}' already exists."}}
        
        search_indexes[index_id] = {
            "model": None, # Lazily loaded
            "documents": {},
            "embeddings": {},
        }
        logging.info(f"Created new search index: {index_id}")
        return {"status": "Success"}

    def delete_index(self, index_id: str) -> dict:
        """Deletes an existing search index."""
        if index_id not in search_indexes:
            return {"status": "Error", "data": {"message": f"Index '{index_id}' not found."}}
        
        del search_indexes[index_id]
        logging.info(f"Deleted search index: {index_id}")
        return {"status": "Success"}

    def add_document(self, index_id: str, file_path: str, content: str) -> dict:
        """Adds or updates a document in an index and generates its embedding."""
        try:
            model = self._get_or_load_model(index_id)
            embedding = model.encode([content])[0]
            
            index = search_indexes[index_id]
            index["documents"][file_path] = content
            index["embeddings"][file_path] = embedding
            
            logging.info(f"Added/updated document '{file_path}' in index '{index_id}'")
            return {"status": "Success"}
        except Exception as e:
            return {"status": "Error", "data": {"message": str(e)}}

    def remove_document(self, index_id: str, file_path: str) -> dict:
        """Removes a document from an index."""
        if index_id not in search_indexes or file_path not in search_indexes[index_id]["documents"]:
            return {"status": "Error", "data": {"message": f"Document '{file_path}' not found in index '{index_id}'."}}
        
        del search_indexes[index_id]["documents"][file_path]
        del search_indexes[index_id]["embeddings"][file_path]
        logging.info(f"Removed document '{file_path}' from index '{index_id}'")
        return {"status": "Success"}

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
