#include "ppu.h"
#include "CPU.h"


static u8 read_memory_ppu(PPU *ppu, u16 address){
    //if(address >= 0x8000 && address <= 0x9FFF){ // VRAM
    //    return ppu->memory->data[address];
    //}
    //else if(address >= 0xFE00 && address <= 0xFE9F){ // OAM
    //    return 0xFF;
    //}

    return ppu->memory->data[address];
}

static void write_memory_ppu(PPU *ppu, u16 address, u8 value){
    // if(address == 0xFF41 || address == 0xFF44){
        ppu->memory->data[address] = value;
    // }
}

u8 read_lcdc(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF40);
}

u8 get_LY(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF44);
}

u8 get_LYC(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF45);
}

u8 get_SCY(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF42);
}

u8 get_SCX(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF43);
}

u8 get_WY(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF4A);
}

u8 get_WX(PPU *ppu){
    return read_memory_ppu(ppu, 0xFF4B);
}

void set_LY(PPU *ppu, u8 value){
    write_memory_ppu(ppu, 0xFF44, value);
}

void increase_LY(PPU *ppu){
    u8 LY = get_LY(ppu);
    LY++;
    set_LY(ppu, LY);
}

void set_stat_ppu_mode(PPU *ppu, u8 mode){
    assert(mode <= 3);
    u8 stat = read_memory_ppu(ppu, 0xFF41);
    stat = (stat & (~0x03)) | mode; // Set STAT register ppu mode.
    write_memory_ppu(ppu, 0xFF41, stat);
}

void init_ppu(PPU *ppu, Memory *memory, SDL_Renderer *renderer){
    ppu->renderer = renderer;
    ppu->framebuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 160, 144);
    ppu->current_pos = 0;
    assert(ppu->framebuffer);

    ppu->memory = memory;
    ppu->mode = MODE_OAM_SCAN;
    ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
    ppu->sprites_processed = 0;
    ppu->sprites        = make_array<Sprite>(40);
    ppu->sprites_active = make_array<Sprite>(10);

    ppu->bg_fifo     = make_array<Pixel>(8);
    ppu->sprite_fifo = make_array<Pixel>(8);
    ppu->sprite_mixing_fifo = make_array<Pixel>(8);

    ppu->colors[0] = Color{255,255,255}; // White
    ppu->colors[1] = Color{0xAA,0xAA,0xAA}; // Light gray
    ppu->colors[2] = Color{0x55,0x55,0x55};    // Dark gray
    ppu->colors[3] = Color{0,0,0};       // Black

    ppu->cycles = 0;
    ppu->oam_offset = 4;
    ppu->oam_initial_address = 0xFE00;
    ppu->current_oam_address = ppu->oam_initial_address;
    ppu->do_dummy_fetch = true;
    ppu->skip_fifo = false;
    ppu->frame_ready = false;
    ppu->stop_fifos = false;
    ppu->fetching_sprite = false;
    ppu->check_sprites = true;
    set_stat_ppu_mode(ppu, 2);
}

static void sort_objects_by_x_position(Array<Sprite> *sprites){
    if(sprites->size <= 1) return;
    for(int i = 0; i < sprites->size - 1; i++){
        Sprite spr = array_get(sprites, i);
        Sprite spr_next = array_get(sprites, i + 1);

        if(spr.x_position > spr_next.x_position){
            array_set(sprites, i, spr_next);
            array_set(sprites, i + 1, spr);
        }
    }
}

static void check_if_sprite_is_in_current_position(PPU *ppu){
    if(ppu->check_sprites){
        for(int i = 0; i < ppu->sprites.size; i++){
            Sprite sprite = array_get(&ppu->sprites, i);
            i32 sprite_x_corrected = sprite.x_position - 8;
            if(sprite_x_corrected >= 0){
                if(sprite_x_corrected == ppu->pixel_count){
                    array_add(&ppu->sprites_active, sprite);

                    ppu->stop_fifos = true;
                    ppu->tile_fetch_state = TILE_FETCH_SPRITE_INDEX;
                    break;
                }
            }
            else if(sprite_x_corrected < 0 && sprite_x_corrected >= -7){
                if(sprite_x_corrected == (i32)(ppu->pixel_count - (8 - sprite.x_position))){
                    array_add(&ppu->sprites_active, sprite);

                    ppu->stop_fifos = true;
                    ppu->tile_fetch_state = TILE_FETCH_SPRITE_INDEX;
                }
            }
            if(i == ppu->sprites_active.capacity - 1) break;
        }
        ppu->check_sprites = false;
    }
}

