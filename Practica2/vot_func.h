/**
 * @file vot_func.h
 * @author Diego Rodríguez y Alejandro García
 * @brief 
 * @version 
 * @date 2023-03-23
 *
 */
#ifndef VOT_FUNC_H
#define VOT_FUNC_H


#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>


#define NO_PID -1

typedef enum
{
  ERROR,
  OK
} STATUS; /*!< Enumeration of the different status values */


/*Returns 0 on succes, -1 on Error and more info in errno*/
int up(sem_t *sem);

/*Returns 0 on succes, -1 on Error and more info in errno*/
int down(sem_t *sem);

/*Returns 0 on succes, -1 on Error and more info in errno*/

/*It is a non blocking down*/
int down_try(sem_t *sem);

/* 60 */
void voters(char * semVoter, char * semCand,int n_procs, sem_t *semV, sem_t *semC);

STATUS votingCarefully();

/* 34 */
void create_sons(int n_procs, char *nameSemV, char *nameSemC, sem_t *semV, sem_t *semC);

/* 98 */
void send_signal_procs(int sig, int n_procs, long pid);

void end_processes(int n_procs);

void end_failure(sem_t * semV, sem_t *semC);

void candidato(int n_procs);
#endif
