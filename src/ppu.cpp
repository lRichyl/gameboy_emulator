#include "ppu.h"



void init_ppu(PPU *ppu, Memory *memory){
    ppu->memory = memory;
    ppu->mode = MODE_OAM_SCAN;
    ppu->sprites = make_array<Sprite>(10);

    ppu->LY = 0;
    ppu->cycles = 0;
    ppu->oam_offset = 4;
    ppu->oam_initial_address = 0xFE00;
    ppu->current_oam_address = ppu->oam_initial_address;
}

u8 read_lcdc(PPU *ppu){
    return read_memory(ppu->memory, 0xFF40, true);
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
                if(sprite.x_position > 0 && (ppu->LY + 16) >= sprite.y_position && (ppu->LY + 16) < (sprite.y_position + sprite_height) && ppu->sprites.size < 10){
                    array_add(&ppu->sprites, sprite);
                }

                ppu->current_oam_address += ppu->oam_offset;
                ppu->cycles += 2;

                if(ppu->cycles == 80){
                    ppu->mode = MODE_DRAW;
                    ppu->memory->is_oam_locked = false;
                    ppu->current_oam_address = ppu->oam_initial_address;
                }
                break;
            }
            case MODE_DRAW:{


                ppu->cycles += 2;
                if(ppu->cycles == 289){
                    ppu->mode = MODE_HBLANK;
                }
                break;
            }
            case MODE_HBLANK:{
                

                ppu->cycles += 2;

                if(ppu->cycles == 456){
                    if(ppu->LY < 143){
                        ppu->mode = MODE_DRAW;
                    }
                    else if(ppu->LY == 143){
                        ppu->mode = MODE_VBLANK;
                    }

                    ppu->LY++;
                }
                break;
            }
            case MODE_VBLANK:{


                ppu->cycles += 2;
                if(ppu->cycles == 456 && ppu->LY == 153){
                    ppu->mode = MODE_OAM_SCAN;
                    ppu->LY = 0;
                }
                break;
            }
        }
    }
    else{
        ppu->current_oam_address = ppu->oam_initial_address;
        ppu->memory->is_oam_locked  = false;
        ppu->memory->is_vram_locked = false;
        ppu->mode = MODE_OAM_SCAN;
        ppu->cycles = 0;
        ppu->LY = 0;
    }
}