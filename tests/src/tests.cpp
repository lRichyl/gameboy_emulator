#include <stdio.h>
#include <string.h>
#include "gameboy.h"

void show_test_result(const char *test_name, bool result){
	if(result)
		printf("%s\tPASSED\n", test_name);
	else
		printf("%s\tFAILED\n", test_name);
}

void check_result(bool *result, bool expression){
	if(!expression && *result) *result = expression;
}

void ld_r16_imm16(){
	const char *test_name = "LD r16, imm16";
	bool result = true;
	{
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[3] = {0x01, 0x01, 0xBC};
		memcpy(cpu->memory, mem, 3);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->BC == 0xBC01);
	}
	{
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[3] = {0x11, 0x02, 0xDE};
		memcpy(cpu->memory, mem, 3);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->DE == 0xDE02);
	}
	{
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[3] = {0x21, 0x03, 0xAA};
		memcpy(cpu->memory, mem, 3);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->HL == 0xAA03);
	}
	{
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[3] = {0x31, 0x04, 0xCC};
		memcpy(cpu->memory, mem, 3);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->SP == 0xCC04);
		
		check_result(&result, cpu->PC == 0x04);
	}
	show_test_result(test_name, result);
}

void ld_memr16_a(){
	const char *test_name = "LD [r16], A";
	bool result = true;
	{	// LD [BC], A test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x02};
		memcpy(cpu->memory, mem, 1);

		cpu->A = 0x01;
		cpu->BC = 0x07FF;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[cpu->BC] == 0x01);
		check_result(&result, cpu->PC == 0x02);
	}

	{	// LD [DE], A test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x12};
		memcpy(cpu->memory, mem, 1);

		cpu->A = 0xA0;
		cpu->DE = 0xFFFF;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[cpu->DE] == 0xA0);
		check_result(&result, cpu->PC == 0x02);
	}

	{ // LD [HL+], A test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x22};
		memcpy(cpu->memory, mem, 1);

		cpu->A = 0xBB;
		cpu->HL = 0x0AAA;
		u16 previous_HL = cpu->HL;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[previous_HL] == 0xBB);
		check_result(&result, cpu->HL == previous_HL + 1);
		check_result(&result, cpu->PC == 0x02);
	}

	{  // LD [HL-], A test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x32};
		memcpy(cpu->memory, mem, 1);

		cpu->A = 0xDC;
		cpu->HL = 0x0AAA;
		u16 previous_HL = cpu->HL;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[previous_HL] == 0xDC);
		check_result(&result, cpu->HL == previous_HL - 1);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}



void ld_a_memr16(){
	const char *test_name = "LD A, [r16]";
	bool result = true;
	{	// LD A, [BC] test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x0A};
		memcpy(cpu->memory, mem, 1);

		cpu->BC = 0x07FF;
		cpu->memory[cpu->BC] = 0x01;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A == 0x01);
		check_result(&result, cpu->PC == 0x02);
	}

	{	// LD A, [DE] test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x1A};
		memcpy(cpu->memory, mem, 1);

		cpu->DE = 0xFFFF;
		cpu->memory[cpu->DE] = 0xA0;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A == 0xA0);
		check_result(&result, cpu->PC == 0x02);
	}

	{ // LD A, [HL+] test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x2A};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0x0AAA;
		cpu->memory[cpu->HL] = 0xBB;
		u16 previous_HL = cpu->HL;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A == 0xBB);
		check_result(&result, cpu->HL == previous_HL + 1);
		check_result(&result, cpu->PC == 0x02);
	}

	{  // LD A, [HL-] test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x3A};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0x0AAA;
		cpu->memory[cpu->HL] = 0xDC;
		u16 previous_HL = cpu->HL;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A == 0xDC);
		check_result(&result, cpu->HL == previous_HL - 1);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void ld_a16_sp(){
	const char *test_name = "LD a16, SP";
	bool result = true;
	{	// LD A, [BC] test
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[3] = {0x08, 0x7F, 0xFF};
		memcpy(cpu->memory, mem, 3);

		cpu->SP = 0x1020;
		while(cpu->PC < 6){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[0xFF7F] == 0x20);
		check_result(&result, cpu->memory[0xFF80] == 0x10);
		check_result(&result, cpu->PC == 0x06);
	}
	show_test_result(test_name, result);
}

void inc_r16(){
	const char *test_name = "INC r16";
	bool result = true;
	{	// INC r16
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x13};
		memcpy(cpu->memory, mem, 1);

		cpu->DE = 0x05;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->DE == 0x06);
		check_result(&result, cpu->PC == 0x02);
	}

	{	// INC SP
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x33};
		memcpy(cpu->memory, mem, 1);

		cpu->SP = 0x0321;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}

		check_result(&result, cpu->SP == 0x0322);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void dec_r16(){
	const char *test_name = "DEC r16";
	bool result = true;
	{	// DEC r16
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x1B};
		memcpy(cpu->memory, mem, 1);

		cpu->DE = 0x05;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->DE == 0x04);
		check_result(&result, cpu->PC == 0x02);
	}

	{	// DEC SP
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x3B};
		memcpy(cpu->memory, mem, 1);

		cpu->SP = 0x0321;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}

		check_result(&result, cpu->SP == 0x0320);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void add_hl_r16(){
	const char *test_name = "ADD HL, r16";
	bool result = true;
	{	// ADD HL, BC
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x09};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0xA2;
		cpu->BC = 0x13;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		check_result(&result, cpu->HL == 0xB5);
		check_result(&result, !cpu->flags);
		check_result(&result, cpu->PC == 0x03);
	}

	{	// ADD HL, SP
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x39};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0xFFFF;
		cpu->SP = 0x2030;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		check_result(&result, cpu->HL == 0x202F);
		check_result(&result, (cpu->flags) & (FLAG_CARRY|FLAG_HALFCARRY));
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

int main(){
	ld_r16_imm16();
	ld_memr16_a();
	ld_a_memr16();
	ld_a16_sp();
	inc_r16();
	dec_r16();
	add_hl_r16();

	return 0;
}