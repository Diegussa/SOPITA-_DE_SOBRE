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

#define NOMBREFICHERO "hijosPID.txt"
#define NOMBREVOTAR "votos.txt"

volatile sig_atomic_t got_sigUSR1 = 0;
volatile sig_atomic_t got_sigUSR2 = 0;
volatile sig_atomic_t got_sigTERM = 0;
sem_t *semapVotar;

void handler_voter(int sig)
{
  switch (sig)
  {
  case SIGTERM:
    printf("%d == %d SIGTERM %ld \n", sig, SIGTERM, (long)getpid());
    got_sigTERM = 1;
    break;
  case SIGUSR1:
    printf("U1 %ld ", (long)getpid());
    got_sigUSR1 = 1;
    break;
  case SIGUSR2:
    printf("U2 %ld ", (long)getpid());
    got_sigUSR2 = 1;
    break;

  default:
    break;
  }
}

STATUS votingCarefully(char *nFichV)
{
  FILE *f = NULL;
  int r;
  char letra;

  f = fopen(nFichV, "ab+");
  if (!f)
  {
    return ERROR;
  }

  r = rand();
  if (r < (RAND_MAX / 2))
    letra = 'N';
  else
    letra = 'Y';

  fwrite(&letra, sizeof(char), 1, f);
  printf("v %c\n", letra);

  fclose(f);

  return OK;
}

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
/*Returns 0 on succes, -1 on Error and more info in errno*/

/*It is a non blocking down*/
int down_try(sem_t *sem)
{
  return sem_trywait(sem);
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
    if ((pid = (int)fork()) == 0)
    {
      /*Function made for the sons*/
      voters(nameSemV, namesemC, n_procs, semV, semC);
    }
    else if (pid == -1)
    {
      i--;
      send_signal_procs(SIGTERM, i, NO_PID);

      exit(EXIT_FAILURE);
    }

    PIDs[i] = pid;
  }

  /*The writing of the PIDs in the file*/
  fHijos = fopen(NOMBREFICHERO, "wb");
  if (!fHijos)
  {
    free(PIDs);
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }

  fwrite(PIDs, sizeof(pid_t), n_procs, fHijos);
  fclose(fHijos);

  free(PIDs);
}

