#include "memory.h"
#include "file_handling.h"
#include "arena.h"
#include <stdlib.h>
#include <stdio.h>

#define CARTRIDGE_TYPE_LOC 0x147
#define ROM_SIZE_LOC 0x148
#define RAM_SIZE_LOC 0x149
#define ROM_BANKS_STARTING_ADDRESS 0x4000

#define ROM_BANK_SIZE 16
#define RAM_BANK_SIZE 8

static void init_mbc_one(Memory *memory, u8 *rom_data){
    MBC *mbc = &memory->mbc;
    
    u8 value = rom_data[ROM_SIZE_LOC];
    assert(value != 0);

    u32 rom_size = 32 * (1 << value);
    mbc->one.used_rom_banks = rom_size / ROM_BANK_SIZE; 
    assert(mbc->one.used_rom_banks <= MBC_ONE_ROM_BANKS);

    switch(value){
        case 0x00:{
            assert(mbc->type != MBC_ONE_RAM && mbc->type != MBC_ONE_RAM_BATTERY);
            break;
        }
        case 0x01:{mbc->one.used_ram_banks = 1 ;break;}
        case 0x02:{mbc->one.used_ram_banks = 1 ;break;}
        case 0x03:{mbc->one.used_ram_banks = 4 ;break;}
        case 0x04:{mbc->one.used_ram_banks = 16 ;break;}
        case 0x05:{mbc->one.used_ram_banks = 8 ;break;}

        default:{
            printf("Unsupported value for RAM banks %X\n", value);
            assert(false);
            break;
        }
    }
    assert(mbc->one.used_ram_banks <= MBC_ONE_RAM_BANKS);

    memcpy(memory->data, rom_data, kilobytes(ROM_BANK_SIZE));
    mbc->one.used_rom_banks--;
    for(int i = 0; i < mbc->one.used_rom_banks; i++){
        mbc->one.rom_banks[i] = (u8*)alloc(kilobytes(ROM_BANK_SIZE));
        u8 *bank = mbc->one.rom_banks[i];

        u8 *data = rom_data + (ROM_BANKS_STARTING_ADDRESS * (i + 1));
        memcpy(bank, data, kilobytes(ROM_BANK_SIZE));
    }

    for(int i = 0; i < mbc->one.used_ram_banks; i++){
        mbc->one.ram_banks[i] = (u8*)alloc(kilobytes(RAM_BANK_SIZE));
    }
}

void init_memory(Memory *memory, const char *rom_path){
    memset(memory->data, 0, array_size(memory->data));

    u32 rom_size;
    u8 *rom_data = load_binary_file(rom_path, &rom_size);
    assert(rom_data);
    u8 cartridge_type = rom_data[CARTRIDGE_TYPE_LOC];
    memory->mbc.type = (MBCType)cartridge_type;

    switch(memory->mbc.type){
        case MBC_NONE:{
            memcpy(memory->data, rom_data, rom_size);
            break;
        }

        case MBC_ONE:
        case MBC_ONE_RAM:
        case MBC_ONE_RAM_BATTERY:{
            init_mbc_one(memory, rom_data);

            break;
        }

        default:
            printf("Unknown mapper type\n");
    }   

    memory->mbc.ROM_bank_number = 0x01;
    memory->is_vram_locked = false;
    memory->is_oam_locked = false;
    free(rom_data);
}

void set_MBC_registers(Memory *memory, u16 address, u8 value){
    switch(memory->mbc.type){
        case MBC_ONE:
        case MBC_ONE_RAM:
        case MBC_ONE_RAM_BATTERY:{
            if(address >= 0x0000 && address <= 0x1FFF){
                if(value & 0x0A)
                    memory->mbc.RAM_enable = true;
                else
                    memory->mbc.RAM_enable = false;
            }
            else if(address >= 0x2000 && address <= 0x3FFF){
                if(value == 0x00) 
                    memory->mbc.ROM_bank_number = 0x01;
                else
                    memory->mbc.ROM_bank_number = value & 0x1F;
            }
            else if(address >= 0x4000 && address <= 0x5FFF){
                memory->mbc.RAM_bank_number = value & 0x03;
            }
            else if(address >= 0x6000 && address <= 0x7FFF){
                memory->mbc.one.mode = value & 0x01;
            }

            break;
        }

        default:
            printf("Mapper type does not support banking\n");
    }
}

u8 read_from_MBC(Memory *memory, u16 address){
    switch(memory->mbc.type){
        case MBC_ONE:
        case MBC_ONE_RAM:
        case MBC_ONE_RAM_BATTERY:{
            MBC *mbc = &memory->mbc;
            if(address >= 0x0000 && address <= 0x3FFF){
                return memory->data[address];
            }
            else if(address >= 0x4000 && address <= 0x7FFF){
                return mbc->one.rom_banks[mbc->ROM_bank_number - 1][address - 0x4000];
            }
            else if(address >= 0xA000 && address <= 0xBFFF){
                if(mbc->RAM_enable){
                    return mbc->one.ram_banks[mbc->RAM_bank_number][address - 0xA000];
                }
                else{
                    return memory->data[address];
                }
            }

            break;
        }

        default:
            printf("Mapper type does not support banking\n");
    }
}

void write_to_mbc_RAM(Memory *memory, u16 address, u8 value){
    switch(memory->mbc.type){
        case MBC_ONE:
        case MBC_ONE_RAM:
        case MBC_ONE_RAM_BATTERY:{
            MBC *mbc = &memory->mbc;
            if(mbc->RAM_enable){
                mbc->one.ram_banks[mbc->RAM_bank_number][address - 0xA000] = value;
            }

            break;
        }

        default:
            printf("Mapper type does not support banking\n");
    }
}

u8 read_ram_from_MBC(Memory *memory, u16 address){
    switch(memory->mbc.type){
        case MBC_ONE:
        case MBC_ONE_RAM:
        case MBC_ONE_RAM_BATTERY:{
            MBC *mbc = &memory->mbc;
            if(mbc->RAM_enable){
                return mbc->one.ram_banks[mbc->RAM_bank_number][address - 0xA000];
            }
            else{
                return 0xFF;
            }

            break;
        }

        default:
            printf("Mapper type does not support banking\n");
    }
}