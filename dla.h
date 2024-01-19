#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>

struct particles_t
{
    int x;
    int y;
};

enum grid_values
{
    EMPTY = 255,
    CRYSTAL = 1
};

void grid_to_img(int width, int height, int **griglia)
{
    FILE *ppm_file = fopen("dla_serial.ppm", "w");
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
