#include "ppu.h"




u8 read_lcdc(PPU *ppu){
    return read_memory(ppu->memory, 0xFF40, true);
}

u8 get_LY(PPU *ppu){
    return read_memory(ppu->memory, 0xFF44, true);
}

u8 get_LYC(PPU *ppu){
    return read_memory(ppu->memory, 0xFF45, true);
}

u8 get_SCY(PPU *ppu){
    return read_memory(ppu->memory, 0xFF42, true);
}

u8 get_SCX(PPU *ppu){
    return read_memory(ppu->memory, 0xFF43, true);
}

u8 get_WY(PPU *ppu){
    return read_memory(ppu->memory, 0xFF4A, true);
}

u8 get_WX(PPU *ppu){
    return read_memory(ppu->memory, 0xFF4B, true);
}

void set_LY(PPU *ppu, u8 value){
    write_memory(ppu->memory, 0xFF44, value, true);
}

void increase_LY(PPU *ppu){
    u8 LY = get_LY(ppu);
    LY++;
    set_LY(ppu, LY);
}

void set_stat_ppu_mode(PPU *ppu, u8 mode){
    assert(mode <= 3);
    u8 stat = read_memory(ppu->memory, 0xFF41, true);
    stat |= mode; // Set STAT register ppu mode.
    write_memory(ppu->memory, 0xFF41, stat, true);
}

void init_ppu(PPU *ppu, Memory *memory, SDL_Renderer *renderer){
    ppu->renderer = renderer;
    ppu->framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 256, 256);
    ppu->current_pos = 0;
    assert(ppu->framebuffer);

    ppu->memory = memory;
    ppu->mode = MODE_OAM_SCAN;
    ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
    ppu->sprite_fetch_state = SPRITE_FETCH_INDEX;
    ppu->sprites = make_array<Sprite>(10);

    ppu->bg_fifo     = make_array<Pixel>(8);
    ppu->sprite_fifo = make_array<Pixel>(8);

    ppu->colors[0] = Color{255,255,255}; // White
    ppu->colors[1] = Color{190,190,190}; // Light gray
    ppu->colors[2] = Color{63,63,63};    // Dark gray
    ppu->colors[3] = Color{0,0,0};       // Black

    ppu->cycles = 0;
    ppu->oam_offset = 4;
    ppu->oam_initial_address = 0xFE00;
    ppu->current_oam_address = ppu->oam_initial_address;
    ppu->do_dummy_fetch = true;
    set_stat_ppu_mode(ppu, 2);
}

static void push_to_screen(PPU *ppu){
    if(ppu->bg_fifo.size == 0) return;

    u32 offset = 3;
    i32 pitch;
    u8 *pixels;
    SDL_LockTexture(ppu->framebuffer, NULL, (void**)&pixels, &pitch);
    Pixel pixel = array_pop(&ppu->bg_fifo);
    pixels[ppu->current_pos] = ppu->colors[pixel.color].r;
    pixels[ppu->current_pos + 1] = ppu->colors[pixel.color].g;
    pixels[ppu->current_pos + 2] = ppu->colors[pixel.color].b;

    ppu->current_pos += offset;

    SDL_UnlockTexture(ppu->framebuffer);
}

void ppu_render(PPU *ppu){
    SDL_RenderTexture(ppu->renderer, ppu->framebuffer, NULL, NULL);
}

