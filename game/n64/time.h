#pragma once

#include <stdint.h>

struct game_time;
struct scheduler;

// Initialize the timing subsystem.
void time_init(void);

// Update the timing system. Returns the time delta.
float time_update(struct game_time *restrict tm, struct scheduler *sc);
