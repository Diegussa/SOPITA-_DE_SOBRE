/**
 * @file vot_utils.h
 * @author Diego Rodríguez y Alejandro García
 * @brief 
 * @version 
 * @date 2023-03-23
 *
 */
#ifndef VOT_FUNC_H
#define VOT_FUNC_H

#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>

#include "vot_utils.h"





/*Voting when the voter has executed an effective down*/
STATUS votingCarefully();

/*Generates n_procs and writes their PID in a file*/
void create_sons(int n_procs, char *nameSemV, char *nameSemC, sem_t *semV, sem_t *semC);

/*Main function executed by the sons*/
void voters(char * semVoter, char * semCand,int n_procs, sem_t *semV, sem_t *semC);

void candidato(int n_procs);
#endif
