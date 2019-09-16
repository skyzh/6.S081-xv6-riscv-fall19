#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int time;

  if(argc < 2){
    fprintf(2, "Usage: sleep seconds\n");
    exit();
  }

  time = atoi(argv[1]);

  sleep(time);

  exit();
}
