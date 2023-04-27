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

#define NO_PARAMETER -1
#define BIG_PRIME 573163
#define MIL 1000
#define MAX_LAG 1000 /*Un segundo*/
#define SHARED 2
#define MAX_MINERS 100
#define MAX_N_VOTES 1000
#define WORD_SIZE 1000
#define SHM_NAME "/shm_seg"
#define MQ_NAME "/mqueue"
#define Q_SIZE 5

#define TESTS

typedef struct
{
  pid_t pid;
  int coins;
} Wallet;

typedef struct
{
  long id, obj, sol;
  long votos_a, n_votos, n_mineros;
  pid_t pid;
  Wallet Wallets[MAX_MINERS];
} Bloque;

typedef struct
{
  sem_t primer_proc;
  sem_t MutexBAct;
  Wallet Wallets[MAX_MINERS];
  long Votes_Min[MAX_MINERS];
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
 * @param sem Pointer to the semaphore
 *
 * @return Returns 0 on succes, -1 on Error and more info in errno
 */
int up(sem_t *sem);

/**
 * @brief Blocking call to get the semaphore
 * @author Alejandro García and Diego Rodríguez
 * @param sem Pointer to the semaphore
 *
 * @return Returns 0 on succes, -1 on Error and more info in errno
 */
int down(sem_t *sem);

/**
 * @brief It is a non blocking down
 * @author Alejandro García and Diego Rodríguez
 * @param sem Pointer to the semaphore
 *
 * @return Returns 0 on succes, -1 on Error and more info in errno
 */
int down_try(sem_t *sem);

void print_bloque(int fd, Bloque *bloque);
void fprint_bloque(FILE *fd, Bloque *bloque);

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
 * @param str String to be printed
 *
 * @return Nothing
 */
void error(char *str);

/**
 * @brief Copies 
 * @author Alejandro García and Diego Rodríguez
 *
 * @return Nothing
 */
void copy_block(Bloque *dest, Bloque *orig);
void copy_wallet(Wallet *dest, Wallet *orig);
int wallet_get_coins(Wallet *wallet);
void wallet_set_coins(Wallet *wallet, int coins);
pid_t wallet_get_pid(Wallet *wallet);
void wallet_set_pid(Wallet *wallet, pid_t pid);


STATUS block_all_signal(sigset_t *oldmask);
void send_signals_miners(Wallet * w, int no_index, int signal);

#endif
