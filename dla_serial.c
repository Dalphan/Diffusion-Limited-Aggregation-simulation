#include "dla.h"

extern void saveOneFrame(int **grid, int width, int height);
extern void saveVideo(int frames);

int main(int argc, char **argv)
{
    // Struct to calculate program execution time
    struct timeval start, stop;

    // VIDEO
    // int skip_frames = 20;

    int width, height, iterations, num_particles, initial_x, initial_y;
    get_input_parameters(argc, argv, &width, &height, &iterations, &num_particles, &initial_x, &initial_y);

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

    // Giving each particles a random position on the grid
    particles_t particles[num_particles];
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
            particles[p].x += randomStepX;
            particles[p].y += randomStepY;

            // Ensure particles stay within the grid size
            // particles[p].x = fmin(width - 1, fmax(0, particles[p].x));
            particles[p].x = 0 > particles[p].x ? 0 : particles[p].x;
            particles[p].x = width - 1 < particles[p].x ? width - 1 : particles[p].x;
            // particles[p].y = fmin(height - 1, fmax(0, particles[p].y));
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
                        particles[p] = particles[num_particles - 1];
                        num_particles--;
                    }
                }
            }
        }
        // DEBUG
        // printf("Iterazione %d finita\n", i);

        // VIDEO
        // if (i % skip_frames == 0)
        //     saveOneFrame(grid, width, height);
    }
    // ------------------- End point of measurement
    gettimeofday(&stop, NULL);

    // Create image from grid
    grid_to_ppm(width, height, grid, "dla_serial.ppm");

    // VIDEO
    // saveVideo(iterations / skip_frames / 60);

    // Free the allocated memory
    for (int i = 0; i < height; i++)
        free(grid[i]);
    free(grid);

    printf("Execution time:  %lu us\n", (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec);
    return 0;
}