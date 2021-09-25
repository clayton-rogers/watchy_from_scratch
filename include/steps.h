#pragma once

void first_time_bma_config();
void update_steps(bool is_midnight);

int get_todays_steps();
int get_total_steps();

int get_step_offset();
void set_step_offset(int new_step_offset);
