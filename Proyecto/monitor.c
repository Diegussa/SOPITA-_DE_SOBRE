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

#define SHM_NAME2 "/shm_name"
#define NAME_SEM_CTRL "/semCTRL"

int main()
{
    sem_t *semCtrl;
    int fd, st;

    if ((semCtrl = sem_open(NAME_SEM_CTRL, O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) /*Apertura de semáforo*/
        error("sem_open SemCtrl");

    switch (fork())
    {
    case -1: /*In case of error*/
        error("Error forking");
        break;

    case 0: /*Monitor*/
        down(semCtrl);
        if ((fd = shm_open(SHM_NAME2, O_RDWR, 0)) == ERROR) /*Control de errores*/
            error(" Error opening the shared memory segment ");
        if (monitor(fd) == ERROR)
            error("Error in monitor");

        exit(EXIT_SUCCESS);
        break;

    default: /*Comporobador*/
        if ((fd = shm_open(SHM_NAME2, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == ERROR)
            error(" Error opening the shared memory segment ");

        if (comprobador(fd, semCtrl) == ERROR)
            error("Error in comprobador");
        break;
    }


    
    
    /*Se desvinculan ambos recursos al ser nosotros los últimos en usarlos*/

    mq_unlink(MQ_NAME);
    shm_unlink(SHM_NAME2);
    sem_unlink(NAME_SEM_CTRL);

    exit(EXIT_SUCCESS);
}