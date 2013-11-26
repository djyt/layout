#ifndef CONTROLPOINT_HPP
#define CONTROLPOINT_HPP

struct ControlPoint
{
    // Position of Change (Index into road)
    int pos;

    // Width / Height
    int type;

    // Used to store width or height index
    // Sprites: Number Of Sprites In Segment [byte]
    int value1;

    // Width Change Speed
    // Sprites: Sprite Data Entry Number From Lookup Table [byte]
    int value2;
};

#endif // CONTROLPOINT_HPP
