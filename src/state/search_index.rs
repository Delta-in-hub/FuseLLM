use std::collections::HashSet;

/// Represents the state of a single semantic search index.
#[derive(Debug, Clone)]
pub struct SearchIndex {
    /// The unique identifier for this index (e.g., "my-codebase").
    pub id: String,
    
    /// A set of file names that have been added to the corpus.
    pub corpus_files: HashSet<String>,

    /// The most recent result from a query operation. Read by `cat query`.
    pub latest_query_result: String,
}

impl SearchIndex {
    /// Creates a new, empty search index with a given ID.
    pub fn new(_id: String) -> Self {
        unimplemented!()
    }
}
