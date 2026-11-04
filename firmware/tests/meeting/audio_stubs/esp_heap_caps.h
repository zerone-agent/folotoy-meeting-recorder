#pragma once
#include <stddef.h>
#define MALLOC_CAP_INTERNAL 1
#define MALLOC_CAP_8BIT 2
void *heap_caps_malloc(size_t n,unsigned caps);
void *heap_caps_realloc(void *ptr,size_t n,unsigned caps);
size_t heap_caps_get_free_size(unsigned caps);
size_t heap_caps_get_largest_free_block(unsigned caps);
