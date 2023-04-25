
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>

#include "pow.h"
#include "utils.h"
#include "funciones_minero.h"

#define INIC 0
#define MAX_THREADS 100
#define MAX POW_LIMIT - 1

int encontrado;

volatile sig_atomic_t got_sigUSR1 = 0;
volatile sig_atomic_t got_sigUSR2 = 0;
volatile sig_atomic_t got_sigTERM = 0;

typedef struct
{
    long ep, eu, res;
} Entrada_Hash;

int minar(int n_threads, int obj);
void *func_minero(void *arg);
void ganador();
void perdedor();
void _handler_minero(int sig);
STATUS set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask, void (*handler)(int));

void _handler_minero(int sig)
{
    switch (sig)
    {
    case SIGTERM:
#ifdef DEBUG
        printf("SIGTERM %ld \n", (long)getpid());
#endif
        got_sigTERM = 1;
        break;
    case SIGUSR1:
#ifdef DEBUG
        printf("USR1 %ld\n", (long)getpid());
#endif
        got_sigUSR1 = 1;
        break;
    case SIGUSR2:
#ifdef DEBUG
        printf("USR2 %ld\n", (long)getpid());
#endif
        got_sigUSR2 = 1;
        break;
    case SIGINT:
#ifdef DEBUG
        printf("SIGINT %ld\n", (long)getpid());
#endif
        break;
    default:
#ifdef DEBUG
        printf("SIGNAL UNKNOWN(%d) %ld\n", sig, (long)getpid());
#endif
        break;
    }
}

/*Changes the handler of the specified signals*/
STATUS set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask, void (*handler)(int))
{
    sigset_t mask2;
    int i = 0;

    sigemptyset(&mask2);
    for (i = 0; i < n_signals; i++)
    {
        if (sigaddset(&mask2, sig[i]) == ERROR)
            return ERROR;
    }
    /*Unblock signals*/
    if (sigprocmask(SIG_BLOCK, &mask2, oldmask) == ERROR)
        return ERROR;

    /*Set the new signal handler*/
    actSIG->sa_handler = handler;
    sigemptyset(&(actSIG->sa_mask));
    actSIG->sa_flags = 0;

    for (i = 0; i < n_signals; i++)
    {
        if (sigaction(sig[i], actSIG, NULL) == ERROR)
            return ERROR;
    }
    /*Unblock signals*/
    if (sigprocmask(SIG_UNBLOCK, &mask2, NULL) == ERROR)
        return ERROR;
}

