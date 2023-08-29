OBJ_DIR := ./obj
SRC_DIR := ./src
INCLUDES_DIR := ./includes
BUILD_DIR := ./bin
objects := $(patsubst %.o, $(OBJ_DIR)/%.o, master.o stringlist.o syncqueue.o workerpool.o utils.o collector.o sckset.o orderedrecordlist.o)
objects_farm := $(patsubst %.o, $(OBJ_DIR)/%.o, master.o stringlist.o syncqueue.o workerpool.o utils.o)
objects_collector := $(patsubst %.o, $(OBJ_DIR)/%.o, collector.o sckset.o orderedrecordlist.o utils.o)

CC = gcc
FLAGS = -Wall -pedantic -I $(INCLUDES_DIR)


.PHONY: test clean all

all: bin/farm bin/collector

bin/farm : $(objects_farm)
	$(CC) $(FLAGS) $^ -o $@

$(BUILD_DIR)/collector : $(objects_collector)
	$(CC) $(FLAGS) $^ -o $@


obj/%.o : source/%.c
	$(CC) -c $(FLAGS) $^ -o $@

clean: 
	-rm $(objects) $(wildcard *~) core


