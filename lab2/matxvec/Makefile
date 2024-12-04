MPICC=mpicc

FLAGS=
STD=c23

CFLAGS=-std=$(STD) $(FLAGS) -O3 -Wall -lm -pthread

INC_DIR=inc
SRC_DIR=src
OUT_DIR=bin

INC=-I$(INC_DIR)/

SRC=$(SRC_DIR)/main.c $(SRC_DIR)/matxvec.c
OUT=$(OUT_DIR)/matxvec.o

all: $(SRC)
	mkdir -p bin/
	$(MPICC) $(CFLAGS) $(INC) $(SRC) -o $(OUT)

test: $(SRC)
	mkdir -p bin/
	$(MPICC) $(CFLAGS) $(INC) $(SRC) -DTEST -o $(OUT)

PHONY: clean

clean:
	rm -f ./$(OUT_DIR)/*.o
