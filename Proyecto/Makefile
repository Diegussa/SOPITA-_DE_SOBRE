CC=gcc 
CFLAGS = -g -Wall -ggdb 


DFLAGS = -DDEBUG
IFLAGS=-I. ##La opción -I se usa para indicar donde se encuentran los ficheros cabeceras (.h)

LIBS= -lm -lpthread -lrt ### -lm enlaza la biblioteca matematica, -pthread enlaza la biblioteca de hilos

MON_OBJS = monitor.o funciones.o utils.o
MIN_OBJS = minero.o pow.o

## EXECUTIVES CREATION ##
#----------------------------------------------------------------------------------------------------------------------------

all: monitor minero

monitor: monitor.o utils.o pow.o
	$(CC) -o $@ $^ $(LIBS)

monitor.o: monitor.c utils.h pow.h
	$(CC) $(CFLAGS) -c $< 

funciones.o: funciones.c funciones.h utils.h pow.h
	$(CC) $(CFLAGS) -c $< 

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $< 


minero: minero.o funciones_minero.o pow.o utils.o
	$(CC) -o $@ $^ $(LIBS)

minero.o: minero.c funciones_minero.h pow.h utils.h
	$(CC) $(CFLAGS) -c $< 

funciones_minero.o: funciones_minero.c funciones_minero.h pow.h utils.h
	$(CC) $(CFLAGS) -c $< 

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c $< 
#----------------------------------------------------------------------------------------------------------------------------
run_all1: ./monitor 1000 ./monitor 500 ./minero 200 100  #Es posible que esto haga que se ejecuten en orden, no simultaneamente

run_mins3: 
	./minero 20 4 & ./minero 24 5 & ./minero 35 8

run_mins10: 
	./minero 20 40 & ./minero 24 50 & ./minero 35 80 & ./minero 20 4 & ./minero 24 5 & ./minero 35 8 & ./minero 45 30 & ./minero 44 5 & ./minero 60 3 & ./minero 34 10

run_mon:
	./monitor 100

run_monv:
	valgrind --leak-check=full --show-leak-kinds=all ./monitor 100

run_min:
	./minero 10 5

run_minv:
	valgrind --leak-check=full --show-leak-kinds=all ./minero 20 100
#----------------------------------------------------------------------------------------------------------------------------
clean:
	rm -v monitor minero *.o
	rm Debug_*
	rm reg*

clean_mon:
	rm -v monitor  $(MON_OBJS)

clean_min:
	rm -v  minero $(MIN_OBJS)
cs:
	ls /dev/shm
	ls /dev/mqueue

rs:
	 rm /dev/shm/shm*
	 rm /dev/shm/sem*
	 rm /dev/mqueue/mqueue
	
kill:
	ps -e | grep "minero" | cut -c-4 | xargs kill -9