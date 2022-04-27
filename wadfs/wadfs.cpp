// File is based off of `fuse-example` from https://engineering.facile.it/blog/eng/write-filesystem-fuse/

#define FUSE_USE_VERSION 26

#include <iostream>
#include "../libWad/Wad.h"
#include <string>
#include <vector>

#include <fuse.h>
#include <string.h>
#include <errno.h>

Wad* loadedWad;

static int getattr_callback(const char *path, struct stat *stbuf) {
  memset(stbuf, 0, sizeof(struct stat));

  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (loadedWad->isDirectory(path)){
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    return 0;
  }

  if (loadedWad->isContent(path)){
    stbuf->st_mode = S_IFREG | 0777;
    stbuf->st_nlink = 1;
    stbuf->st_size = loadedWad->getSize(path);
    return 0;
  }

  return -ENOENT;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi) {
  (void) offset;
  (void) fi;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  std::vector<std::string> dirContents;
  int numEntries = loadedWad->getDirectory(path, &dirContents);
  for (std::string entry : dirContents) {
    filler(buf, entry.c_str(), NULL, 0);
  }

  return 0;
}

static int open_callback(const char *path, struct fuse_file_info *fi) {
  return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {

  if (loadedWad->isContent(path)) {
    size_t len = loadedWad->getSize(path);
    if (offset >= len) {
      return 0;
    }

    int size = loadedWad->getContents(path, buf, len, offset);
    return size;
  }

  return -ENOENT;
}

static struct fuse_operations wad_ops = {
  .getattr = getattr_callback,
  .open = open_callback,
  .read = read_callback,
  .readdir = readdir_callback,
};

int main(int argc, char *argv[]) {
  if (argc != 3)  {
    std::cerr << "Usage: ./wadfs file.WAD <path/to/mount/point>" << std::endl;
    exit(-1);
  } else {
    std::string wadName(argv[1]);
    std::string mntPath(argv[2]);

    char* vargs[] = {argv[0], argv[2]};

    loadedWad = Wad::loadWad(wadName);
    std::cout << "Successfully loaded wad." << std::endl;
    return fuse_main(argc-1, vargs, &wad_ops, NULL);
    
  }
}