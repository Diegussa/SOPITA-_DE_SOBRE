#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROCS 1000
#define NOMBREFICHERO "hijosPID.txt"
volatile sig_atomic_t got_sigINT = 0;
volatile sig_atomic_t got_sigUSR1 = 0;
volatile sig_atomic_t got_sigTERM = 0;
/*Returns 0 on succes, -1 on Error and more info in errno*/

int up(sem_t *sem)
{
  // return sem_post(sem);
}

/*Returns 0 on succes, -1 on Error and more info in errno*/
int down(sem_t *sem)
{
  // return sem_wait(sem);
}
/*Returns 0 on succes, -1 on Error and more info in errno*/
/*It is a non blocking down*/
int down_try(sem_t *sem)
{
  // return sem_waittry(sem);
}

void handlerSIGINT(int sig)
{
  printf("\n%d == %d SIGINT %ld\n", sig, SIGINT, (long)getpid());
  got_sigINT = 1;
}

void handlerSIGUSR1(int sig)
{

  printf("%d == %d SIGUSR1 %ld\n", sig, SIGUSR1, (long)getpid());
  got_sigUSR1 = 1;
}

void handlerSIGTERM(int sig)
{
  printf("%d == %d SIGTERM %ld \n", sig, SIGTERM, (long)getpid());
  got_sigTERM = 1;
}

void quitar(int sig){
  printf("%d == %d SIGINT %ld \n", sig, SIGINT, (long)getpid());
}

void voter();
void send_signal_sons(int sig, int n_hijos);

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
      voter();
    }
    if (pid == -1)
    {
      for (i--; i >= 0; i--)
      {
        send_signal_sons(SIGTERM, i);
      }
      exit(EXIT_FAILURE);
    }

    fwrite(&pid, sizeof(pid_t), 1, fHijos);
  }
  fclose(fHijos);
}

void send_signal_sons(int sig, int n_hijos)
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
  send_signal_sons(SIGUSR1, n_procs);

  /*Change the SIGINT handler*/
  actSIGINT.sa_handler = handlerSIGINT;
  sigemptyset(&(actSIGINT.sa_mask));
  actSIGINT.sa_flags = 0;
  if (sigaction(SIGINT, &actSIGINT, NULL) < 0)
  {
    perror("sigaction SIGINT");
    exit(EXIT_FAILURE);
  }

  while (!got_sigINT)
    ;

  /*Enviar a cada hijo un SIGTERM*/
  send_signal_sons(SIGTERM, n_procs);

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

void voter()
{

  int i = 0;
  struct sigaction actSIGTERM;
  struct sigaction actSIGUSR1;
  sigset_t mask, oldmask;

  /*Quitar*/
  actSIGTERM.sa_handler = quitar;
  sigemptyset(&(actSIGTERM.sa_mask));
  actSIGTERM.sa_flags = 0;

  if (sigaction(SIGINT, &actSIGTERM, NULL) < 0)
  {
    perror("sigaction SIGTERM");
    exit(EXIT_FAILURE);
  }
  /*Quitar*/
 
  actSIGUSR1.sa_handler = handlerSIGUSR1;
  sigemptyset(&(actSIGUSR1.sa_mask));
  actSIGUSR1.sa_flags = 0;

  actSIGTERM.sa_handler = handlerSIGTERM;
  sigemptyset(&(actSIGTERM.sa_mask));
  actSIGTERM.sa_flags = 0;

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
  printf("Soy el hijo %ld\n", (long)getpid());

  /*Mask to block signals SIGUSR1*/
  sigemptyset(&mask);
  sigaddset(&mask, SIGUSR1);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
  while (!got_sigUSR1)
    sigsuspend(&oldmask);

  sigprocmask(SIG_UNBLOCK, &mask, NULL);
  /* Blocking of signals SIGUSR1 and SIGUSR2 in the process. */

  sigemptyset(&mask);
  sigaddset(&mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);
  while (!got_sigTERM)
    sigsuspend(&mask);
  printf("Sigterm recibido\n");

  printf("Hijo con PID=%ld sale por señal\n", (long)getpid());
  exit(0);
}