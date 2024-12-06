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

void inc_r8(){
	const char *test_name = "INC r8";
	bool result = true;
	{	// INC B
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x04};
		memcpy(cpu->memory, mem, 1);

		cpu->BC = 0xAABB;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->B == 0xAB);
		check_result(&result, cpu->BC == 0xABBB);
		check_result(&result, !cpu->flags);
		check_result(&result, cpu->PC == 0x02);
	}

	{	// INC E
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x1C};
		memcpy(cpu->memory, mem, 1);

		cpu->DE = 0x10FF;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->E == 0x00);
		check_result(&result, cpu->DE == 0x1000);
		check_result(&result, (cpu->flags) & (FLAG_HALFCARRY|FLAG_ZERO));
		check_result(&result, cpu->PC == 0x02);
	}

	{	// INC L
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x2C};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0x10FF;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->L == 0x00);
		check_result(&result, cpu->HL == 0x1000);
		check_result(&result, (cpu->flags) & (FLAG_HALFCARRY|FLAG_ZERO));
		check_result(&result, cpu->PC == 0x02);
	}

	{	// INC A
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x3C};
		memcpy(cpu->memory, mem, 1);

		cpu->A = 0xFF;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A == 0x00);
		check_result(&result, (cpu->flags) & (FLAG_HALFCARRY|FLAG_ZERO));
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void inc_memhl(){
	const char *test_name = "INC [HL]";
	bool result = true;
	{	// INC [HL]
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x34};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0xAABB;
		cpu->memory[cpu->HL] = 0xFF;
		u8 previous_mem = cpu->memory[cpu->HL];
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[cpu->HL] == u8(previous_mem + 1));
		check_result(&result, (cpu->flags) & (FLAG_HALFCARRY|FLAG_ZERO));
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void dec_r8(){
	const char *test_name = "DEC r8";
	bool result = true;
	{	// DEC B
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x05};
		memcpy(cpu->memory, mem, 1);

		cpu->BC = 0xA5BB;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->B == 0xA4);
		check_result(&result, cpu->BC == 0xA4BB);
		check_result(&result, (cpu->flags) & (FLAG_SUB));
		check_result(&result, cpu->PC == 0x02);
	}

	{	// DEC E
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x1D};
		memcpy(cpu->memory, mem, 1);

		cpu->DE = 0x1000;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->E == 0xFF);
		check_result(&result, cpu->DE == 0x10FF);
		check_result(&result, (cpu->flags) & (FLAG_HALFCARRY|FLAG_SUB));
		check_result(&result, cpu->PC == 0x02);
	}

	{	// DEC L
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x2D};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0x10FF;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->L == 0xFE);
		check_result(&result, cpu->HL == 0x10FE);
		check_result(&result, (cpu->flags) & (FLAG_SUB));
		check_result(&result, cpu->PC == 0x02);
	}

	{	// DEC A
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x3D};
		memcpy(cpu->memory, mem, 1);

		cpu->A = 0x01;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A == 0x00);
		check_result(&result, (cpu->flags) & (FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void dec_memhl(){
	const char *test_name = "DEC [HL]";
	bool result = true;
	{	// INC [HL]
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x35};
		memcpy(cpu->memory, mem, 1);

		cpu->HL = 0xAABB;
		cpu->memory[cpu->HL] = 0x00;
		u8 previous_mem = cpu->memory[cpu->HL];
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[cpu->HL] == u8(previous_mem - 1));
		check_result(&result, (cpu->flags) & (FLAG_HALFCARRY|FLAG_SUB));
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void ld_r_imm8(){
	const char *test_name = "LD r, imm8";
	bool result = true;
	{   // LD B, imm8
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x06, 0xBB};
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->B == 0xBB);
		check_result(&result, cpu->PC == 0x04);
	}

	{   // LD H, imm8
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x26, 0xDD};
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->H == 0xDD);
		check_result(&result, cpu->PC == 0x04);
	}

	{   // LD L, imm8
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x2E, 0x25};
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->L == 0x25);
		check_result(&result, cpu->PC == 0x04);
	}

	{   // LD A, imm8
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x3E, 0x5A};
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->A  == 0x5A);
		check_result(&result, cpu->PC == 0x04);
	}

	show_test_result(test_name, result);
}

void ld_memhl_imm8(){
	const char *test_name = "LD [HL], imm8";
	bool result = true;
	{   // LD [HL], imm8
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x36, 0x68};
		cpu->HL = 0xB2A3;
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 5){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[cpu->HL]  == 0x68);
		check_result(&result, cpu->PC == 0x05);
	}

	show_test_result(test_name, result);
}

