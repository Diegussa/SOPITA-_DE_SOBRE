#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pow.h"


int main(){
    printf("El valor de f(%d) es %ld", 1, pow_hash(1));


    return 0;
}