static void push_to_screen(PPU *ppu){
    // TODO: Here we should return when scrolling.

    

    if(ppu->bg_fifo.size == 0) return;
    if(ppu->stop_fifos) return;

    u32 offset = 3;
    Pixel bg_pixel = array_pop(&ppu->bg_fifo);
    u16 bg_palette_address = 0xFF47;
    u8 palette = read_memory_ppu(ppu, bg_palette_address);
    u8 bg_color_ids[4] = {(palette & 0x3), ((palette & 0xC) >> 2), ((palette & 0x30) >> 4), ((palette & 0xC0) >> 6)};

    Color color;
    if(ppu->sprite_fifo.size > 0 && (!ppu->skip_fifo)){
        Pixel sp_pixel = array_pop(&ppu->sprite_fifo);
        if(sp_pixel.color == 0 || (sp_pixel.bg_priority && bg_pixel.color != 0)){

            color = ppu->colors[bg_color_ids[bg_pixel.color]];
            // if(ppu->render_window){
            //     color = {255,0,0};
            // }
        }
        else{
            u16 address;
            sp_pixel.palette == 0 ? address = 0xFF48 : address = 0xFF49; 
            u8 palette = read_memory_ppu(ppu, address);

            u8 color_ids[4] = {0, ((palette & 0xC) >> 2), ((palette & 0x30) >> 4), ((palette & 0xC0) >> 6)};

            color = ppu->colors[color_ids[sp_pixel.color]];
            // color = {255,0,0};
        }
    }
    else{
        color = ppu->colors[bg_color_ids[bg_pixel.color]];
        // if(ppu->render_window){
        //     color = {255,0,0};
        // }
    }

    if(!ppu->skip_fifo){
        ppu->pixel_count++;
        ppu->buffer[ppu->current_pos] = color.r;
        ppu->buffer[ppu->current_pos + 1] = color.g;
        ppu->buffer[ppu->current_pos + 2] = color.b;
        ppu->current_pos += offset;

        
        ppu->check_sprites = true;
        
        if(ppu->LY_equals_WY && ((get_WX(ppu)-7) == ppu->pixel_count) && (read_lcdc(ppu) & LCDC_WINDOW_ENABLE)){
            ppu->render_window = true;
            ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
            array_clear(&ppu->bg_fifo);
        }
    }

    

    if(ppu->bg_fifo.size == 0) ppu->skip_fifo = false;
    
}



void ppu_render(PPU *ppu){
    i32 pitch;
    u8 *pixels;
    SDL_LockTexture(ppu->framebuffer, NULL, (void**)&pixels, &pitch);
    memcpy(pixels, ppu->buffer, BUFFER_SIZE);

    SDL_UnlockTexture(ppu->framebuffer);

    // SDL_UpdateTexture(ppu->framebuffer, NULL, ppu->buffer, 160*3);
    SDL_RenderTexture(ppu->renderer, ppu->framebuffer, NULL, NULL);
    memset(ppu->buffer, 0, BUFFER_SIZE);
}

void set_LYC_LY(PPU *ppu){
    u8 stat = read_memory_ppu(ppu, 0xFF41);
    stat |= (LCDSTAT_LYC_LY);
    write_memory_ppu(ppu, 0xFF41, stat);
}

void unset_LYC_LY(PPU *ppu){
    u8 stat = read_memory_ppu(ppu, 0xFF41);
    stat &= ~(LCDSTAT_LYC_LY);
    write_memory_ppu(ppu, 0xFF41, stat);
}

