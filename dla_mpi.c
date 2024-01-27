#include "dla.h"
#include <mpi.h>

int main(int argc, char **argv)
{
    // Initialize MPI
    int my_rank, thread_count;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &thread_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Variables to calculate program execution time
    double my_start, my_finish, my_elapsed, elapsed;

    int width, height, iterations, num_particles, initial_x, initial_y;
    // Only the thread 0 has access to stdin
    if (my_rank == 0)
        get_input_parameters(argc, argv, &width, &height, &iterations, &num_particles, &initial_x, &initial_y);

    // Send input data to each thread
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_particles, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&initial_x, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&initial_y, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // DEBUG
    // printf("Thread %d, var %d %d %d %d %d %d\n", my_rank, width, height, iterations, num_particles, initial_x, initial_y);

    // ------------------- Starting point of measurement
    MPI_Barrier(MPI_COMM_WORLD);
    my_start = MPI_Wtime();

    // In this version only the first thread has the grid
    int **grid;
    if (my_rank == 0)
    {
        // Inizialize the grid dynamically, in order to avoid segmentation fault with sizes too large
        // 255 = empty cell
        // 1 = crystal
        // check enum grid_values
        grid = (int **)malloc(height * sizeof(int *));
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
    }

    // Dividing particles for each thread
    int my_num_particles = num_particles / thread_count;
    // if (my_rank == 1)
    //     my_num_particles += num_particles % thread_count;
    if (my_rank == 0)
        my_num_particles = num_particles;
    int my_particles[my_num_particles][3];
    if (my_rank == 0)
        my_num_particles = num_particles / thread_count;

    // particles_t particles[num_particles];

    // Seed the random number generator for each thread
    srand(time(NULL) + my_rank);

    // Giving each particles a random position on the grid
    for (int i = 0; i < my_num_particles; i++)
    {
        my_particles[i][0] = rand() % width;
        my_particles[i][1] = rand() % height;
        my_particles[i][2] = 0;
        // printf("Particella %d posizione x: %d y: %d\n", i, my_particles[i][0], my_particles[i][1]);
    }
    // Starting simulation
    for (int i = 0; i < iterations; i++)
    {
        // For each particle simulate Brownian motion.
        for (int p = 0; p < my_num_particles; p++)
        {
            if (my_particles[p][2] == 1)
                continue;

            // Generate random number among -1, 0 and 1
            int randomStepX = rand() % 3 - 1;
            int randomStepY = rand() % 3 - 1;

            // Update particle position
            my_particles[p][0] += randomStepX;
            my_particles[p][1] += randomStepY;

            // Ensure particles stay within the grid size
            // my_particles[p][0] = fmin(width - 1, fmax(0, my_particles[p][0]));
            my_particles[p][0] = 0 > my_particles[p][0] ? 0 : my_particles[p][0];
            my_particles[p][0] = width - 1 < my_particles[p][0] ? width - 1 : my_particles[p][0];
            // my_particles[p][1] = fmin(height - 1, fmax(0, my_particles[p][1]));
            my_particles[p][1] = 0 > my_particles[p][1] ? 0 : my_particles[p][1];
            my_particles[p][1] = height - 1 < my_particles[p][1] ? height - 1 : my_particles[p][1];
        }

        // Now check surrounding cells for a crystallized particle, for each particle
        if (my_rank == 0)
        {
            // Send thread particles to thread 0 in order to check for crystallized surroundings
            int particles[num_particles][3];
            MPI_Gather(my_particles, 3 * my_num_particles, MPI_INT, particles, 3 * my_num_particles, MPI_INT, 0, MPI_COMM_WORLD);
            for (int p = 0; p < num_particles; p++)
            {
                if (my_particles[p][2] == 1)
                    continue;
                for (int y = -1; y <= 1; y++)
                {
                    for (int x = -1; x <= 1; x++)
                    {
                        int checkX = particles[p][0] + x;
                        int checkY = particles[p][1] + y;

                        // Check if surrounding is within buondaries and the check if it's a crystal
                        if (checkX >= 0 && checkX < width && checkY >= 0 && checkY < height && grid[checkY][checkX] == CRYSTAL)
                        {
                            grid[particles[p][1]][particles[p][0]] = CRYSTAL;

                            // Set particle crystallized
                            particles[p][2] = 1;
                        }
                    }
                }
            }
            // Send updated crystallized particles to other threads
            MPI_Scatter(particles, 3 * my_num_particles, MPI_INT, my_particles, 3 * my_num_particles, MPI_INT, 0, MPI_COMM_WORLD);
        }
        else
        {
            // Send updated particles movements
            MPI_Gather(my_particles, 3 * my_num_particles, MPI_INT, NULL, 3 * my_num_particles, MPI_INT, 0, MPI_COMM_WORLD);
            // Receive updated crystallized particles from thread 0
            MPI_Scatter(NULL, 3 * my_num_particles, MPI_INT, my_particles, 3 * my_num_particles, MPI_INT, 0, MPI_COMM_WORLD);
        }

        // DEBUG
        // printf("Iterazione %d finita\n", i);
    }
    // ------------------- End point of measurement
    my_finish = MPI_Wtime();
    my_elapsed = my_finish - my_start;
    MPI_Reduce(&my_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Free the allocated memory
    if (my_rank == 0)
    {
        // Create image from grid
        grid_to_ppm(width, height, grid, "dla_mpi.ppm");

        for (int i = 0; i < height; i++)
            free(grid[i]);
        free(grid);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (my_rank == 0)
        printf("Execution time = %f us\n", my_elapsed * 1000000);
    MPI_Finalize();

    return 0;
}