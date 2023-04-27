/**
 * @file funciones.c
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <mqueue.h>

#include "funciones_monitor.h"

#define QUEUE_SIZE 6 /*Creo que pasa a 5*/
#define N_SIZE 4

#define SIZE 7
#define MQ_NAME "/mqueue"

typedef enum
{
    OBJ,
    SOL,
    RES
} NUMS; /*!< Enumeration of the different num values */

struct _Shm_struct
{
    sem_t sem_vacio, sem_mutex, sem_lleno;
    Bloque bloq[QUEUE_SIZE];
    int in, out, val[QUEUE_SIZE];
}; /*!< Structure of the messages between Monitor and Comprobador*/
/*Esto debería alojarse en utils ya que lo comparten minero y comprobador*/
void monitor_print_block(Bloque *blq, int res);

/*Función privada que finaliza el programa Comprobador en caso de error, indicandoselo a Monitor*/
STATUS finish_comprobador(char *str, int fd_shm, Shm_struct *mapped, int n_sems, STATUS retorno)
{
    if (str)
        perror(str);

    if (n_sems >= 1)
        sem_destroy(&mapped->sem_vacio);
    if (n_sems >= 2)
        sem_destroy(&mapped->sem_mutex);
    if (n_sems >= 3)
        sem_destroy(&mapped->sem_lleno);

    if (fd_shm <= 0)
        close(fd_shm);
    if (mapped)
        if (munmap(mapped, sizeof(Shm_struct)) == ERROR)
            retorno = ERROR;

#ifdef TEST
    nanorandsleep();
#endif
    return retorno;
}

