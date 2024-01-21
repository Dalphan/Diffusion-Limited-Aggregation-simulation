serial: dla_serial.c dla.h
	gcc -Wall dla_serial.c -o serial

run_serial_s:
	./serial 500 500 50000 22000
	feh dla_serial.ppm

run_serial_xl:
	./serial 1000 1000 100000 100000
	feh dla_serial.ppm

openmp: dla_openmp.c dla.h
	gcc -Wall -g -fopenmp dla_openmp.c -o openmp

run_openmp_xs:
	./openmp 100 100 10000 2000 -1 -1 12
	feh dla_openmp.ppm

run_openmp_s:
	./openmp 500 500 50000 22000 -1 -1 12
	feh dla_openmp.ppm