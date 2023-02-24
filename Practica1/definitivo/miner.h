/**
 * @file mrush.h
 * @author Diego Rodriguez y Alejandro Garcia
 * @version 1
 * @date 2023-02-24
 *
 * @copyright Copyright (c) 2022
 *
 */

#define MAX POW_LIMIT-1
#define MAX_HILOS 100

typedef struct _entradaHash entradaHash;


void *func_minero(void *arg);

int monitor(int pipeLectura, int pipeEscritura,int nbusquedas);

long minero(int nHilos,long nbusquedas, long busq);

long minar(int nHilos,long nbusquedas, long busq, int pipeLectura,int pipeEscritura);