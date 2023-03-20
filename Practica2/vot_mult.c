#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PROCS 1000


/*Returns 0 on succes, -1 on Error and more info in errno*/

int up(sem_t *sem){
 return sem_post(sem);
}

/*Returns 0 on succes, -1 on Error and more info in errno*/
int down( sem_t *sem){
  return sem_wait(sem);
}
/*Returns 0 on succes, -1 on Error and more info in errno*/
/*It is a non blocking down*/
int down_try( sem_t *sem){
  return sem_waittry(sem);
}

int main(int argc, char *argv[]) {
  int n_procs, n_sec;
  int i;
  int pid_hijos[MAX_PROCS]=0;
  int padre=0;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <N_PROCS> <N_SECS>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  n_procs = atoi(argv[1] );
  n_sec = atoi(argv[2]);

    /*Generates n_procs */
  for(i=0;i<n_procs ;i++){
    if((padre=fork())==0){
        break;
    }
    pid_hijos[i]=padre;
  }
    sigsuspend
  for(i=0;i<n_procs && padre!=0; i++ ){
    ret= kill(pid[i],SIGUSR1);
  }



}