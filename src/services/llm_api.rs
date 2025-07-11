use crate::config::Config;
use crate::error::Result;
use crate::state::conversation::{Conversation, Message};
use async_openai::Client;
use async_openai::config::OpenAIConfig;

/// A service for interacting with the OpenAI API.
pub struct LlmService {
    client: Client<OpenAIConfig>,
}

/// The response from an LLM API call.
pub struct LlmResponse {
    pub content: String,
    // Could also include usage statistics, etc.
}

impl LlmService {
    /// Creates a new LlmService with the given API key.
    pub fn new(_api_key: &str) -> Self {
        unimplemented!()
    }

    /// Sends a request to the LLM API based on the current conversation state.
    pub async fn query_conversation(&self, _conversation: &Conversation) -> Result<LlmResponse> {
        unimplemented!()
    }

    /// Sends a stateless, one-off request to a model.
    pub async fn query_stateless(&self, _model_name: &str, _prompt: &str, _config: &Config) -> Result<LlmResponse> {
        unimplemented!()
    }
}
