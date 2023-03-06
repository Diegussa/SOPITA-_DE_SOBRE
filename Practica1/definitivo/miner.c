#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pow.h"
#include "miner.h"

int encontrado;

struct _entradaHash
{
    long ep, eu, res;
};

void *func_minero(void *arg)
{
    int i;
    long x = -1;
    entradaHash *e = (entradaHash *)arg;

    for (i = e->ep; (i < e->eu) && (x <= 0) && (encontrado == 0); i++)
    {
        if ((long)pow_hash(i) == e->res)
        {
            x = i;
            encontrado = 1;
        }
    }

    pthread_exit((void *)x);
}

void minero(int nHilos, long nbusquedas, long busq)
{
    int newpid, status, pipeMon_min[2], pipeMin_mon[2];
    /*Creación de las pipeline MON->MIN y MIN->MON*/
    status = pipe(pipeMon_min);
    if (status == -1)
    {
        perror("pipe creation");
        exit(EXIT_FAILURE);
    }
    status = pipe(pipeMin_mon);
    if (status == -1)
    {
        perror("pipe creation");
        exit(EXIT_FAILURE);
    }
    /*Creacion del proceso monitor*/
    newpid = fork();
    if (newpid)
    {
        /*Proceso MIN*/
        /*Cierro los ficheros que no vamos a usar*/
        close(pipeMon_min[1]); /*Escritura MON->MIN*/
        close(pipeMin_mon[0]); /*Lectura MIN->MON*/
                               /*Funcion MINERO que crea todos los hilos*/
        if (minar(nHilos, nbusquedas, busq, pipeMon_min[0], pipeMin_mon[1]) == 0)
        {
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        /*Código del hijo, monitor*/
        /*Cierro los ficheros que no vamos a usar*/
        close(pipeMin_mon[1]); /*Escritura MIN->MON*/
        close(pipeMon_min[0]); /*Lectura MON->MIN*/
                               /*Funcion MONITOR */
        monitor(pipeMin_mon[0], pipeMon_min[1], nbusquedas);
    }
}

int monitor(int pipeLectura, int pipeEscritura, int nBusquedas)
{
    int nbytes, Exito = 0;
    long solucion, busq, parSol[2];
    /*Es necesario leer de la tubería la solucion y el numero a buscar */
    /*Creacion del fork monitor-terminal*/

    /*Impresion de la solucion*/
    /*Cierre de lectura al final en el hijo */
    while (1)
    {
        nbytes = read(pipeLectura, &parSol, sizeof(long) * 2);
        if (nbytes == -1)
        {
            perror("read monitor");
            close(pipeEscritura);
            close(pipeLectura);
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0)
        {
            close(pipeEscritura);
            close(pipeLectura);
            if (nBusquedas == 0 && pow_hash(solucion) == busq)
                exit(EXIT_SUCCESS);
            exit(EXIT_FAILURE);
        }
        solucion = (long)parSol[0];
        busq = (long)parSol[1];
        nBusquedas--;
        if (pow_hash(solucion) == busq)
        {
            printf("Solution accepted: %08ld --> %08ld\n", busq, solucion);
            Exito = 1;
        }
        else
        {
            printf("Solution rejected: %08ld !-> %08ld\n", busq, solucion);
            printf("The soluction has been invalidated\n");
            Exito = 0;
        }

        /*Comunicar al minero que ha acertado o no*/

        nbytes = write(pipeEscritura, &Exito, sizeof(int) * 1);
        if (nbytes == -1)
        {
            perror("write monitor");
            close(pipeEscritura);
            close(pipeLectura);
            exit(EXIT_FAILURE);
        }
    }
}

long minar(int nHilos, long nbusquedas, long busq, int pipeLectura, int pipeEscritura)
{

    int i, j, incr, rc[MAX_HILOS], nbytes, Exito = 0, Status;
    entradaHash t[MAX_HILOS];
    long solucion, parSol[2];
    pthread_t threads[MAX_HILOS];
    void *sol[MAX_HILOS];

    incr = ((int)MAX) / nHilos;
    for (j = 0; j < nbusquedas; j++)
    {
        encontrado = 0;
        /*Creación de hilos*/
        for (i = 0; i < nHilos; i++)
        {
            t[i].ep = incr * i;
            t[i].eu = incr * (i + 1);
            if (i == nHilos - 1)
            { /*Para el caso en el que MAX no sea divisible por NHilos*/
                t[i].eu = MAX;
            }
            t[i].res = busq;
            rc[i] = pthread_create(&threads[i], NULL, func_minero, (void *)(t + i));

            if (rc[i])
            {
                perror("Thread creation");
                close(pipeEscritura);
                wait(&Status);
                printf("Monitor exited with status %d\n", Status);
                close(pipeLectura);
                exit(EXIT_FAILURE);
            }
        }
        /*Joins de los hilos*/
        for (i = 0; i < nHilos; i++)
        {
            rc[i] = pthread_join(threads[i], sol + i);
            if (rc[i])
            {
                perror("Thread joining");
                close(pipeEscritura);
                wait(&Status);
                printf("Monitor exited with status %d\n", Status);
                close(pipeLectura);
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < nHilos; i++)
        {
            if ((long)sol[i] != -1)
            {
                solucion = (long)sol[i];
                break;
            }
        }
        parSol[0] = solucion;
        parSol[1] = busq;

        nbytes = write(pipeEscritura, &parSol, sizeof(long) * 2);
        if (nbytes == -1)
        {
            perror("write miner");
            close(pipeEscritura);
            wait(&Status);
            printf("Monitor exited with status %d\n", Status);
            close(pipeLectura);
            exit(EXIT_FAILURE);
        }

        /*Código de comunicación de la solucion al proceso hijo (monitor)*/

        nbytes = read(pipeLectura, &Exito, sizeof(int) * 1);
        if (nbytes == -1)
        {
            perror("read miner");
            close(pipeEscritura);
            wait(&Status);
            printf("Monitor exited with status %d\n", Status);
            close(pipeLectura);
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0)
        {
            perror("read miner 0");
            close(pipeEscritura);
            wait(&Status);
            printf("Monitor exited with status %d\n", Status);
            close(pipeLectura);
            exit(EXIT_FAILURE);
        }

        if (!Exito)
        {
            close(pipeEscritura);
            wait(&Status);
            printf("Monitor exited with status %d\n", Status);
            close(pipeLectura);
            exit(EXIT_FAILURE);
        }
        busq = solucion;
        /*Comprobar que es correcto*/
    }
    close(pipeEscritura);
    wait(&Status);
    printf("Monitor exited with status %d\n", Status);
    close(pipeLectura);
    exit(EXIT_SUCCESS);
    return 0;
}