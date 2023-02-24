/**
 * @file mrush.h
 * @author Diego Rodriguez y Alejandro Garcia
 * @version 1
 * @date 2023-02-24
 *
 */

#define MAX POW_LIMIT-1
#define MAX_HILOS 100

typedef struct _entradaHash entradaHash;

/**
 * @brief Creates a threat that searches between 2 parameters
 *
 * @param arg Argument needed to create the threat:
 * Smallest value, biggest value and result to be found
 * @return The answer, if it finds the answer or -1
 */
void *func_minero(void *arg);

/**
 * @brief Prints the answer
 *
 * @param pipeLectura pipe to be readen from
 * @param pipeEscritura pipe where is going to be writen
 * @param nbusquedas number of rounds that are going to be done
 * @return EXIT_SUCCESS, if everything goes well or EXIT_FAILURE if there has been an error
 */
int monitor(int pipeLectura, int pipeEscritura,int nbusquedas);

void minero(int nHilos,long nbusquedas, long busq);

long minar(int nHilos,long nbusquedas, long busq, int pipeLectura,int pipeEscritura);