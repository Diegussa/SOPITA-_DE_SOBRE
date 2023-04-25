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


void minero(int n_threads, int n_secs, int pid, int PipeLect, int PipeEscr, sem_t *mutex);

void registrador(int PipeLect, int PipeEscr);

#endif