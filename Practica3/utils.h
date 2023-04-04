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
#define MILLON 1000000
#define MAX_LAG 1000 /*Un segundo*/
// #define DEBUG

typedef struct
{
  int obj, sol, fin;
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
void ournanosleep(int t);

/**
 * @brief Prints error message and finishes with error
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Nothing
 */
void error(char *str);

#endif