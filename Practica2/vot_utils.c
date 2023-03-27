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

#include "vot_utils.h"

/*Adds 1 unit to the semaphore*/
int up(sem_t *sem)
{
    return sem_post(sem);
}

/*Blocking call to get the semaphore*/
int down(sem_t *sem)
{
    return sem_wait(sem);
}

/*It is a non blocking down*/
int down_try(sem_t *sem)
{
    return sem_trywait(sem);
}

/*Finishes the processes and waits for the sons*/
void end_processes(int n_procs)
{
    int i, status;
    /*Enviar a cada hijo un SIGTERM*/
    if(send_signal_procs(SIGTERM, n_procs, NO_PID)==ERROR){
      return;
    }

    /*Waiting fot the voters*/
    for (i = 0; i < n_procs; i++)
    {
        wait(&status);
#ifdef DEBUG
        printf("EXIT_STATUS %d\n", WEXITSTATUS(status));
#endif
    }
}

/*Closes the semaphores and exits with failure*/
void end_failure(sem_t *semV, sem_t *semC)
{
    if (semV)
      sem_close(semV);
    if(semC)
      sem_close(semC);

    exit(EXIT_FAILURE);
}

/*Changes the handler of the specified signals*/
STATUS set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask, void (*handler)(int))
{
  sigset_t mask2;
  int i = 0;

  sigemptyset(&mask2);
  for (i = 0; i < n_signals; i++)
  {
    if (sigaddset(&mask2, sig[i]) == ERROR)
      return ERROR;
  }
  /*Unblock signals*/
  if (sigprocmask(SIG_BLOCK, &mask2, oldmask) == ERROR)
    return ERROR;

  /*Set the new signal handler*/
  actSIG->sa_handler = handler;
  sigemptyset(&(actSIG->sa_mask));
  actSIG->sa_flags = 0;
  
  for (i = 0; i < n_signals; i++)
  {
    if (sigaction(sig[i], actSIG, NULL) == ERROR)
      return ERROR;
  }
  /*Unblock signals*/
  if (sigprocmask(SIG_UNBLOCK, &mask2, NULL) == ERROR)
    return ERROR;
}

/*Sends a signal to all the sons except the one specified in the third argument*/
STATUS send_signal_procs(int sig, int n_hijos, long pid)
{
  int i;
  pid_t *PIDs = NULL;
  FILE *fHijos;

  /*Memory allocation*/
  PIDs = (pid_t *)malloc(n_hijos * sizeof(pid_t));
  if (!PIDs)
  {
    printf("Error allocating memory for PIDs\n");
    return ERROR;
  }

  /*Opening and reading the file*/
  fHijos = fopen(NOMBREFICHERO, "rb");
  if (!fHijos)
  {
    free(PIDs);
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    return ERROR;
  }
  if(fread(PIDs, sizeof(pid_t), n_hijos, fHijos) == -1)
    return ERROR;

  /*Sending the signal to every son*/
  for (i = 0; i < n_hijos; i++)
  {
    if (PIDs[i] != pid){
        if(kill(PIDs[i], sig) == ERROR){
            return ERROR;
        }
          
    }
        
  }

  fclose(fHijos);
  free(PIDs);

  return OK;
}