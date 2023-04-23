
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
 * @file monitor.c
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */
/*Quitar Mutex*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "funciones.h"

#define SHM_NAME "/shm_name"
#define NAME_SEM_CTRL "/semCTRL"

int main()
{
    sem_t *semCtrl;
    int pid, fd;
    STATUS st;

    if ((semCtrl = sem_open(NAME_SEM_CTRL, O_CREAT, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) /*Apertura de semáforo*/
        error("sem_open SemCtrl");

    /*Petición a memoria compartida para que los procesos divergan*/
    if ((fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) == ERROR)
    {
         /*Control de errores*/
            error(" Error creating the shared memory segment ");
    }
    else 
    if((pid = fork()) == -1){
        error(" Error forking ");

    }else if(pid){
        if (comprobador(fd, lag, semCtrl) == ERROR)
                    error("Error in comprobador");
    }else{
        if (monitor(fd, lag, semCtrl) == ERROR)
                    error("Error in monitor");
    }
    
    

    exit(EXIT_SUCCESS);
}