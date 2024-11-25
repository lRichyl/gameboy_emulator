#include <stdio.h>
#include <string.h>
#include "gameboy.h"

void show_test_result(const char *test_name, bool result){
	if(result)
		printf("%s   PASSED\n", test_name);
	else
		printf("%s   FAILED\n", test_name);
}

void ld_r16_imm16(){
	const char *test_name = "LD r16, imm16";
	bool result = false;
	{
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[3] = {0x01, 0x01, 0xBC};
		memcpy(cpu->memory, mem, 3);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		result = cpu->BC == 0xBC01;
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
		result = cpu->DE == 0xDE02;
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
		result = cpu->HL == 0xAA03;
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
		result = cpu->SP == 0xCC04;
		
		result = cpu->PC == 0x04;
	}
	show_test_result(test_name, result);
}

void ld_memr16_a(){
	const char *test_name = "LD [r16], A";
	bool result = false;
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
		result = cpu->memory[cpu->BC] == 0x01;
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
		result = cpu->memory[cpu->DE] == 0xA0;
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
		result = cpu->memory[previous_HL] == 0xBB;
		result = (cpu->HL == previous_HL + 1);
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
		result = cpu->memory[previous_HL] == 0xDC;
		result = (cpu->HL == previous_HL - 1);
	}

	show_test_result(test_name, result);
}

int main(){
	ld_r16_imm16();
	ld_memr16_a();

	return 0;
}