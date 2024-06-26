#include "dla.h"
#include <omp.h>

int main(int argc, char **argv)
{
    // Struct to measure program execution time
    struct timeval start, stop;

    int width, height, iterations, num_particles, initial_x, initial_y;
    get_input_parameters(argc, argv, &width, &height, &iterations, &num_particles, &initial_x, &initial_y);

    int thread_count = 1;
    if (argc >= 8)
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
    particles_t particles[num_particles];

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
        }

        // Starting simulation, partilces divided for each thread
        int my_num_particles = num_particles / thread_count;
        if (my_rank == 0)
            my_num_particles += num_particles % thread_count;

        // Modified version with an array of particles for each thread, this way it is possible to remove crystallized
        // particles without worrying about race conditions
        int start_index = my_rank * my_num_particles;
        int length = (start_index + my_num_particles) - start_index;
        particles_t my_particles[my_num_particles];
        memcpy(my_particles, particles + start_index, length * sizeof(particles_t));

        for (int i = 0; i < iterations; i++)
        {
            // For each particle simulate Brownian motion, then examine the surroundings for crystallized particles.
            for (int p = 0; p < my_num_particles; p++)
            {
                // Generate random number among -1, 0 and 1
                int randomStepX = rand_r(&my_seed) % 3 - 1;
                int randomStepY = rand_r(&my_seed) % 3 - 1;

                // Update particle position
                my_particles[p].x += randomStepX;
                my_particles[p].y += randomStepY;

                // Ensure particles stay within the grid size
                my_particles[p].x = 0 > my_particles[p].x ? 0 : my_particles[p].x;
                my_particles[p].x = width - 1 < my_particles[p].x ? width - 1 : my_particles[p].x;
                my_particles[p].y = 0 > my_particles[p].y ? 0 : my_particles[p].y;
                my_particles[p].y = height - 1 < my_particles[p].y ? height - 1 : my_particles[p].y;

                // Now check surrounding cells for a crystallized particle
                for (int y = -1; y <= 1; y++)
                {
                    int checkY = my_particles[p].y + y;
                    if (checkY < 0 || checkY >= height)
                        continue;

                    for (int x = -1; x <= 1; x++)
                    {
                        int checkX = my_particles[p].x + x;

                        // Check if surrounding is within buondaries and the check if it's a crystal
                        if (checkX >= 0 && checkX < width && grid[checkY][checkX] == CRYSTAL)
                        {
                            grid[my_particles[p].y][my_particles[p].x] = CRYSTAL;

                            // Remove crystallized particle from array of particles
                            my_particles[p] = my_particles[my_num_particles - 1];
                            my_num_particles--;
                            p--;

                            y = 2; // In order to exit outer loop
                            break;
                        }
                    }
                }
            }
            #pragma omp barrier
        }
    }
    // ------------------- End point of measurement
    gettimeofday(&stop, NULL);

    // Create image from grid
    grid_to_ppm(width, height, grid, "dla_2openmp.ppm");

    // Free the allocated memory
    for (int i = 0; i < height; i++)
        free(grid[i]);
    free(grid);

    printf("Execution time:  %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    return 0;
}