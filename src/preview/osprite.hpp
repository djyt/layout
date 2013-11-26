/***************************************************************************
    Hardware Sprites.
    
    This class stores sprites in the converted format expected by the
    OutRun graphics hardware.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include "../stdint.hpp"

class osprite
{
public:
    uint16_t data[0x10];
	uint32_t scratch;
    bool selected;

    // The width is only calculated at render-time!
    int width;

	osprite(void);
	~osprite(void);
    void init();

    int32_t get_screen_x1();
    int32_t get_screen_x2();
    int32_t get_screen_y1();
    int32_t get_screen_y2();
    int32_t get_screen_width();
    int32_t get_screen_height();
    int32_t get_x_delta();
    int32_t get_y_delta();
    bool is_hidden();
	void set_x(uint16_t);
	void inc_x(uint16_t);
	void set_y(uint16_t);
	void set_pitch(uint8_t);
	void set_vzoom(uint16_t);
	void set_hzoom(uint16_t);
	void set_priority(uint8_t);
	void set_offset(uint16_t o);
	void inc_offset(uint16_t o);
	void set_render(uint8_t b);
	void set_pal(uint8_t);
	void set_height(uint8_t);
	void sub_height(uint8_t);
	void set_bank(uint8_t);
	void hide();

private:

};
