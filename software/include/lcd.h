#ifndef LCD_H
#define LCD_H

#include <LiquidCrystal_I2C.h>

struct lcd {
	unsigned int width, height;

	unsigned int cur_x, cur_y;

	char *cur_scr;    /* row-major */
	char *prev_scr;   /* row-major */

	LiquidCrystal_I2C lcdc;

	lcd(uint8_t addr, unsigned int width, unsigned int height); /* c++ is dumb */
};

int init_lcd(struct lcd *lcd, unsigned int width, unsigned int height);
void printf_lcd(struct lcd *lcd, char *fmt, ...);
void wipe_line(struct lcd *lcd);
void set_cur_lcd(struct lcd *lcd, unsigned int x, unsigned int y);
void flush_lcd(struct lcd *lcd);

#endif

