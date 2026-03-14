#include "lcd.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

lcd::lcd(uint8_t addr, unsigned int width, unsigned int height)
	: lcdc(addr, width, height) { }

int
init_lcd(struct lcd *lcd, unsigned int width, unsigned int height)
{
	lcd->width = width;
	lcd->height = height;
	lcd->cur_x = lcd->cur_y = 0;

	if((lcd->cur_scr = (char *)malloc(width * height)) == NULL) {
		return -1;
	}
	memset(lcd->cur_scr, ' ', width * height);
	if((lcd->prev_scr = (char *)malloc(width * height)) == NULL) {
		free(lcd->cur_scr);
		return -1;
	}
	memset(lcd->prev_scr, ' ', width * height);

	lcd->lcdc.init();
	lcd->lcdc.backlight();

	return 0;
}

void
printf_lcd(struct lcd *lcd, char *fmt, ...)
{
	va_list args, arg_cpy;
	unsigned int pos, remaining;
	unsigned int written;

	va_start(args, fmt);

	pos = lcd->cur_x + lcd->cur_y*lcd->width;
	remaining = lcd->width * lcd->height - pos;

	written = vsnprintf(lcd->cur_scr + pos, remaining, fmt, args);
	lcd->cur_y += written / lcd->width;
	lcd->cur_x += written % lcd->width;

	va_end(args);
}

void
wipe_line(struct lcd *lcd)
{
	unsigned int pos;
	unsigned int line_remaining;

	pos = lcd->cur_x + lcd->cur_y*lcd->width;
	line_remaining = lcd->width - lcd->cur_x;

	memset(lcd->cur_scr + pos, ' ', line_remaining);
}

void
set_cur_lcd(struct lcd *lcd, unsigned int x, unsigned int y)
{
	lcd->cur_x = x;
	lcd->cur_y = y;
	
	if(lcd->cur_x >= lcd->width) lcd->cur_x = lcd->width - 1;
	if(lcd->cur_y >= lcd->height) lcd->cur_y = lcd->height - 1;
}

void
flush_lcd(struct lcd *lcd)
{
#define TO_IDX(X,Y) ((X) + lcd->width*(Y))
	unsigned int i, j;

	for(i = 0; i < lcd->width; ++i) {
		for(j = 0; j < lcd->height; ++j) {
			if(lcd->cur_scr[TO_IDX(i, j)] != lcd->prev_scr[TO_IDX(i, j)]) {
				lcd->prev_scr[TO_IDX(i, j)] = lcd->cur_scr[TO_IDX(i, j)];

				if(lcd->cur_y != j || lcd->cur_x + 1 != i) {
					lcd->lcdc.setCursor(i, j);
				}
				lcd->cur_x = i;
				lcd->cur_y = j;

				lcd->lcdc.write(lcd->cur_scr[TO_IDX(i, j)]);
			}
		}
	}
#undef TO_IDX
}

