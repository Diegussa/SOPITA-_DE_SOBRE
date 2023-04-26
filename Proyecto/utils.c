/**
 * @file utils.c
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#include "utils.h"

int up(sem_t *sem)
{
  return sem_post(sem);
}

int down(sem_t *sem)
{
  return sem_wait(sem);
}

int down_try(sem_t *sem)
{
  return sem_trywait(sem);
}

void print_bloque(int fd, Bloque *bloque)
{
  int i;
  if (fd < 0 || !bloque)
    return;
  dprintf(fd, "Id:  %ld \n", bloque->id);
  dprintf(fd, "Winner:  %d \n", bloque->pid);
  dprintf(fd, "Target:  %ld \n", bloque->obj);

  if ((bloque->votos_a)  <= (bloque->n_votos / 2))
    dprintf(fd, "Target:  %ld (rejected)\n", bloque->sol);
  else
    dprintf(fd, "Target:  %ld (validated)\n", bloque->sol);

  dprintf(fd, "Votes:  %ld/%ld\n", bloque->votos_a, bloque->n_votos);
  dprintf(fd, "Wallets:");

  for (i = 0; i < bloque->n_mineros; i++)
    dprintf(fd, " %d:%d ",wallet_get_pid(&(bloque->Wallets[i])),  wallet_get_coins(&(bloque->Wallets[i])));
  
  dprintf(fd, " \n");
}

void nanorandsleep()
{
  struct timespec time = {0, rand() % BIG_PRIME};
  nanosleep(&time, NULL);
}

void ournanosleep(long t)
{
  long aux = t;
  struct timespec time;

  t = t / MIL;
  t = t / MIL;
  t = t / MIL;
  time.tv_sec = (time_t)t;
  time.tv_nsec = (time_t)aux - t * MIL * MIL * MIL;
  nanosleep(&time, NULL);
}

void error(char *str)
{
  if (str)
    perror(str);

  exit(EXIT_FAILURE);
}

void copy_block(Bloque *dest, Bloque *orig)
{
    int i;

    if (orig == NULL || dest == NULL)
        return;

    dest->n_votos = orig->n_votos;
    dest->votos_a = orig->votos_a;
    dest->n_mineros = orig->n_mineros;
    dest->obj = orig->obj;
    dest->pid = orig->pid;
    dest->sol = orig->sol;
    dest->id = orig->id;

    for (i = 0; i < MAX_MINERS; i++)
        copy_wallet(&(dest->Wallets[i]), &(orig->Wallets[i]));
}

void copy_wallet(Wallet *dest, Wallet *orig)
{
    if (orig == NULL || dest == NULL)
        return;

    dest->pid = orig->pid;
    dest->coins = orig->coins;
}

int wallet_get_coins(Wallet *wallet){
    if(!wallet)
        return -1;

    return wallet->coins;
}

void wallet_set_coins(Wallet *wallet, int coins){
    if(!wallet)
        return;

    wallet->coins=coins;
}

pid_t wallet_get_pid(Wallet *wallet){
    if(!wallet)
        return -1;

    return wallet->pid;
}

void wallet_set_pid(Wallet *wallet, pid_t pid){
    if(!wallet)
        return;

    wallet->pid=pid;
}