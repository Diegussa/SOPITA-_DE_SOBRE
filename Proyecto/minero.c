/*La red se compone de un conjunto de mineros*/

/*Cada minero est´a formado por dos procesos: el proceso padre, Minero, encargado de resolver
la POW usando m´ultiples hilos en paralelo, y el proceso hijo, Registrador, encargado de
mantener actualizado un fichero de registro. Estos procesos se comunican por medio de
una tuberıa*/
/*Realizado en la practica 1 cambiando el proceso monitor por el porceso Registrador*/

/*El minero que consiga ser el primero en resolver la POW solicita al resto de mineros que
validen su solucion mediante una votacion, obteniendo una moneda si se aprueba. Ademas,
envia el bloque resultante al monitor, si est´a activo, a trav´es de una cola de mensajes.*/

/*Los mineros pueden acabar y salir de la red en cualquier momento.
La informaci´on sobre la red debe adaptarse de forma din´amica a los cambios anteriores.*/

/*Cola de mensajes Se utiliza una cola de mensajes para realizar la comunicaci´on entre los
mineros y el monitor.*/

/**
 * @file minero.c
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>

#include "funciones_minero.h"
#include "pow.h"
#include "utils.h"

#define SIZE 7 /*Debe ser menor o igual a 10*/
#define MQ_NAME "/mqueue"
#define nameSemNmin "/semNmin"
#define MAX_THREADS 100
#define MAX_SECS 100

#define MAX POW_LIMIT - 1

int main(int argc, char *argv[])
{
    int n_seconds, n_threads, i, pid, st, pipeReg_Min[2], pipeMin_Reg[2]; /*Indica la posición a insertar el siguiente mensaje en el array msg*/
    struct mq_attr attributes;
    mqd_t mq;
    Message msg;
    sem_t *mutex_nmin;

    if (argc != 3) /*Control de parámetros de entrada*/
    {
        fprintf(stderr, "Usage: %s <ROUNDS> <LAG>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((n_seconds = atoi(argv[1])) < 1 || n_seconds > MAX_SECS || (n_threads = atoi(argv[2])) < 1 || n_threads > MAX_THREADS)
    {
        fprintf(stderr, "%d < <N_SECONDS> < %d and %d < <N_TRHEADS> < %d\n", 1, MAX_SECS, 1, MAX_THREADS);
        exit(EXIT_FAILURE);
    }

    /*Creación de semáforos*/

    /*Creación de Pipes*/
    /*Creación de las pipeline REG->MIN y MIN->REG*/
    if (pipe(pipeReg_Min) == -1)
        error("pipe creation");

    if (pipe(pipeMin_Reg) == -1)
        error("pipe creation");

    if ((mutex_nmin = sem_open(nameSemNmin, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
    {
        if ((mutex_nmin = sem_open(nameSemNmin, O_CREAT, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
        {
            error("Error openning sem");
            exit(EXIT_FAILURE);
        }
    }
    /*Creación del proceso Registrador*/
    if ((pid = fork()) == -1)
        error("Error forking");

    else if (pid) /*Minero*/
    {
        /*Cierro los ficheros que no vamos a usar*/
        close(pipeReg_Min[1]); /*Escritura REG->MIN*/
        close(pipeMin_Reg[0]); /*Lectura MIN->REG*/
        minero(n_threads, n_seconds, pid, pipeReg_Min[0], pipeMin_Reg[1], mutex_nmin);
        wait(&st);
        if (WIFEXITED(st) == EXIT_FAILURE)
            error("Registrador exited with ERROR\n");
    }
    else /*Registrador*/
    {
        exit(EXIT_SUCCESS);
        /*Cierro los ficheros que no vamos a usar*/
        close(pipeMin_Reg[1]); /*Escritura MIN->REG*/
        close(pipeReg_Min[0]); /*Lectura REG->MIN*/
        /*El encargado de registrar los bloques en un fichero. Comunicación mediante pipes*/
        registrador(pipeMin_Reg[0], pipeReg_Min[1]);
    }

    exit(EXIT_SUCCESS);
}