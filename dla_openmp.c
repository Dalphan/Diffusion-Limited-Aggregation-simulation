#include "dla.h"
#include <omp.h>

int main(int argc, char **argv)
{
    // Struct to calculate program execution time
    struct timeval start, stop;

    int width, height, iterations, num_particles, initial_x, initial_y;
    get_input_parameters(argc, argv, &width, &height, &iterations, &num_particles, &initial_x, &initial_y);

    int thread_count = 1;
    if (argc >= 7)
        thread_count = atoi(argv[7]);

    // ------------------- Starting point of measurement
    gettimeofday(&start, NULL);

    // Inizialize the grid dynamically, in order to avoid segmentation fault with sizes too large
    // 255 = empty cell
    // 1 = crystal
    // check enum grid_values
    int **grid = (int **)malloc(height * sizeof(int *));
    if (grid == NULL)
    {
        printf("Could not allocate memory\n");
        return -1;
    }
    for (int i = 0; i < height; i++)
    {
        grid[i] = (int *)malloc(width * sizeof(int));
        if (grid[i] == NULL)
        {
            printf("Could not allocate memory\n");
            return -1;
        }
        for (int j = 0; j < width; j++)
            grid[i][j] = EMPTY;
    }

    // Starting crystal
    grid[initial_y][initial_x] = CRYSTAL;

    srand(time(NULL));
    struct particles_t particles[num_particles];

// Divide work for each thread
#pragma omp parallel num_threads(thread_count)
    {
        int my_rank = omp_get_thread_num();
        unsigned int my_seed = time(NULL) ^ my_rank;

        // Giving each particles a random position on the grid, parallelized for each thread
#pragma omp for
        for (int i = 0; i < num_particles; i++)
        {
            particles[i].x = rand_r(&my_seed) % width;
            particles[i].y = rand_r(&my_seed) % height;
            // DEBUG
            // printf("Particella %d posizione x: %d y: %d\n", i, particles[i].x, particles[i].y);
        }

        // Starting simulation, iterations parallelized among threads
        // int my_iterations = iterations / thread_count;
        int my_iterations = iterations;
        int my_num_particles = num_particles / thread_count;
        if (my_rank == 0)
        {
            // my_iterations += iterations % thread_count;
            my_num_particles += num_particles % thread_count;
        }
        int start_for = my_rank * my_num_particles;
        int end_for = start_for + my_num_particles;

        for (int i = 0; i < my_iterations; i++)
        {
            // For each particle simulate Brownian motion, then examine the surroundings for crystallized particles.
            for (int p = start_for; p < end_for; p++)
            {
                if (grid[particles[p].y][particles[p].x] == CRYSTAL)
                    continue;

                // Generate random number among -1, 0 and 1
                int randomStepX = rand_r(&my_seed) % 3 - 1;
                int randomStepY = rand_r(&my_seed) % 3 - 1;

                // Update particle position
                particles[p].x += randomStepX;
                particles[p].y += randomStepY;

                // Ensure particles stay within the grid size
                particles[p].x = 0 > particles[p].x ? 0 : particles[p].x;
                particles[p].x = width - 1 < particles[p].x ? width - 1 : particles[p].x;
                particles[p].y = 0 > particles[p].y ? 0 : particles[p].y;
                particles[p].y = height - 1 < particles[p].y ? height - 1 : particles[p].y;

                // Now check surrounding cells for a crystallized particle
                for (int y = -1; y <= 1; y++)
                {
                    for (int x = -1; x <= 1; x++)
                    {
                        int checkX = particles[p].x + x;
                        int checkY = particles[p].y + y;

                        // Check if surrounding is within buondaries and the check if it's a crystal
                        if (checkX >= 0 && checkX < width && checkY >= 0 && checkY < height && grid[checkY][checkX] == CRYSTAL)
                        {
                            grid[particles[p].y][particles[p].x] = CRYSTAL;
                            // Remove crystallized particle from array of particles
                            // particles[p] = particles[num_particles - 1];
                            // num_particles--;
                        }
                    }
                }
            }
            // DEBUG
            // printf("Iterazione %d finita del thread %d\n", i, my_rank);
        }
    }
    // ------------------- End point of measurement
    gettimeofday(&stop, NULL);

    // Create image from grid
    grid_to_ppm(width, height, grid, "dla_openmp.ppm");

    // Free the allocated memory
    for (int i = 0; i < height; i++)
        free(grid[i]);
    free(grid);

    printf("execution time:  %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    return 0;
}