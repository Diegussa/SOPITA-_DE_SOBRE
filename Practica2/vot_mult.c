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
    printf("\n%d == %d\n",sig, SIGINT);
    got_sigINT = 1;
  
  
}

void handlerSIGUSR1(int sig)
{

    printf("%d == %d\n",sig, SIGUSR1);
  got_sigUSR1 = 1;
}

void handlerSIGTERM(int sig)
{
    printf("%d == %d\n",sig, SIGTERM);
  got_sigTERM = 1;
}

void votante();

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

  /*Generates n_procs and writes their PID in a file*/
  fHijos = fopen(NOMBREFICHERO, "wb");
  if (!fHijos)
  {
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < n_procs; i++)
  {
    if ((padre = fork()) == 0)
    {
      /*Function made for the sons*/
      votante();
    }
    fwrite(&padre, 1, sizeof(int), fHijos);
  }
  fclose(fHijos);

  /*Send every son the signal SIGUSR1*/
  fHijos = fopen(NOMBREFICHERO, "rb");
  if (!fHijos)
  {
    fprintf(stderr, "ERROR opening the file %s to read\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }
  for (i = 0; i < n_procs && padre != 0; i++)
  {
    /*Enviar a cada hijo un SIGUSR1*/
    fread(&pid, sizeof(int), 1, fHijos);
    ret = kill(pid, SIGUSR1);
    if (ret)
    {
      fprintf(stderr, "ERROR sending SIGUSR1 to the son number %d \n", i + 1);
      exit(EXIT_FAILURE);
    }
  }
  fclose(fHijos);

  /*Change the SIGINT handler*/
  actSIGINT.sa_handler = handlerSIGINT;
  sigemptyset(&(actSIGINT.sa_mask));
  actSIGINT.sa_flags = 0;
  if (sigaction(SIGINT, &actSIGINT, NULL) < 0)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  /**/
  if (!(fHijos = fopen(NOMBREFICHERO, "rb")))
  {
    fprintf(stderr, "ERROR opening the file %s to read\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }

  while (!got_sigINT);

  for (i = 0; i < n_procs && padre != 0; i++)
  {
    /*Enviar a cada hijo un SIGTERM*/
    fread(&pid, sizeof(int), 1, fHijos);
    printf("PID: %d\n", pid);
    ret = kill(pid, SIGTERM);
    if (ret)
    {
      fprintf(stderr, "ERROR sending SIGTERM to the son number %d \n", i + 1);
      exit(EXIT_FAILURE);
    }
  }
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

  fclose(fHijos);

  return 0;
}

void votante()
{
  int i = 0;
  struct sigaction actSIGTERM, actSIGUSR1;
  sigset_t mask, oldmask;

  actSIGUSR1.sa_handler = handlerSIGUSR1;
  sigfemptyset(&(actSIGUSR1.sa_mask));
  sigaddset(SIGTERM, &(actSIGUSR1.sa_mask));
  actSIGUSR1.sa_flags = 0;

  actSIGTERM.sa_handler = handlerSIGTERM;
  sigemptyset(&(actSIGTERM.sa_mask));
  sigaddset(SIGUSR1, &(actSIGTERM.sa_mask));
  actSIGTERM.sa_flags = 0;

  if (sigaction(SIGTERM, &actSIGTERM, NULL) < 0)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGUSR1, &actSIGUSR1, NULL) < 0)
  {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

 /* Mask to block signals SIGUSR1 and SIGUSR2 . */
 sigemptyset (&mask);
 sigaddset (&mask, SIGUSR1);
 sigprocmask (SIG_BLOCK, &mask, &oldmask);

  while (!got_sigUSR1)
    sigsuspend (&oldmask);
    
  sigprocmask (SIG_UNBLOCK, &mask, NULL);
    /* Blocking of signals SIGUSR1 and SIGUSR2 in the process. */
 
  printf("Hijo con PID=%ld\n", (long)getpid());

  
  

  while (!got_sigTERM)
  {
  };
  printf("2\n");
  printf("Hijo con PID=%ld sale por señal\n", (long)getpid());
  exit(0);
}