#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define R 0
#define W 1

int
main(int argc, char *argv[])
{
  int numbers[100], cnt = 0, i;
  int fd[2];
  for (i = 2; i <= 35; i++) {
    numbers[cnt++] = i;
  }
  while (cnt > 0) {
    pipe(fd);
    if (fork() == 0) {
      int prime, this_prime = 0;
      close(fd[W]);
      cnt = -1;
      while (read(fd[R], &prime, sizeof(prime)) != 0) {
        if (cnt == -1) {
          this_prime = prime;
          cnt = 0;
        } else {
          if (prime % this_prime != 0) numbers[cnt++] = prime;
        }
      }
      printf("prime %d\n", this_prime);
      close(fd[R]);
    } else {
      close(fd[R]);
      for (i = 0; i < cnt; i++) {
        write(fd[W], &numbers[i], sizeof(numbers[0]));
      }
      close(fd[W]);
      wait();
      break;
    }
  }
  exit();
}
