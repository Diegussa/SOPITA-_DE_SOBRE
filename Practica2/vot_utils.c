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


/*Returns 0 on succes, -1 on Error and more info in errno*/
int up(sem_t *sem)
{
    return sem_post(sem);
}

/*Returns 0 on succes, -1 on Error and more info in errno*/
int down(sem_t *sem)
{
    return sem_wait(sem);
}

/*It is a non blocking down*/
int down_try(sem_t *sem)
{
    return sem_trywait(sem);
}

void end_processes(int n_procs)
{
    int i, status;
    /*Enviar a cada hijo un SIGTERM*/
    send_signal_procs(SIGTERM, n_procs, NO_PID);
    /*Waiting fot the voters*/
    for (i = 0; i < n_procs; i++)
    {
        wait(&status);
        if (WEXITSTATUS(status) == EXIT_FAILURE)
            printf("Error waiting\n");
#ifdef DEBUG
        printf("EXIT_STATUS %d\n", WEXITSTATUS(status));
#endif
    }
}

void end_failure(sem_t *semV, sem_t *semC)
{
    if (semV && semC)
    {
        sem_close(semV);
        sem_close(semC);
    }

    exit(EXIT_FAILURE);
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
    if (PIDs[i] != pid)
        kill(PIDs[i], sig);
  }

  fclose(fHijos);
  free(PIDs);

  return OK;
}