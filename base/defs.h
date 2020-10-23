// Common definitions.
#pragma once

// ARRAY_COUNT evaluates to the compile-time size of the array, which must be an
// array and not a pointer to an element.
#define ARRAY_COUNT(x) (sizeof(x) / sizeof(*(x)))
