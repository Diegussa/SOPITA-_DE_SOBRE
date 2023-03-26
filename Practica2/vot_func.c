#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "vot_func.h"

#define NOMBREVOTAR "votos.txt"
#define NSIGNALS 3
//#define DEBUG
#ifdef DEBUG
  #define PRIME 70309087
#endif
volatile sig_atomic_t got_sigUSR1 = 0;
volatile sig_atomic_t got_sigUSR2 = 0;
volatile sig_atomic_t got_sigTERM = 0;

/*Private function that defines the new handler for the signals SIGTERM, SIGUSR1 and SIGUSR2*/
void _handler_voter(int sig)
{
  switch (sig)
  {
  case SIGTERM:
#ifdef DEBUG
    printf("%d == %d SIGTERM %ld \n", sig, SIGTERM, (long)getpid());
#endif
    got_sigTERM = 1;
    break;

  case SIGUSR1:
#ifdef DEBUG
    printf("U1 %ld ", (long)getpid());
#endif
    got_sigUSR1 = 1;
    break;

  case SIGUSR2:
#ifdef DEBUG
    printf("U2 %ld ", (long)getpid());
#endif
    got_sigUSR2 = 1;
    break;

  default:
    break;
  }
}

/*Sends a signal to end */
void _error_in_voters(){
  printf("ERROR ocurred in the son with PID=%ld\n", (long)getpid());
  if (kill(getppid(), SIGUSR1))
    fprintf(stderr, "ERROR sending the error signal in %ld\n", (long)getppid());
  
  exit(EXIT_FAILURE);
}

/*Changes the handler of the specified signals*/
int set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask)
{
  sigset_t mask2;
  int i = 0;

  sigemptyset(&mask2);
  for (i = 0; i < n_signals; i++)
  {
    if(sigaddset(&mask2, sig[i]) == ERROR)
      _error_in_voters();
  }
  /*Unblock signals*/
  if(sigprocmask(SIG_BLOCK, &mask2, oldmask) == ERROR )
      _error_in_voters();
  /*Set the new signal handler*/
  actSIG->sa_handler = _handler_voter;
  sigemptyset(&(actSIG->sa_mask));
  actSIG->sa_flags = 0;
  for (i = 0; i < n_signals; i++)
  {
    if (sigaction(sig[i], actSIG, NULL) == ERROR )
       _error_in_voters();
  }
  /*Unblock signals*/
  if (sigprocmask(SIG_UNBLOCK, &mask2, NULL) == ERROR )
     _error_in_voters();

  return 0;
}

/*Voting when the voter has executed an effective down*/
STATUS votingCarefully(char *nFichV)
{
  FILE *f = NULL;
  char letra;

  if (!(f = fopen(nFichV, "ab+"))){
    _error_in_voters();

     return ERROR;
  }

  if (rand() < (RAND_MAX / 2))
    letra = 'N';
  else
    letra = 'Y';

  if (fwrite(&letra, sizeof(char), 1, f) == ERROR)
    _error_in_voters();
#ifdef DEBUG
  printf("v %ld: %c\n", (long)getpid(), letra);
#endif
  fclose(f);

  return OK;
}

/*Generates n_procs and writes their PID in a file*/
void create_sons(int n_procs, char *nameSemV, char *namesemC, sem_t *semV, sem_t *semC)
{
  int i;
  pid_t pid, *PIDs = NULL;
  FILE *fHijos;

  PIDs = (pid_t *)malloc(n_procs * sizeof(pid_t));
  if (!PIDs)
  {
    printf("Error allocating memory for PIDs\n");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < n_procs; i++)
  {
    if ((pid = (int)fork()) == 0) /*Function made for the sons*/
      voters(nameSemV, namesemC, n_procs, semV, semC);
    else if (pid == ERROR)
    {
      i--;
      end_processes(i);
    }
    PIDs[i] = pid;
  }

  /*The writing of the PIDs in the file*/
  if (!(fHijos = fopen(NOMBREFICHERO, "wb")))
  {
    free(PIDs);
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    end_processes(n_procs);
  }

  if(fwrite(PIDs, sizeof(pid_t), n_procs, fHijos) == ERROR)
    end_processes(n_procs);
  fclose(fHijos);

  free(PIDs);
}

