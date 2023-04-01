/**
 * @file funciones.h
 * @author Diego Rodríguez y Alejandro García
 * @brief 
 * @version 
 * @date 2023-04-1
 *
 */
#ifndef FUNCIONES_H
#define FUNCIONES_H

#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "utils.h"

/**
 * @brief Funcion principal de la rutina comprobador
 * @author Alejandro García and Diego Rodríguez
 * @pre fd Descriptor al segmento de memoria compartida
 * @pre lag Espera especificada por el usuario
 * @return ERROR si algo ha salido mal y OK en caso contrario
 */
STATUS comprobador(int fd, int lag);

/**
 * @brief Funcion principal de la rutina monitor
 * @author Alejandro García and Diego Rodríguez
 * @pre fd Descriptor al segmento de memoria compartida
 * @pre lag Espera especificada por el usuario
 * @return ERROR si algo ha salido mal y OK en caso contrario
 */
STATUS monitor(int fd,int lag);


#endif