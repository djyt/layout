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

#pragma once

#include "../globals.hpp"
#include "oentry.hpp"

class OSprites;
class ORoad;
class RomLoader;

class OLevelObjs
{
    public:
        OLevelObjs(OSprites* osprites, ORoad* oroad, RomLoader *rom);
        ~OLevelObjs(void);

        void init_entries(uint32_t, const uint8_t start_index, const uint8_t);
        void setup_sprites(uint32_t);
        void do_sprite_routine();
        void hide_sprite(oentry*);

    private:
        OSprites* osprites;
        ORoad* oroad;
        RomLoader* rom;

	    void setup_sprite(oentry*, uint32_t);
	    void setup_sprite_routine(oentry*);		
        void sprite_collision_z1c(oentry*);
        void sprite_lights(oentry*);
        void sprite_lights_countdown(oentry*);
        void sprite_grass(oentry* sprite);
        void sprite_water(oentry* sprite);
        void sprite_rocks(oentry* sprite);
        void sprite_debris(oentry* sprite);
        void sprite_minitree(oentry* sprite);
        void do_thickness_sprite(oentry* sprite, const uint32_t);
        void sprite_clouds(oentry* sprite);
	    void sprite_normal(oentry*, uint8_t);
	    void set_spr_zoom_priority(oentry*, uint8_t);
        void set_spr_zoom_priority2(oentry*, uint8_t);
        void set_spr_zoom_priority_rocks(oentry*, uint8_t);

        // 68000 Assembly Helper Functions
        inline static void sub16(int32_t src, int32_t& dst)
        {
            dst = (dst & 0xFFFF0000) + (((dst & 0xFFFF) - (src & 0xFFFF)) & 0xFFFF);
        }

        inline static void swap32(int32_t& v)
        {
            v = ((v & 0xFFFF0000) >> 16) + ((v & 0xFFFF) << 16);
        }

        inline static void swap32(uint32_t& v)
        {
            v = ((v & 0xFFFF0000) >> 16) + ((v & 0xFFFF) << 16);
        }
};
