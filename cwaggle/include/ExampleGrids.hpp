#pragma once

// For loading a grid from an image file.
#include <SFML/Graphics/Image.hpp>

#include "ValueGrid.hpp"
#include "Vec2.hpp"

namespace ExampleGrids
{
    ValueGrid GetInverseCenterDistanceGrid(size_t width, size_t height)
    {
        ValueGrid grid(width, height);

        for (size_t x = 0; x < width; x++)
        {
            for (size_t y = 0; y < height; y++)
            {
                grid.set(x, y, Vec2(x, y).dist(Vec2(width / 2.0, height / 2.0)));
            }
        }

        grid.normalize();
        grid.invert();
        return grid;
    }

};