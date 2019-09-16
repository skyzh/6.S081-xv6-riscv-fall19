#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


void find(const char* path, const char* next, const char* filename, int bootstrap) {
  int fd;
  struct dirent de;
  struct stat st;

  char this_path[DIRSIZ + 1];
  strcpy(this_path, path);
  if (bootstrap == 0) {
    int len = strlen(this_path);
    this_path[len] = '/';
    strcpy(this_path + len + 1, next);
  }
  if((fd = open(this_path, 0)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_DIR:
    while(read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0) continue;
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) continue;
      find(this_path, de.name, filename, 0);
    }
    break;
    case T_FILE:
    if (strcmp(filename, next) == 0) {
      printf("%s\n", this_path);
    }
    break;
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 2){
    fprintf(2, "Usage: find path filename\n");
    exit();
  }

  const char* path = argv[1];
  const char* filename = argv[2];

  find(path, "", filename, 1);

  exit();
}
