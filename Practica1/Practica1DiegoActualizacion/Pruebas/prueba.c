#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define POW_LIMIT 99997669
#define MAX POW_LIMIT - 1

void *func_minero(void *arg)
{
    int i;
    long x =2;
    x=(*(int*)arg)*x;
    pthread_exit((void *)x);
}

int main(){
    int i,K,t1[2]={10,2},a;
    pthread_t thread1,thread2;
    int *sol[2];
   
     K = pthread_create(&thread1, NULL, func_minero, (void *)t1);
     if(K){
        printf("Error 1\n");
        return EXIT_FAILURE;
     }
    K = pthread_create(&thread2, NULL, func_minero, (void *)(t1+1));
    if(K){
        printf("Error 2\n");
        return EXIT_FAILURE;
     }
     
     K = pthread_join(thread1, sol);
     if(K){
        printf("Error 3\n");
        return EXIT_FAILURE;
     }
      
     K = pthread_join(thread2, sol+1);
     if(K){
        printf("Error 4\n");
        return EXIT_FAILURE;
     }
     printf (" %d=2*%d %d=2*%d",sol[0],t1[0],sol[1],t1[1]);
     return EXIT_SUCCESS;
}