
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>

#include "pow.h"
#include "utils.h"

int encontrado;

typedef struct
{
    long ep, eu, res;
} Entrada_Hash;

int minar(int n_threads,int obj);
void *func_minero(void *arg);
void ganador();
void perdedor();


/*Si hay error se gestiona internamente y se realiza un exit()*/
void minero(int n_threads, int n_secs,int pid){
    int  i,obj,sol; /*Indica la posición a insertar el siguiente mensaje en el array msg*/
    
    /*Creará una conexión a memoria para la comunicación de información del sistema :n_mineros, ... */

    /*Si es el primero en abrirla debe inicializar el sistema dando tamaño al segmento y enlazandolo a su espacio
     de memoria e inicializando los distintos campos (n_mineros = 1...)  y los semáforos necesarios*/

    /*Si no es el primero simplemente abré el segmento, lo enlaza. Debe garantizarse que el sistema esta listo (Semáforo)*/

    /*Registrar sus datos en el sistema (PIDs ,n_mineros...)*/

    /*Preparación para la ronda: */
    /*Si es el primero establece un objetivo inicial*/obj = INIC;
        /*Arranca el sistema enviando un SIGUSR1 */
    /*Si no*/
        /*Espera el inicio de una ronda (SIGUSR1)*/

    /*Salvo en la primera se debe enviar el bloque de la ronda enterior al proceso Registrador*/

    /*Ronda de minado:*/
    /*Si se recibe una SIGUSR2 significa que un proceso a encontrado la soulución*/

    /*Bucle de minería y mandar las soluciones hasta que nos llegue un alarm de n_secs o un SIGINT*/
    for (i = 0, ; i < n_rounds; i++, obj = sol)
    {
        /*Minado*/
        if ((sol = minar(n_threads,obj)) == ERROR)
            /*ERROR*/
#ifdef DEBUG
        printf("Minamos %d->%d-\n", obj, sol);
#endif
    }

    /*Tras salir de la minería se elige a un ganador de forma que evite las condiciones de carrera (semáforo)*/

    /*El primero se proclama proceso: Ganador*/
        ganador();
    /*El resto procesos: Perdedor*/
        perdedor();

    /*Gestionar finalización:*/
    /*Razones por las que finaliza el proceso : SIGALARM o SIGINT
    Si es el último minero se libera todo y se envia al monitor (si está activo) el cierre del sistema
    Finalmente esperar al proceso Registrador  e imprime un mensaje en caso de EXIT_FAILURE (de Registrador)
    */
    exit(EXIT_SUCCESS);
}


int minar(int n_threads,int obj)
{
    int i, incr = ((int)MAX) / n_threads;
    long solucion;
    Entrada_Hash t[MAX_THREADS];
    pthread_t threads[MAX_THREADS];
    void *sol[MAX_THREADS];

    encontrado = 0;
    /*Creación de hilos*/
    for (i = 0; i < n_threads; i++)
    {
        t[i].ep = incr * i;
        if (i == n_threads - 1)
            t[i].eu = MAX;
        else
            t[i].eu = incr * (i + 1);
        t[i].res = obj;

        if (pthread_create(&threads[i], NULL, func_minero, (void *)(t + i)))
        {
            for (; i >= 0; i--)
                (void)pthread_join(threads[i], sol + i);

            return ERROR;
        }
    }
    /*Si se recibe una SIGUSR2 significa que un proceso a encontrado la soulución*/

    /*Joins de los hilos*/
    for (i = 0; i < n_threads; i++)
    {
        if (pthread_join(threads[i], sol + i))
        {
            for (i++; i < n_threads; i++)
                (void)pthread_join(threads[i], sol + i);
            perror(" mq_open en la unión de hilos");
            return ERROR;
        }
        if ((long)sol[i] != -1)
            solucion = (long)sol[i];
    }

    return (int)solucion;
}

void *func_minero(void *arg)
{
    long i;
    Entrada_Hash *e = (Entrada_Hash *)arg;

    for (i = e->ep; (i < e->eu) && (encontrado == 0); i++)
    {
        if (pow_hash(i) == e->res)
        {
            encontrado = 1;
            pthread_exit((void *)i);
        }
    }

    pthread_exit((void *)ERROR);
}
/*Debemos elegir si el error se gestiona aquí o devolvemos error*/
void ganador(){
    /*Prepara memoria compartida para la votación introduciendo la solución en el bloque actual 
      e inicializando el listado de votos*/

    /*Enviar la señal SIGUSR2 indicando que la votación puede arrancar*/

    /*Realizaz esperas no activas cortas hasta que todos los procesos hallan votado o transcurra
       el máximo numero de intentos(podemos calentarnos y intentar quitar eso)*/

    /*Una vez terminada la votación se comprueba el resulatado y se introduce el bloque correspondiente de memoria */
    /* Si se aceptado recibe una moneda ( aumenta el contador )*/

    /*Ahora se debe enviar el bloque resuelto por cola de mensajes al monitor si hay alguno activo*/
    /* Se deber´an implementar los mecanismos adecuados para garantizar que el env´ıo se realiza si y solo si 
    el monitor est´a esperando para recibirlo, por ejemplo a trav´es de un semaforo con nombre.*/

    /*Reiniciar la votación con el envio de un SIGUSR1 a todos los mineros*/
}

/*Debemos elegir si el error se gestiona aquí o devolvemos error*/
void perdedor(){
    /*Si no le ha llegado aún la señal SIGUSR2 se espera a que la votación este lista*/

    /*Una vez recibido comprueba el bloque actual y registra su voto en función de si la solución es correcta*/

    /*Entra en una espera no activa (Se realizará fuera de la función) hasta la siguiente votación*/
}
/*Si hay error se gestiona internamente y se realiza un exit()*/
void recibidor(){
    /*Mientras las tuberías esten abiertas: se leen bloques usando dprintf (impresión en bytes)
     en un archivo que dependa del pid del padre (Minero)*/

     /*Formato del bloque: */
     /*
     Id :  <Id del bloque>
     Winner : <PID>
     Target: <TARGET>
     Solution: <Solución propuesta>
     Votes : <N_VOTES_ACCEPT>/<N_VOTES>
     Wallets : <PID>:<N_MONEDAS> ...
     */

    /*Cuando se cierre la pipe se liberan los recursos y se llama a exit()*/
}
