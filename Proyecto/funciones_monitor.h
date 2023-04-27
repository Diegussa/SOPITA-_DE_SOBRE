/**
 * @file funciones_monitor.h
 * @author Diego Rodríguez y Alejandro García
 * @brief
 * @version 3
 * @date 2023-04-1
 *
 */
#ifndef FUNCIONES_MONITOR_H
#define FUNCIONES_MONITOR_H

#include "utils.h"
#include "pow.h"

typedef struct _Shm_struct Shm_struct;


/**
 * @brief Funcion principal de la rutina comprobador
 * @author Alejandro García and Diego Rodríguez
 * @pre fd Descriptor al segmento de memoria compartida
 * @pre lag Espera indicada por el usuario
 * @pre semCTRL Semáforo que indica cuando se crean el resto de semáforos
 * @return ERROR si algo ha salido mal y OK en caso contrario
 */
STATUS comprobador(int fd, sem_t *semCtrl);

/**
 * @brief Funcion principal de la rutina monitor
 * @author Alejandro García and Diego Rodríguez
 * @pre fd Descriptor al segmento de memoria compartida
 * @pre lag Espera indicada por el usuario
 * @pre semCTRL Semáforo que indica cuando se han creado el resto de semáforos
 * @return ERROR si algo ha salido mal y OK en caso contrario
 */
STATUS monitor(int fd);

STATUS finish_comprobador(char *str, int fd_shm, Shm_struct *mapped, int n_sems, STATUS retorno);

#endif