
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
#define WAIT 100*MIL*MIL
#define MAX_TRY 100

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
void ganador(System_info *sys, int obj, int sol, int proc_index, sem_t *mutex);
void perdedor(System_info *sys, sigset_t *oldmask, int index_proc);
void _handler_minero(int sig);
STATUS set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask, void (*handler)(int));
void init_block(Bloque *b, long **sys_Wallet, long id, long obj, long sol);
void copy_block(Bloque *dest, Bloque *orig);

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
    System_info *s_info;
    int sig[4] = {SIGUSR1, SIGTERM, SIGUSR2, SIGINT};
    struct sigaction actSIG;
    sigset_t mask2, oldmask;

    /*Cambio del manejador de señales*/
    if (set_handlers(sig, 4, &actSIG, &oldmask, _handler_minero) == ERROR)
        error("Error setting the new signal handler");

    /*Creará una conexión a memoria para la comunicación de información del sistema :n_mineros, ... */
    up(mutex);
    down(mutex);
    if ((handler_info_sist = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) != ERROR) /*Si la memoria no está creada*/
    {printf("1\n");
    
        proc_index = 0;

        /*Asignación de tamaño*/
        if (ftruncate(handler_info_sist, sizeof(System_info)) == ERROR) /*CORREGIR, en caso de error se lleva el down del mutex*/
        {
            close(handler_info_sist);
            error("Error truncating the shared memory segment info_sist");
        }
        /*Enlazamiento al espacio de memoria*/
        
        if ((s_info = (System_info *)mmap(NULL, sizeof(System_info), PROT_READ | PROT_WRITE, MAP_SHARED, handler_info_sist, 0)) == MAP_FAILED)
        {
            close(handler_info_sist);
            error("Error mapping the shared memory segment info_sist");
        }
printf("2\n");
        /*Inicializacion de la memoria compartida*/
        if (sem_init((s_info->primer_proc), SHARED, 1) == ERROR)
            error("sem_open primer_proc");
        if (sem_init((s_info->MutexBAct), SHARED, 1) == ERROR)
            error("sem_open MutexBact");
printf("3\n");
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
        s_info->BloqueActual.id = 0;

        got_sigUSR1 = 1;
    }
    else /*Si no es el primero simplemente abré el segmento, lo enlaza. Debe garantizarse que el sistema esta listo (Semáforo)*/
    {printf("r1\n");
        if ((handler_info_sist = shm_open(SHM_NAME, O_RDWR, 0)) == ERROR) /*Control de errores*/
        {
            close(handler_info_sist);
            error(" Error opening the shared memory segment info_sist");
        }
        if ((s_info = (System_info *)mmap(NULL, sizeof(System_info), PROT_READ | PROT_WRITE, MAP_SHARED, handler_info_sist, 0)) == MAP_FAILED)
        {
            close(handler_info_sist);
            error("Error mapping the shared memory segment info_sist");
        }
        /*Looks for the first empty space*/
        for (proc_index = 0; proc_index < MAX_MINERS && s_info->Wallets[i][0]; proc_index++)
            ;printf("r2\n");
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
exit(EXIT_SUCCESS);
    /*Preparación de la ronda: */
    /*Si es el primero establece un objetivo inicial*/

    /*Si no*/

    while (!got_sigTERM)
    {
        /*Espera el inicio de una ronda (SIGUSR1)*/
        while (!got_sigUSR1)
            sigsuspend(&oldmask);
        got_sigUSR1 = 0;

        /*Salvo en la primera se debe enviar el bloque de la ronda anterior al proceso Registrador*/
        if (s_info->UltimoBloque.id != -1)
        {
            if (write(PipeEscr, &s_info->UltimoBloque, sizeof(Bloque)) == -1)
                exit(EXIT_FAILURE);
            obj = s_info->UltimoBloque.sol;
        }
        /*Ronda de minado:*/
        /*Si se recibe una SIGUSR2 significa que un proceso a encontrado la solución*/

        /*Bucle de minería y mandar las soluciones hasta que nos llegue un alarm de n_secs o un SIGINT*/

        /*Minado*/
        sol = minar(n_threads, obj);

#ifdef DEBUG
        printf("Minamos %d->%d-\n", obj, sol);
#endif

        /*Tras salir de la minería se elige a un ganador de forma que evite las condiciones de carrera (semáforo)*/
        if (down_try(s_info->primer_proc) != ERROR)
        { /*El primero se proclama proceso: Ganador*/
            down(s_info->MutexBAct);
            ganador(s_info, obj, sol,proc_index,mutex);
        }
        else /*El resto procesos: Perdedor*/
            perdedor(s_info, &oldmask, proc_index);
    }
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
            error(" mq_open en la unión de hilos");
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

    for (i = e->ep; (i < e->eu) && (encontrado == 0) && (got_sigUSR2 == 0); i++)
    {
        if (pow_hash(i) == e->res)
        {
            encontrado = 1;
            pthread_exit((void *)i);
        }
    }

    pthread_exit((void *)(-1));
}

