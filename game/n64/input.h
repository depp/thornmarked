#pragma once

struct sys_input;

// Initialize the input system.
void input_init(struct sys_input *restrict inp);

// Read the controller, if ready.
void input_update(struct sys_input *restrict inp);
