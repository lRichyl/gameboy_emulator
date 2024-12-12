#pragma once
#include "memory.h"
#include "array.h"
#include "SDL3/SDL.h"

enum PPUMode{
    MODE_OAM_SCAN,
    MODE_DRAW,
    MODE_HBLANK,
    MODE_VBLANK
};

enum TileFetchState{
    TILE_FETCH_TILE_INDEX,
    TILE_FETCH_TILE_LOW,
    TILE_FETCH_TILE_HIGH,
    TILE_FETCH_PUSH_FIFO,
};

enum SpriteFetchState{
    SPRITE_FETCH_INDEX,
    SPRITE_FETCH_LOW,
    SPRITE_FETCH_HIGH,
    SPRITE_FETCH_PUSH_FIFO,
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

enum LCDSTATBits{
    LCDSTAT_PPU_MODE = 0x03,
    LCDSTAT_LYC_LY   = 0x04,
    LCDSTAT_MODE_0   = 0x08,
    LCDSTAT_MODE_1   = 0x10,
    LCDSTAT_MODE_2   = 0x20,
    LCDSTAT_LYC_INT  = 0x40,
};

struct Sprite{
    u8 y_position;
    u8 x_position;
    u8 tile_index;
    u8 attributes;
};

struct Pixel{
    u8 color;
    u8 palette;
    u8 bg_priority;
};

struct Color{
    u8 r;
    u8 g;
    u8 b;
};

struct PPU{
    SDL_Texture *framebuffer;
    SDL_Renderer *renderer;
    u32 current_pos;

    PPUMode mode;
    TileFetchState tile_fetch_state;
    SpriteFetchState sprite_fetch_state;
    Memory *memory;
    Array<Sprite> sprites;
    u32 cycles;

    Color colors[4];

    u8 oam_offset;
    u16 oam_initial_address;
    u16 current_oam_address;

    u8 tile_low;
    u8 tile_high;
    u8 tile_index;
    u8 tile_x;
    bool do_dummy_fetch;

    Array<Pixel> bg_fifo;
    Array<Pixel> sprite_fifo;
};

void init_ppu(PPU *ppu, Memory *memory, SDL_Renderer *renderer);
void ppu_tick(PPU *ppu);
void ppu_render(PPU *ppu);