void ganador(System_info *sys, int obj, int sol, int proc_index, sem_t *mutex)
{
    int i;
    /*Prepara memoria compartida para la votación introduciendo la solución en el bloque actual
    e inicializando el listado de votos*/
    copy_block(&(sys->UltimoBloque), &(sys->BloqueActual));
    init_block(&(sys->BloqueActual), (long**)sys->Wallets, sys->UltimoBloque.id + 1, obj, sol);

    /*Enviar la señal SIGUSR2 indicando que la votación puede arrancar*/
    down(mutex);
    for (i = 0; i < MAX_MINERS; i++)
    {
        if ((sys->Wallets[i][0]) && (i != proc_index))
            (void)kill(sys->Wallets[i][0], SIGUSR2);
    }
    up(mutex);
    /*Realiza esperas no activas cortas hasta que todos los procesos hallan votado o transcurra
       el máximo numero de intentos(podemos calentarnos y intentar quitar eso)*/
    for (i = 0; i < MAX_TRY; i++)
    {
        if (sys->BloqueActual.n_mineros == sys->BloqueActual.n_votos)
            break;
        ournanosleep(WAIT);
    }
    /*Una vez terminada la votación se comprueba el resulatado y se introduce el bloque correspondiente de memoria */
    /* Si se aceptado recibe una moneda ( aumenta el contador )*/
    if (sys->BloqueActual.votos_a <= sys->BloqueActual.n_votos / 2)
        sys->BloqueActual.Wallets[proc_index][1]++;
    /*Ahora se debe enviar el bloque resuelto por co    la de mensajes al monitor si hay alguno activo*/
    /* Se deber´an implementar los mecanismos adecuados para garantizar que el env´ıo se realiza si y solo si
    el monitor est´a esperando para recibirlo, por ejemplo a trav´es de un semaforo con nombre.*/

    /*Reiniciar la votación con el envio de un SIGUSR1 a todos los mineros*/
    down(mutex);
    for (i = 0; i < MAX_MINERS; i++)
    {
        if (sys->Wallets[i][0])
            (void)kill(sys->Wallets[i][0], SIGUSR1);
    }
    up(mutex);
}

/*Debemos elegir si el error se gestiona aquí o devolvemos error*/
void perdedor(System_info *sys, sigset_t *oldmask, int index_proc)
{
    /*Si no le ha llegado aún la señal SIGUSR2 se espera a que la votación este lista*/
    while (!got_sigUSR2)
        sigsuspend(oldmask);
    got_sigUSR2 = 0;
    /*Exclusion Mutua Votar*/
    while (down(sys->MutexBAct) == ERROR)
        ;
#ifdef TEST
    nanorandsleep();
#endif
    sys->BloqueActual.n_votos++;
    sys->Votes_Min[index_proc][sys->BloqueActual.id] = 0;
    if (pow_hash(sys->BloqueActual.sol) == sys->BloqueActual.obj)
    {
        sys->BloqueActual.votos_a++;
        sys->Votes_Min[index_proc][sys->BloqueActual.id] = 1;
    }

    if (up(sys->MutexBAct) == ERROR)
        error("Error in voters");
    /*Una vez recibido comprueba el bloque actual y registra su voto en función de si la solución es correcta*/

    /*Entra en una espera no activa (Se realizará fuera de la función) hasta la siguiente votación*/
}
/*Si hay error se gestiona internamente y se realiza un exit()*/
void registrador(int PipeLect, int PipeEscr)
{
    int nbytes, val, fd;
    Bloque bloque;
    char Filename[WORD_SIZE];

    sprintf(Filename, "%d.dat", getppid());
    
    if (!(fd = open(Filename, O_APPEND)))
    {
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        if (read(PipeLect, &bloque, sizeof(Bloque)) == -1)
        {
            perror("Error reading bloque");
            close(PipeEscr);
            close(PipeLect);
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0)
        {
            close(PipeEscr);
            close(PipeLect);
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

void init_block(Bloque *b, long **sys_Wallet, long id, long obj, long sol)
{
    int i;

    if (b == NULL)
        return;

    b->n_votos = 1;
    b->votos_a = 1;
    b->obj = obj;
    b->pid = getpid();
    b->sol = sol;
    b->id = id;

    for (i = 0; i < MAX_MINERS; i++)
    {
        b->Wallets[i][0] = sys_Wallet[i][0];
        b->Wallets[i][1] = sys_Wallet[i][1];
    }
}

void copy_block(Bloque *dest, Bloque *orig)
{
    int i;

    if (orig == NULL || dest == NULL)
        return;

    dest->n_votos =    orig->n_votos;
    dest->votos_a = orig->votos_a;
    dest->n_mineros = orig->n_mineros;
    dest->obj = orig->obj;
    dest->pid = orig->pid;
    dest->sol = orig->sol;
    dest->id = orig->id;

    for (i = 0; i < MAX_MINERS; i++)
    {
        dest->Wallets[i][0] = orig->Wallets[i][0];
        dest->Wallets[i][1] = orig->Wallets[i][1];
    }
}