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
#include <errno.h>

#include "vot_func.h"

#define NOMBREFICHERO "hijosPID.txt"
#define NOMBREVOTAR "votos.txt"
#define NO_PID -1

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
    printf("%d == %d SIGUSR1 %ld\n", sig, SIGUSR1, (long)getpid());
    got_sigUSR1 = 1;
    break;
  case SIGUSR2:
    printf("%d == %d SIGUSR2 %ld\n", sig, SIGUSR2, (long)getpid());
    got_sigUSR2 = 1;
    break;

  default:
    printf("Me ha llegado SIGINT %d == %d\n", sig, SIGINT);
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
void create_sons(int n_procs, char *nameSemV, char *namesemC, sem_t *semV, sem_t *semC)
{
  int pid = 0, i, *PIDs=NULL;
  FILE *fHijos;

  PIDs=(int*)malloc(n_procs*sizeof(int));
  if(!PIDs){
    printf("Error allocating memory for PIDs\n");
    exit(EXIT_FAILURE);
  }  

  for (i = 0; i < n_procs; i++)
  {
    if ((pid = (int)fork()) == 0)
    {
      /*Function made for the sons*/
      voters(nameSemV, namesemC,n_procs,semV,semC);
    }
    else if (pid == -1)
    {
      i--;
      send_signal_procs(SIGTERM, i,NO_PID);

      exit(EXIT_FAILURE);
    }

    PIDs[i]=pid;
  }

  /*The writing of the PIDs in the file*/
  fHijos = fopen(NOMBREFICHERO, "wb");
  if (!fHijos)
  {
    free(PIDs);
    fprintf(stderr, "ERROR opening the file %s to write\n", NOMBREFICHERO);
    exit(EXIT_FAILURE);
  }

  fwrite(PIDs, sizeof(int), n_procs, fHijos);
  fclose(fHijos);

  free(PIDs);
}

void send_signal_procs(int sig, int n_hijos,long pid)
{
  int i, ret, *PIDs=NULL;
  FILE *fHijos;

  /*Memory allocation*/
  PIDs=(int*)malloc(n_hijos*sizeof(int));
  if(!PIDs){
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
  fread(PIDs, sizeof(int), n_hijos, fHijos);
  
  /*Sending the signal to every son*/
  for (i = 0; i < n_hijos; i++)
  {
    if(PIDs[i]!= pid){
  
      ret = kill(PIDs[i], sig);
      if (ret)
      {
        fprintf(stderr, "ERROR sending SIGUSR1 to the son number %d \n", i + 1);
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
  send_signal_procs(SIGTERM, n_procs,NO_PID);
  /*Waiting fot the voters*/
  for (i = 0; i < n_procs; i++)
  {
    wait(&status);
    if (WEXITSTATUS(status) == EXIT_FAILURE && 1==0)
    {
      printf("Error waiting\n");
      exit(EXIT_FAILURE);
    }
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
  int i = 0;
  struct sigaction actSIG;
  sigset_t mask, mask2, oldmask;

  srand(time(NULL));
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
  if (sigaction(SIGINT, &actSIG/*Esto deberiás ser su propoa sigaction*/, NULL) < 0)
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
  while (1)
  {
    /*Mask to block signals SIGUSR1*/
    while (!got_sigUSR1)
      sigsuspend(&oldmask);
    got_sigUSR1=0;
    
    printf("predown=%ld\n", (long)getpid());

    /*Proposing a candidate*/
    if (down(semC) == -1) /*Non candidate*/
    {
      if(errno == EINTR || !got_sigUSR2 ){

        /*while( errno == EINTR || !got_sigUSR2){ /*Interrupted by a signal but not sigUSR2
            down(semC) == -1 ;
        }*/
        got_sigUSR2 = 0;
        /*Exclusion Mutua Votar*/
        printf("Votante=%ld\n", (long)getpid());
        down(semV);
        votingCarefully(NOMBREVOTAR);
        up(semV);
        
      }else
      { /*ERROR, something that was not a signal made the process to quit the down*/
        end_failure(semV,semC);
      }
    }else
    {/*Candidate*/
      candidato(n_procs);
      
      send_signal_procs(SIGUSR1,n_procs,NO_PID);
      up(semC);
    }

    /* Blocking of signals SIGUSR1 and SIGUSR2 in the process. */

    /*sigemptyset(&mask);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);*/
      printf("Hijo con PID=%ldl\n", (long)getpid());

    if (got_sigTERM)
    {
      got_sigTERM = 0;
      /*  sigsuspend(&mask);
      printf("Sigterm recibido\n");*/

      printf("Hijo con PID=%ld sale por señal\n", (long)getpid());
      sigprocmask(SIG_UNBLOCK, &mask, NULL);

      sem_close(semV);
      sem_close(semC);

      exit(EXIT_SUCCESS);
    }
  }
}


void candidato(int n_procs){
  int reading=1, i, cont=0;
  char *votes=NULL;
  FILE *f;
  
  votes=(char*)malloc((n_procs-1)*sizeof(char));
  if(!votes){
    /*Mirar*/
  }
  
  /*MIrar: mandarlo al final*/

  /*Empty voting file*/
  f=fopen(NOMBREVOTAR,"wb");
  fclose(f);
  send_signal_procs(SIGUSR2,n_procs,getpid());
  
  /*Opens file to read the votes*/
  f=fopen(NOMBREVOTAR,"rb");
  if(!f){
    /*MIRAR*/
    free(votes);
  }
  
  /*Read the votes every 0.1 seconds until everybody votes*/
  while(reading){
      sleep(0.1);/*CAMBIAR por un sigsuspend con alarma*/
      if(fread(votes, sizeof(char), n_procs-1, f)==(n_procs-1)){
        reading=0;
      }
  }

  /*Count the votes*/
  for (i=0; i<n_procs-1; i++){
    if(votes[i]=='Y'){
      cont++;
    }
  }
  
  fclose(f);
  free(votes);
}