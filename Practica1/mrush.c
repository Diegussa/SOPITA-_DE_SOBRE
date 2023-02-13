#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX 100000000

typedef struct
{
    long ep, eu, res;
} entradaHash;

void *print_message_function(void *threadarg);

int main(int argc, char *argv[]) {
   pthread_t thread1;
   int rc;
   entradaHash t;
   void *status;

/*Creacion del hilo*/
    t.ep = 0;
    t.eu = MAX;
    t.res = atoi(argv[1]);

   rc = pthread_create(&thread1, NULL, print_message_function, (void *)&t);
   if (rc) {
      printf("Error creating thread 1\n");
      exit(-1);
   }

   rc = pthread_join(thread1, &status);
   if (rc) {
      printf("Error joining thread 1\n");
      exit(-1);
   }

   printf("Thread 1 returned value: %ld\n", (long)status);

   return 0;
}

void *print_message_function(void *arg) {
    int i;
    long x=-1;
    entradaHash *e=(entradaHash*)arg;

    for (i = e->ep; (i < e->eu) && (x <= 0); i++)
    {
        if ((long)pow_hash(i) == e->res)
        {
            x = i;
        }
    }

   pthread_exit((void *)x);
}