#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

#include "vot_func.h"

#define NOMBREFICHERO "hijosPID.txt"
#define NOMBREVOTAR "votos.txt"

volatile sig_atomic_t got_sigUSR1 = 0;
volatile sig_atomic_t got_sigTERM = 0;
sem_t *semapVotar;

void handlerSIGUSR1(int sig)
{

  printf("%d == %d SIGUSR1 %ld\n", sig, SIGUSR1, (long)getpid());
  got_sigUSR1 = 1;
}

void handlerSIGTERM(int sig)
{
  if (sig == SIGTERM)
  {
    printf("%d == %d SIGTERM %ld \n", sig, SIGTERM, (long)getpid());
    got_sigTERM = 1;
  }
}

STATUS votar(char *nFichV)
{
  FILE *f = NULL;
  int r;
  char letra;

  f = fopen(nFichV, "wb+");
  if (!f)
  {
    return ERROR;
  }

  srand(time(NULL));
  r = rand();
  if (r < (RAND_MAX / 2))
    letra = 'N';
  else
    letra = 'Y';

  fwrite(&letra, sizeof(char), 1, f);
  /*fwrite(' ', sizeof(char), 1, f);*/

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

/*It is a non blocking down
int down_try(sem_t *sem)
{
  return sem_waittry(sem);
}*/

/*Generates n_procs and writes their PID in a file*/
void create_sons(int n_procs)
{
  int pid = 0, i;
  FILE *fHijos;

  fHijos = fopen(NOMBREFICHERO, "wb");
  if (!fHijos)
  {
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < n_procs; i++)
  {
    if ((pid = fork()) == 0)
    {
      /*Function made for the sons*/
      voters();
    }
    if (pid == -1)
    {
      for (i--; i >= 0; i--)
      {
        send_signal_procs(SIGTERM, i);
      }
      exit(EXIT_FAILURE);
    }

    fwrite(&pid, sizeof(pid_t), 1, fHijos);
  }
  fclose(fHijos);
}

void send_signal_procs(int sig, int n_hijos)
{
  int pid = 0, i, ret;
  FILE *fHijos;

  fHijos = fopen(NOMBREFICHERO, "rb");
  if (!fHijos)
  {
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < n_hijos; i++)
  {
    fread(&pid, sizeof(pid_t), 1, fHijos);
    ret = kill(pid, sig);
    if (ret)
    {
      fprintf(stderr, "ERROR sending SIGUSR1 to the son number %d \n", i + 1);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fHijos);
}

void voters()
{

  int i = 0;
  struct sigaction actSIGTERM;
  struct sigaction actSIGUSR1;
  struct sigaction actSIGINT;
  sigset_t mask, mask2, oldmask;

  /*Blocking SIG_INT*/
  /*sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);*/

  /*Quitar
  actSIGTERM.sa_handler = quitar;
  sigemptyset(&(actSIGTERM.sa_mask));
  actSIGTERM.sa_flags = 0;

  if (sigaction(SIGINT, &actSIGTERM, NULL) < 0)
  {
    perror("sigaction SIGTERM");
    exit(EXIT_FAILURE);
  }
  /*Quitar*/
  /*
  actSIGINT.sa_handler=ignore;
  sigemptyset(&(actSIGINT.sa_mask));
  actSIGINT.sa_flags == 0;
  */

  /* Preguntar a Javi por esto, cuando lo pongo, los procesos están zombies: sleep(10);*/

  actSIGUSR1.sa_handler = handlerSIGUSR1;
  sigemptyset(&(actSIGUSR1.sa_mask));
  actSIGUSR1.sa_flags = 0;

  actSIGTERM.sa_handler = handlerSIGTERM;
  sigemptyset(&(actSIGTERM.sa_mask));
  actSIGTERM.sa_flags = 0;

  if (sigaction(SIGINT, &actSIGTERM /*Esto deberiás ser su propoa sigaction*/, NULL) < 0)
  {
    perror("sigaction SIGINT (IGNORE)");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGTERM, &actSIGTERM, NULL) < 0)
  {
    perror("sigaction SIGTERM");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGUSR1, &actSIGUSR1, NULL) < 0)
  {
    perror("sigaction SIGUSR1");
    exit(EXIT_FAILURE);
  }

  printf("Soy el hijo: %ld\n", (long)getpid());

  /*Mask to block signals SIGUSR1*/
  sigemptyset(&mask2);
  sigaddset(&mask2, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask2, &oldmask);
  while (!got_sigUSR1)
    sigsuspend(&oldmask);

  sigprocmask(SIG_UNBLOCK, &mask2, NULL);
  /* Blocking of signals SIGUSR1 and SIGUSR2 in the process. */

  /*sigemptyset(&mask);
  sigaddset(&mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);*/
  while (!got_sigTERM)
    ;
  /*  sigsuspend(&mask);
  printf("Sigterm recibido\n");*/

  printf("Hijo con PID=%ld sale por señal\n", (long)getpid());
  sigprocmask(SIG_UNBLOCK, &mask, NULL);

  exit(0);
}
