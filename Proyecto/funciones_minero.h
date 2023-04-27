/**
 * @file funciones_minero.h
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */
#ifndef FUNCIONES_MINERO_H
#define FUNCIONES_MINERO_H

#include "utils.h"
#include "pow.h"

#define nameSemNmin "/semNmin"


void minero(int n_threads, int n_secs, int pid, int PipeEscr, sem_t *mutex);

void registrador( int PipeLect);

long minar(int n_threads, int obj);
void *func_minero(void *arg);
void ganador(System_info *sys, int obj, int sol, int proc_index, sem_t *mutexSysInfo, mqd_t mq);
void perdedor(System_info *sys, sigset_t *oldmask, int index_proc);
void _handler_minero(int sig);
STATUS set_handlers(int *sig, int n_signals, struct sigaction *actSIG, sigset_t *oldmask1,sigset_t *oldmask, void (*handler)(int));

#endif