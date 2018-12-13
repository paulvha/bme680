# makefile for BME680. october 2018 / paulvha

CC = gcc
DEPS = bcm2835.h twowire.h rasp_BME680.h bme680_defs.h bme680.h
OBJ = bme680_lib.o bme680.o bme680m.o
LIBS = -lm -ltwowire -lbcm2835

.cpp.o: %c $(DEPS)
	$(CC) -Wall -Werror -c -o $@ $<

bme680m : $(OBJ)
	$(CC) -o $@ $^ $(LIBS)

.PHONY : clean

clean :
	rm bme680m $(OBJ)