void rlca(){
	const char *test_name = "RLCA";
	bool result = true;
	{   // RLCA.  With carry.
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x07};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x80;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x01);
		check_result(&result, cpu->PC == 0x02);
	}

	{   // RLCA.  With no carry.
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x07};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x40;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x80);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void rrca(){
	const char *test_name = "RRCA";
	bool result = true;
	{   // RRCA.  With carry.
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x0F};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x01;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x80);
		check_result(&result, cpu->PC == 0x02);
	}

	{   // RRCA.  With no carry.
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x0F};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x02;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x01);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void rla(){
	const char *test_name = "RLA";
	bool result = true;
	{   // RLA.  Previously set carry. 
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x17};
		memcpy(cpu->memory, mem, 1);
		
		cpu->A = 0x40;
		cpu->flags |= FLAG_CARRY;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x81);
		check_result(&result, cpu->PC == 0x02);
	}

	{   // RLA.  With no previously set carry.
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x17};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x80;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void rra(){
	const char *test_name = "RRA";
	bool result = true;
	{   // RRA.  Previously set carry. 
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x1F};
		memcpy(cpu->memory, mem, 1);
		
		cpu->A = 0x02;
		cpu->flags |= FLAG_CARRY;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x81);
		check_result(&result, cpu->PC == 0x02);
	}

	{   // RRA.  With no previously set carry.
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x1F};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x01;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_CARRY));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void daa(){
	const char *test_name = "DAA";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x27};
		memcpy(cpu->memory, mem, 1);
		
		cpu->A = sum_and_set_flags(cpu, 0x38, 0x45, true, true);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->A  == 0x83);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x27};
		memcpy(cpu->memory, mem, 1);
		
		cpu->A = sum_and_set_flags(cpu, 0x38, 0x41, true, true);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->A  == 0x79);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x27};
		memcpy(cpu->memory, mem, 1);
		
		cpu->A = sum_and_set_flags(cpu, 0x24, 0x36, true, true);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->A  == 0x60);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void cpl(){
	const char *test_name = "CPL";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x2F};
		memcpy(cpu->memory, mem, 1);
		
		cpu->A = 0xAA;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->A  == 0x55);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void scf(){
	const char *test_name = "SCF";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x37};
		memcpy(cpu->memory, mem, 1);
		
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_CARRY));
		check_result(&result, !(cpu->flags & FLAG_SUB));
		check_result(&result, !(cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void ccf(){
	const char *test_name = "CCF";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[1] = {0x3F};
		memcpy(cpu->memory, mem, 1);
		set_flag(cpu, FLAG_CARRY);
		u8 previous_carry = cpu->flags & FLAG_CARRY;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_CARRY) == !(previous_carry));
		check_result(&result, !(cpu->flags & FLAG_SUB));
		check_result(&result, !(cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void jr_imm8(){
	const char *test_name = "JR e8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x18, 0x0A};
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->PC == 0x0B);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[2] = {0x18, 0xFF};
		memcpy(cpu->memory, mem, 2);
		for(int i = 0; i < 3; i++){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->PC == 0x00);
	}

	show_test_result(test_name, result);
}

void ld_r8_r8(){
	const char *test_name = "LD r8, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x43};
		cpu->E = 0x45;
		cpu->B = 0x00;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->B  == 0x45);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x6F};
		cpu->A = 0xBB;
		cpu->L = 0x00;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->L  == 0xBB);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x79};
		cpu->A = 0x00;
		cpu->C = 0x69;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->A  == 0x69);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x74};
		cpu->HL = 0x2000;
		cpu->memory[cpu->HL] = 0x00;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->memory[cpu->HL] == 0x20);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x7E};
		cpu->HL = 0x2000;
		cpu->memory[cpu->HL] = 0x25;
		cpu->A = 0x00;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->A == 0x25);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void add_r8(){
	const char *test_name = "ADD A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x80};
		cpu->A = 0x0F;
		cpu->B = 0x03;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x12);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x87};
		cpu->A = 0xFF;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY));
		check_result(&result, cpu->A  == 0xFE);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x86};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xFF;
		cpu->HL = 0xAAAA;
		cpu->memory[cpu->HL] = 0x01;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void adc_r8(){
	const char *test_name = "ADC A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x88};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x0F;
		cpu->B = 0x03;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x13);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x8F};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xFF;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x8E};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xFF;
		cpu->HL = 0xAAAA;
		cpu->memory[cpu->HL] = 0x01;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_ZERO));
		check_result(&result, cpu->A  == 0x01);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void sub_r8(){
	const char *test_name = "SUB A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x90};
		cpu->A = 0x00;
		cpu->B = 0x01;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_SUB));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x95};
		cpu->A = 0x01;
		cpu->L = 0x01;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x96};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xFF;
		cpu->HL = 0xAAAA;
		cpu->memory[cpu->HL] = 0x01;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_SUB));
		check_result(&result, cpu->A  == 0xFE);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void sbc_r8(){
	const char *test_name = "SBC A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x98};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x01;
		cpu->B = 0x01;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_SUB));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x9D};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x02;
		cpu->L = 0x01;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0x9E};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xFF;
		cpu->HL = 0xAAAA;
		cpu->memory[cpu->HL] = 0x01;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_SUB));
		check_result(&result, cpu->A  == 0xFD);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void and_r8(){
	const char *test_name = "AND A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xA1};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xAA;
		cpu->C = 0x22;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x22);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xA7};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x0A;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x0A);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xA6};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xAA;
		cpu->HL = 0xABAB;
		cpu->memory[cpu->HL] = 0x55;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void xor_r8(){
	const char *test_name = "XOR A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xAA};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xAA;
		cpu->D = 0x55;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xAF};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x0A;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags | FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xAE};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x00;
		cpu->HL = 0xABAB;
		cpu->memory[cpu->HL] = 0x00;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void or_r8(){
	const char *test_name = "OR A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xB3};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x0A;
		cpu->E = 0x55;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags));
		check_result(&result, cpu->A  == 0x5F);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xB7};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x0A;
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags));
		check_result(&result, cpu->A  == 0x0A);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xAE};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0x00;
		cpu->HL = 0xABAB;
		cpu->memory[cpu->HL] = 0x00;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void cp_r8(){
	const char *test_name = "CP A, r8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xB8};
		cpu->A = 0x00;
		cpu->B = 0x01;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_SUB));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xBD};
		cpu->A = 0x01;
		cpu->L = 0x01;
		memcpy(cpu->memory, mem, 1);
		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->A  == 0x01);
		check_result(&result, cpu->PC == 0x02);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xBE};
		memcpy(cpu->memory, mem, 1);
		cpu->A = 0xFF;
		cpu->HL = 0xAAAA;
		cpu->memory[cpu->HL] = 0x01;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_SUB));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void add_imm8(){
	const char *test_name = "ADD A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xC6, 0x03};
		cpu->A = 0x0F;
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x12);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xC6, 0x02};
		cpu->A = 0xFF;
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY));
		check_result(&result, cpu->A  == 0x01);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void adc_imm8(){
	const char *test_name = "ADC A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xCE, 0x03};
		memcpy(cpu->memory, mem, 2);
		cpu->A = 0x0F;
		cpu->B = 0x03;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x13);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xCE, 0xFF};
		memcpy(cpu->memory, mem, 2);
		cpu->A = 0xFF;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void sub_imm8(){
	const char *test_name = "SUB A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xD6, 0x01};
		memcpy(cpu->memory, mem, 2);
		cpu->A = 0x00;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_SUB));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xD6, 0x01};
		cpu->A = 0x01;
		memcpy(cpu->memory, mem, 2);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}


	show_test_result(test_name, result);
}

