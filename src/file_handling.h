#pragma once

#include "common.h"

char* text_file_to_string(const char* file_name);
u8* load_binary_file(const char* file_name, u32 *file_size);
bool file_exists(const char * filename);