
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <mqueue.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include "pow.h"
#include "utils.h"
#include "funciones_minero.h"

#define INIC 0
#define MAX_THREADS 100
#define MAX POW_LIMIT - 1
#define WAIT 100 * MIL *MIL
#define MAX_TRY 100
#define DEBUG

int encontrado;

volatile sig_atomic_t got_sigUSR1 = 0;
volatile sig_atomic_t got_sigUSR2 = 0;
volatile sig_atomic_t got_sigTERM = 0;

typedef struct
{
    long ep, eu, res;
} Entrada_Hash;

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

/*Si hay error se gestiona internamente y se realiza un exit()*/
void minero(int n_threads, int n_secs, int pid, int PipeEscr, sem_t *mutexSysInfo)
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
    down(mutexSysInfo);
    if ((handler_info_sist = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) != ERROR) /*Si la memoria no está creada*/
    {
        printf("Creación de memoria\n");
        proc_index = 0;

        /*Asignación de tamaño*/
        if (ftruncate(handler_info_sist, sizeof(System_info)) == ERROR) /*CORREGIR, en caso de error se lleva el down del mutexSysInfo*/
            errorClose("Error truncating the shared memory segment info_sist", handler_info_sist);

        /*Enlazamiento al espacio de memoria*/
        if ((s_info = (System_info *)mmap(NULL, sizeof(System_info), PROT_READ | PROT_WRITE, MAP_SHARED, handler_info_sist, 0)) == MAP_FAILED)
            errorClose("Error mapping the shared memory segment info_sist", handler_info_sist);

        /*Inicializacion de la memoria compartida*/
        if (sem_init(&(s_info->primer_proc), SHARED, 1) == ERROR)
            error("sem_open primer_proc");
        if (sem_init(&(s_info->MutexBAct), SHARED, 1) == ERROR)
            error("sem_open MutexBact");

        for (i = 0; i < MAX_MINERS; i++)
        {
            wallet_set_pid(&(s_info->Wallets[i]), 0);
            wallet_set_coins(&(s_info->Wallets[i]), 0);

            s_info->Votes_Min[i] = 0;
        }
        s_info->n_mineros = 1;

        s_info->UltimoBloque.id = -1;
        s_info->BloqueActual.obj = INIC;
        s_info->BloqueActual.id = 0;
        obj = INIC;
        got_sigUSR1 = 1;
    }
    else /*Si no es el primero simplemente abré el segmento, lo enlaza. Debe garantizarse que el sistema esta listo (Semáforo)*/
    {
        printf("r1\n");
        if ((handler_info_sist = shm_open(SHM_NAME, O_RDWR, 0)) == ERROR) /*Control de errores*/
            errorClose(" Error opening the shared memory segment info_sist", handler_info_sist);
        printf("r2\n");
        if ((s_info = (System_info *)mmap(NULL, sizeof(System_info), PROT_READ | PROT_WRITE, MAP_SHARED, handler_info_sist, 0)) == MAP_FAILED)
        {
            close(handler_info_sist);
            error("Error mapping the shared memory segment info_sist");
        }
        printf("r3\n");
        /*Looks for the first empty space*/
        for (proc_index = 0; proc_index < MAX_MINERS && wallet_get_pid(&(s_info->Wallets[i])) != 0; proc_index++)
            ;

        printf("r4\n");
        if (proc_index == MAX_MINERS) /*En caso de que ya halla el máximo de mineros*/
            errorClose("Number of miners reached the maximum", handler_info_sist);

        /*Registrar sus datos en el sistema (PIDs ,n_mineros...)*/
        wallet_set_pid(&(s_info->Wallets[proc_index]), getpid());
        wallet_set_coins(&(s_info->Wallets[proc_index]), 0);
        s_info->n_mineros++;
        obj = s_info->BloqueActual.obj;
    }
    close(handler_info_sist);
    up(mutexSysInfo);
    /*Preparación de la ronda: */
    /*Si es el primero establece un objetivo inicial*/
    printf("salgo de lo anterior\n");
    /*Si no*/
    if (write(PipeEscr, &(s_info->BloqueActual), sizeof(Bloque)) == -1)
        exit(EXIT_FAILURE);
    while (!got_sigTERM)
    {
        /*Espera el inicio de una ronda (SIGUSR1)*/
        while (!got_sigUSR1)
            sigsuspend(&oldmask);
        got_sigUSR1 = 0;

        /*Salvo en la primera se debe enviar el bloque de la ronda anterior al proceso Registrador*/
        if (s_info->UltimoBloque.id != -1) /*Comprobación de que no es la primera ronda*/
        {
            if (write(PipeEscr, &(s_info->UltimoBloque), sizeof(Bloque)) == -1)
                exit(EXIT_FAILURE);
            obj = s_info->UltimoBloque.sol;
        }
        printf("Acabo de mandar un paquete por el pipe\n");
        /*Ronda de minado:*/
        /*Bucle de minería y mandar las soluciones hasta que nos llegue un alarm de n_secs o un SIGINT*/
        printf("Obj : %d\n", obj);
        /*Minado*/
        sol = minar(n_threads, obj);

#ifdef DEBUG
        printf("Minamos %d->%d-\n", obj, sol);
#endif
        printf("Sol : %d\n", sol);

        /*Tras salir de la minería se elige a un ganador de forma que evite las condiciones de carrera (semáforo)*/
        if (down_try(&(s_info->primer_proc)) != ERROR)
        { /*El primero se proclama proceso: Ganador*/
            ganador(s_info, obj, sol, proc_index, mutexSysInfo);
        }
        else /*El resto procesos: Perdedor*/
        {
            perdedor(s_info, &oldmask, proc_index);
        }
        printf("178\n");
    }

    /*Gestionar finalización:*/
    /*Razones por las que finaliza el proceso : SIGALARM o SIGINT

    Si es el último minero se libera todo y se envia al monitor (si está activo) el cierre del sistema*/
    printf("179\n");
    down(mutexSysInfo);
    s_info->n_mineros--;
    if (s_info->n_mineros == 0)
    {
        if (munmap(s_info, sizeof(System_info)) == ERROR)
            exit(EXIT_FAILURE);
    }
    up(mutexSysInfo);
    printf("180\n");

    /*Finalmente esperar al proceso Registrador  e imprime un mensaje en caso de EXIT_FAILURE (de Registrador)
     */

    close(PipeEscr);

    return;
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

    for (i = e->ep; (i < e->eu) && (encontrado == 0) && (got_sigUSR2 == 0); i++) /*Si se recibe una SIGUSR2 u otro hilo halla una solución significa que un proceso a encontrado la solución*/
    {
        if (pow_hash(i) == e->res)
        {
            encontrado = 1;
            pthread_exit((void *)i);
        }
    }

    pthread_exit((void *)(-1));
}