u32 count = 0;
void ppu_tick(PPU *ppu, CPU *cpu){
    static bool once = false;
    if(read_lcdc(ppu) & LCDC_LCD_PPU_ENABLE){

        if(get_LY(ppu) == get_LYC(ppu)){
            set_LYC_LY(ppu);
            if(ppu->cycles == 0x04){
                if(get_LY(ppu) == get_LYC(ppu)){
                    if(read_memory_ppu(ppu, 0xFF41) & LCDSTAT_LYC_INT && !ppu->stat_interrupt_set){
                        ppu->stat_interrupt_set = true;
                        set_interrupt(cpu, INT_LCD);
                    }
                }
            }
        }
        else{
            unset_LYC_LY(ppu);
        }
        if(get_LY(ppu) == 0) unset_LYC_LY(ppu);
        
        if(get_LY(ppu) == 0) unset_LYC_LY(ppu);
        switch(ppu->mode){
            case MODE_OAM_SCAN:{
                if(get_LY(ppu) == get_WY(ppu)){
                    ppu->LY_equals_WY = true;
                }

                set_stat_ppu_mode(ppu, 2);
                 if((read_memory_ppu(ppu, 0xFF41) & LCDSTAT_MODE_2)  && !ppu->stat_interrupt_set){
                     ppu->stat_interrupt_set = true;
                     set_interrupt(cpu, INT_LCD);
                 }
                ppu->memory->is_oam_locked = true;
                
                Sprite sprite;
                sprite.y_position = read_memory_ppu(ppu, ppu->current_oam_address);
                sprite.x_position = read_memory_ppu(ppu, ppu->current_oam_address + 1);
                sprite.tile_index = read_memory_ppu(ppu, ppu->current_oam_address + 2);
                sprite.attributes = read_memory_ppu(ppu, ppu->current_oam_address + 3);

                u8 sprite_height = 8;
                if(read_lcdc(ppu) & LCDC_OBJ_SIZE){
                    sprite_height = 16;
                }
                if(sprite.x_position > 0 && (get_LY(ppu) + 16) >= sprite.y_position && (get_LY(ppu) + 16) < (sprite.y_position + sprite_height) && ppu->sprites.size < 40){
                    array_add(&ppu->sprites, sprite);
                }
               

                ppu->current_oam_address += ppu->oam_offset;
                count += 2;
                ppu->cycles += 2;
                // printf("Cycles %d\n", ppu->cycles);
                assert(ppu->cycles <= 80);
                if(ppu->cycles == 80){
                    ppu->mode = MODE_DRAW;
                    ppu->current_oam_address = ppu->oam_initial_address;
                    sort_objects_by_x_position(&ppu->sprites);
                }
                break;
            }
            case MODE_DRAW:{
                ppu->memory->is_vram_locked = true;
                set_stat_ppu_mode(ppu, 3);
                switch(ppu->tile_fetch_state){
                    case TILE_FETCH_TILE_INDEX:{
                        u32 offset;
                        u16 address;
                        if(ppu->render_window){
                            offset = (ppu->window_tile_x) + (32 * (ppu->window_line_counter / 8));
                            (read_lcdc(ppu) & LCDC_WIN_TILEMAP) ? address = 0x9C00 : address = 0x9800;
                        }
                        else{
                            offset = ((ppu->tile_x + (get_SCX(ppu) / 8)) & 0x1F) + (32 * (((get_LY(ppu) + get_SCY(ppu)) & 0xFF) / 8));
                            (read_lcdc(ppu) & LCDC_BG_TILEMAP) ? address = 0x9C00 : address = 0x9800;
                        }
                        ppu->tile_index = read_memory_ppu(ppu, address + offset); 
                        ppu->tile_fetch_state = TILE_FETCH_TILE_LOW;
                        break;
                    }
                    case TILE_FETCH_TILE_LOW:{
                        if((read_lcdc(ppu) & LCDC_BG_WIN_TILEDATA)){ // $8000 addressing mode
                            u32 offset;
                            if(ppu->render_window  && (read_lcdc(ppu) & LCDC_WINDOW_ENABLE)){
                                offset = 2 * (ppu->window_line_counter % 8); 
                            }
                            else{
                                offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                            }
                            u16 address = 0x8000 + (ppu->tile_index * 16) + offset; 
                            ppu->tile_low = read_memory_ppu(ppu, address);
                        }
                        else{ // $8800 addressing mode
                            u32 offset;
                            if(ppu->render_window && (read_lcdc(ppu) & LCDC_WINDOW_ENABLE)){
                                offset = 2 * (ppu->window_line_counter % 8); 
                            }
                            else{
                                offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                            }
                            u16 address;
                            if(offset > 127){
                                offset -= 128;
                                address = 0x8800 + ((i8)ppu->tile_index * 16) + offset; 
                            }
                            else{
                                address = 0x9000 + ((i8)ppu->tile_index * 16) + offset;
                            }
                            ppu->tile_low = read_memory_ppu(ppu, address);
                        }
                        if(!(read_lcdc(ppu) & LCDC_BG_WIN_ENABLE)){
                            ppu->tile_low = 0x00;
                        }

                        ppu->tile_fetch_state = TILE_FETCH_TILE_HIGH;
                        break;
                    }
                    case TILE_FETCH_TILE_HIGH:{
                        if((read_lcdc(ppu) & LCDC_BG_WIN_TILEDATA)){ // $8000 addressing mode
                            u32 offset;
                            if(ppu->render_window && (read_lcdc(ppu) & LCDC_WINDOW_ENABLE)){
                                offset = 2 * (ppu->window_line_counter % 8); 
                            }
                            else{
                                offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                            } 
                            u16 address = 0x8000 + (ppu->tile_index * 16) + offset + 1; 
                            ppu->tile_high = read_memory_ppu(ppu, address);
                        }
                        else{ // $8800 addressing mode
                            u32 offset;
                            if(ppu->render_window && (read_lcdc(ppu) & LCDC_WINDOW_ENABLE)){
                                offset = 2 * (ppu->window_line_counter % 8); 
                            }
                            else{
                                offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                            }
                            u16 address;
                            if(offset > 127){
                                offset -= 128;
                                address = 0x8800 + ((i8)ppu->tile_index * 16) + offset; 
                            }
                            else{
                                address = 0x9000 + ((i8)ppu->tile_index * 16) + offset;
                            }
                            ppu->tile_high = read_memory_ppu(ppu, address + 1);
                        }
                        if(!(read_lcdc(ppu) & LCDC_BG_WIN_ENABLE)){
                            ppu->tile_high = 0x00;
                        }

                        if(ppu->bg_fifo.size == 0 && !(read_lcdc(ppu) & LCDC_BG_WIN_ENABLE)){ // Background FIFO is empty so we can push a row of pixels.
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
                            ppu->tile_x--;
                            ppu->skip_fifo = true;
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
                            if(ppu->render_window && (read_lcdc(ppu) & LCDC_WINDOW_ENABLE)) ppu->window_tile_x++;
                        }
                        break;
                    }
                    // SPRITE FETCHING
                    case TILE_FETCH_SPRITE_INDEX:{
                        if((read_lcdc(ppu) & LCDC_OBJ_SIZE)){
                            ppu->sprite = array_get(&ppu->sprites_active, ppu->sprites_processed);
                            ppu->tile_index = ppu->sprite.tile_index;

                            u8 LY = get_LY(ppu);
                            if(LY < ppu->sprite.y_position - 8){
                                ppu->tile_index &= ~(0x01);
                                if((ppu->sprite.attributes & ATTRIBUTE_Y_FLIP))
                                    ppu->tile_index |= (0x01);
                            }
                            else{
                                ppu->tile_index |= (0x01);
                                if((ppu->sprite.attributes & ATTRIBUTE_Y_FLIP))
                                    ppu->tile_index &= ~(0x01);
                            }

                            ppu->tile_fetch_state = TILE_FETCH_SPRITE_LOW;
                        }
                        else{
                            ppu->sprite = array_get(&ppu->sprites_active, ppu->sprites_processed);
                            ppu->tile_index = ppu->sprite.tile_index;
                            ppu->tile_fetch_state = TILE_FETCH_SPRITE_LOW;
                        }
                        break;
                    }
                    case TILE_FETCH_SPRITE_LOW:{
                        if((read_lcdc(ppu) & LCDC_OBJ_ENABLE)){
                            if((ppu->sprite.attributes & ATTRIBUTE_Y_FLIP)){
                                u32 offset; 
                                if(ppu->sprite.y_position % 8){
                                    offset = 14 - (2 * (16 - (ppu->sprite.y_position - (get_LY(ppu) + get_SCY(ppu))))); 
                                }
                                else{
                                    offset = 14 - (2 * ((get_LY(ppu) + get_SCY(ppu)) % 8)); 
                                }
                                
                                u16 address = 0x8000 + (ppu->tile_index * 16) + offset; // Sprites always use $8000 addressing mode.
                                ppu->tile_low = read_memory_ppu(ppu, address);
                            }
                            else{
                                u32 offset;
                                if(ppu->sprite.y_position % 8){
                                    offset = 2 * (16 - (ppu->sprite.y_position - (get_LY(ppu) + get_SCY(ppu)))); 
                                }
                                else{
                                    offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                                }
                                
                                u16 address = 0x8000 + (ppu->tile_index * 16) + offset; // Sprites always use $8000 addressing mode.
                                ppu->tile_low = read_memory_ppu(ppu, address);
                            }
                        }
                        else{
                            ppu->tile_low = 0x00; // Transparent.
                        }
                        ppu->tile_fetch_state = TILE_FETCH_SPRITE_HIGH;
                        break;
                    }
                    case TILE_FETCH_SPRITE_HIGH:{
                        if((read_lcdc(ppu) & LCDC_OBJ_ENABLE)){
                            if((ppu->sprite.attributes & ATTRIBUTE_Y_FLIP)){
                                u32 offset; 
                                if(ppu->sprite.y_position % 8){
                                    offset = 14 - (2 * (16 - (ppu->sprite.y_position - (get_LY(ppu) + get_SCY(ppu))))); 
                                }
                                else{
                                    offset = 14 - (2 * ((get_LY(ppu) + get_SCY(ppu)) % 8)); 
                                }
                               
                               
                                u16 address = 0x8000 + (ppu->tile_index * 16) + offset + 1; // Sprites always use $8000 addressing mode.
                                ppu->tile_high = read_memory_ppu(ppu, address);
                            }
                            else{
                                u32 offset;
                                if(ppu->sprite.y_position % 8){
                                    offset = 2 * (16 - (ppu->sprite.y_position - (get_LY(ppu) + get_SCY(ppu)))); 
                                }
                                else{
                                    offset = 2 * ((get_LY(ppu) + get_SCY(ppu)) % 8); 
                                }
                                u16 address = 0x8000 + (ppu->tile_index * 16) + offset + 1; // Sprites always use $8000 addressing mode.
                                ppu->tile_high = read_memory_ppu(ppu, address);
                            }
                        }
                        else{
                            ppu->tile_high = 0x00; // Transparent.
                        }
                        ppu->tile_fetch_state = TILE_FETCH_SPRITE_PUSH;
                        break;
                    }
                    case TILE_FETCH_SPRITE_PUSH:{
                        if(ppu->sprite.attributes & ATTRIBUTE_X_FLIP){
                            for(int i = 7; i >= 0; i--){
                                Pixel pixel;
                                u8 color_low  = (ppu->tile_low  >> i) & 0x1;
                                u8 color_high = (ppu->tile_high >> i) & 0x1; 
                                pixel.color   = (color_high << 1) | color_low;
                                pixel.bg_priority = (ppu->sprite.attributes & ATTRIBUTE_PRIORITY) >> 7;
                                pixel.palette     = (ppu->sprite.attributes & ATTRIBUTE_PALETTE) >> 3;
                                array_add(&ppu->sprite_mixing_fifo, pixel);
                            }
                        }
                        else{
                            for(int i = 0; i < 8; i++){
                                Pixel pixel;
                                u8 color_low  = (ppu->tile_low  >> i) & 0x1;
                                u8 color_high = (ppu->tile_high >> i) & 0x1; 
                                pixel.color   = (color_high << 1) | color_low;
                                pixel.bg_priority = (ppu->sprite.attributes & ATTRIBUTE_PRIORITY) >> 7;
                                pixel.palette     = (ppu->sprite.attributes & ATTRIBUTE_PALETTE) >> 3;
                                array_add(&ppu->sprite_mixing_fifo, pixel);
                            }

                        }
                            
                            i32 sprite_x_corrected = (i32)(ppu->sprite.x_position) - 8;
                            if(sprite_x_corrected >= 0){
                                if(ppu->sprite_fifo.size == 0){
                                    array_copy(&ppu->sprite_fifo, &ppu->sprite_mixing_fifo);
                                }
                                else{
                                    // When there is already pixel data in the pixel fifo we mix them.
                                    ppu->sprite_fifo.size = 8; // Bad
                                    for(int i = 0; i < ppu->sprite_fifo.size; i++){
                                        u8 current_color = array_get(&ppu->sprite_fifo, i).color;
                                        assert(current_color <= 3);
                                        Pixel pixel = array_get(&ppu->sprite_mixing_fifo, i);
                                        if(current_color == 0){
                                            array_set(&ppu->sprite_fifo, i, pixel);
                                        }
                                    }
                                }
                            }
                            else if(sprite_x_corrected < 0){
                                
                                u8 clipping_offset =  8 - (ppu->sprite.x_position);
                                for(int i = 0; i < clipping_offset; i++){
                                    array_pop(&ppu->sprite_mixing_fifo);
                                }

                                if(ppu->sprite_fifo.size == 0){
                                    for(int i = 0; i <  8 - clipping_offset; i++){
                                        Pixel pixel = array_get(&ppu->sprite_mixing_fifo, i);
                                        array_add(&ppu->sprite_fifo, pixel);                                    
                                    }
                                }
                                else{
                                    ppu->sprite_fifo.size = 8; // Bad
                                   for(int i = 0; i < 8 - clipping_offset - 1; i++){
                                       u8 current_color = array_get(&ppu->sprite_fifo, ppu->sprite_fifo.size - i - 1).color;
                                       assert(current_color <= 3);
                                       Pixel pixel = array_pop(&ppu->sprite_mixing_fifo);
                                       if(current_color == 0){
                                           array_set(&ppu->sprite_fifo, ppu->sprite_fifo.size - i - 1, pixel);
                                       }
                                   }
                                }
                            }

                        array_clear(&ppu->sprite_mixing_fifo);
                        ppu->sprites_processed++; 

                        if(ppu->sprites_processed < ppu->sprites_active.size){
                            ppu->tile_fetch_state = TILE_FETCH_SPRITE_INDEX;    
                                    
                        }
                        else if(ppu->sprites_processed == ppu->sprites_active.size){
                            ppu->stop_fifos = false;
                            ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
                            ppu->fetching_sprite = false;
                            ppu->sprites_processed = 0;
                            array_clear(&ppu->sprites_active);
                        }
                        
                        break;
                    }
                }

                
                check_if_sprite_is_in_current_position(ppu);
                push_to_screen(ppu);
                check_if_sprite_is_in_current_position(ppu);
                push_to_screen(ppu);
                

                ppu->cycles += 2;
                count += 2;
                if(ppu->pixel_count == 160){ 
                    array_clear(&ppu->bg_fifo);
                    array_clear(&ppu->sprite_fifo);
                    ppu->mode = MODE_HBLANK;
                    ppu->tile_fetch_state = TILE_FETCH_TILE_INDEX;
                        
                    ppu->memory->is_vram_locked = false;
                    ppu->memory->is_oam_locked  = false;
                    ppu->tile_x = 0;
                    ppu->pixel_count = 0;
                    ppu->sprites_processed = 0;
                        
                    array_clear(&ppu->sprites); // Clear the list of sprites for the current scanline.
                    
                    ppu->stat_interrupt_set = false;
                    
                    if(ppu->render_window){
                        ppu->window_line_counter++;
                    }
                    ppu->render_window = false;
                    ppu->window_tile_x = 0;

                }
                break;
            }
            case MODE_HBLANK:{
                set_stat_ppu_mode(ppu, 0);
                 if((read_memory_ppu(ppu, 0xFF41) & LCDSTAT_MODE_0) && !ppu->stat_interrupt_set){
                     ppu->stat_interrupt_set = true;
                     set_interrupt(cpu, INT_LCD);
                 }
                count += 2;
                ppu->cycles += 2;
                if(ppu->cycles == 456){
                    increase_LY(ppu);
                    if(get_LY(ppu) < 144){
                        ppu->mode = MODE_OAM_SCAN;
                    }
                    else if(get_LY(ppu) == 144){
                        ppu->mode = MODE_VBLANK;
                        ppu->LY_equals_WY = false;
                        ppu->render_window = false;
                    }

                    ppu->do_dummy_fetch = true;
                    ppu->cycles = 0;
                }
                break;
            }
            case MODE_VBLANK:{
                set_stat_ppu_mode(ppu, 1);

                 if((read_memory_ppu(ppu, 0xFF41) & LCDSTAT_MODE_1)  && !ppu->stat_interrupt_set){
                     ppu->stat_interrupt_set = true;
                     set_interrupt(cpu, INT_LCD);
                 }
                count += 2;
                ppu->cycles += 2;
                if(ppu->cycles == 4)
                    set_interrupt(cpu, INT_VBLANK); 
                if(ppu->cycles == 456){
                    if(get_LY(ppu) == 153){
                        ppu->mode = MODE_OAM_SCAN;
                        
                        set_LY(ppu, 0);
                        ppu->current_pos = 0;
                        ppu->window_line_counter = 0;
                        count = 0;
                        
                        ppu->frame_ready = true;
                        ppu_render(ppu);
                    }
                    else{
                        increase_LY(ppu);
                    }
                      
                    ppu->cycles = 0;
                }
                break;
            }
        }


        
        once = true;

    }
    else{
        if(!once) return;
        ppu->current_oam_address = ppu->oam_initial_address;
        ppu->memory->is_oam_locked  = false;
        ppu->memory->is_vram_locked = false;
        ppu->mode = MODE_OAM_SCAN;
        // set_stat_ppu_mode(ppu, 2);
        ppu->tile_x = 0;
        ppu->current_pos = 0;

        ppu_render(ppu);
        ppu->frame_ready = true;

        
        ppu->cycles = 0;
        set_LY(ppu, 0);

        once = false;
    }
}