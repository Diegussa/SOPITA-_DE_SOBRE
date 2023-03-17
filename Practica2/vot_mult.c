#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PROCS 1000

int main(int argc, char *argv[]) {
  int n_procs, n_sec;
  int i;
  int pid_hijos[MAX_PROCS]=0;
  int padre=0;

  if (argc != 3) {
    fprintf(stderr, "Usage: %s -<signal> <pid>\n", argv[0]);
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
    
  }



}