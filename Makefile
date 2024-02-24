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
2OPENMP := $(BIN_DIR)/2openmp
MPI := $(BIN_DIR)/mpi
2MPI := $(BIN_DIR)/2mpi
3MPI := $(BIN_DIR)/3mpi
CIMG_WRAPPER := $(BIN_DIR)/cimg_wrapper.so

# Phony targets
.PHONY: all clean

all: $(SERIAL) $(OPENMP) $(2OPENMP) $(MPI) $(2MPI) $(3MPI) 

serial: $(SRC_DIR)/dla_serial.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/$@

serial_video: $(SRC_DIR)/dla_serial.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/$@ -L$(BIN_DIR) -lcimg_wrapper -lstdc++ -lX11

openmp: $(SRC_DIR)/dla_openmp.c $(SRC_DIR)/dla.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -fopenmp $< -o $(BIN_DIR)/$@

2openmp: $(SRC_DIR)/dla_2openmp.c $(SRC_DIR)/dla.h | $(BIN_DIR)
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
 

serial_test: $(SERIAL)
	$(SERIAL) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_serial.ppm

openmp_test5: $(OPENMP)
	$(OPENMP) 500 500 50000 20000 250 250 5
	feh $(RES_DIR)/dla_openmp.ppm

openmp_test10: $(OPENMP)
	$(OPENMP) 500 500 50000 20000 250 250 10
	feh $(RES_DIR)/dla_openmp.ppm

2openmp_test5: $(2OPENMP)
	$(2OPENMP) 500 500 50000 20000 250 250 5
	feh $(RES_DIR)/dla_2openmp.ppm

2openmp_test10: $(2OPENMP)
	$(2OPENMP) 500 500 50000 20000 250 250 10
	feh $(RES_DIR)/dla_2openmp.ppm

mpi_test5: $(MPI)
	mpirun -np 5 $(MPI) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_mpi.ppm

mpi_test10: $(MPI)
	mpirun -np 5 $(MPI) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_mpi.ppm

2mpi_test5: $(2MPI)
	mpirun -np 5 $(2MPI) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_2mpi.ppm

2mpi_test10: $(2MPI)
	mpirun -np 10 $(2MPI) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_2mpi.ppm

3mpi_test5: $(3MPI)
	mpirun -np 5 $(3MPI) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_3mpi.ppm

3mpi_test10: $(3MPI)
	mpirun -np 10 $(3MPI) 500 500 50000 20000 250 250
	feh $(RES_DIR)/dla_3mpi.ppm

big_2openmp: $(2OPENMP)
	$(2OPENMP) 1000 1000 100000 100000 500 500 10
	feh $(RES_DIR)/dla_2openmp.ppm

big_3mpi: $(3MPI)
	$(3MPI) 1000 1000 100000 100000 500 500 10
	feh $(RES_DIR)/dla_3mpi.ppm
