
/* Opcionalmente, el sistema puede contar con un monitor, que permite a los mineros
enviar datos informativos sobre la ejecuci´on del sistema, de manera que se cree una salida ´unica
para toda la red */

/*El monitor est´a formado por dos procesos, el proceso padre, Comprobador, encargado de
recibir los bloques por cola de mensajes y validarlos, y el proceso hijo, Monitor, encargado
de mostrar la salida unificada. Estos procesos se comunican usando memoria compartida
y un esquema de productor–consumidor.*/

/*El monitor podr´a terminar en cualquier momento, e incluso volver a arrancar m´as adelante.
La red debe ser capaz de adaptarse a estos cambios, de forma que no se acumulen mensajes
pendientes si no hay un monitor activo.*/

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
#include <sys/wait.h>
#include <mqueue.h>

#include "funciones_monitor.h"

int monitor();
int comprobador();
#define SHM_NAME "/shm_name"
#define NAME_SEM_CTRL "/semCTRL"

int main()
{
    sem_t *semCtrl;
    pid_t pid;
    
    if ((semCtrl = sem_open(NAME_SEM_CTRL, O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) /*Apertura de semáforo*/
        error("sem_open SemCtrl");
    
    /*Petición a memoria compartida para que los procesos divergan*/
    if ((fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == ERROR)
    {
        if (errno == EEXIST)
        {
            if ((fd = shm_open(SHM_NAME, O_RDWR, 0)) == ERROR) /*Control de errores*/
                error(" Error opening the shared memory segment ");
            else /*Caso monitor*/
            {
#ifdef DEBUG
                printf("Monitor %d\n", getpid());
#endif
                if (monitor(fd, lag, semCtrl) == ERROR)
                    perror("Error in monitor");
                /*Se desvinculan ambos recursos al ser nosotros los últimos en usarlos*/
                shm_unlink(SHM_NAME);
                sem_unlink(NAME_SEM_CTRL);
            }
        }
        else /*Control de errores*/
            error(" Error creating the shared memory segment ");
    }
    else /*Caso comprobador*/
    {
#ifdef DEBUG
        printf("Comprobador %d\n", getpid());
#endif
        if (comprobador(fd, semCtrl) == ERROR)
            error("Error in comprobador");
    }

    exit(EXIT_SUCCESS);
    switch (fork())
    {
    case -1:
        printf("ERRORES");
        break;
    case 0:
        own(semCtrl);
        monitor();
        break;
    default:
        if((fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == ERROR){
             error(" Error opening the shared memory segment ");

        }
        comprobador();
        break;
    }

    printf("Monitor se va\n");

    exit(EXIT_SUCCESS);
}
/*
int comprobador()
{
    int pid, fd;
    STATUS st;
    Bloque msg;
    struct mq_attr attributes;
    mqd_t mq;
    int i;

    attributes.mq_maxmsg = Q_SIZE;
    attributes.mq_msgsize = sizeof(Bloque);
    if ((mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes)) == (mqd_t)ERROR)
        error(" mq_open en Monitor");

    printf("Esperando los mensajes:\n\n");
    while (1)
    {   sleep(3);
        if (mq_receive(mq, (char *)&msg, sizeof(Bloque), 0) == ERROR)
            error("Error recibiendo mensajes en Monitor\n");

        if (msg.id == -1)
            break;

        if (pow_hash(msg.sol) == msg.obj)
            printf("\n\nComprobador dice que está bien\n");
        else
            printf("\n\nComprobador dice que está mal\n");
        print_bloque(STDOUT_FILENO, &msg);
    }
   
    if (mq_close(mq) == ERROR)
        error("mq_close en monitor");

    if (mq_unlink(MQ_NAME) == ERROR)
        error("mq_unlink en monitor");
}*/
/*
int monitor(){
    return 0;
}*/