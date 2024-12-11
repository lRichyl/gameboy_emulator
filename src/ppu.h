#pragma once
#include "memory.h"
#include "array.h"

enum PPUMode{
    MODE_OAM_SCAN,
    MODE_DRAW,
    MODE_HBLANK,
    MODE_VBLANK
};

struct Sprite{
    u8 y_position;
    u8 x_position;
    u8 tile_index;
    u8 attributes;
};

enum LCDCBits{
    LCDC_BG_WIN_ENABLE   = 0x01,
    LCDC_OBJ_ENABLE      = 0x02,
    LCDC_OBJ_SIZE        = 0x04,
    LCDC_BG_TILEMAP      = 0x08,
    LCDC_BG_WIN_TILEDATA = 0x10,
    LCDC_WINDOW_ENABLE   = 0x20,
    LCDC_WIN_TILEMAP     = 0x40,
    LCDC_LCD_PPU_ENABLE  = 0x80
};

struct PPU{
    PPUMode mode;
    Memory *memory;
    Array<Sprite> sprites;
    u32 LY;
    u32 cycles;

    u8 oam_offset;
    u16 oam_initial_address;
    u16 current_oam_address;
};

void init_ppu(PPU *ppu, Memory *memory);
void ppu_tick(PPU *ppu);