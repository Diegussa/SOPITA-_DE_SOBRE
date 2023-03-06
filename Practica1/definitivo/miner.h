/**
 * @file miner.h
 * @author Diego Rodriguez y Alejandro Garcia
 * @version 1
 * @date 2023-02-24
 *
 */

#ifndef MINER_H
#define MINER_H

#define MAX POW_LIMIT-1
#define MAX_HILOS 100

typedef struct _entradaHash entradaHash;

/**
 * @brief Prints the answer
 *
 * @param nHilos numero de hilos a crear
 * @param nbusquedas numero de rondas que se haran
 * @param busq solucion a buscar
 * 
 * @return nada
 */
void minero(int nHilos,long nbusquedas, long busq);

#endif