void ppu_tick(PPU *ppu){
    if(read_lcdc(ppu) & LCDC_LCD_PPU_ENABLE){
        switch(ppu->mode){
            case MODE_OAM_SCAN:{
                ppu->memory->is_oam_locked = true;
                
                Sprite sprite;
                sprite.y_position = read_memory(ppu->memory, ppu->current_oam_address, true);
                sprite.x_position = read_memory(ppu->memory, ppu->current_oam_address + 1, true);
                sprite.tile_index = read_memory(ppu->memory, ppu->current_oam_address + 2, true);
                sprite.attributes = read_memory(ppu->memory, ppu->current_oam_address + 3, true);

                u8 sprite_height = 8;
                if(read_lcdc(ppu) & LCDC_OBJ_SIZE){
                    sprite_height = 16;
                }
                if(sprite.x_position > 0 && (get_LY(ppu) + 16) >= sprite.y_position && (get_LY(ppu) + 16) < (sprite.y_position + sprite_height) && ppu->sprites.size < 10){
                    array_add(&ppu->sprites, sprite);
                }

                ppu->current_oam_address += ppu->oam_offset;
                ppu->cycles += 2;

                if(ppu->cycles == 80){
                    ppu->mode = MODE_DRAW;
                    set_stat_ppu_mode(ppu, 3);
                    ppu->current_oam_address = ppu->oam_initial_address;
                }
                break;
            }
            case MODE_DRAW:{
                ppu->memory->is_vram_locked = true;

                switch(ppu->tile_fetch_state){
                    case TILE_FETCH_TILE_INDEX:{
                        u32 offset = ((ppu->tile_x + (get_SCX(ppu) / 8)) & 0x1F) + (32 * (((get_LY(ppu) + get_SCY(ppu)) & 0xFF) / 8));
                        u16 address = 0;
                        (read_lcdc(ppu) & LCDC_BG_TILEMAP) ? address = 0x9C00 : address = 0x9800;
                        
                        ppu->tile_index = read_memory(ppu->memory, address + offset, true); 
                        ppu->tile_fetch_state = TILE_FETCH_TILE_LOW;
                        break;
                    }
                    case TILE_FETCH_TILE_LOW:{
                        if(read_lcdc(ppu) & LCDC_BG_WIN_TILEDATA){ // $8000 addressing mode
                            u32 offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                            ppu->tile_low = read_memory(ppu->memory, 0x8000 + (ppu->tile_index * 16) + offset, true);
                        }
                        else{ // $8800 addressing mode
                             // TODO:
                        }

                        ppu->tile_fetch_state = TILE_FETCH_TILE_HIGH;
                        break;
                    }
                    case TILE_FETCH_TILE_HIGH:{
                        if(read_lcdc(ppu) & LCDC_BG_WIN_TILEDATA){ // $8000 addressing mode
                            u32 offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                            ppu->tile_low = read_memory(ppu->memory, 0x8000 + (ppu->tile_index * 16) + offset + 1);
                        }
                        else{ // $8800 addressing mode
                             // TODO:
                        }

                        if(ppu->bg_fifo.size == 0){ // Background FIFO is empty so we can push a row of pixels.
                            for(int i = 0; i < 8; i++){
                                Pixel pixel;
                                u8 color_low  = (ppu->tile_low  >> i) & 0x1;
                                u8 color_high = (ppu->tile_high >> i) & 0x1; 
                                pixel.color   = (color_high << 1) | color_low;
                                array_add(&ppu->bg_fifo, pixel);
                            }

                            ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
                            ppu->tile_x++;
                        }
                        else{
                            ppu->tile_fetch_state = TILE_FETCH_PUSH_FIFO;
                        }
                        if(ppu->do_dummy_fetch){
                            ppu->do_dummy_fetch = false;
                            ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
                        }
                        break;
                    }
                    case TILE_FETCH_PUSH_FIFO:{ // If pixels could not be pushed during tile high fetch we keep trying until it succeeds when the FIFO is empty.
                        if(ppu->bg_fifo.size == 0){ // Background FIFO is empty so we can push a row of pixels.
                            for(int i = 0; i < 8; i++){
                                Pixel pixel;
                                u8 color_low  = (ppu->tile_low  >> i) & 0x1;
                                u8 color_high = (ppu->tile_high >> i) & 0x1; 
                                pixel.color   = (color_high << 1) | color_low;
                                array_add(&ppu->bg_fifo, pixel);
                            }
                            ppu->tile_fetch_state =  TILE_FETCH_TILE_INDEX;
                            ppu->tile_x++;
                        }
                        break;
                    }
                }

                push_to_screen(ppu);
                push_to_screen(ppu);

                ppu->cycles += 2;
                if(ppu->tile_x == 32){
                    ppu->mode = MODE_HBLANK;
                    set_stat_ppu_mode(ppu, 0);
                    ppu->memory->is_vram_locked = false;
                    ppu->memory->is_oam_locked  = false;
                    ppu->tile_x = 0;
                    array_clear(&ppu->sprites); // Clear the list of sprites for the current scanline.
                }
                break;
            }
            case MODE_HBLANK:{
                

                ppu->cycles += 2;

                if(ppu->cycles == 456){
                    if(get_LY(ppu) < 143){
                        ppu->mode = MODE_OAM_SCAN;
                        set_stat_ppu_mode(ppu, 2);
                    }
                    else if(get_LY(ppu) == 143){
                        ppu->mode = MODE_VBLANK;
                        set_stat_ppu_mode(ppu, 1);
                    }

                    ppu->cycles = 0;
                    increase_LY(ppu);
                }
                break;
            }
            case MODE_VBLANK:{


                ppu->cycles += 2;
                if(ppu->cycles == 4560 && get_LY(ppu) == 153){
                    ppu->mode = MODE_OAM_SCAN;
                    set_LY(ppu, 0);
                    set_stat_ppu_mode(ppu, 2);
                    ppu->current_pos = 0;
                    ppu->cycles = 0;
                }
                break;
            }
        }

        if(get_LY(ppu) == get_LYC(ppu)){
            u8 stat = read_memory(ppu->memory, 0xFF41, true);
            stat |= LCDSTAT_LYC_LY;
            write_memory(ppu->memory, 0xFF41, stat, true);
        }
        else{
            u8 stat = read_memory(ppu->memory, 0xFF41, true);
            stat &= ~(LCDSTAT_LYC_LY);
            write_memory(ppu->memory, 0xFF41, stat, true);
        }
        
    }
    else{
        ppu->current_oam_address = ppu->oam_initial_address;
        ppu->memory->is_oam_locked  = false;
        ppu->memory->is_vram_locked = false;
        ppu->mode = MODE_OAM_SCAN;
        set_stat_ppu_mode(ppu, 2);
        ppu->current_pos = 0;
        
        ppu->cycles = 0;
        set_LY(ppu, 0);
    }
}