/*Si hay error se gestiona internamente y se realiza un exit()*/
void minero(int n_threads, int n_secs, int pid, int PipeLect, int PipeEscr, sem_t *mutex)
{
    int i, j, obj, sol = -1, handler_info_sist, proc_index, n_bytes;
    System_info s_info;
    int sig[NSIGNALS] = {SIGUSR1, SIGTERM, SIGUSR2, SIGINT};
    struct sigaction actSIG;
    sigset_t mask2, oldmask;

    /*Creará una conexión a memoria para la comunicación de información del sistema :n_mineros, ... */

    down(mutex);
    if ((handler_info_sist = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) != ERROR) /*Si la memoria no está creada*/
    {
        /*Asignación de tamaño*/
        if (ftruncate(handler_info_sist, sizeof(System_info)) == ERROR) /*CORREGIR, en caso de error se lleva el down del mutex*/
        {
            close(handler_info_sist);
            error("Error truncating the shared memory segment info_sist");
        }
        /*Enlazamiento al espacio de memoria*/
        if ((handler_info_sist = (System_info *)mmap(NULL, sizeof(System_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        {
            close(handler_info_sist);
            error("Error mapping the shared memory segment info_sist");
        }

        /*Inicializacion de la memoria compartida*/
        if ((s_info->primer_proc = sem_open(nameSemNmin, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED)
            error("sem_open primer_proc");
        for (i = 0; i < MAX_MINERS; i++)
        {
            s_info->Wallets[i][0] = 0;
            s_info->Wallets[i][1] = 0;
            for (j = 0; j < MAX_N_VOTES; j++)
                s_info->Votes_Min[i][j] = 0;
        }
        s_info->n_mineros = 1;

        s_info->UltimoBloque.id = -1;
        s_info->BloqueActual.obj = INIC;
        /*Se manda la señal a sí mismo por que es el único proceso*/
        if (kill((long)(getpid()), SIGUSR1) == ERROR)
            error("seding the signal to the first miner");
    }
    else /*Si no es el primero simplemente abré el segmento, lo enlaza. Debe garantizarse que el sistema esta listo (Semáforo)*/
    {
        if ((handler_info_sist = shm_open(SHM_NAME, O_RDWR, 0)) == ERROR) /*Control de errores*/
        {
            close(handler_info_sist);
            error(" Error opening the shared memory segment info_sist");
        }
        if ((handler_info_sist = (System_info *)mmap(NULL, sizeof(System_info), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        {
            close(handler_info_sist);
            error("Error mapping the shared memory segment info_sist");
        }
        /*Looks for the first empty space*/
        for (proc_index = 0; proc_index < MAX_MINERS && handler_info_sist->WALLET[i] == 0; proc_index++)
            ;
        if (proc_index == MAX_MINERS) /*En caso de que ya halla el máximo de mineros*/
        {
            close(handler_info_sist);
            error("Number of miners reached the maximum");
        }

        /*Registrar sus datos en el sistema (PIDs ,n_mineros...)*/
        s_info->Wallets[proc_index][0] = (long)getpid();
        s_info->Wallets[proc_index][1] = 0;
        s_info->n_mineros++;
    }
    close(handler_info_sist);
    up(mutex);

    if (set_handlers(sig, NSIGNALS, &actSIG, &oldmask, _handler_minero) == ERROR)
        errror("Error setting the new signal handler");

    /*Preparación de la ronda: */
    /*Si es el primero establece un objetivo inicial*/
    /*Arranca el sistema enviando un SIGUSR1 */
    down(mutex);
    for (i = 0; i < MAX_MINERSM; ++)
    {
        if (s_info->Wallets[proc_index][i] != -1)
            (void)kill(s_info->Wallets[proc_index][i], SIGUSR1);
    }
    up(mutex);
    /*Si no*/
    /*Espera el inicio de una ronda (SIGUSR1)*/
    while (!got_sigUSR1) /*Suspend the process waiting for SIGUSR1*/
        sigsuspend(&oldmask);
    got_sigUSR1 = 0;

    /*Salvo en la primera se debe enviar el bloque de la ronda enterior al proceso Registrador*/
    if (s_info->UltimoBloque.id != -1)
    {
        /*Enviar*/
    }
    /*Ronda de minado:*/
    /*Si se recibe una SIGUSR2 significa que un proceso a encontrado la solución*/

    /*Bucle de minería y mandar las soluciones hasta que nos llegue un alarm de n_secs o un SIGINT*/
    while (1)
    {
        if (sol != -1)
        {
            nbytes = write(PipeEscr, &bloque, sizeof(Bloque));
            if (nbytes == -1)
            {
                exit(EXIT_FAILURE);
            }
        }

        /*Minado*/
        if ((sol = minar(n_threads, obj)) == ERROR)
        /*ERROR*/
#ifdef DEBUG
            printf("Minamos %d->%d-\n", obj, sol);
#endif
        obj = sol;
    }

    /*Tras salir de la minería se elige a un ganador de forma que evite las condiciones de carrera (semáforo)*/

    if (down_try(s_info->primer_proc) != ERROR) /*El primero se proclama proceso: Ganador*/
        ganador();
    else /*El resto procesos: Perdedor*/
        perdedor();

    /*Gestionar finalización:*/
    /*Razones por las que finaliza el proceso : SIGALARM o SIGINT

    Si es el último minero se libera todo y se envia al monitor (si está activo) el cierre del sistema*/
    down(mutex);
    s_info->n_mineros--;
    if (s_info->n_mineros == 0)
    {
        if (munmap(s_info, sizeof(System_info)) == ERROR)
            exit(EXIT_FAILURE);
    }
    up(mutex);

    /*Finalmente esperar al proceso Registrador  e imprime un mensaje en caso de EXIT_FAILURE (de Registrador)
     */
    exit(EXIT_SUCCESS);
}

int minar(int n_threads, int obj)
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

    pthread_exit((void *)(-1));
}
/*Debemos elegir si el error se gestiona aquí o devolvemos error*/
void ganador()
{
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
void perdedor()
{
    /*Si no le ha llegado aún la señal SIGUSR2 se espera a que la votación este lista*/

    /*Una vez recibido comprueba el bloque actual y registra su voto en función de si la solución es correcta*/

    /*Entra en una espera no activa (Se realizará fuera de la función) hasta la siguiente votación*/
}
/*Si hay error se gestiona internamente y se realiza un exit()*/
void registrador(int PipeLect, int PipeEscr)
{
    int nbytes, val;
    Bloque bloque;
    char Filename[WORD_SIZE];

    sprintf(Filename, "%d.dat\0", getppid());
    fd = open(Filename, "w");
    if (!fd)
    {
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        nbytes = read(pipeLect, &bloque, sizeof(Bloque));
        if (nbytes == -1)
        {
            perror("read regis %d", getpid());
            close(pipeEscr);
            close(pipeLect);
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0)
        {
            close(pipeEscr);
            close(pipeLect);
            exit(EXIT_SUCCESS);
        }
        print_bloque(fd, &bloque);
    }
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
