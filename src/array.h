#pragma once

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DEFAULT_ARRAY_SIZE 10

template<typename T>
struct Array{
    T *data;
    u32 element_size;
    u32 size;
    u32 capacity;
};

// Creates an array with an initial default capacity, unless a value is specified.
template<typename T> 
Array<T> make_array(u32 size = DEFAULT_ARRAY_SIZE){
	Array<T> array;
	array.capacity = size;
	array.size     = 0;
	array.element_size = sizeof(T);
	array.data = (T*)malloc(array.capacity * sizeof(T));
	memset(array.data, 0, array.capacity * sizeof(T));

    return array;
}




// Adds an element to the back of the array.
template<typename T>
void array_add(Array<T> *array, T data) {
	assert(array->data);
	if(array->size < array->capacity){
		array->data[array->size] = data;
		array->size++;
	}else{
		T *new_array = (T*)malloc(array->capacity * 2 * sizeof(T));
		assert(new_array);
		memcpy(new_array, array->data, array->capacity * sizeof(T));
		array->capacity *= 2;
		free(array->data);
		array->data = new_array;
		
		array->data[array->size] = data;
		array->size++;
	}
    
}

template<typename T>
void array_set(Array<T> *array, u32 index, T data){
	assert(array);
	if(index < array->capacity){
		array->data[index] = data;
	}
	else{
		printf("Array: Trying to write to an index out of bounds");
	}
}

// Get the last element of the array and reduce its size by one.
template<typename T>
T array_pop(Array<T> *array){
	assert(array);
	T value {};
	if(array->size == 0){
		printf("Cannot pop value from empty array, returned default value\n");
		return value;
	}
		 
	array->size--;
	assert(array->size >= 0);
	value = array->data[array->size];
	array->data[array->size + 1] = T {};
	return value;
}

template<typename T>
void array_copy(Array<T> *dst, Array<T> *src){
	assert(dst);
	assert(src);

	if (src->size > dst->capacity){
		T *new_array = (T*)malloc(src->size * sizeof(T));
		assert(new_array);
		memcpy(new_array, dst->data, dst->capacity * sizeof(T));
		dst->capacity = src->size;
		free(dst->data);
		dst->data = new_array;

	}


	dst->size = src->size;
	dst->element_size = src->element_size;
	memcpy(dst->data, src->data, sizeof(T) * src->size);
}

// Gets the element a the indicated index.
template<typename T>
T array_get(Array<T> *array, u32 index){
	assert(array->data);
	if(index < array->size){
		return array->data[index];
	}
	printf("Indexing array out of bounds\n");
	assert(false);
	return T{};
}

template<typename T>
T* array_get_ptr(Array<T> *array, u32 index){
	assert(array->data);
	if(index < array->size){
		return &array->data[index];
	}
	printf("Indexing array out of bounds\n");
	assert(false);
}

// Sets the array size to 0.
template<typename T>
void array_clear(Array<T> *array){
	array->size = 0;
}

// Frees the memory used by the array.
// void delete_array(Array *array);