/*Main function executed by the sons*/
void voters(char *nameSemV, char *nameSemC, int n_procs, sem_t *semV, sem_t *semC)
{
  int i = 0, val, sig[NSIGNALS] = {SIGUSR1, SIGTERM, SIGUSR2};
  struct sigaction actSIG;
  sigset_t mask, mask2, oldmask;

  srand((int)getpid());
  /*Block permanently SIGINT*/
  sigemptyset(&mask);
  if(sigaddset(&mask2, SIGINT)==ERROR)
    _error_in_voters();
  if(sigprocmask(SIG_BLOCK, &mask, NULL)==ERROR)
    _error_in_voters();

  /*Block temporarily SIGUSR1. SIGUSR2, SIGTERM to set their handers*/

  if (set_handlers(sig, NSIGNALS, &actSIG, &oldmask) == ERROR)
    _error_in_voters(n_procs);

  if (sigaction(SIGINT, &actSIG, NULL) < 0)
    _error_in_voters(semV, semC);
  /*Setting a mask with SIGTERM*/
  sigemptyset(&mask2);
  if(sigaddset(&mask2, SIGTERM)==ERROR)
    _error_in_voters();
  
  while (1)
  {/*Main loop*/
    while (!got_sigUSR1) /*Suspend the process waiting for SIGUSR1*/
      sigsuspend(&oldmask);
    got_sigUSR1 = 0;

    /* Unblocking SIG_TERM */
    if(sigprocmask(SIG_UNBLOCK, &mask2, NULL)==ERROR)
        _error_in_voters();
    #ifdef DEBUG
      usleep((getpid() % n_procs) * PRIME);
    #endif
      /*Blocking SIGTERM in order to avoid Invalid elections where one or more voters have exited due to SIG_TERM*/
    if(sigprocmask(SIG_BLOCK, &mask2, NULL)==ERROR)
        _error_in_voters();

    /*Proposing a candidate*/
    if (down_try(semC) == ERROR)
    {/*Non candidate*/
      while (!got_sigUSR2)
        sigsuspend(&oldmask);
      got_sigUSR2 = 0;

      /*Exclusion Mutua Votar*/
      down(semV);
      votingCarefully(NOMBREVOTAR);
      up(semV);
    }
    else
    { /*Candidate*/
      candidato(n_procs);

      /*Release the sem to choose a new candidate + send USR1 to start a new voting*/
      up(semC);

#ifdef DEBUG
      sleep(1);
#endif
      if (send_signal_procs(SIGUSR1, n_procs, NO_PID) == ERROR )
        _error_in_voters();
    }

    if (got_sigTERM)
    {
#ifdef DEBUG
      printf("Hijo con PID=%ld sale por señal\n", (long)getpid());
#endif
      sem_close(semV);
      sem_close(semC);

      exit(EXIT_SUCCESS);
    }
  }
}

void candidato(int n_procs)
{
  int reading = 1, i, cont = 0, j;
  char *votes = NULL, result[15];
  FILE *f;

  votes = (char *)malloc((n_procs - 1) * sizeof(char));
  if (!votes)
    _error_in_voters();

  /*Empty voting file*/
  if(!(f = fopen(NOMBREVOTAR, "wb")))
    _error_in_voters();
  fclose(f);

  if (send_signal_procs(SIGUSR2, n_procs, getpid())==ERROR)
    _error_in_voters();
  

  /*Opens file to read the votes*/
  if (!fopen(NOMBREVOTAR, "rb")){
    _error_in_voters();

    free(votes);
  }

  /*Read the votes every 0.1 seconds until everybody votes*/
  while (reading)
  {
    usleep(1000); /*CAMBIAR por un sigsuspend con alarma*/
    fseek(f, 0, SEEK_SET);

    j=fread(votes, sizeof(char), n_procs - 1, f);
    if (j == (n_procs - 1))
      reading = 0;
    else if (j == ERROR)
      _error_in_voters();

  }
  fclose(f);

  /*Print the result of the votation*/
  printf("Candidate %d => [", (int)getpid());

  /*Count the votes*/
  for (i = 0; i < n_procs - 1; i++)
  {
    if (votes[i] == 'Y')
      cont++;
    printf(" %c", votes[i]);
  }

  if (cont <= (n_procs / 2))
    printf(" ] => Rejected\n");
  else
    printf(" ] => Accepted\n");

  usleep(250000); /*Espera no activa de 250ms*/
  free(votes);
}