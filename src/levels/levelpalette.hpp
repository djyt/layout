/***************************************************************************
    Shared Level Palettes Structure

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef ROADPALETTE_HPP
#define ROADPALETTE_HPP

#include "../stdint.hpp"

struct LevelPalette
{
    static const int SKY_PALS    = 15;
    static const int SKY_LENGTH  = 32;
    static const int GND_PALS    = 16;
    static const int GND_LENGTH  = 8;
    static const int ROAD_PALS   = 18; // OutRun has three additional bonus palettes
    static const int ROAD_LENGTH = 8;

    static const int ROAD1   = 0;
    static const int SIDE1   = 1;
    static const int STRIPE1 = 2;
    static const int CENTRE1 = 3;
    static const int ROAD2   = 4;
    static const int SIDE2   = 5;
    static const int STRIPE2 = 6;
    static const int CENTRE2 = 7;

    uint32_t road[ROAD_PALS][ROAD_LENGTH]; // Road Palettes
    uint32_t sky[SKY_PALS][SKY_LENGTH];    // Sky Palettes
    uint32_t gnd[GND_PALS][GND_LENGTH];    // Ground Palettes
};

#endif // ROADPALETTE_HPP