void send_signal_procs(int sig, int n_hijos, long pid)
{
  int i, ret;
  pid_t *PIDs = NULL;
  FILE *fHijos;

  /*Memory allocation*/
  PIDs = (pid_t *)malloc(n_hijos * sizeof(pid_t));
  if (!PIDs)
  {
    printf("Error allocating memory for PIDs\n");
    exit(EXIT_FAILURE);
  }

  /*Opening and reading the file*/
  fHijos = fopen(NOMBREFICHERO, "rb");
  if (!fHijos)
  {
    free(PIDs);
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }
  fread(PIDs, sizeof(pid_t), n_hijos, fHijos);

  /*Sending the signal to every son*/
  for (i = 0; i < n_hijos; i++)
  {
    if (PIDs[i] != pid)
    {

      ret = kill(PIDs[i], sig);
      if (ret)
      {
        fprintf(stderr, "ERROR sending %d to the son number %led \n", sig, PIDs[i]);
        exit(EXIT_FAILURE);
      }
    }
  }

  fclose(fHijos);
  free(PIDs);
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
    if (WEXITSTATUS(status) == EXIT_FAILURE && 1 == 0)
    {
      printf("Error waiting\n");
      exit(EXIT_FAILURE);
    }
    printf("EXIT_STATUS %d\n", WEXITSTATUS(status));
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

void voters(char *nameSemV, char *nameSemC, int n_procs, sem_t *semV, sem_t *semC)
{
  int i = 0, val;
  struct sigaction actSIG;
  sigset_t mask, mask2, oldmask;

  srand((int)getpid());
  /*Block permanently SIGINT*/
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigprocmask(SIG_BLOCK, &mask, NULL);

  /*Block temporarily SIGUSR1. SIGUSR2, SIGTERM*/
  sigemptyset(&mask2);
  sigaddset(&mask2, SIGUSR1);
  sigaddset(&mask2, SIGUSR2);
  sigaddset(&mask2, SIGTERM);

  sigprocmask(SIG_BLOCK, &mask2, &oldmask);

  /*Set the new signal handler*/
  actSIG.sa_handler = handler_voter;
  sigemptyset(&(actSIG.sa_mask));
  actSIG.sa_flags = 0;

  /*quitar*/
  if (sigaction(SIGINT, &actSIG /*Esto deberiás ser su propoa sigaction*/, NULL) < 0)
  {
    perror("sigaction SIGINT (IGNORE)\n");
    end_failure(semV, semC);
  }
  /*quitar*/

  if (sigaction(SIGTERM, &actSIG, NULL) < 0)
  {
    perror("sigaction SIGTERM\n");
    end_failure(semV, semC);
  }

  if (sigaction(SIGUSR1, &actSIG, NULL) < 0)
  {
    perror("sigaction SIGUSR1\n");
    end_failure(semV, semC);
  }

  if (sigaction(SIGUSR2, &actSIG, NULL) < 0)
  {
    perror("sigaction SIGUSR2\n");
    end_failure(semV, semC);
  }

  /*Unblock SIGUSR1. SIGUSR2, SIGTERM*/
  sigprocmask(SIG_UNBLOCK, &mask2, NULL);

  /*Main bucle*/
  while (i < 10)
  {
    i++;
    /*Mask to block signals SIGUSR1*/
    while (!got_sigUSR1)
    {
      sigsuspend(&oldmask);
    }

    got_sigUSR1 = 0;
    sem_getvalue(semC, &val);
    printf("predown %d\n", val);
    sleep((rand() % 100) / 100);

    /*Proposing a candidate*/
    if (down_try(semC) == -1)
    {
      /*Non candidate*/
      printf("postdown %d\n", val);

      while (!got_sigUSR2)
      {
        sigsuspend(&oldmask);
      }

      got_sigUSR2 = 0;
      /*Exclusion Mutua Votar*/
      printf("Votante=%ld\n", (long)getpid());
      down(semV);
      votingCarefully(NOMBREVOTAR);
      up(semV);
    }
    else
    { /*Candidate*/
      candidato(n_procs);
      up(semC);
      send_signal_procs(SIGUSR1, n_procs, NO_PID);
    }
    if (got_sigTERM)
    {
      got_sigTERM = 0;

      printf("Hijo con PID=%ld sale por señal\n", (long)getpid());
      sigprocmask(SIG_UNBLOCK, &mask, NULL);

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
  {
    /*Mirar*/
  }

  /*Empty voting file*/
  f = fopen(NOMBREVOTAR, "wb");
  fclose(f);

  send_signal_procs(SIGUSR2, n_procs, getpid());

  /*Opens file to read the votes*/
  f = fopen(NOMBREVOTAR, "rb");
  if (!f)
  {
    /*MIRAR*/
    free(votes);
  }

  /*Read the votes every 0.1 seconds until everybody votes*/
  while (reading)
  {
    sleep(0.1); /*CAMBIAR por un sigsuspend con alarma*/
    fseek(f, 0, SEEK_SET);

    j = fread(votes, sizeof(char), n_procs - 1, f);
    if (j == (n_procs - 1))
    {
      reading = 0;
    }
  }
  fclose(f);

  /*Print the result of the votation*/
  printf("Candidate %d => [", (int)getpid());
  /*Count the votes*/
  for (i = 0; i < n_procs - 1; i++)
  {
    if (votes[i] == 'Y')
    {
      cont++;
    }
    printf(" %c", votes[i]);
  }

  if (cont <= (n_procs / 2))
  {
    strcpy(result, "Rejected");
  }
  else
  {
    strcpy(result, "Accepted");
  }

  printf(" ] => %s\n", result);

  free(votes);
}