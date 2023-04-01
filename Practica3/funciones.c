/**
 * @file funciones.h
 * @author Diego Rodríguez y Alejandro García
 * @brief 
 * @version 
 * @date 2023-04-1
 *
 */
#include "funciones.h"


STATUS comprobador(int fd_shm, int lag){
    /*Mapeará el segmento de memoria*/

    /*Recibirá un bloque a través de mensajes por parte del minero*/



    /*Comproborá con la función hash si es correcta y añadirá una bandera que indique si es correcta o no*/


    /*Se la trasladará al monitor a través de la memoria compartida*/
    /* Esquema de productro-consumidor (El es el productor)*/


    /*Realizará una espera de <lag> milisegundos */

    /*Repitirá el proceso hasta que reciba (de minero un bloque especial de finalización)*/

    /*Cuando se reciba dicho bloque lo introducirá en memoria compartida para avisar a Monitor de la fincalización*/

    /*Liberación de recursos y fin*/
}

STATUS monitor(int fd_shm, int lag){
    /*Mapeará el segmento de memoria*/

    /*Extaerá un bloque realizando de consumidor en el esquema productor-consumidor*/

    /*Mostrará por pantalla el resultado con la siguiente sintaxis*/
    /*Solution accepted: %08ld --> %08ld o Solution rejected: %08ld !-> %08ld (Siendo el promer numero el objetivo y el segundo la solución)*/

    /*Realizará una espera de <lag> milisegundos */

    /*Repetirá el ciclo de extracción y muestra hasta recibir un bloque especial que indique la finalización del sistema*/

    /*Liberación de recursos y fin*/

}


/*Estructura productor-consumidor recomenda: */

/*Productor {                            • Se garantizara que no se pierde ningun bloque con independencia del retraso que se introduzca en cada proceso.
    Down ( sem_empty ) ;
    Down ( sem_mutex ) ;                 • Se usara un buffer circular de 6 bloques en el segmento de memoria compartida
    A ~n adirElemento () ;
    Up ( sem_mutex ) ;                   • Se alojaran en el segmento de memoria compartida tres semaforos sin nombre para implementar el algoritmo de productor–consumidor.
    Up ( sem_fill ) ;
}
Consumidor {
    Down ( sem_fill ) ;
    Down ( sem_mutex ) ;
    ExtraerElemento () ;
    Up ( sem_mutex ) ;
    Up ( sem_empty ) ;
}
*/