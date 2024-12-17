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
    TILE_FETCH_DEFAULT,
    TILE_FETCH_SPRITE_INDEX,
    TILE_FETCH_SPRITE_LOW,
    TILE_FETCH_SPRITE_HIGH,
    TILE_FETCH_SPRITE_PUSH
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

enum SpriteAttributes{
    ATTRIBUTE_PALETTE  = 0x10,
    ATTRIBUTE_X_FLIP   = 0x20,
    ATTRIBUTE_Y_FLIP   = 0x40,
    ATTRIBUTE_PRIORITY = 0x80
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

#define BUFFER_SIZE 160 * 3 * 144

struct PPU{
    SDL_Texture *framebuffer;
    SDL_Renderer *renderer;
    i32 current_pos;
    u8 buffer[BUFFER_SIZE];

    u8 *pixels;
    i32 pitch;

    PPUMode mode;
    TileFetchState tile_fetch_state;
    Memory *memory;
    Array<Sprite> sprites;
    Array<Sprite> sprites_active;
    u32 cycles;

    Color colors[4];

    u8 oam_offset;
    u16 oam_initial_address;
    u16 current_oam_address;

    u8 tile_low;
    u8 tile_high;
    u8 tile_index;
    u8 tile_x;
    u16 pixel_count;
    bool do_dummy_fetch;
    bool skip_fifo;
    bool frame_ready;
    bool stat_interrupt_set;

    bool stop_fifos;
    bool fetching_sprite;
    bool check_sprites;
    Array<Pixel> bg_fifo;
    Array<Pixel> sprite_fifo;
    Array<Pixel> sprite_mixing_fifo;

    u8 sprites_processed;
    Sprite sprite;
};
struct CPU;
void init_ppu(PPU *ppu, Memory *memory, SDL_Renderer *renderer);
void ppu_tick(PPU *ppu, CPU *cpu);
void ppu_render(PPU *ppu);