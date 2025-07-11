use anyhow::{Result, anyhow};
use serde::{Deserialize, Serialize};
use zmq::{Context, SocketType};

/// Represents the request sent to the semantic search service.
#[derive(Debug, Serialize)]
struct SemanticRequest<'a> {
    action: &'a str, // "index", "query", "delete"
    index_id: &'a str,
    document_id: Option<&'a str>,
    content: Option<&'a str>,
}

/// Represents the response received from the semantic search service.
#[derive(Debug, Deserialize)]
struct SemanticResponse {
    status: String, // "ok" or "error"
    message: Option<String>,
    results: Option<Vec<SearchResult>>,
}

/// A single search result from the semantic search service.
#[derive(Debug, Deserialize)]
pub struct SearchResult {
    pub score: f32,
    pub source: String,
    pub content: String,
}

/// A client for the semantic search service.
pub struct SemanticClient {
    socket: zmq::Socket,
}

impl SemanticClient {
    /// Creates a new client and connects to the given ZMQ endpoint.
    pub fn new(service_url: &str) -> Result<Self> {
        let context = Context::new();
        let socket = context.socket(SocketType::REQ)?;
        socket.connect(service_url)?;
        Ok(Self { socket })
    }

    /// Sends a request to the service and waits for a reply.
    fn send_request(&self, request: &SemanticRequest) -> Result<SemanticResponse> {
        let msg = serde_json::to_string(request)?;
        self.socket.send(&msg, 0)?;
        let reply = self
            .socket
            .recv_string(0)?
            .map_err(|v| anyhow!("Failed to receive reply: {:#?}", v))?;
        let response: SemanticResponse = serde_json::from_str(&reply)?;
        if response.status == "error" {
            return Err(anyhow!(
                response
                    .message
                    .unwrap_or_else(|| "Unknown error".to_string()),
            ));
        }
        Ok(response)
    }

    /// Sends a document to be indexed.
    pub fn index(&self, index_id: &str, document_id: &str, content: &str) -> Result<()> {
        let request = SemanticRequest {
            action: "index",
            index_id,
            document_id: Some(document_id),
            content: Some(content),
        };
        self.send_request(&request).map(|_| ())
    }

    /// Deletes a document from the index.
    pub fn delete(&self, index_id: &str, document_id: &str) -> Result<()> {
        let request = SemanticRequest {
            action: "delete",
            index_id,
            document_id: Some(document_id),
            content: None,
        };
        self.send_request(&request).map(|_| ())
    }

    /// Performs a semantic query.
    pub fn query(&self, index_id: &str, query_text: &str) -> Result<Vec<SearchResult>> {
        let request = SemanticRequest {
            action: "query",
            index_id,
            document_id: None,
            content: Some(query_text),
        };
        let response = self.send_request(&request)?;
        Ok(response.results.unwrap_or_default())
    }
}
