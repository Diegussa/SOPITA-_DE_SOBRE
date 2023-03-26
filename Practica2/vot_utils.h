/**
 * @file vot_utils.h
 * @author Diego Rodríguez y Alejandro García
 * @brief 
 * @version 
 * @date 2023-03-23
 *
 */
#ifndef VOT_UTILS_H
#define VOT_UTILS_H

#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define NOMBREFICHERO "hijosPID.txt"
#define NO_PID -1
typedef enum
{
  ERROR = -1,
  OK
} STATUS; /*!< Enumeration of the different status values */
/*Returns 0 on succes, -1 on Error and more info in errno*/
int up(sem_t *sem);

/*Returns 0 on succes, -1 on Error and more info in errno*/
int down(sem_t *sem);

/*It is a non blocking down*/
int down_try(sem_t *sem);

void end_processes(int n_procs);
void end_failure(sem_t *semV, sem_t *semC);



/*Changes the handler of the specified signals*/
STATUS set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask, void (*handler)(int));
/*Sends a signal to all the sons except the one specified in the third argument*/
STATUS send_signal_procs(int sig, int n_hijos, long pid);

#endif