#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "vot_func.h"


#define MAX_PROCS 1000

volatile sig_atomic_t got_sigINT = 0;

void handlerSIGINT(int sig)
{
  printf("\n%d == %d SIGINT %ld\n", sig, SIGINT, (long)getpid());
  got_sigINT = 1;
}


/*
void quitar(int sig){
  printf("%d == %d SIGINT %ld \n", sig, SIGINT, (long)getpid());
}*/

int main(int argc, char *argv[])
{
  FILE *fHijos = NULL;
  int n_procs, n_sec, i, padre = 0, pid, ret, status;
  struct sigaction actSIGINT;

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <N_PROCS> <N_SECS>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  n_procs = atoi(argv[1]);
  n_sec = atoi(argv[2]);

  create_sons(n_procs);

  /*Send every son the signal SIGUSR1*/
  send_signal_procs(SIGUSR1, n_procs);

  /*Change the SIGINT handler*/
  actSIGINT.sa_handler = handlerSIGINT;
  sigemptyset(&(actSIGINT.sa_mask));
  actSIGINT.sa_flags = 0;

  if (sigaction(SIGINT, &actSIGINT, NULL) < 0)
  {
    perror("sigaction SIGINT");
    exit(EXIT_FAILURE);
  }
  while (!got_sigINT);

  /*Enviar a cada hijo un SIGTERM*/
  send_signal_procs(SIGTERM, n_procs);

  /*Waiting fot the voters*/
  for (i = 0; i < n_procs; i++)
  {
    wait(&status);
    if (/*WEXITSTATUS(status) ==*/0)
    {
      printf("Error waiting\n");
      exit(EXIT_FAILURE);
    }
  }
  /*Liberar todos los recursos del sistema*/
  printf("Finishing by signal\n");
  exit(EXIT_SUCCESS);

  return 0;
}
