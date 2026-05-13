#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

static const char *source_dir;

static void generate_tujuan(char *result);

static void fullpath(char fpath[1024], const char *path)
{
    sprintf(fpath, "%s%s", source_dir, path);
}

static int xmp_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi)
{
    (void) fi;

    int res;
    char fpath[1024];

    memset(stbuf, 0, sizeof(struct stat));

    // HANDLE FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0) {

    char content[4096];

    generate_tujuan(content);

    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(content);

    return 0;
    }

    fullpath(fpath, path);

    res = lstat(fpath, stbuf);

    if (res == -1)
        return -errno;

    return 0;
}


static int xmp_readdir(const char *path, void *buf,
                       fuse_fill_dir_t filler,
                       off_t offset,
                       struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    DIR *dp;
    struct dirent *de;

    char fpath[1024];

    fullpath(fpath, path);

    dp = opendir(fpath);

    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        filler(buf, de->d_name, NULL, 0, 0);
    }

    closedir(dp);

    // TAMBAH FILE VIRTUAL
    filler(buf, "tujuan.txt", NULL, 0, 0);

    return 0;
}

static int xmp_open(const char *path, struct fuse_file_info *fi)
{
    int res;
    char fpath[1024];

    // FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0)
        return 0;

    fullpath(fpath, path);

    res = open(fpath, fi->flags);

    if (res == -1)
        return -errno;

    close(res);

    return 0;
}

static void generate_tujuan(char *result)
{
    char line[1024];
    char fragment[1024];
    char filepath[1024];

    strcpy(result, "Tujuan Mas Amba: ");

    for (int i = 1; i <= 7; i++) {

        sprintf(filepath, "%s/%d.txt", source_dir, i);

        FILE *fp = fopen(filepath, "r");

        if (!fp)
            continue;

        while (fgets(line, sizeof(line), fp)) {

            if (strncmp(line, "KOORD:", 6) == 0) {

                strcpy(fragment, line + 6);

                fragment[strcspn(fragment, "\n")] = 0;

                strcat(result, fragment);
            }
        }

        fclose(fp);
    }

    strcat(result, "\n");
}

static int xmp_read(const char *path,
                    char *buf,
                    size_t size,
                    off_t offset,
                    struct fuse_file_info *fi)
{
    (void) fi;

    // HANDLE FILE VIRTUAL
    if (strcmp(path, "/tujuan.txt") == 0) {

        char content[4096];

        generate_tujuan(content);

        size_t len = strlen(content);

        if (offset >= len)
            return 0;

        if (offset + size > len)
            size = len - offset;

        memcpy(buf, content + offset, size);

        return size;
    }

    int fd;
    int res;

    char fpath[1024];

    fullpath(fpath, path);

    fd = open(fpath, O_RDONLY);

    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);

    if (res == -1)
        res = -errno;

    close(fd);

    return res;
}

static const struct fuse_operations xmp_oper = {
    .getattr = xmp_getattr,
    .readdir = xmp_readdir,
    .open    = xmp_open,
    .read    = xmp_read,
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mountpoint>\n", argv[0]);
        return 1;
    }

    int foreground = 0;
    int source_index = 1;
    int mount_index = 2;

    // kalau ada -f
    if (strcmp(argv[1], "-f") == 0) {
        foreground = 1;
        source_index = 2;
        mount_index = 3;
    }

    source_dir = realpath(argv[source_index], NULL);

    // argumen untuk fuse
    char *fuse_argv[3];
    int fuse_argc = 0;

    fuse_argv[fuse_argc++] = argv[0];

    if (foreground)
        fuse_argv[fuse_argc++] = "-f";

    fuse_argv[fuse_argc++] = argv[mount_index];

    return fuse_main(fuse_argc, fuse_argv, &xmp_oper, NULL);
}
