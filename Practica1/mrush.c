#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <sys/wait.h>
#include "pow.h"

#define MAX POW_LIMIT - 1
#define MAX_HILOS 10

typedef struct
{
    long ep, eu, res;
} entradaHash;

void *func_minero(void *arg);

int minero(int nHilos, long busq, int rondas);

int main(int argc, char *argv[])
{
    int rc[MAX_HILOS], t1, t2, i, status;

    /*Control de errores*/
    t1 = clock();
    if ((atoi(argv[1]) < 0) || (atoi(argv[1]) > POW_LIMIT) || (atoi(argv[2]) < 0) || (atoi(argv[3]) < 0) || (atoi(argv[3]) > MAX_HILOS))
    {
        printf("\n\nError en los parametros de entrada");
        return 1;
    }

    /*Buscamos los elementos*/
    status = minero(atoi(argv[3]), atol(argv[1]), (atoi(argv[2])));
    if (status == EXIT_SUCCESS)
    {
        printf("\nMiner exited with status 0");
    }
    else
    {
        printf("\nMiner exited with status 1");
    }

    t2 = clock();
    printf("\nTime spent %d\n", t2 - t1);

    return 0;
}

void *func_minero(void *arg)
{
    int i;
    long x = -1;
    entradaHash *e = (entradaHash *)arg;

    for (i = e->ep; (i < e->eu) && (x <= 0); i++)
    {
        if ((long)pow_hash(i) == e->res)
        {
            x = i;
        }
    }

    pthread_exit((void *)x);
}

int minero(int nHilos, long busq, int rondas)
{
    int i, j, rc[MAX_HILOS], pipe1[2], pipe2[2], nbytes, exitV;
    entradaHash t[MAX_HILOS];
    long parPH[2], comp[2] = {0, 0};
    pthread_t threads[MAX_HILOS];
    void *sol[MAX_HILOS];
    pid_t childpid;

    /*Creacion de las dos pipes de comunicacion*/
    if (pipe(pipe1) == -1)
    {
        perror("pipe1");
        exit(EXIT_FAILURE);
    }
    if (pipe(pipe2) == -1)
    {
        perror("pipe2");
        exit(EXIT_FAILURE);
    }

    childpid = fork(); /*0 si es el hijo y pid del hijo en el caso del padre*/

    if (childpid == -1)
    { /*Control de errores*/
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (childpid == 0) /*Caso hijo (terminal)*/
    {
        close(pipe1[1]);
        close(pipe2[0]);

        for (j = 0; j < rondas; j++)
        {
            do
            {
                nbytes = read(pipe1[0], parPH, sizeof(long) * 2);
                if (nbytes == -1)
                {
                    printf("Error");
                    exit(EXIT_FAILURE);
                }
            } while (nbytes == 0);

            /*Impresion de la solucion*/
            if (pow_hash(parPH[1]) == parPH[0])
            {
                printf("\nSolution accepted: %08ld --> %08ld", parPH[0], parPH[1]);
                nbytes = write(pipe2[1], parPH, sizeof(long) * 2);
            }
            else
            {
                printf("\nSolution rejected: %08ld !-> %08ld", parPH[0], parPH[1]);
                printf("\nThe solution has been invalidated");
                parPH[0] = -1;
                parPH[1] = -1;
                nbytes = write(pipe2[1], parPH, sizeof(long) * 2);
                exit(EXIT_FAILURE);
                return -1;
            }
        }

        exit(EXIT_SUCCESS);
    }
    else
    {
        close(pipe1[0]);
        close(pipe2[1]);

        for (j = 0; j < rondas; j++)
        {
            /*Creacion de los hilos*/
            for (i = 0; i < nHilos; i++)
            {
                t[i].ep = ((long)MAX / nHilos) * i;
                t[i].eu = ((long)MAX / nHilos) * (i + 1);
                t[i].res = busq;
                rc[i] = pthread_create(&threads[i], NULL, func_minero, (void *)&t[i]);
                if (rc[i])
                {
                    printf("Error creando el hilo %d", i);
                    return 1;
                }
            }

            /*Joins de los hilos*/
            for (i = 0; i < nHilos; i++)
            {
                rc[i] = pthread_join(threads[i], &sol[i]);
                if (rc[i])
                {
                    printf("Error joining thread %d\n", i);
                    return -1;
                }
            }

            /*Almacenar la solucion de los hilos*/
            for (i = 0; i < nHilos; i++)
            {
                if ((long)sol[i] != -1)
                {
                    parPH[0] = (long)busq;
                    parPH[1] = (long)sol[i];
                    nbytes = write(pipe1[1], parPH, sizeof(long) * 2);
                    i = nHilos; /*Para salir del bucle*/
                }
            }

            do
            {
                nbytes = read(pipe2[0], comp, sizeof(long) * 2);
                if ((nbytes == -1) || comp[0] == -1)
                {
                    printf("Error leyendo pipe2");
                    exit(EXIT_FAILURE);
                    return -1;
                }
            } while (comp[0] != parPH[0]);
            busq = parPH[1];
        }
    }

    wait(&exitV); // espera a que el proceso hijo termine
    
    if (exitV == EXIT_FAILURE)
    {
        printf("\nMonitor exited with status 1");
        return EXIT_FAILURE;
    }

    printf("\nMonitor exited with status 0");
    return EXIT_SUCCESS;
}