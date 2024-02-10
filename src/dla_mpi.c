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

    // ------------------- Starting point of measurement
    MPI_Barrier(MPI_COMM_WORLD);
    my_start = MPI_Wtime();

    // In this version only the first thread has the grid
    int *grid;
    MPI_Win window;
    if (my_rank == 0)
    {
        // Inizialize the grid dynamically, in order to avoid segmentation fault with sizes too large
        // 255 = empty cell
        // 1 = crystal
        // check enum grid_values
        MPI_Win_allocate((MPI_Aint)(height * width * sizeof(int)), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &grid, &window);

        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
                // grid[i][j] = EMPTY;
                grid[i * width + j] = EMPTY;
        }

        grid[initial_y * width + initial_x] = CRYSTAL;
    }
    else
    {
        // Window for other threads
        MPI_Win_allocate(0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &grid, &window);
    }

    // Dividing particles for each thread
    int my_num_particles = num_particles / thread_count;
    if (my_rank == 0)
        my_num_particles += num_particles % thread_count;
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

    int trewidth = 3 * width;
    int buffer[trewidth];
    particles_t particles_to_crystalize[width];
    int n_to_crystalize = 0;
    // int crystalize = 0;
    MPI_Aint position;
    MPI_Request req;

    MPI_Barrier(MPI_COMM_WORLD);

    // Starting simulation
    MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
    for (int i = 0; i < iterations; i++)
    {
        // For each particle simulate Brownian motion.
        for (int p = 0; p < my_num_particles; p++)
        {
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
            position = ((my_particles[p].y == 0 ? 1 : my_particles[p].y) - 1) * width;
            // MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
            MPI_Get(&buffer, trewidth, MPI_INT, 0, position, trewidth, MPI_INT, window);
            MPI_Win_flush(0, window);
            // MPI_Win_unlock(0, window);

            // buffer[(my_particles[p].y == 0 ? 0 : 1) * width + my_particles[p].x] = 2;

            // printf("BUFFER da posizione %ld per particella in posizione y %d x %d \n", position, my_particles[p].y, my_particles[p].x);
            // for (int y = 0; y < 3; y++)
            // {
            //     printf("RIGA %d ", y);
            //     for (int x = 0; x < width; x++)
            //     {
            //         printf("%d ", buffer[y * width + x]);
            //     }
            //     printf("\n");
            // }

            for (int y = -1; y <= 1; y++)
            {
                int checkY = my_particles[p].y + y;
                if (checkY < 0 || checkY >= height)
                    continue;

                for (int x = -1; x <= 1; x++)
                {
                    int checkX = my_particles[p].x + x;

                    // Check if surrounding is within buondaries and the check if it's a crystal
                    if (checkX >= 0 && checkX < width && buffer[(y + 1) * width + checkX] == CRYSTAL)
                    {
                        // In order to exit outer loop
                        y = 2;

                        // Save particle to crystalize
                        particles_to_crystalize[n_to_crystalize] = my_particles[p];
                        n_to_crystalize++;

                        // Remove crystalized particle
                        my_particles[p] = my_particles[my_num_particles - 1];
                        my_num_particles--;
                        p--;
                        break;
                    }
                }
                // if (crystalize)
                //     break;
            }

            // Crystalize the particle
            // if (crystalize)
            // {
            //     position = (MPI_Aint)(width * my_particles[p].y + my_particles[p].x);
            //     int value = CRYSTAL;
            //     MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, window);
            //     MPI_Put(&value, 1, MPI_INT, 0, position, 1, MPI_INT, window);
            //     MPI_Win_unlock(0, window);

            //     my_particles[p] = my_particles[my_num_particles - 1];
            //     my_num_particles--;
            //     p--;
            //     crystalize = 0;
            // }
        }

        // MPI_Barrier(MPI_COMM_WORLD);

        if (n_to_crystalize > 0)
        {
            int value = CRYSTAL;
            // MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);

            // MPI_Win_unlock(0, window);
            // MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
            for (int i = 0; i < n_to_crystalize; i++)
            {
                position = (MPI_Aint)(width * particles_to_crystalize[i].y + particles_to_crystalize[i].x);
                // MPI_Put(&value, 1, MPI_INT, 0, position, 1, MPI_INT, window);
                MPI_Rput(&value, 1, MPI_INT, 0, position, 1, MPI_INT, window, &req);
            }
            // MPI_Win_flush(0, window);

            // MPI_Win_unlock(0, window);

            // MPI_Win_unlock(0, window);
            // MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
            n_to_crystalize = 0;
        }

        // MPI_Barrier(MPI_COMM_WORLD);

        // DEBUG
        // if (i % 100 == 0)
        //     printf("Iterazione %d finita di rank %d\n", i, my_rank);
    }
    MPI_Win_unlock(0, window);

    // DEBUG
    // if (my_rank == 1)
    // {
    //     printf("width and height %d %d\n", width, height);
    //     int value = 0;
    //     for (int y = 0; y < height; y++)
    //     {
    //         printf("RIGA %d ", y);
    //         for (int x = 0; x < width; x++)
    //         {
    //             MPI_Win_lock(MPI_LOCK_SHARED, 0, 0, window);
    //             MPI_Get(&value, 1, MPI_INT, 0, (MPI_Aint)(y * width + x), 1, MPI_INT, window);
    //             MPI_Win_unlock(0, window);

    //             printf("%d ", value);
    //         }
    //         printf("\n");
    //     }
    // }

    // ------------------- End point of measurement
    my_finish = MPI_Wtime();
    my_elapsed = my_finish - my_start;
    MPI_Reduce(&my_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Free the allocated memory
    MPI_Barrier(MPI_COMM_WORLD);

    if (my_rank == 0)
    {
        // Create image from grid
        array_to_ppm(width, height, grid, "dla_mpi.ppm");

        // MPI_Free_mem(grid);
        printf("Execution time = %d us\n", (int)(my_elapsed * 1000000));
    }
    MPI_Win_free(&window);

    MPI_Finalize();

    return 0;
}