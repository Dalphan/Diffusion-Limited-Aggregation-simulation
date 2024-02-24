# Diffusion-Limited-Aggregation-simulation
Progetto di Programmazione di Sistemi Embedded e Multicore 

Diffusion-limited aggregation (DLA) è un processo di formazione di cristalli nel quale le particelle si
muovono in uno spazio 2D con moto browniano (cioè in modo casuale) e si combinano tra loro
quando si toccano. DLA può essere simulato utilizzando una griglia 2D in cui ogni cella può essere
occupata da uno o più particelle in movimento. Una particella diventa parte di un cristallo (e si
ferma) quando si trova in prossimità di un cristallo già formato. I parametri di base della simulazione
sono la dimensione della griglia 2D, il numero iniziale di particelle, il numero di iterazioni e il "seme"
cristallino iniziale. Implementare l’algoritmo di DLA utilizzando 2 tra i seguenti approcci: MPI,
PThread/OpenMP, CUDA.
Verificare la correttezza degli algoritmi implementati, confrontando i risultati con quelli ottenuti da
una versione single-thread. Valutare le prestazioni degli algoritmi sviluppati in termini di speed-up
ed efficienza al variare del numero di processi/thread e delle dimensioni del problema (numero di
particelle, numero di iterazioni e dimensioni della griglia).

Usage
-----
To run, first download the folder:
    $ git clone https://github.com/Dalphan/Diffusion-Limited-Aggregation-simulation.git
    $ cd Diffusion-Limited-Aggregation-simulation
    $ make all

The executables will be in /bin directory.

Requirements
------------
OpenMP and MPI
