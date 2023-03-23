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

void handler_main(int sig)
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
  char nameSemV[10] = "/semV57", nameSemC[10] = "/semC57";
  sem_t *semV, *semC;

  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <N_PROCS> <N_SECS>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  n_procs = atoi(argv[1]);
  n_sec = atoi(argv[2]);


  /*Creation of the semaphores (to vote and to select the candidate)*/
  semV = sem_open(nameSemV, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
  if (semV == SEM_FAILED)
  {
    printf("Holas!\n");
    perror("sem_open SemV");
    end_processes(n_procs);

    exit(EXIT_FAILURE);
  }

  semC = sem_open(nameSemC, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
  if (semC == SEM_FAILED)
  {
    printf("Holas\n");
    end_processes(n_procs);
    sem_close(semV);
    sem_unlink(nameSemV);
    perror("sem_open SemC");
    exit(EXIT_FAILURE);
  }

  create_sons(n_procs, nameSemV, nameSemC, semV, semC);


  /*Send every son the signal SIGUSR1*/
  send_signal_procs(SIGUSR1, n_procs);

  /*Change the SIGINT handler*/
  actSIGINT.sa_handler = handler_main;
  sigemptyset(&(actSIGINT.sa_mask));
  actSIGINT.sa_flags = 0;

  if (sigaction(SIGINT, &actSIGINT, NULL) < 0)
  {
    end_processes(n_procs);
    sem_close(semV);
    sem_unlink(nameSemV);
    perror("sigaction SIGINT");
    exit(EXIT_FAILURE);
  }
  
  while (!got_sigINT);

  end_processes(n_procs);
  /*Liberar todos los recursos del sistema*/
  printf("Finishing by signal\n");

  sem_close(semV);
  sem_unlink(nameSemV);
  sem_close(semC);
  sem_unlink(nameSemC);

  return 0;
}