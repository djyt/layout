#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include "stdint.hpp"

// Internal Sega OutRun Screen Properties
const uint16_t S16_WIDTH      = 600;
const uint16_t S16_HEIGHT     = 224;
const uint16_t S16_X_OFF     = (S16_WIDTH - 320) / 2;

const uint16_t S16_PALETTE_ENTRIES = 0x1000;

// Number of levels
const static uint8_t LEVELS = 15;

// TODO: Move the below back inside importOutRun once factored out!!

// Sprite List Address
const static uint32_t SPRITELIST_ADR = 0x11ED2;

// Width / Height Table
const static uint32_t WH_TABLE = 0x20000;

// Height Map Details
const static int HEIGHT_LENGTH = 255;

// OutRun hard-codes the start width of the first track
const static uint16_t START_WIDTH_L1 = 0x1C2;
const static uint16_t START_WIDTH    = 0xD4;

enum
{
    BIT_0 = 0x01,
    BIT_1 = 0x02,
    BIT_2 = 0x04,
    BIT_3 = 0x08,
    BIT_4 = 0x10,
    BIT_5 = 0x20,
    BIT_6 = 0x40,
    BIT_7 = 0x80,
    BIT_8 = 0x100,
    BIT_9 = 0x200,
    BIT_A = 0x400
};

#endif // GLOBALS_HPP
