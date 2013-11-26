/***************************************************************************
    Height Segment Format

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#ifndef HEIGHTFORMAT_HPP
#define HEIGHTFORMAT_HPP

#include "../stdint.hpp"

struct HeightSegment
{
    QString name;    // Editor Friendly Name Of Section
    int     type;    // Height Segment Type (correspond to heightlabels.hpp)
    int     step;    // Length Of Each Section
    int     value1;
    int     value2;

    // Height Segment Data Points
    QList<int16_t> data;
};

#endif // HEIGHTFORMAT_HPP
