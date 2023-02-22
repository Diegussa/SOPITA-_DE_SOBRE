#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define POW_LIMIT 99997669
#define MAX POW_LIMIT - 1
int a;

void *func_minero(void *arg)
{
    int i,*m=(int*)arg;
    long x =-1;

    for (i = 0; (i < 1000000) && (x <= 0) && (a == 0); i++)
    {
        

        if ((long)i == (long)*m)
        {
            a=1;
            x=i;
        }
    }
    printf("%d\n",i);
    pthread_exit((void *)x);
}

int main(){
    int i,K,t1[100];
    pthread_t threads[100];
    int *sol[100];
    a=0;
   
     /*Creacion de los hilos*/
    for (i = 0; i < 100; i++)
    {
         t1[i]=100000000;
         if( i == 73)   
            t1[i]= 17;
         
        K= pthread_create(&threads[i], NULL, func_minero, (void *)&t1[i]);
        if (K)
        {
            printf("Error creando el hilo %d", i);
            return 1;
        }
    }

    /*Joins de los hilos*/
    for (i = 0; i < 100; i++)
    {
        K= pthread_join(threads[i], &sol[i]);
        if (K)
        {
            printf("Error joining thread %d\n", i);
            return -1;
        }
    }

    for (i = 0; i < 100; i++)
    {
        if ((long)sol[i] != -1)
        {
            printf("%d %d",i, (long)sol[i]);

        }
    }
     return EXIT_SUCCESS;
}