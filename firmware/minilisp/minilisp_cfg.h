#include <stdbool.h>

/// Minilisp configuration file
#pragma once

// The size of the heap in byte
#define MEMORY_SIZE (128UL * 1024UL)

#define GC_RUNNING true
#define GC_DEBUG true
#define GC_ALWAYS false