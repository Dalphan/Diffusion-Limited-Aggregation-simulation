#include "CImg.h"
#include <cstdint>
#include <stdio.h>

cimg_library::CImgList<int> imageList;

extern "C"
{
    void saveOneFrame(int **grid, int width, int height)
    {
        using namespace cimg_library;

        CImg<int> image(width, height, 1, 1, 0);

        int color[1];

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                color[0] = grid[y][x];
                image.draw_point(x, y, color, 1.0);
            }
        }
        imageList.push_back(image);
    }

    void saveVideo(int frames)
    {
        if (imageList.size() > 0)
        {
            printf("Saving video\n");
            imageList.save_video("DLA_VIDEO.mp4", frames, "H264");
            printf("Video saved\n");
            imageList.clear();
        }
        else
        {
            printf("Couldn't save video\n");
        }
    }
}