void ganador(System_info *sys, int obj, int sol, int proc_index, sem_t *mutexSysInfo)
{
    int i;
    /*Prepara memoria compartida para la votación introduciendo la solución en el bloque actual
    e inicializando el listado de votos*/

    while (down(&(sys->MutexBAct)) == ERROR)
        ;

    copy_block(&(sys->UltimoBloque), &(sys->BloqueActual));
    init_block(&(sys->BloqueActual), sys->Wallets, sys->UltimoBloque.id + 1, obj, sol);

    /*Enviar la señal SIGUSR2 indicando que la votación puede arrancar*/
    down(mutexSysInfo);
    sys->BloqueActual.n_mineros = sys->n_mineros;
    for (i = 0; i < MAX_MINERS; i++)
    {
        if (wallet_get_pid(&(sys->Wallets[i])) && (i != proc_index))
            (void)kill(wallet_get_pid(&(sys->Wallets[i])), SIGUSR2);
    }
    up(mutexSysInfo);
    up(&(sys->MutexBAct));

    printf("347\n");
    /*Realiza esperas no activas cortas hasta que todos los procesos hallan votado o transcurra
       el máximo numero de intentos(podemos calentarnos y intentar quitar eso)*/
    for (i = 0; i < MAX_TRY; i++)
    {
        if (sys->BloqueActual.n_mineros == sys->BloqueActual.n_votos)
            break;
        printf("esperando\n");
        ournanosleep(WAIT);
    }
    printf("356\n");

    /*Una vez terminada la votación se comprueba el resulatado y se introduce el bloque correspondiente de memoria */
    /* Si se aceptado recibe una moneda ( aumenta el contador )*/
    if (sys->BloqueActual.votos_a <= sys->BloqueActual.n_votos / 2)
        wallet_set_coins(&(sys->BloqueActual.Wallets[proc_index]), wallet_get_coins(&(sys->BloqueActual.Wallets[proc_index])) + 1);
    /*Ahora se debe enviar el bloque resuelto por cola de mensajes al monitor si hay alguno activo*/
    /* Se deber´an implementar los mecanismos adecuados para garantizar que el envío se realiza si y solo si
    el monitor est´a esperando para recibirlo, por ejemplo a trav´es de un semaforo con nombre.*/
    printf("365\n");

    /*Reiniciar la votación con el envio de un SIGUSR1 a todos los mineros*/
    down(mutexSysInfo);
    for (i = 0; i < MAX_MINERS; i++)
    {
        if (wallet_get_pid(&(sys->Wallets[i])) != 0)
            (void)kill(wallet_get_pid(&(sys->Wallets[i])), SIGUSR1);
    }
    up(mutexSysInfo);
    printf("43\n");
    return;
}