/*Hay que repasar esto pero tiene beuna pinta hay que cambiar Shm_struct por un bloque*/
STATUS comprobador(int fd_shm, sem_t *semCtrl)
{
    long obj, sol;
    int res=0; /*Index apunta a la primera direccción vacía*/
    Shm_struct *mapped;
    struct mq_attr attributes;
    mqd_t mq;
    Bloque msg;

    /*Mapeará el segmento de memoria*/
    if (ftruncate(fd_shm, sizeof(Shm_struct)) == ERROR)
        return _finish_comprobador("ftruncate", fd_shm, NULL, 0, ERROR);

#ifdef DEBUG
    printf("[%d] Checking blocks...\n", getpid());
    printf("Pasa ftruncate %d\n", getpid());
#endif

    if ((mapped = (Shm_struct *)mmap(NULL, sizeof(Shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        return _finish_comprobador("mmap", fd_shm, NULL, 0, ERROR);
    close(fd_shm); /*Cerramos el descriptor ya que tenemos un puntero a la "posición" de memoria compartida*/

#ifdef DEBUG
    printf("Pasa mmap %d \n", getpid());
#endif
#ifdef TEST
    nanorandsleep();
#endif
    /*Inicialización de los semaforos*/
    if (ERROR == sem_init(&(mapped->sem_mutex), SHARED, 1))
        return _finish_comprobador("Sem_init mutex", 0, mapped, 0, ERROR);

    if (ERROR == sem_init(&(mapped->sem_vacio), SHARED, QUEUE_SIZE))
        return _finish_comprobador("Sem_init vacio", 0, mapped, 1, ERROR);

    if (ERROR == sem_init(&(mapped->sem_lleno), SHARED, 0))
        return _finish_comprobador("Sem_init lleno", 0, mapped, 2, ERROR);

    mapped->out = 0;
    mapped->in = 0;
#ifdef TEST
    nanorandsleep();
#endif
#ifdef DEBUG
    printf("Semáforos inicializados %d\n", getpid());
#endif
    up(semCtrl); /*Indica a comprobador que los semáforos ya están inicializados*/
    sem_close(semCtrl);

    attributes.mq_maxmsg = SIZE;
    attributes.mq_msgsize = sizeof(Bloque);
#ifdef TEST
    nanorandsleep();
#endif
    if ((mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes)) == (mqd_t)ERROR)
        return _finish_comprobador("Open mq en comprobador", 0, mapped, 3, ERROR);

    while (res != -1)
    {
        printf("Receiving\n");
        /*Recepción del mensaje enviado por el Ganador*/
        if (mq_receive(mq, (char *)&msg, sizeof(Bloque), 0) == ERROR)
        {
            down(&mapped->sem_vacio);
            down(&mapped->sem_mutex);
            /*Insertar fin de ejecución*/
            mapped->val[mapped->in] = -1;
            copy_block(&(mapped->bloq[mapped->in]), &msg);
            mapped->in = (mapped->in + 1) % QUEUE_SIZE;

            up(&mapped->sem_mutex);
            up(&mapped->sem_lleno);
            mq_close(mq);
            return _finish_comprobador("Receive mq en comprobador", 0, mapped, 3, ERROR);
        }
        obj = msg.obj;
        sol = msg.sol;
#ifdef TEST
        nanorandsleep();
#endif
        res = (obj == pow_hash(sol)); /*Comprobación de la solución*/
#ifdef DEBUG
        printf("Comprueba %ld -> %ld: %ld\n", obj, sol, pow_hash(sol));
#endif
        if(msg.id == -1)
            res = -1;
        /*Se la trasladará al monitor a través de la memoria compartida*/
        down(&mapped->sem_vacio);
        down(&mapped->sem_mutex);
#ifdef TEST
        nanorandsleep();
#endif
        mapped->val[mapped->in] = res;
        copy_block(&(mapped->bloq[mapped->in]), &msg);
        mapped->in = (mapped->in + 1) % QUEUE_SIZE;
#ifdef TEST
        nanorandsleep();
#endif
        up(&mapped->sem_mutex);
        up(&mapped->sem_lleno);
    }
#ifdef TEST
    nanorandsleep();
#endif
#ifdef DEBUG
    printf("FIN \n");
#endif
    /*Liberación de recursos y fin*/
    printf("[%d] Finishing...\n", getpid());

    return finish_comprobador(NULL,fd_shm,mapped,3,OK);
}

STATUS monitor(int fd_shm)
{
    int res; /*Index apunta a la primera direccción llena*/
    Shm_struct *mapped;
    Bloque blq;

    printf("[%d] Printing blocks...\n", getpid());

    /*Mapeará el segmento de memoria y cerrará el descriptor ya que tenemos un puntero a la "posición" de mamoria compartida*/
    if ((mapped = (Shm_struct *)mmap(NULL, sizeof(Shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
    {
        close(fd_shm);
        error("Error doing mmap in monitor");
    }
    close(fd_shm);
#ifdef TEST
    nanorandsleep();
#endif

    while (res != -1) /*Extraerá un bloque realizando de consumidor en el esquema productor-consumidor*/
    {
        down(&mapped->sem_lleno);
        down(&mapped->sem_mutex);
#ifdef TEST
        nanorandsleep();
#endif
        copy_block(&blq, &(mapped->bloq[mapped->out]));
        res = mapped->val[mapped->out];
        mapped->out = (mapped->out + 1) % QUEUE_SIZE;
#ifdef TEST
        nanorandsleep();
#endif
        up(&mapped->sem_mutex);
        up(&mapped->sem_vacio);

        /*Impresión por pantalla del resultado*/
        monitor_print_block(&blq, res);
    }
    printf("[%d] Finishing...\n", getpid());
    
    /*Liberación de recursos y fin. Cerramos los recursos abiertos ya que nosotros somos los últimos en usarlos*/
    
    munmap(mapped, sizeof(Shm_struct));

    return OK;
}

void monitor_print_block(Bloque *bloque, int res)
{
    int i;

    if (res == -1 || !bloque)
        return;

    printf("\nId:  %ld \n", bloque->id);
    printf("Winner:  %d \n", bloque->pid);
    printf("Target:  %ld \n", bloque->obj);
    if (res)
        printf("Solution:  %ld (validated)\n", bloque->sol);
    else
        printf("Solution:  %ld (rejected) \n", bloque->sol);

    printf("Votes:  %ld/%ld\n", bloque->votos_a, bloque->n_votos);
    printf("Wallets:");

    for (i = 0; i < MAX_MINERS; i++)
        if (wallet_get_pid(&(bloque->Wallets[i])) != 0)
            printf(" %d:%d ", wallet_get_pid(&(bloque->Wallets[i])), wallet_get_coins(&(bloque->Wallets[i])));
    printf(" \n");
}
