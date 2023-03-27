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
#define NSIGPRINC 3
#define nameSemV "/semV"
#define nameSemC "/semC"

volatile sig_atomic_t got_sigINT = 0;
volatile sig_atomic_t got_sigALRM = 0;
volatile sig_atomic_t Error_in_voters = 0;

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

  case SIGUSR1:
    Error_in_voters = 1;

  default:
    break;
  }
}

/*Finishes process + closes the sempahores if !NULL + unlinks the sempahores if !NULL*/
void finishProg(int n_procs, sem_t *semV, sem_t *semC, int ERROR)
{
  if (!ERROR)
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
  int n_procs, n_sec, i, padre = 0, pid, ret, status, signals[NSIGPRINC] = {SIGINT, SIGALRM, SIGUSR1};
  struct sigaction actPRINC;
  sem_t *semV, *semC;
  sigset_t mask, oldmask;

  /*Control error*/
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <N_PROCS> <N_SECS>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  n_procs = atoi(argv[1]);
  n_sec = atoi(argv[2]);

  /*Blocking temporarly SIG_INT, SIG_ALARM ad SIG_USR1 to set their handlers*/
  if(set_handlers(signals, NSIGPRINC, &actPRINC, &oldmask, handler_main) == ERROR){
    finishProg(0, NULL, NULL, ERROR);
    perror("Error setting handlers");
    exit(EXIT_FAILURE);
  }
    
  /*Creation of the semaphores (to vote and to select the candidate)*/
  if ((semV = sem_open(nameSemV, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
  {
    finishProg(0, NULL, NULL, ERROR);
    perror("sem_open SemV");
    exit(EXIT_FAILURE);
  }

  if ((semC = sem_open(nameSemC, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
  {
    finishProg(0, semV, NULL, ERROR);
    perror("sem_open SemC");
    exit(EXIT_FAILURE);
  }

  /*Creation of the voters*/
  create_sons(n_procs, nameSemV, nameSemC, semV, semC);

  /*Send every son the signal SIGUSR1*/
  if (send_signal_procs(SIGUSR1, n_procs, NO_PID) == ERROR)
    finishProg(n_procs, semV, semC, OK);

  /*Setting of the alarm*/
  if (alarm(n_sec))
    fprintf(stderr, "There is a previously established alarm\n");

  while (!got_sigINT && !got_sigALRM && !Error_in_voters)
    sigsuspend(&oldmask);

  /*Liberar todos los recursos del sistema*/
  if (!Error_in_voters)
    finishProg(n_procs, semV, semC, OK);

  if (got_sigALRM)
    printf("Finishing by alarm\n");
  else if (got_sigINT)
    printf("Finishing by signal\n");
  else
  {
    finishProg(n_procs, semV, semC, ERROR);
    (void)send_signal_procs(SIGKILL, n_procs, NO_PID);
    printf("Finishing by an error\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}