# Compiler options
CC := gcc
CXX := g++
CFLAGS := -Wall -g
CXXFLAGS := -Wall -g

# Directories
SRC_DIR := src
BIN_DIR := bin
RES_DIR := res
LIB_DIR := lib

# Targets
SERIAL := $(BIN_DIR)/serial
OPENMP := $(BIN_DIR)/openmp
FASTER_OPENMP := $(BIN_DIR)/faster_openmp
MPI := $(BIN_DIR)/mpi
CIMG_WRAPPER := $(BIN_DIR)/cimg_wrapper.so

# Phony targets
.PHONY: all clean

all: $(SERIAL) $(OPENMP) $(FASTER_OPENMP) $(MPI) $(CIMG_WRAPPER)

serial: $(SRC_DIR)/dla_serial.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/$@ -L$(BIN_DIR) -lcimg_wrapper -lstdc++ -lX11

openmp: $(SRC_DIR)/dla_openmp.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $< -o $(BIN_DIR)/$@

faster_openmp: $(SRC_DIR)/dla_openmp_faster.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $< -o $(BIN_DIR)/$@

mpi: $(SRC_DIR)/dla_mpi.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	mpicc -g -Wall $< -o $(BIN_DIR)/$@

2mpi: $(SRC_DIR)/dla_2mpi.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	mpicc -g -Wall $< -o $(BIN_DIR)/$@

3mpi: $(SRC_DIR)/dla_3mpi.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	mpicc -g -Wall $< -o $(BIN_DIR)/$@

cimg_wrapper: $(SRC_DIR)/cimg_wrapper.cpp | $(BIN_DIR)
	$(CXX) -shared -fPIC -o $(BIN_DIR)/libcimg_wrapper.so $< -I$(LIB_DIR)/CImg/

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)/*

run_serial_xs: $(SERIAL)
	$(SERIAL) 100 100 10000 2000
	feh $(RES_DIR)/dla_serial.ppm

run_serial_s: $(SERIAL)
	$(SERIAL) 500 500 50000 22000
	feh $(RES_DIR)/dla_serial.ppm

run_serial_xl: $(SERIAL)
	$(SERIAL) 1000 1000 100000 100000
	feh $(RES_DIR)/dla_serial.ppm

run_openmp_xs: $(OPENMP)
	$(OPENMP) 100 100 10000 2000 -1 -1 12
	feh $(RES_DIR)/dla_openmp.ppm

run_openmp_s: $(OPENMP)
	$(OPENMP) 500 500 50000 22000 -1 -1 12
	feh $(RES_DIR)/dla_openmp.ppm

run_faster_openmp_s: $(FASTER_OPENMP)
	$(FASTER_OPENMP) 500 500 50000 22000 -1 -1 12
	feh $(RES_DIR)/dla_openmp_faster.ppm

run_mpi: $(MPI)
	mpirun -np 12 $(MPI) 100 100 10000 1500 -1 -1
