/***************************************************************************
    Level Object Logic
    
    This class handles rendering most of the objects that comprise a typical
    level. 
    
    - Configures rendering properties (co-ordinates, zoom etc.) 
    - Object specific logic, including collision checks & start lights etc.

    The original codebase contains a large amount of code duplication,
    much of which is duplicated here.
    
    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "osprites.hpp"
#include "oroad.hpp"
#include "../sprites/spriteformat.hpp"
#include "../import/romloader.hpp"
#include "olevelobjs.hpp"

// ------------------------------------------------------------------------------------------------
// OutRun ROM Addresses
// ------------------------------------------------------------------------------------------------

const uint32_t MOVEMENT_LOOKUP_Z = 0x30900;

// Table to alter sprite based on its y position. Input = Y Position.
//
// Format:
// +0: Frame Number To Use
// +2: Entry In Zoom Lookup Table
const uint32_t MAP_Y_TO_FRAME = 0x30A00;

// Sprites: Hard Coded Addresses.
const uint32_t SPRITE_CLOUD_FRAMES    = 0x4246;
const uint32_t SPRITE_MINITREE_FRAMES = 0x435C;
const uint32_t SPRITE_GRASS_FRAMES    = 0x4548;
const uint32_t SPRITE_SAND_FRAMES     = 0x4588;
const uint32_t SPRITE_STONE_FRAMES    = 0x45C8;
const uint32_t SPRITE_WATER_FRAMES    = 0x4608;

// ------------------------------------------------------------------------------------------------

OLevelObjs::OLevelObjs(OSprites* osprites, ORoad *oroad, RomLoader* rom)
{
    this->osprites = osprites;
    this->oroad    = oroad;
    this->rom      = rom;
}

OLevelObjs::~OLevelObjs(void)
{
}


void OLevelObjs::init_entries(uint32_t a4, const uint8_t start_index, const uint8_t no_entries)
{
    // next_sprite:
    for (uint8_t i = start_index; i <= no_entries; i++)
    {
        oentry *sprite = &osprites->jump_table[i];

        sprite->control    = rom->read8(&a4);
        sprite->draw_props = rom->read8(&a4);
        sprite->shadow     = rom->read8(&a4);
        sprite->pal_src    = rom->read8(&a4);
        sprite->type       = rom->read16(&a4);
        sprite->addr       = rom->read32(SPRITELIST_ADR + sprite->type);
        sprite->xw1        = 
        sprite->xw2        = rom->read16(&a4);
        sprite->yw         = rom->read16(&a4);

        uint16_t z_orig    = rom->read16(&a4);

        uint32_t z = z_orig;
        swap32(z);
        sprite->z = z;

        int16_t road_x = oroad->road0_h[z_orig];
        int16_t xw1 = sprite->xw1;

        if (xw1 >= 0 && (sprite->control & OSprites::WIDE_ROAD) == 0)
        {
            xw1 += (oroad->road_width << 1) << 16;
        }

        // on_lhs
        int16_t multiply = (xw1 * z_orig) >> 9; // only used as a 16 bit value so truncate here
        sprite->x = road_x + multiply;

        a4 += 4; // Throw away

        // Hack to choose correct routine, and not use lookup table from ROM
        if (i >= 0 && i <= 27)
            sprite->function_holder = 0; // SpriteCollisionZ1
        else if (i >= 28 && i <= 43)
            sprite->function_holder = 7; // SpriteCollisionZ1C
        //else if (no_entries == HISCORE_SPRITE_ENTRIES)
        //    sprite->function_holder = 8; // SpriteNoCollisionZ2;
        // Vertical Sign on LHS with Lights
        else if (i == 44)
            sprite->function_holder = 4; // Lights
        // Vertical Sign on RHS
        else if (i == 45)
            sprite->function_holder = 0; // SpriteCollisionZ1
        // Two halves of start sign
        else if (i == 46 || i == 47)
            sprite->function_holder = 0; // SpriteNoCollisionZ1
        // Crowd of people
        else if (i >= 48 && i <= 67)
            sprite->function_holder = 8; // SpriteNoCollisionZ2;

        osprites->map_palette(sprite);
    }
}

// Setup Sprites
//
// Source: 3CB2
//
// Input: Default Zoom Value
void OLevelObjs::setup_sprites(uint32_t z)
{
    // Setup entries that have not yet been enabled
    for (uint8_t i = 0; i < osprites->no_sprites; i++)
    {
        if ((osprites->jump_table[i].control & OSprites::ENABLE) == 0)
        {
            setup_sprite(&osprites->jump_table[i], z);
            return;
        }
    }
}

// Setup Sprite from ROM format for use in game
//
// Source: 3CDE
//
// ROM Setup:
//
// +0: [Byte] Bit 0 = H-Flip Sprite
//            Bit 1 = Enable Shadows
//            
//            Bits 4-7 = Routine Draw Number
// +1: [Byte] Sprite X World Position
// +2: [Word] Sprite Y World Position
// +5: [Byte] Sprite Type
// +7: [Byte] Sprite Palette
void OLevelObjs::setup_sprite(oentry* sprite, uint32_t z)
{
    SpriteEntry entry = osprites->currentSection.sprites.at(osprites->seg_spr_offset1);

    sprite->control |= OSprites::ENABLE; // Turn sprite on
    // Set sprite x,y (world coordinates)
    sprite->xw1     = 
    sprite->xw2     = ((int8_t) entry.x << 4);
    sprite->yw      = entry.y << 7;
    sprite->type    = entry.type << 2;
    sprite->addr    = rom->read32(SPRITELIST_ADR + sprite->type);
    sprite->pal_src = entry.pal;
    osprites->map_palette(sprite);

    sprite->width = 0;
    sprite->reload = 0;
    sprite->z = z; // Set default zoom

    // Is Sprite Selected In Editor?
    sprite->selected = entry.selected;

    if (entry.props & 1)
        sprite->control |= OSprites::HFLIP;
    else
        sprite->control &=~ OSprites::HFLIP;

    if (entry.props & 2)
        sprite->control |= OSprites::SHADOW;
    else
        sprite->control &=~ OSprites::SHADOW;

    if ((int16_t) (oroad->road_width >> 16) > 0x118)
        sprite->control |= OSprites::WIDE_ROAD;
    else
        sprite->control &=~ OSprites::WIDE_ROAD;

    sprite->draw_props = entry.props & 0xF0;
    sprite->function_holder = sprite->draw_props >> 4; // set sprite type

    setup_sprite_routine(sprite);
}

// Index | Description         | Zoom     | Collision | X Anchor | Y Anchor | Priority | Notes                      |
// ------------------------------------------------------------------------------------------------------------------
// 0       Normal Collision    |  1.0     |    Yes    | Centre   | Bottom   | Low      | Collision Depends on yw
// 1       Stray Strip: Low Pr |  Custom  |   Spray   | Left     | Centre   | Low      |
// 2       Clouds              |  Custom  |    No     | Left     | Bottom   | High     |
// 3       Spray Strip: High Pr|  Custom  |   Spray   | Left     | Centre   | High     |
// 4       Start Lights        |  1.0     |    Yes    | Centre   | Bottom   | Low      | Custom Palette Changing Code
// 5       Normal No Collision |  1.0     |    No     | Centre   | Bottom   | Low      |
// 6       Checkpoint Marker   |  1.0     |    No     | Centre   | Bottom   | Low      | Custom Code To Denote Passing Marker
// 7       Centre Collision    |  1.0     |    Yes    | Left     | Bottom   | Low      | Detects Collision From Centre
// 8       Half-Size Sprite    |  0.5     |    Yes    | Centre   | Bottom   | Low      | Shrinks Sprite By Half
// 9                           |  1.0     |    Yes    | Centre   | Bottom   | Low      |
// 10      Sand Strips         |  Custom  |    No     | Left     | Centre   | High     |
// 11      Stone Strips        |  Custom  |    No     | Centre   | Centre   | Low      |
// 12                          |  1.0     |    Yes    | Right    | Bottom   | Low      |
// 13      Track Debris        |  0.5     |   Spray   | Centre   | Centre   | High     |
// 14      Sand Strips         |  Custom  |    No     | Left     | Centre   | High     | Exactly the same as Entry 10.

// The X Anchor is usually inverted if H-Flip is applied to the sprite.

void OLevelObjs::setup_sprite_routine(oentry* sprite)
{
    switch (sprite->function_holder)
    {
        // Normal Sprite: (Possible With/Without Collision)
        case 0:
            sprite->shadow = 7;
            sprite->draw_props |= 8; // x = anchor centre, y = anchor bottom
            break;
        
        case 1: // Grass Sprite
        case 11:  // Stone Strips
            sprite->shadow = 7;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 2;
            else
                sprite->draw_props |= 1;
            break;

        // Overhead Clouds
        case 2: 
            sprite->shadow = 3;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 0xA;
            else
                sprite->draw_props |= 9;

            if (sprite->draw_props & BIT_0)
                sprite->reload = -0x20;
            else
                sprite->reload = 0x20;

            sprite->xw2 = 0;
            break;

        // Water Sprite
        case 3:
            sprite->shadow = 3;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 2; // anchor x right
            else
                sprite->draw_props |= 1; // anchor x left
            break;
        
        case 4: // Sprite Lights
        case 5: // Checkpoint Bottom
        case 6: // Checkpoint Top
        case 8: // No Collision Check
        case 9: // Wide Rocks (Stage 2)
            sprite->shadow = 7;
            sprite->draw_props |= 8; // x = anchor centre, y = anchor bottom
            break;
        
        // Draw From Top Left Collision Check
        case 7:
            sprite->shadow = 7;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 9;
            else
                sprite->draw_props |= 0xA;
            break;

        // Sand Strips
        case 10:
        case 14: // version for wider road widths
            sprite->shadow = 3;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 2;
            else
                sprite->draw_props |= 1;
            break;
    
        // Mini Tree
        case 12:
            sprite->shadow = 7;
            if (sprite->control & OSprites::HFLIP)
                sprite->draw_props |= 0xA;
            else
                sprite->draw_props |= 9;
            break;

        // Spray
        case 13:
            sprite->shadow = 3;
            sprite->draw_props |= 0;
            break;
    }
}

void OLevelObjs::do_sprite_routine()
{
    for (uint8_t i = 0; i < osprites->no_sprites; i++)
    {
        oentry* sprite = &osprites->jump_table[i];

        if (sprite->control & OSprites::ENABLE)
        {
            switch (sprite->function_holder)
            {
                // Normal Sprite: (Possible With/Without Collision, Zoom 1)
                case 0:
                    //if (sprite->yw == 0)
                      // sprite_normal(sprite, 1);
                    //else
                       set_spr_zoom_priority(sprite, 1);
                    break;

                // Grass Sprite
                case 1:
                    //sprite_grass(sprite);
                    do_thickness_sprite(sprite, SPRITE_GRASS_FRAMES);
                    break;

                // Sprite based clouds that span entire sky
                case 2:
                    sprite_clouds(sprite);
                    break;

                // Water on LHS of Stage 1
                case 3:
                    //sprite_water(sprite);
                    do_thickness_sprite(sprite, SPRITE_WATER_FRAMES);
                    break;

                // Start Lights & Base Pillar of Checkpoint Sign
                case 4:
                    //sprite_lights(sprite);
                    set_spr_zoom_priority(sprite, 1);
                    break;
                
                // 5 - Checkpoint (Bottom Of Sign)
                case 5:
                    set_spr_zoom_priority(sprite, 1);
                    break;

                // 6 - Checkpoint (Top Of Sign)
                case 6:
                    set_spr_zoom_priority(sprite, 1);
                    // Have we passed the checkpoint?
                    //if (!(sprite->control & OSprites::ENABLE))
                    //    oinitengine.checkpoint_marker = -1;
                    break;

                // Draw From Centre Collision Check
                case 7:
                    //sprite_collision_z1c(sprite);
                    set_spr_zoom_priority2(sprite, 1);
                    break;

                // Normal Sprite: (Collision, Zoom 2)
                case 8:
                    //sprite_normal(sprite, 2);
                    set_spr_zoom_priority(sprite, 2);
                    break;

                // Wide Rocks on Stage 2
                case 9:
                    //sprite_rocks(sprite);
                    set_spr_zoom_priority(sprite, 1);
                    break;

                // Sand Strips
                case 10:
                    do_thickness_sprite(sprite, SPRITE_SAND_FRAMES);
                    break;

                // Stone Strips
                case 11:
                    do_thickness_sprite(sprite, SPRITE_STONE_FRAMES);
                    break;

                // Mini-Tree (Stage 5, Level ID: 0x24)
                case 12:
                    sprite_minitree(sprite);
                    break;
                
                // Track Debris on Stage 3a
                case 13:
                    //sprite_debris(sprite);
                    set_spr_zoom_priority(sprite, 2);
                    break;

                // Sand (Again) - Used in end sequence #2
                case 14:
                    do_thickness_sprite(sprite, SPRITE_SAND_FRAMES);
                    break;
            }
        }
    }
}

// Set Sprite Priority To Other Sprites
// Set Sprite Priority To Road
// Set Index To Lookup Sprite Settings (Width/Height) From Zoom Value
//
// Source Address: 0x404A
void OLevelObjs::set_spr_zoom_priority(oentry *sprite, uint8_t zoom)
{
    osprites->move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
    sprite->road_priority = z16;
    sprite->priority = z16;        
    sprite->zoom = z16 >> zoom;

    // Set Sprite Y Position [SCREEN]
    // 1/ Use Y Offset From Road Position [Screen]
    // 2/ Use Sprite Y World Data if not 0, converted to a screen position [World]

    int32_t road_y = -(oroad->road_y[oroad->road_p0 + z16] >> 4) + 223;

    if (sprite->yw != 0)
    {
        uint32_t yw = sprite->yw * z16; // Note the final product is a LONG, not a word here
        swap32(yw);
        sub16(yw, road_y);
    }
    sprite->y = road_y;

    //    Set Sprite X Position [SCREEN]
    //    1/ Use X Offset From Road Position [Screen]
    //    2/ Use Sprite X World Data
    
    int16_t road_x = oroad->road0_h[z16];
    int16_t xw1 = sprite->xw1;
   
    if (xw1 >= 0)
    {
        // Bit of a hack here to avoid code duplication
        if (sprite->function_holder >= 4 && sprite->function_holder <= 6) 
            xw1 += (oroad->road_width >> 16) << 1;
        else
        {
            if ((sprite->control & OSprites::WIDE_ROAD) == 0)
                xw1 += (oroad->road_width >> 16) << 1;
        }
    }

    int32_t multiply = (xw1 * z16) >> 9; // only used as a 16 bit value so truncate here
    sprite->x = road_x + multiply;

    osprites->do_spr_order_shadows(sprite);
}

// Almost identical to set_spr_zoom_priority
// 
// Differences highlighted below. 
// 
// Set Sprite Priority To Other Sprites
// Set Sprite Priority To Road
// Set Index To Lookup Sprite Settings (Width/Height) From Zoom Value
//
// Source Address: 0x404A
void OLevelObjs::set_spr_zoom_priority2(oentry *sprite, uint8_t zoom)
{
    osprites->move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
    sprite->road_priority = z16;
    sprite->priority = z16;    
    sprite->zoom = z16 >> zoom;

    // Code differs from below

    // Set Sprite X Position [SCREEN]
    // 1/ Use X Offset From Road Position [Screen]
    // 2/ Use Sprite X World Data

    int16_t road_x = oroad->road0_h[z16];
    int16_t xw1 = sprite->xw1;

    if (xw1 >= 0 && (sprite->control & OSprites::WIDE_ROAD) == 0)
    {
        xw1 +=  ((int16_t) (oroad->road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9; // only used as a 16 bit value so truncate here
    road_x += multiply;

    // Nuked for purposes of level editor as was spoiling widescreen mode
    //if (road_x > 160 || road_x < -160) return; // NEW LINE compared with original routine (added for ROM REV. A)
    sprite->x = road_x;

    // Set Sprite Y Position [SCREEN]
    // 1/ Use Y Offset From Road Position [Screen]
    // 2/ Use Sprite Y World Data if not 0, converted to a screen position [World]
    sprite->y = -(oroad->road_y[oroad->road_p0 + z16] >> 4) + 223;

    osprites->do_spr_order_shadows(sprite);
}

// Sprite: MiniTrees 
//
// - Rows of short tree/shrubs found on Stage 5
// - Frame Changes Based On Current Y Position Of Sprite
// 
// Note: This routine is very similar to do_thickness_sprites and can probably be refactored
//
// Source: 0x428A

void OLevelObjs::sprite_minitree(oentry* sprite)
{
    osprites->move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
     
    sprite->road_priority = z16;
    sprite->priority = z16;    

    // 44c4
    int16_t road_x = oroad->road0_h[z16];
    int16_t xw1 = sprite->xw1;

    if (xw1 >= 0)
    {
        xw1 += ((int16_t) (oroad->road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9;
    road_x += multiply;

    //if (road_x >= 160 || road_x < -160) return;
    sprite->x = road_x;

    // 44fc
    int32_t road_y = -(oroad->road_y[oroad->road_p0 + z16] >> 4) + 223;
    sprite->y = road_y; // Set Sprite Y (Screen/Camera)

    uint16_t z = z16 >> 1;

    if (z >= 0x80) 
    {
        // don't choose a custom frame
        sprite->zoom = (uint8_t) z; // Set Entry Number For Zoom Lookup Table
        sprite->addr = rom->read32(SPRITE_MINITREE_FRAMES); // Set to first frame in table
    }
    // Use Table to alter sprite based on its y position.
    //
    // Input = Y Position
    //
    // Format:
    // +0: Frame Number To Use
    // +2: Entry In Zoom Lookup Table
    else
    {
        z <<= 1; // Note we can't use original z16, so don't try to optimize this
        uint8_t offset = rom->read8(MAP_Y_TO_FRAME + z);
        sprite->zoom = rom->read8(MAP_Y_TO_FRAME + z + 1);
        sprite->addr = rom->read32(SPRITE_MINITREE_FRAMES + offset);
    }
    // order_sprites
    osprites->do_spr_order_shadows(sprite);
}

// Sprite based clouds that span entire sky.
//
// Appear on Stage 3 - Rightmost Route.
//
// Source: 0x4144
void OLevelObjs::sprite_clouds(oentry* sprite)
{
    osprites->move_sprite(sprite, 1);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4)
    {
        int32_t road_x = oroad->road0_h[z16];
        sprite->type = road_x;
        return;
    }
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
     
    sprite->road_priority = z16;
    sprite->priority      = z16;
    sprite->y = -((z16 * oroad->horizon_y2) >> 9) + oroad->horizon_y2; // muls (two 16 bit values, 32 bit result)

    uint16_t type = sprite->type; // d1
    int16_t road_x = oroad->road0_h[z16]; // d0
    sprite->type = road_x;
    road_x -= type;
    type = (z16 >> 2);
    
    if (type != 0)
    {
        //41b6
        road_x += sprite->xw2;
        if (road_x >= 0)
        {
            // 41be
            do
            {
                road_x -= type;
            }
            while (road_x >= 0);
            road_x += type;
        }
        else
        {
            // 41c8
            do
            {
                road_x += type;
            }
            while (road_x < 0);
            road_x -= type;
        }
        // 41ce set_sprite_x_world
        sprite->xw2 = road_x;
    }

    // 41d2 - reload is used as an x-offset here, not as a reload value
    sprite->x = sprite->xw2 + sprite->reload;
    sprite->pal_src = 205;

    uint16_t z = z16 >> 1;

    if (z >= 0x80)
    {
        // 421c
        sprite->zoom = (uint8_t) z; // Set Entry Number For Zoom Lookup Table
        sprite->addr = rom->read32(SPRITE_CLOUD_FRAMES);
    }
    else
    {
        // 41f8
        z <<= 1;
        uint8_t lookup_z = rom->read8(MOVEMENT_LOOKUP_Z + z);
        sprite->addr = rom->read32(SPRITE_CLOUD_FRAMES + lookup_z);
        sprite->zoom = rom->read8(MOVEMENT_LOOKUP_Z + z + 1);
    }
    // end
    osprites->map_palette(sprite);
    osprites->do_spr_order_shadows(sprite);
}

void OLevelObjs::do_thickness_sprite(oentry* sprite, const uint32_t sprite_table_address)
{
    osprites->move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
     
    sprite->road_priority = z16;
    sprite->priority = z16;    

    // 44c4
    int16_t road_x = oroad->road0_h[z16];
    int16_t xw1 = sprite->xw1;

    if (xw1 >= 0)
    {
        if (sprite->id != 14 || (sprite->control & OSprites::WIDE_ROAD) == 0) // Rolled in separate routine with this one line. (Sand 2 used in bonus sequence)
            xw1 += ((int16_t) (oroad->road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9;
    road_x += multiply;

    //if (road_x >= 160 || road_x < -160) return;
    sprite->x = road_x;

    // 44fc
    int32_t road_y = -(oroad->road_y[oroad->road_p0 + z16] >> 4) + 223;
    sprite->y = road_y; // Set Sprite Y (Screen/Camera)
    // 4518 - Sprite thickness code

    uint16_t z = z16 >> 1;

    if (z >= 0x80) 
    {
        //use_large_frame (don't choose a custom frame)
        sprite->zoom = (uint8_t) z; // Set Entry Number For Zoom Lookup Table
        sprite->addr = rom->read32(0x3C + sprite_table_address); // Set default frame for larger sprite
    }
    else
    {
        // use custom frame for sprite
        sprite->zoom = 0x80; // cap sprite_z minimum to 0x80
        z = (z >> 1) & 0x3C; // Mask over lower 2 bits, so the frame aligns to a word
        sprite->addr = rom->read32(z + sprite_table_address); // Set Frame Data Based On Zoom Value
    }
    // order_sprites
    osprites->do_spr_order_shadows(sprite);
}

// Set Sprite Priority To Other Sprites
// Set Sprite Priority To Road
// Set Index To Lookup Sprite Settings (Width/Height) From Zoom Value
//
// Source Address: 0x404A
void OLevelObjs::set_spr_zoom_priority_rocks(oentry *sprite, uint8_t zoom)
{
    osprites->move_sprite(sprite, 0);
    uint16_t z16 = sprite->z >> 16;

    if (z16 < 4) return;
    if (z16 >= 0x200)
    {
        hide_sprite(sprite);
        return;
    }
    sprite->road_priority = z16;
    sprite->priority = z16;        
    sprite->zoom = z16 >> zoom;

    // Set Sprite Y Position [SCREEN]
    // 1/ Use Y Offset From Road Position [Screen]
    // 2/ Use Sprite Y World Data if not 0, converted to a screen position [World]

    int32_t road_y = -(oroad->road_y[oroad->road_p0 + z16] >> 4) + 223;
    sprite->y = road_y;

    //    Set Sprite X Position [SCREEN]
    //    1/ Use X Offset From Road Position [Screen]
    //    2/ Use Sprite X World Data
    
    int16_t road_x = oroad->road0_h[z16];
    int16_t xw1 = sprite->xw1;
   
    if (xw1 >= 0 && (sprite->control & OSprites::WIDE_ROAD) == 0)
    {
        xw1 +=  ((int16_t) (oroad->road_width >> 16)) << 1;
    }

    int32_t multiply = (xw1 * z16) >> 9; // only used as a 16 bit value so truncate here
    sprite->x = road_x + multiply;

    // Additional Draw Check Here, to prevent sprite from wrapping around screen and redrawing on left
    // Remember this routine is setup to draw from centre x

    //uint16_t width = (sprite->width >> 1) + 160;
    //if (sprite->x >= width || sprite->x + width < 0) return;

    osprites->do_spr_order_shadows(sprite);
}

// Source Address: 0x4648
void OLevelObjs::hide_sprite(oentry *sprite)
{
    sprite->z = 0;
    sprite->zoom = 0; // Hide the sprite
    sprite->control &= ~OSprites::ENABLE; // Disable entry in jump table
}
