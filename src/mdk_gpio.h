// Copyright (c) 2022 Cesanta
// All rights reserved

void gpio_output_enable(int pin, bool enable);
void gpio_input(int pin);
void gpio_output(int pin);
void gpio_write(int pin, bool value);
void gpio_toggle(int pin);
bool gpio_read(int pin);
