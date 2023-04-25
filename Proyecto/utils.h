/**
 * @file utils.h
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */
#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "pow.h"

#define NO_PID -1
#define BIG_PRIME 573163
#define MIL 1000
#define MAX_LAG 1000 /*Un segundo*/
#define SHARED 2
#define MAX_MINERS 100
#define MAX_N_VOTES 1000
#define WORD_SIZE 1000
#define SHM_NAME "/shm_seg"

typedef struct
{
  long id, pid, obj, sol, votos_a, n_votos, n_mineros;
  long Wallets[MAX_MINERS][2];
} Bloque;

typedef struct
{
  sem_t *primer_proc;
  sem_t *MutexBAct;
  long Wallets[MAX_MINERS][2];
  long Votes_Min[MAX_MINERS][MAX_N_VOTES];
  long n_mineros;
  Bloque UltimoBloque;
  Bloque BloqueActual;
} System_info;

typedef struct
{
  int obj, sol;
} Message;

typedef enum
{
  ERROR = -1,
  OK
} STATUS; /*!< Enumeration of the different status values */

/**
 * @brief Adds 1 unit to the semaphore
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Returns 0 on succes, -1 on Error and more info in errno
 */
int up(sem_t *sem);

/**
 * @brief Blocking call to get the semaphore
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Returns 0 on succes, -1 on Error and more info in errno
 */
int down(sem_t *sem);

/**
 * @brief It is a non blocking down
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Returns 0 on succes, -1 on Error and more info in errno
 */
int down_try(sem_t *sem);

void print_bloque(int fd, Bloque *bloque);


/**
 * @brief Waits a random number of nanoseconds
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Nothing
 */
void nanorandsleep();

/**
 * @brief Waits a given number of nanoseconds
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Nothing
 */
void ournanosleep(long t);

/**
 * @brief Prints error message and finishes with error
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Nothing
 */
void error(char *str);

#endif