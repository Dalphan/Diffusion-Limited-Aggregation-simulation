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
    // MPI_Bcast(&initial_x, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // MPI_Bcast(&initial_y, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // DEBUG
    // printf("Thread %d, var %d %d %d %d %d %d\n", my_rank, width, height, iterations, num_particles, initial_x, initial_y);

    // ------------------- Starting point of measurement
    MPI_Barrier(MPI_COMM_WORLD);
    my_start = MPI_Wtime();

    // In this version only the first thread has the grid
    int **grid;
    MPI_Win window;
    if (my_rank == 0)
    {
        // Inizialize the grid dynamically, in order to avoid segmentation fault with sizes too large
        // 255 = empty cell
        // 1 = crystal
        // check enum grid_values
        MPI_Alloc_mem(height * sizeof(int *), MPI_INFO_NULL, &grid);
        if (grid == NULL)
        {
            printf("Could not allocate memory\n");
            return -1;
        }
        for (int i = 0; i < height; i++)
        {
            MPI_Alloc_mem(width * sizeof(int), MPI_INFO_NULL, &grid[i]);
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

        // Creating window for other threads
        MPI_Win_create(grid, height * width * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &window);
    }
    else
    {
        // Window for other threads
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &window);
        // Associate windows object to grid
        // MPI_Win_attach(window, grid, height * width * sizeof(int));
    }

    // Dividing particles for each thread
    int my_num_particles = num_particles / thread_count;
    if (my_rank == 0)
        my_num_particles = num_particles % thread_count;
    particles_t my_particles[my_num_particles];

    // Seed the random number generator for each thread
    srand(time(NULL) ^ my_rank);

    // Giving each particles a random position on the grid
    for (int i = 0; i < my_num_particles; i++)
    {
        my_particles[i].x = rand() % width;
        my_particles[i].y = rand() % height;
        // my_particles[i][2] = 0;
        // printf("Particella %d posizione x: %d y: %d\n", i, my_particles[i][0], my_particles[i][1]);
    }
    // Starting simulation
    for (int i = 0; i < iterations; i++)
    {
        // For each particle simulate Brownian motion.
        for (int p = 0; p < my_num_particles; p++)
        {
            // if (my_particles[p][2] == 1)
            //     continue;

            // Generate random number among -1, 0 and 1
            int randomStepX = rand() % 3 - 1;
            int randomStepY = rand() % 3 - 1;

            // Update particle position
            my_particles[p].x += randomStepX;
            my_particles[p].y += randomStepY;

            // Ensure particles stay within the grid size
            // my_particles[p].x = fmin(width - 1, fmax(0, my_particles[p].x));
            my_particles[p].x = 0 > my_particles[p].x ? 0 : my_particles[p].x;
            my_particles[p].x = width - 1 < my_particles[p].x ? width - 1 : my_particles[p].x;
            // my_particles[p].y = fmin(height - 1, fmax(0, my_particles[p].y));
            my_particles[p].y = 0 > my_particles[p].y ? 0 : my_particles[p].y;
            my_particles[p].y = height - 1 < my_particles[p].y ? height - 1 : my_particles[p].y;

            // Check if the particles has to be crystallized
            MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
            int crystalize = 0;
            MPI_Aint position;
            for (int y = -1; y <= 1; y++)
            {
                for (int x = -1; x <= 1; x++)
                {
                    int checkX = my_particles[p].x + x;
                    int checkY = my_particles[p].y + y;

                    // Check if surrounding is within buondaries and the check if it's a crystal
                    if (checkX >= 0 && checkX < width && checkY >= 0 && checkY < height)
                    {
                        position = (MPI_Aint)width * checkY + checkX;

                        int check;
                        MPI_Get(&check, 1, MPI_INT, 0, position, 1, MPI_INT, window);

                        if (check == CRYSTAL)
                        {
                            crystalize = 1;
                            break;
                        }
                    }
                }
            }
            MPI_Win_unlock(0, window);

            // Crystalize the particle
            // Crystalize the particle
            if (crystalize)
            {
                MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
                // grid[my_particles[p].x][my_particles[p].y] = CRYSTAL;

                position = (MPI_Aint)width * my_particles[p].y + my_particles[p].x;
                int value = 1;
                MPI_Put(&value, 1, MPI_INT, 0, position, 1, MPI_INT, window);

                MPI_Win_unlock(0, window);

                my_particles[p] = my_particles[my_num_particles - 1];
                my_num_particles--;
            }
        }

        // DEBUG
        printf("Iterazione %d finita di rank %d\n", i, my_rank);
    }
    // ------------------- End point of measurement
    my_finish = MPI_Wtime();
    my_elapsed = my_finish - my_start;
    MPI_Reduce(&my_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Free the allocated memory
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Win_free(&window);
    if (my_rank == 0)
    {
        // Create image from grid
        grid_to_ppm(width, height, grid, "dla_mpi.ppm");

        for (int i = 0; i < height; i++)
            MPI_Free_mem(grid[i]);
        MPI_Free_mem(grid);
    }
    else
    {
        // MPI_Win_detach(window, grid);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (my_rank == 0)
        printf("Execution time = %f us\n", my_elapsed * 1000000);
    MPI_Finalize();

    return 0;
}