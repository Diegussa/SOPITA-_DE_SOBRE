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

#include "funciones.h"

#define FILE_NAME "mrush_sequence.dat"
#define QUEUE_SIZE 6
#define N_SIZE 4

#define SIZE 7
#define MQ_NAME "/mqueue"

typedef enum
{
    OBJ,
    SOL,
    RES, 
    FIN
} NUMS; /*!< Enumeration of the different num values */

struct _Shm_struct
{
    sem_t sem_vacio, sem_mutex, sem_lleno;
    long num[N_SIZE][QUEUE_SIZE];
};
/*Esto debería alojarse en utils ya que lo comparten minero y comprobador*/

STATUS comprobador(int fd_shm, int lag, sem_t *semCtrl)
{
    FILE *f;
    long new_objetivo, new_solucion, obj, sol;
    int fin = 0, res, index = 0; /*Index apunta a la primera direccción vacía*/
    Shm_struct *mapped;
    struct mq_attr attributes;
    mqd_t mq;
    Message msg;

    printf("[%d] Checking blocks...\n", getpid());
    /*Mapeará el segmento de memoria*/
    if (ftruncate(fd_shm, sizeof(Shm_struct)) == ERROR)
    {
        close(fd_shm);
        error("ftruncate");
    }
#ifdef DEBUG
    printf("Pasa ftruncate %d\n", getpid());
#endif
    if ((mapped = (Shm_struct *)mmap(NULL, sizeof(Shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
    {
        close(fd_shm);
        error("mmap");
    }
#ifdef DEBUG
    printf("Pasa mmap %d \n", getpid());
#endif
    
    close(fd_shm); /*Cerramos el descriptor ya que tenemos un puntero a la "posición" de memoria compartida*/
    
    if (ERROR == sem_init(&(mapped->sem_mutex), 2, 1))
    { /*Inicialización de los semaforos*/
        munmap(mapped, sizeof(Shm_struct));
        error("Sem_init mutex");
    }
#ifdef DEBUG
    printf("Pasa inicializa sem mutex %d\n", getpid());
#endif
    if (ERROR == sem_init(&(mapped->sem_vacio), 2, QUEUE_SIZE))
    {
        sem_destroy(&mapped->sem_mutex);
        /*Comunicar de alguna forma a los otro proceso que fin*/
        munmap(mapped, sizeof(Shm_struct));
        error("Sem_init vacio");
    }
#ifdef DEBUG
    printf("Pasa inicializa sem vacio %d\n", getpid());
#endif
    if (ERROR == sem_init(&(mapped->sem_lleno), 2, 0))
    {
        
        sem_destroy(&mapped->sem_vacio);
        sem_destroy(&mapped->sem_mutex);
        munmap(mapped, sizeof(Shm_struct));
        
        error("Sem_init lleno");
    }
#ifdef DEBUG
    printf("Pasa inicializa sem _lleno %d\n", getpid());
    sleep(2);
#endif
    up(semCtrl); /*Indica a comprobador que los semáforos ya están inicializados*/
    sem_close(semCtrl);
    /*CAMBIOS: Preparando la cola de mensajes*/
    attributes.mq_maxmsg = SIZE;
    attributes.mq_msgsize = sizeof(Message);

    if ((mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes)) == (mqd_t)ERROR)
    {
        error(" mq_open ");
    }

    while (1)
    {
        /*CAMBIOS:Recibirá un bloque a través de mensajes por parte del minero*/
        if (mq_receive(mq, (char *)&msg, sizeof(Message), 0) == ERROR)
        {
            mq_close(mq);
            /*Habría que comunicar al resto de procesos que ha habido un error*/
            error("mq_receive");
        }
        obj = msg.obj;
        sol = msg.sol;
        fin = msg.fin;
        
        if (fin)
        {
/*Forma de decir que se ha acabado el fichero del que leíamos*/
#ifdef DEBUG
            printf("FIN %ld -> %ld %ld fin: %d\n", obj, sol, pow_hash(sol), fin);
#endif
        }
        
        if (obj == pow_hash(sol)) /*Comprobación de la solución*/
            res = 1; /*Respuesta correcta*/
        else
            res = 0; /*Respuesta incorrecta*/
#ifdef DEBUG
        printf("Comprueba %ld -> %ld %ld fin: %d\n", obj, sol, pow_hash(sol), fin);
#endif

        /*Se la trasladará al monitor a través de la memoria compartida*/
        /* Esquema de productro-consumidor (El es el productor)*/
        down(&mapped->sem_vacio);
        down(&mapped->sem_mutex);

        mapped->num[OBJ][index] = obj;
        mapped->num[SOL][index] = sol;
        mapped->num[RES][index] = res;
        mapped->num[FIN][index] = fin;
        index = (index + 1) % QUEUE_SIZE;

        up(&mapped->sem_mutex);
        up(&mapped->sem_lleno);

        /*Realizará una espera de <lag> milisegundos */
        ournanosleep(lag * MILLON);
        /*Cuando se reciba dicho bloque lo introducirá en memoria compartida para avisar a Monitor de la fincalización*/
        if (fin)
            break;
        /*Repetirá el proceso hasta que reciba (de minero un bloque especial de finalización)*/
    }

    printf("[%d] Finishing...\n", getpid());

    /*Liberación de recursos y fin*/
    mq_unlink(MQ_NAME);
#ifdef DEBUG
    sleep(1);
#endif
    if (munmap(mapped, sizeof(Shm_struct)) == ERROR)
        error("munmap comprobador\n");

    return OK;
}

STATUS monitor(int fd_shm, int lag, sem_t *semCtrl)
{
    long obj, sol;
    int fin = 0, res, index = 0; /*Index apunta a la primera direccción llena*/
    Shm_struct *mapped;

    printf("[%d] Printing blocks...\n", getpid());

    /*Mapeará el segmento de memoria*/
    if ((mapped = (Shm_struct *)mmap(NULL, sizeof(Shm_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
    {
        close(fd_shm);
        error("Error doing mmap in monitor");
    }
    /*Cerramos el descriptor ya que tenemos un puntero a la "posición" de mamoria compartida*/
    close(fd_shm);

    down(semCtrl); /*Espera a que comprobador inicialice los semáforos*/
    sem_close(semCtrl);

    while (1)
    { /*Extraerá un bloque realizando de consumidor en el esquema productor-consumidor*/
        down(&mapped->sem_lleno);
        down(&mapped->sem_mutex);

        obj = mapped->num[OBJ][index];
        sol = mapped->num[SOL][index];
        res = mapped->num[RES][index];
        fin = mapped->num[FIN][index];
        index = (index + 1) % QUEUE_SIZE;

        up(&mapped->sem_mutex);
        up(&mapped->sem_vacio);

        if (fin)
        {
#ifdef DEBUG
            printf("MON FIN %ld -> %ld %ld fin: %d\n", obj, sol, pow_hash(sol), fin);
#endif
            break;
        }

        /*Mostrará por pantalla el resultado con la siguiente sintaxis*/
        /*Solution accepted: %08ld --> %08ld o Solution rejected: %08ld !-> %08ld (Siendo el promer numero el objetivo y el segundo la solución)*/
        if (res)
            printf("Solution accepted: %08ld --> %08ld\n", obj, sol);
        else
            printf("Solution rejected: %08ld !-> %08ld\n", obj, sol);
        
        ournanosleep(lag * MILLON); /*Espera de <lag> milisegundos */
        /*Repetirá el ciclo de extracción y muestra hasta recibir un bloque especial que indique la finalización del sistema*/
    }
    printf("[%d] Finishing...\n", getpid());

    /*Liberación de recursos y fin*/
    /*Cerramos los recursos abiertos ya que nosotros somos los últimos en usarlos*/
    sem_destroy(&mapped->sem_vacio);
    sem_destroy(&mapped->sem_mutex);
    sem_destroy(&mapped->sem_lleno);
    munmap(mapped, sizeof(Shm_struct));

    return OK;
}