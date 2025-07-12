#include "RootHandler.h"
#include <cerrno>
#include <string.h> // For memset

namespace fusellm {

int RootHandler::getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
    // 清空缓冲区总是一个好主意
    memset(stbuf, 0, sizeof(struct stat));

    // 检查是否是根目录
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        // 根目录的链接数 = 2 (自身, .) + 子目录数
        // 为了简单起见，可以先写死，或者动态计算
        stbuf->st_nlink = 2 + 4; // ., ..(虽然根目录的..是它自己), models,
                                 // config, conversations, semantic_search
        return 0;
    }

    // 检查是否是我们定义的虚拟子目录
    if (strcmp(path, "/models") == 0 || strcmp(path, "/config") == 0 ||
        strcmp(path, "/conversations") == 0 ||
        strcmp(path, "/semantic_search") == 0) {

        stbuf->st_mode = S_IFDIR | 0755;
        // 这些是空目录，链接数为 2 (一个来自父目录'/', 一个来自它们自身的'.')
        stbuf->st_nlink = 2;
        return 0;
    }

    // 如果不是以上任何一个，那么它就不存在
    return -ENOENT;
}

int RootHandler::readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi,
                         enum fuse_readdir_flags flags) {
    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    // Standard entries for any directory
    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);

    // Our top-level directories
    filler(buf, "models", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "config", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "conversations", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "semantic_search", NULL, 0, (fuse_fill_dir_flags)0);

    return 0;
}

} // namespace fusellm