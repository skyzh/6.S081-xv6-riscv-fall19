#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define MAX_CHAR 128
#define MAX_ARGS 32
#define R 0
#define W 1

int
readline(char* buf, int n) {
  gets(buf, n);
  if (buf[0] == 0) return -1;
  buf[strlen(buf) - 1] = 0;
  return 0;
}

int
run(char* path, char** argv) {
  char** pipe_argv = 0;
  char* stdin = 0;
  char* stdout = 0;
  for (char** v = argv; *v != 0; ++v) {
    if (strcmp(*v, "<") == 0) {
      *v = 0;
      stdin = *(v + 1);
      ++v;
    }
    if (strcmp(*v, ">") == 0) {
      *v = 0;
      stdout = *(v + 1);
      ++v;
    }
    if (strcmp(*v, "|") == 0) {
      *v = 0;
      pipe_argv = v + 1;
      break;
    }
  }

  if (fork() == 0) {
    int fd[2];
    if (pipe_argv != 0) {
      pipe(fd);
      if (fork() == 0) {
        close(0);
        if (dup(fd[R]) != 0) {
          fprintf(2, "redirect read pipe failed!");
          exit(1);
        }
        run(pipe_argv[0], pipe_argv);
        close(fd[R]);
        close(fd[W]);
        close(0);
        exit(0);
      }
      close(1);
      if(dup(fd[W]) != 1) {
        fprintf(2, "redirect write pipe failed!");
        exit(1);
      }
    }
    if (stdin != 0) {
      close(0);
      if (open(stdin, O_RDONLY) != 0) {
        fprintf(2, "open stdin %s failed!", stdin);
        exit(1);
      }
    }

    if (stdout != 0) {
      close(1);
      if (open(stdout, O_CREATE|O_WRONLY) != 1) {
        fprintf(2, "open stdout %s failed!", stdout);
        exit(1);
      }
    }
    exec(path, argv);
    if (pipe_argv != 0) {
      close(fd[R]);
      close(fd[W]);
      close(1);
      wait(0);
    }
    exit(0);
  } else {
    wait(0);
  }
  return 0;
}

int
main(int argc, char *argv[])
{
  char buf[MAX_CHAR] = { 0 };
  char* aargv[MAX_ARGS] = { 0 };
  int aargc = 0;
  printf("@ ");
  while(!readline(buf, MAX_CHAR)) {
    int _len = strlen(buf);
    aargc = 0;
    aargv[aargc++] = buf;
    for (int i = 0; i < _len; i++) {
      if (buf[i] == ' ') {
        buf[i] = '\0';
        aargv[aargc++] = &buf[i + 1];
      }
    }
    aargv[aargc] = 0;
    run(aargv[0], aargv);
    printf("@ ");
  }
  exit(0);
}
