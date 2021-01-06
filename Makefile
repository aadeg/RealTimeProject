MAIN = airport
CC = gcc
CFLAGS = -std=gnu99 -Wpedantic -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=5 -Waggregate-return -Wcast-qual  -Wswitch-default -Wswitch-enum  -Wconversion -Wunreachable-code -Wdouble-promotion

SRC_DIR = src
INCLUDE_DIRS = -Iinclude -I/usr/include

#---------------------------------------------------
# LIBS are the external libraries to be linked
#---------------------------------------------------
LIBS = -pthread -lrt -lm `allegro-config --libs`

#---------------------------------------------------
# Dependencies
#---------------------------------------------------
$(MAIN): main.o ptask.o graphics.o structs.o
	$(CC) $(CFLAGS) -o $(MAIN) main.o ptask.o graphics.o structs.o $(LIBS)

main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $(SRC_DIR)/main.c

ptask.o: $(SRC_DIR)/ptask.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $(SRC_DIR)/ptask.c

graphics.o: $(SRC_DIR)/graphics.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $(SRC_DIR)/graphics.c

structs.o: $(SRC_DIR)/structs.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $(SRC_DIR)/structs.c


#---------------------------------------------------
# Command that can be specified inline: make clean
#---------------------------------------------------
clean:
	rm ./*.o $(MAIN)

