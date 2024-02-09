#include "dla.h"
#include <mpi.h>

int main(int argc, char **argv)
{
    // Initialize MPI
    int my_rank, process_count;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    // Variables to measure program execution time
    double my_start, my_finish, my_elapsed, elapsed;

    int width, height, iterations, num_particles, initial_x, initial_y;
    // Only the process 0 has access to stdin
    if (my_rank == 0)
        get_input_parameters(argc, argv, &width, &height, &iterations, &num_particles, &initial_x, &initial_y);

    // Send input data to each process
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&iterations, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&num_particles, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // These variables are not needed
    // MPI_Bcast(&initial_x, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // MPI_Bcast(&initial_y, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Create comm for shared memory
    MPI_Comm shcomm;
    MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &shcomm);

    int shm_rank, shm_procs;
    MPI_Comm_size(shcomm, &shm_procs);
    MPI_Comm_rank(shcomm, &shm_rank);

    // Check if all the process are in the share communicator
    if (shm_procs != process_count)
        MPI_Abort(MPI_COMM_WORLD, 1);

    // ------------------- Starting point of measurement
    MPI_Barrier(MPI_COMM_WORLD);
    my_start = MPI_Wtime();

    // In this version the first process initialize the grid in the shared memory
    int *grid;
    MPI_Win window;
    if (my_rank == 0)
    {
        // Inizialize the grid dynamically, in order to avoid segmentation fault with sizes too large
        // 255 = empty cell
        // 1 = crystal
        // check enum grid_values
        MPI_Win_allocate_shared((MPI_Aint)(height * width * sizeof(int)), sizeof(int), MPI_INFO_NULL, shcomm, &grid, &window);

        for (int i = 0; i < height * width; i++)
            grid[i] = EMPTY;

        grid[initial_y * width + initial_x] = CRYSTAL;
    }
    else
    {
        // Window for other processes
        MPI_Win_allocate_shared(0, 1, MPI_INFO_NULL, shcomm, &grid, &window);
    }

    MPI_Barrier(shcomm);

    // CHECK WHETHER UNIFIED MEMORY IS SUPPORTED
    int *mem_model = NULL, flag = 0;
    MPI_Win_get_attr(window, MPI_WIN_MODEL, &mem_model, &flag);
    if (*mem_model != MPI_WIN_UNIFIED)
    {
        fprintf(stderr, "MPI supports accessing shared memory windows only in "
                        "UNIFIED memory model.\n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1); /* abort if memory model is unsupported */
    }

    // Getting relative pointer to shared memory in window for each process
    MPI_Aint win_sz;
    int disp_unit;
    int *grid_ptr;
    MPI_Win_shared_query(window, 0, &win_sz, &disp_unit, &grid_ptr);

    // Dividing particles for each process
    int my_num_particles = num_particles / process_count;
    if (my_rank == 0)
        my_num_particles += num_particles % process_count;
    particles_t my_particles[my_num_particles];

    // Seed the random number generator for each process
    srand(time(NULL) ^ my_rank);

    // Giving each particles a random position on the grid
    for (int i = 0; i < my_num_particles; i++)
    {
        my_particles[i].x = rand() % width;
        my_particles[i].y = rand() % height;
    }

    particles_t particles_to_crystalize[width];
    int n_to_crystalize = 0;

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
            my_particles[p].x = 0 > my_particles[p].x ? 0 : my_particles[p].x;
            my_particles[p].x = width - 1 < my_particles[p].x ? width - 1 : my_particles[p].x;
            my_particles[p].y = 0 > my_particles[p].y ? 0 : my_particles[p].y;
            my_particles[p].y = height - 1 < my_particles[p].y ? height - 1 : my_particles[p].y;

            // Check if the particles has to be crystallized
            for (int y = -1; y <= 1; y++)
            {
                int checkY = my_particles[p].y + y;
                // Check if checkY is a valid y
                if (checkY < 0 || checkY >= height)
                    continue;

                for (int x = -1; x <= 1; x++)
                {
                    int checkX = my_particles[p].x + x;

                    // Check if surrounding is within buondaries and then check if it's a crystal
                    if (checkX >= 0 && checkX < width && grid_ptr[checkY * width + checkX] == CRYSTAL)
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
            }
        }
        // MPI_Barrier(MPI_COMM_WORLD);
        // Crystallize particles
        if (n_to_crystalize > 0)
        {
            for (int i = 0; i < n_to_crystalize; i++)
            {
                grid_ptr[width * particles_to_crystalize[i].y + particles_to_crystalize[i].x] = CRYSTAL;
            }
            n_to_crystalize = 0;
        }
        // MPI_Barrier(MPI_COMM_WORLD);
    }

    // Sync updates to shared memory
    MPI_Win_sync(window);
    MPI_Win_unlock(0, window);

    // ------------------- End point of measurement
    my_finish = MPI_Wtime();
    my_elapsed = my_finish - my_start;
    MPI_Reduce(&my_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD);

    // Save image and print execution time
    if (my_rank == 0)
    {
        // Create image from grid
        array_to_ppm(width, height, grid, "dla_mpi.ppm");
        printf("Execution time = %d us\n", (int)(my_elapsed * 1000000));
    }
    // Free allocated memory
    MPI_Win_free(&window);
    MPI_Comm_free(&shcomm);

    MPI_Finalize();

    return 0;
}