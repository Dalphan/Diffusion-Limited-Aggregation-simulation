#include "dla.h"

int main(int argc, char **argv)
{
    // Struct to calculate program execution time
    struct timeval start, stop;

    // Getting variables from line command
    if (argc < 4)
    {
        printf("Not enough parameters\nUsage: ./serial width height iterations particles [initial_x] [initial_y]\n");
        return 1;
    }
    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    int iterations = atoi(argv[3]);
    int num_particles = atoi(argv[4]);

    // If not given this is the default starting point
    int initial_x, initial_y;
    if (argc >= 6)
    {
        initial_x = atoi(argv[5]);
        initial_y = atoi(argv[6]);

        // Check if the given values are over the size of the grid
        if (initial_x >= width)
            initial_x = width / 2;
        if (initial_y >= height)
            initial_y = height / 2;
    }
    else
    {
        initial_x = width / 2;
        initial_y = height / 2;
    }

    // DEBUG: Checking parameters
    printf("Parameters: %d %d %d %d %d %d\n", width, height, iterations, num_particles, initial_x, initial_y);

    // ------------------- Starting point of measurement
    gettimeofday(&start, NULL);

    // Inizialize the grid dynamically, in order to avoid segmentation fault with sizes too large
    // 0 = empty cell
    // 1 = crystal
    int **grid = (int **)malloc(height * sizeof(int *));
    for (int i = 0; i < height; i++)
    {
        grid[i] = (int *)malloc(width * sizeof(int));
    }
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            grid[i][j] = 0;
        }
    }

    // Starting crystal
    grid[initial_y][initial_x] = 1;

    // Seed the random number generator with the current time
    srand(time(NULL));

    // Giving each particles a random position on the grid
    struct particles_t particles[num_particles];
    for (int i = 0; i < num_particles; i++)
    {
        particles[i].x = rand() % width;
        particles[i].y = rand() % height;
        // printf("Particella %d posizione x: %d y: %d\n", i, particles[i].x, particles[i].y);
    }

    // Starting simulation
    for (int i = 0; i < iterations; i++)
    {
        // For each particle simulate Brownian motion, then examine the surroundings for crystallized particles.
        for (int p = 0; p < num_particles; p++)
        {
            // Generate random number among -1, 0 and 1
            int randomStepX = rand() % 3 - 1;
            int randomStepY = rand() % 3 - 1;

            // Update particle position
            particles[i].x += randomStepX;
            particles[i].y += randomStepY;

            // Ensure particles stay within the grid size
            particles[i].x = fmin(width, fmax(0, particles[i].x));
            particles[i].y = fmin(height, fmax(0, particles[i].y));
        }
        printf("Iterazione %d finita\n", i);
    }

    // ------------------- End point of measurement
    gettimeofday(&stop, NULL);

    // Free the allocated memory
    for (int i = 0; i < height; i++)
    {
        free(grid[i]);
    }
    free(grid);

    printf("execution time:  %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    return 0;
}