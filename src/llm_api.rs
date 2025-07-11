use crate::config::{GlobalConfig, ModelConfig};
use crate::state::{HistoryEntry, Role};
use anyhow::{Context, Result};
use async_openai::Client;
use async_openai::config::OpenAIConfig;
use async_openai::types::{
    ChatCompletionRequestAssistantMessageArgs, ChatCompletionRequestMessage,
    ChatCompletionRequestSystemMessageArgs, ChatCompletionRequestUserMessageArgs,
    CreateChatCompletionRequestArgs, CreateChatCompletionResponse,
};

/// 用于与大语言模型交互的客户端。
#[derive(Debug, Clone)]
pub struct LlmClient {
    client: Client<OpenAIConfig>,
}

impl LlmClient {
    /// 根据全局配置创建一个新的 LLM 客户端。
    /// 这个方法现在更清晰地构建配置。
    pub fn new(global_config: &GlobalConfig) -> Self {
        let mut config = OpenAIConfig::new().with_api_key(&global_config.api_key);

        if let Some(base_url) = &global_config.base_url {
            config = config.with_api_base(base_url);
        }
        Self {
            client: Client::with_config(config),
        }
    }

    /// 向 LLM 发送请求并返回响应。
    pub async fn chat(
        &self,
        model_id: &str,
        config: &ModelConfig,
        history: &[HistoryEntry],
    ) -> Result<CreateChatCompletionResponse> {
        let mut messages: Vec<ChatCompletionRequestMessage> = Vec::new();

        if let Some(system_prompt) = &config.system_prompt {
            let msg = ChatCompletionRequestSystemMessageArgs::default()
                .content(system_prompt.as_str())
                .build()?
                .into();
            messages.push(msg);
        }

        // 遍历历史记录，构建用户和助手的消息
        for entry in history {
            let msg: ChatCompletionRequestMessage = match entry.role {
                Role::User => ChatCompletionRequestUserMessageArgs::default()
                    .content(entry.content.clone())
                    .build()?
                    .into(),
                Role::LLM => ChatCompletionRequestAssistantMessageArgs::default()
                    .content(entry.content.clone())
                    .build()?
                    .into(),
            };
            messages.push(msg);
        }

        // 使用 CreateChatCompletionRequestArgs 建造者来构建最终请求
        let mut request_builder = CreateChatCompletionRequestArgs::default();

        request_builder.model(model_id).messages(messages);

        if let Some(temp) = config.temperature {
            request_builder.temperature(temp);
        }

        // 如果需要，可以在这里添加其他参数，如 max_tokens, top_p 等
        // request_builder.max_tokens(1024u32);

        let request = request_builder
            .build()
            .context("Failed to build chat completion request")?;

        let response = self
            .client
            .chat()
            .create(request)
            .await
            .context("Failed to get chat completion response from API")?;

        Ok(response)
    }
}
