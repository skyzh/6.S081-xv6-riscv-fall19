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
open_stdin(char* path) {
  close(0);
  if (open(path, O_RDONLY) != 0) {
    fprintf(2, "open stdin %s failed!\n", path);
    exit(1);
  }
  return 0;
}

int
redirect_stdin(int fd) {
  close(0);
  if (dup(fd) != 0) {
    fprintf(2, "redirect stdin failed!\n");
    exit(1);
  }
  return 0;
}

int
open_stdout(char* path) {
  close(1);
  if (open(path, O_CREATE | O_WRONLY) != 1) {
    fprintf(2, "open stdout %s failed!\n", path);
    exit(1);
  }
  return 0;
}

int
redirect_stdout(int fd) {
  close(1);
  if (dup(fd) != 1) {
    fprintf(2, "redirect stdout failed!\n");
    exit(1);
  }
  return 0;
}

int
destroy_stdin() {
  close(0);
  return 0;
}

int
destroy_stdout() {
  close(1);
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
        close(fd[W]);
        redirect_stdin(fd[R]);
        run(pipe_argv[0], pipe_argv);
        close(fd[R]);
        destroy_stdin();
        exit(0);
      }
      close(fd[R]);
      redirect_stdout(fd[W]);
    }

    if (stdin != 0) open_stdin(stdin);
    if (stdout != 0) open_stdout(stdout);
    
    exec(path, argv);

    if (stdin != 0) destroy_stdin(stdin);
    if (stdout != 0) destroy_stdout(stdout);
  
    if (pipe_argv != 0) {
      close(fd[W]);
      destroy_stdout();
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
