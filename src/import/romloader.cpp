/***************************************************************************
    Binary File Loader. 
    
    Handles loading an individual binary file to memory.
    Supports reading bytes, words and longs from this area of memory.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <iostream>
#include <fstream>
#include <cstddef>       // for std::size_t
#include <QDir>
#include "romloader.hpp"

RomLoader::RomLoader()
{
    rom    = NULL;
    length = 0;
}

RomLoader::~RomLoader()
{
}

void RomLoader::init(const uint32_t length)
{
    unload();

    this->length = length;
    rom = new uint8_t[length];
}

void RomLoader::unload(void)
{
    if (rom)
    {
        delete[] rom;
        rom = NULL;
    }
}

void RomLoader::setRomPath(std::string path)
{
    romPath = path;
}

int RomLoader::load(const char* filename, const int offset, const int length, const int, const uint8_t interleave)
{
    if (romPath.empty())
        return 1;

    std::string path = romPath;
    path += QDir::separator().toLatin1();
    path += std::string(filename);

    // Open rom file
    std::ifstream src(path.c_str(), std::ios::in | std::ios::binary);
    if (!src)
    {
        std::cout << "cannot open rom: " << romPath.c_str() << " : " << filename << std::endl;
        return 1; // fail
    }

    // Read file
    char* buffer = new char[length];
    src.read(buffer, length);

    // Interleave file as necessary
    for (int i = 0; i < length; i++)
    {
        rom[(i * interleave) + offset] = buffer[i];
    }

    // Clean Up
    delete[] buffer;
    src.close();
    return 0; // success
}
