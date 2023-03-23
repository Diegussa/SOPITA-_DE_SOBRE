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
void voters();

STATUS votar();

/* 34 */
void create_sons(int n_procs);

/* 98 */
void send_signal_procs(int sig, int n_procs);



#endif