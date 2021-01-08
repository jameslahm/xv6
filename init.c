// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = {"desktop", 0};

int main(void)
{
  int pid, wpid;

  if (open("console", O_RDWR) < 0)
  {
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);
  dup(0);

  for (;;)
  {
    pid = fork();
    if (pid < 0)
    {
      exit();
    }
    if (pid == 0)
    {
      exec("desktop", argv);
      exit();
    }
    while ((wpid = wait()) >= 0 && wpid != pid)
      ;
  }
}
