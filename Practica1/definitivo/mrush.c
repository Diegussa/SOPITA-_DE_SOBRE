#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "pow.h"
#include "mrush.h"

int encontrado;


int main(int argc, char *argv[])
{
    int rc[MAX_HILOS], i,newpid,Status;
    long solucion, busq;
    struct timespec inicio, fin;
    double tiempo_transcurrido;

    // Obtenemos el tiempo actual
    clock_gettime(CLOCK_REALTIME, &inicio);
    /*Control de errores*/
    
    if ((atoi(argv[1]) < 0) || (atoi(argv[1]) > POW_LIMIT) || (atoi(argv[2]) < 0) || (atoi(argv[3]) < 0) || (atoi(argv[3]) > MAX_HILOS))
    {
        printf("\n\nError en los parametros de entrada");
        return 1;
    }
    
    /*Buscamos los elementos*/
    busq = atol(argv[1]);
    newpid=fork();
    if(newpid){
            wait(&Status); 
            printf("Miner exited with status %d\n",Status);

        
    }else{
       
        busq = minero(atoi(argv[3]),atoi(argv[2]), busq); /*Queremos que minero devuelve el proximo numero a buscar*/

    }

    /*Medimos el tiempo actual*/
    clock_gettime(CLOCK_REALTIME, &fin);
    tiempo_transcurrido = (double)(fin.tv_sec - inicio.tv_sec) + (double)(fin.tv_nsec - inicio.tv_nsec) / 1000000000.0;
    printf("El tiempo de ejecución fue de %.9f segundos.\n", tiempo_transcurrido);

    return 0;
}