void sbc_imm8(){
	const char *test_name = "SBC A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xDE, 0x01};
		memcpy(cpu->memory, mem, 2);
		cpu->A = 0x01;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_SUB));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xDE, 0x01};
		memcpy(cpu->memory, mem, 2);
		cpu->A = 0x02;
		set_flag(cpu, FLAG_CARRY);
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void and_imm8(){
	const char *test_name = "AND A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xE6, 0x22};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0xAA;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY));
		check_result(&result, cpu->A  == 0x22);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xE6, 0x55};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0xAA;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void xor_imm8(){
	const char *test_name = "XOR A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xEE, 0x55};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0xAA;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xEE, 0x00};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0x00;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void or_imm8(){
	const char *test_name = "OR A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xF6, 0x55};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0x0A;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, !(cpu->flags));
		check_result(&result, cpu->A  == 0x5F);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xF6, 0x00};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0x00;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void cp_imm8(){
	const char *test_name = "CP A, imm8";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xFE, 0x01};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0x00;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_HALFCARRY|FLAG_CARRY|FLAG_SUB));
		check_result(&result, cpu->A  == 0x00);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xFE, 0x01};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0x01;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_ZERO|FLAG_SUB));
		check_result(&result, cpu->A  == 0x01);
		check_result(&result, cpu->PC == 0x03);
	}

	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xFE, 0x01};
		memcpy(cpu->memory, mem, array_size(mem));
		cpu->A = 0xFF;
		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		
		check_result(&result, (cpu->flags & FLAG_SUB));
		check_result(&result, cpu->A  == 0xFF);
		check_result(&result, cpu->PC == 0x03);
	}

	show_test_result(test_name, result);
}

