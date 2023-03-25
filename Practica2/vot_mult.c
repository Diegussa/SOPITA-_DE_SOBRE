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
volatile sig_atomic_t got_sigALRM = 0;

/*Handler where the the signals SIGINT and SIGALRM are going to be redirected*/
void handler_main(int sig)
{
  switch (sig)
  {
  case SIGINT:
    got_sigINT = 1;
    break;

  case SIGALRM:
    got_sigALRM = 1;
    break;

  default:
    break;
  }
}

/*Finishes process + closes the sempahores if !NULL + unlinks the sempahores if !NULL*/
void finishProg(int n_procs, sem_t *semV, char *nameSemV, sem_t *semC, char *nameSemC)
{
  end_processes(n_procs);
  if (semV)
    sem_close(semV);
  if (nameSemV)
    sem_unlink(nameSemV);
  if (semC)
    sem_close(semC);
  if (nameSemC)
    sem_unlink(nameSemC);
}

int main(int argc, char *argv[])
{
  FILE *fHijos = NULL;
  int n_procs, n_sec, i, padre = 0, pid, ret, status;
  struct sigaction actPRINC;
  char nameSemV[10] = "/semV58", nameSemC[10] = "/semC58";
  sem_t *semV, *semC;

  /*Control error*/
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
    finishProg(0, NULL, NULL, NULL, NULL);
    perror("sem_open SemV");
    exit(EXIT_FAILURE);
  }

  semC = sem_open(nameSemC, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
  if (semC == SEM_FAILED)
  {
    finishProg(0, semV, nameSemV, NULL, NULL);
    perror("sem_open SemC");
    exit(EXIT_FAILURE);
  }

  /*Creation of the voters*/
  create_sons(n_procs, nameSemV, nameSemC, semV, semC);

  /*Send every son the signal SIGUSR1*/
  send_signal_procs(SIGUSR1, n_procs, NO_PID);

  /*Change the SIGINT +SIGALRM handler*/
  actPRINC.sa_handler = handler_main;
  sigemptyset(&(actPRINC.sa_mask));
  actPRINC.sa_flags = 0;

  if (sigaction(SIGINT, &actPRINC, NULL) < 0)
  {
    finishProg(n_procs, semV, nameSemV, semC, nameSemC);
    perror("sigaction SIGINT");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGALRM, &actPRINC, NULL) < 0)
  {
    finishProg(n_procs, semV, nameSemV, semC, nameSemC);
    perror("sigaction SIGALRM");
    exit(EXIT_FAILURE);
  }

  /*Setting of the alarm*/
  if (alarm(n_sec))
    fprintf(stderr, "There is a previously established alarm\n");

  while (!got_sigINT && !got_sigALRM)
    ;
  /*Liberar todos los recursos del sistema*/
  finishProg(n_procs, semV, nameSemV, semC, nameSemC);

  if (got_sigALRM)
    printf("Finishing by alarm\n");
  else if (got_sigINT)
    printf("Finishing by signal\n");
  else
    printf("Finishing by an error\n");

  exit(EXIT_SUCCESS);
}