/*Debemos elegir si el error se gestiona aquí o devolvemos error*/
void perdedor(System_info *sys, sigset_t *oldmask, int index_proc)
{
    /*Si no le ha llegado aún la señal SIGUSR2 se espera a que la votación este lista*/
    while (!got_sigUSR2)
        sigsuspend(oldmask);
    got_sigUSR2 = 0;
    /*Exclusion Mutua Votar*/
    while (down(&(sys->MutexBAct)) == ERROR)
        ;
#ifdef TEST
    nanorandsleep();
#endif
    sys->BloqueActual.n_votos++;
    sys->Votes_Min[index_proc] = 0;
    if (pow_hash(sys->BloqueActual.sol) == sys->BloqueActual.obj)
    {
        sys->BloqueActual.votos_a++;
        sys->Votes_Min[index_proc] = 1;
    }

    if (up(&(sys->MutexBAct)) == ERROR)
        error("Error in voters");
    /*Una vez recibido comprueba el bloque actual y registra su voto en función de si la solución es correcta*/

    /*Entra en una espera no activa (Se realizará fuera de la función) hasta la siguiente votación*/
}
/*Si hay error se gestiona internamente y se realiza un exit()*/
void registrador(int PipeLect)
{
    int nbytes, val, fd;
    Bloque bloque;
    char Filename[WORD_SIZE];

    sprintf(Filename, "reg%d.dat", getppid());
printf("%s\n", Filename);

    if (!(fd = open(Filename, O_APPEND)))
    {
        exit(EXIT_FAILURE);
    }
    while (1)
    {
        printf("2\n");
        if (read(PipeLect, &bloque, sizeof(Bloque)) == -1)
        {
            perror("Error reading bloque");
            close(PipeLect);
            exit(EXIT_FAILURE);
        }
        if (nbytes == 0) /*Si no hay escritores*/
        {
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

void init_block(Bloque *b, Wallet *sys_Wallets, long id, long obj, long sol)
{
    int i;

    if (b == NULL || !sys_Wallets)
        return;
    b->n_votos = 1;
    b->votos_a = 1;
    b->obj = obj;
    b->pid = getpid();
    b->sol = sol;
    b->id = id;


    for (i = 0; i < MAX_MINERS; i++)
        copy_wallet(b->Wallets, sys_Wallets);
}