void ret_cc(){
	const char *test_name = "RET cc";
	bool result = true;
	{   // RET NZ
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xC0};
		memcpy(cpu->memory, mem, array_size(mem));
		unset_flag(cpu, FLAG_ZERO);

		cpu->SP = 0xFFFF;
		push_stack(cpu, 0x20);
		push_stack(cpu, 0x50);

		while(cpu->PC != 0x2051){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->PC == 0x2051);
	}

	{   // RET Z
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xC8};
		memcpy(cpu->memory, mem, array_size(mem));
		unset_flag(cpu, FLAG_ZERO);

		cpu->SP = 0xFFFF;
		push_stack(cpu, 0x20);
		push_stack(cpu, 0x50);

		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->PC == 0x02);
	}

	{   // RET NC
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xD0};
		memcpy(cpu->memory, mem, array_size(mem));
		set_flag(cpu, FLAG_CARRY);

		cpu->SP = 0xFFFF;
		push_stack(cpu, 0x20);
		push_stack(cpu, 0x50);

		while(cpu->PC < 0x02){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->PC == 0x02);
	}

	{   // RET C
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xD8};
		memcpy(cpu->memory, mem, array_size(mem));
		set_flag(cpu, FLAG_CARRY);

		cpu->SP = 0xFFFF;
		push_stack(cpu, 0x20);
		push_stack(cpu, 0x50);

		while(cpu->PC != 0x2051){
			run_cpu(cpu);
		}
		
		check_result(&result, cpu->PC == 0x2051);
	}

	show_test_result(test_name, result);
}

void pop(){
	const char *test_name = "POP r16";
	bool result = true;
	{   // POP BC
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xC1};
		memcpy(cpu->memory, mem, array_size(mem));

		cpu->SP = 0xFFFF;
		push_stack(cpu, 0x20);
		push_stack(cpu, 0x50);

		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->BC == 0x2050);
		check_result(&result, cpu->PC == 0x04);
	}

	{   // POP AF
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xF1};
		memcpy(cpu->memory, mem, array_size(mem));

		cpu->SP = 0xFFFF;
		push_stack(cpu, 0x33);
		push_stack(cpu, 0xFF);

		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, cpu->AF == 0x33FF);
		check_result(&result, cpu->PC == 0x04);
	}

	show_test_result(test_name, result);
}

void push(){
	const char *test_name = "PUSH r16";
	bool result = true;
	{   // PUSH DE
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xD5};
		memcpy(cpu->memory, mem, array_size(mem));

		cpu->SP = 0xFFFF;
		cpu->DE = 0x20AA;

		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, pop_stack(cpu) == 0xAA);
		check_result(&result, pop_stack(cpu) == 0x20);
		check_result(&result, cpu->PC == 0x04);
	}

	{   // PUSH HL
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xE5};
		memcpy(cpu->memory, mem, array_size(mem));

		cpu->SP = 0xFFFF;
		cpu->HL = 0x4AB5;

		while(cpu->PC < 4){
			run_cpu(cpu);
		}
		check_result(&result, pop_stack(cpu) == 0xB5);
		check_result(&result, pop_stack(cpu) == 0x4A);
		check_result(&result, cpu->PC == 0x04);
	}

	show_test_result(test_name, result);
}

void ldh_c_a(){
	const char *test_name = "LDH [C], A";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xE2};
		memcpy(cpu->memory, mem, array_size(mem));

		cpu->A = 0x6;
		cpu->C = 0x80;

		while(cpu->PC < 2){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[0xFF00 + cpu->C] == 0x06);
		check_result(&result, cpu->PC == 0x02);
	}

	show_test_result(test_name, result);
}

void ldh_imm8_a(){
	const char *test_name = "LDH [imm8], A";
	bool result = true;
	{   
		Gameboy gmb = {};
		init_gameboy(&gmb);

		CPU *cpu = &gmb.cpu;
		u8 mem[] = {0xE0, 0x50};
		memcpy(cpu->memory, mem, array_size(mem));

		cpu->A = 0xAA;

		while(cpu->PC < 3){
			run_cpu(cpu);
		}
		check_result(&result, cpu->memory[0xFF00 + 0x50] == 0xAA);
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
	inc_r8();
	inc_memhl();
	dec_r8();
	dec_memhl();
	ld_r_imm8();
	ld_memhl_imm8();
	rlca();
	rrca();
	rla();
	rra();
	daa();
	cpl();
	scf();
	ccf();
	jr_imm8();
	ld_r8_r8();
	add_r8();
	adc_r8();
	sub_r8();
	sbc_r8();
	and_r8();
	xor_r8();
	or_r8();
	cp_r8();
	add_imm8();
	adc_imm8();
	sub_imm8();
	sbc_imm8();
	and_imm8();
	xor_imm8();
	or_imm8();
	cp_imm8();
	ret_cc();
	pop();
	push();
	ldh_c_a();
	ldh_imm8_a();

	return 0;
}