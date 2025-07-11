/// `FsNode` is an enumeration of all possible virtual files and directories
/// within the FuseLLM filesystem. Each variant represents a distinct entity
/// and carries the necessary identifiers (e.g., conversation ID, model name).
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FsNode {
    // Root and Top-Level Dirs
    Root,
    ModelsDir,
    ConfigDir,
    ConversationsDir,
    SemanticSearchDir,

    // Models
    ModelFile { name: String },
    ModelDefaultSymlink,
    
    // Config
    GlobalSettingsFile,
    ConfigModelsDir,
    ConfigModelDir { name: String },
    ConfigModelSettingsFile { name: String },

    // Conversations
    ConversationsLatestSymlink,
    ConversationDir { id: String },
    PromptFile { conv_id: String },
    HistoryFile { conv_id: String },
    ContextFile { conv_id: String },
    ConversationConfigDir { conv_id: String },
    ConversationModelFile { conv_id: String },
    ConversationSystemPromptFile { conv_id: String },
    ConversationSettingsFile { conv_id: String },

    // Semantic Search
    SearchIndexDir { index_id: String },
    SearchIndexDefaultSymlink,
    CorpusDir { index_id: String },
    CorpusFile { index_id: String, file_name: String },
    QueryFile { index_id: String },

    // Sentinel for unresolved paths
    NotFound,
}

impl FsNode {
    /// Returns the inode number for the given node.
    /// This needs a consistent mapping scheme.
    pub fn inode(&self) -> u64 {
        unimplemented!()
    }

    /// Returns the parent inode number for the given node.
    pub fn parent_inode(&self) -> u64 {
        unimplemented!()
    }
}
