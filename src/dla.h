#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

typedef struct
{
    int x;
    int y;
} particles_t;

enum grid_values
{
    EMPTY = 255,
    CRYSTAL = 1
};

const char *directory = "res/";

void get_input_parameters(int argc, char **argv, int *width, int *height, int *iterations, int *num_particles, int *initial_x, int *initial_y)
{
    // Seed the random number generator with the current time
    srand(time(NULL));

    // Getting variables from line command
    if (argc < 4)
    {
        printf("Not enough parameters\nUsage: ./serial width height iterations particles [initial_x] [initial_y]\n");
        exit(-1);
    }
    *width = atoi(argv[1]);
    *height = atoi(argv[2]);
    *iterations = atoi(argv[3]);
    *num_particles = atoi(argv[4]);

    // If not given or over the boundaries, the default starting point is random
    if (argc >= 6)
    {
        *initial_x = atoi(argv[5]);
        *initial_y = atoi(argv[6]);

        // Check if the given values are over the size of the grid
        if (*initial_x >= *width || *initial_x < 0)
            *initial_x = rand() % *width;
        if (*initial_y >= *height || *initial_y < 0)
            *initial_y = rand() % *height;
    }
    else
    {
        *initial_x = rand() % *width;
        *initial_y = rand() % *height;
    }

    // DEBUG: Checking parameters
    printf("Parameters: %d %d %d %d %d %d\n", *width, *height, *iterations, *num_particles, *initial_x, *initial_y);
}

void grid_to_ppm(int width, int height, int **griglia, char *file_name)
{
    char path[100];
    snprintf(path, sizeof(path), "%s%s", directory, file_name);
    FILE *ppm_file = fopen(path, "w");
    fprintf(ppm_file, "P3\n %d %d 255\n", width, height);

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            fprintf(ppm_file, "%i %i %i   ", griglia[i][j], griglia[i][j], griglia[i][j]);
        }
        fprintf(ppm_file, "\n");
    }
    fclose(ppm_file);
}

void array_to_ppm(int width, int height, int *griglia, char *file_name)
{
    char path[100];
    snprintf(path, sizeof(path), "%s%s", directory, file_name);
    FILE *ppm_file = fopen(path, "w");
    fprintf(ppm_file, "P3\n %d %d 255\n", width, height);

    int position;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            position = i * width + j;
            fprintf(ppm_file, "%i %i %i   ", griglia[position], griglia[position], griglia[position]);
        }
        fprintf(ppm_file, "\n");
    }
    fclose(ppm_file);
}
