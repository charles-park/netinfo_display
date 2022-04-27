//------------------------------------------------------------------------------
//
// 2022.04.22 I2C-LCD Control Lib. (chalres-park)
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "i2c-lcd.h"
#include "i2c-ctl.h"
#include "typedefs.h"
//------------------------------------------------------------------------------
/* ----------------------------------------------------------------------- *
 * PCF8574T backpack module uses 4-bit mode, LCD pins D0-D3 are not used.  *
 * backpack module wiring:                                                 *
 *                                                                         *
 *  PCF8574T     LCD                                                       *
 *  ========     ======                                                    *
 *     P0        RS                                                        *
 *     P1        RW                                                        *
 *     P2        Enable                                                    *
 *     P3        Led Backlight                                             *
 *     P4        D4                                                        *
 *     P5        D5                                                        *
 *     P6        D6                                                        *
 *     P7        D7                                                        *
 *                                                                         *
 * I2C-byte: D7 D6 D5 D4 BL EN RW RS                                       *
 * ----------------------------------------------------------------------- */
/*
	DDRAM Address (16x4 Line)
	|00|01|02|03|04|05|06|07|08|09|0A|0C|0D|0E|0F| --+
	|40|41|42|43|44|45|46|47|48|49|4A|4C|4D|4E|4F| --|--+
	|10|11|12|13|14|15|16|17|18|19|1A|1C|1D|1E|1F| <-+  |
	|50|51|52|53|54|55|56|57|58|59|5A|5C|5D|5E|5F| <----+

	DDRAM Address (20x4 Line)
	|00|01|02|03|04|05|06|07|08|09|0A|0B|0C|0D|0E|0F|10|11|12|13| --+
	|40|41|42|43|44|45|46|47|48|49|4A|4B|4C|4D|4E|4F|50|51|52|53| --|--+
	|14|15|16|17|18|19|1A|1B|1C|1D|1E|1F|20|21|22|23|24|25|26|27| <-+  |
	|54|55|56|57|58|59|5A|5B|5C|5D|5E|5F|60|61|62|63|64|65|66|67| <----+
*/
//------------------------------------------------------------------------------
static 	int		i2c_send        (int fd, bool d_type, bool bl,
									byte_t *sdata, int size, int udelay);
static int 		i2c_write 		(int fd, bool d_type, bool bl,
									byte_t *sdata, int size, int udelay);
static 	int		lcd_goto_xy		(int fd, int x, int y);
		int		lcd_printf      (int fd, int x, int y, char *fmt, ...);
		int  	lcd_clear       (int fd, int line);
		int  	lcd_backlight   (int fd, bool onoff);
		int  	lcd_disp_control(int fd, bool bl, bool disp, bool cursor, bool blink);
		void 	lcd_close       (int fd);
		int  	lcd_init        (int fd, int lcd_width, int lcd_height, bool lcd_bl);
		int  	lcd_open 		(char *dev, byte_t id);

//------------------------------------------------------------------------------
static int 	LCDWidth 	= DEFAULT_LCD_WIDTH;
static int 	LCDHeight 	= DEFAULT_LCD_HEIGHT;
static bool	LCDBL 		= DEFAULT_LCD_BL;

//------------------------------------------------------------------------------
// i2c file write
//------------------------------------------------------------------------------
static int i2c_write (int fd, bool d_type, bool bl,
							byte_t *sdata, int size, int udelay)
{
	i2clcd_u ldata;
	int s_cnt, i;
	bool iflag = (size == 0) ? true : false;

	// startup command parsing
	if (iflag)	size = 1;

	ldata.bits.bl = bl;	ldata.bits.rs = d_type;	ldata.bits.rw = 0;
	for (i = 0, s_cnt = 0; i < size; i++) {
		ldata.bits.dat = (sdata[i] >> 4) & 0x0F;		
		ldata.bits.e	= 1;
		s_cnt += write (fd, &ldata.byte, 1);
		ldata.bits.e	= 0;
		s_cnt += write (fd, &ldata.byte, 1);

		// lcd startup command.
		if (iflag)	break;

		ldata.bits.dat = (sdata[i]     ) & 0x0F;		
		ldata.bits.e	= 1;
		s_cnt += write (fd, &ldata.byte, 1);
		ldata.bits.e	= 0;
		s_cnt += write (fd, &ldata.byte, 1);
	}
	udelay ? usleep(udelay) : usleep(DEFAULT_I2C_DELAY);

	if (iflag)	return ((s_cnt / 2) == size) ? true : false;
	else		return ((s_cnt / 4) == size) ? true : false;
}

//------------------------------------------------------------------------------
// i2c smbus write
//------------------------------------------------------------------------------
static int i2c_send (int fd, bool d_type, bool bl,
							byte_t *sdata, int size, int udelay)
{
	i2clcd_u ldata;
	int s_cnt, i, ret;
	bool iflag = (size == 0) ? true : false;
	byte_t sbuf[64];

	memset(sbuf, 0, sizeof(sbuf));
	// startup command parsing
	if (iflag)	size = 1;

	ldata.bits.bl = bl;	ldata.bits.rs = d_type;	ldata.bits.rw = 0;
	for (i = 0, s_cnt = 0; i < size; i++) {
		ldata.bits.dat = (sdata[i] >> 4) & 0x0F;
		ldata.bits.e   = 1;	sbuf[s_cnt++]  = ldata.byte;
		ldata.bits.e   = 0;	sbuf[s_cnt++]  = ldata.byte;
		// lcd startup command.
		if (iflag)	break;

		ldata.bits.dat = (sdata[i]     ) & 0x0F;
		ldata.bits.e   = 1;	sbuf[s_cnt++] = ldata.byte;
		ldata.bits.e   = 0;	sbuf[s_cnt++] = ldata.byte;
	}
	if (s_cnt > 32) {
		ret  = i2c_smbus_write_block_data(fd, 0,         32, &sbuf[0]);
		ret += i2c_smbus_write_block_data(fd, 0, s_cnt - 32, &sbuf[32]);
	}
	else
		ret = i2c_smbus_write_block_data(fd, 0, s_cnt -  0, &sbuf[0]);

	usleep(udelay);		usleep(DEFAULT_I2C_DELAY);

	return	ret ? false : true;
}

//------------------------------------------------------------------------------
static int lcd_goto_xy (int fd, int x, int y)
{
	byte_t d;

	switch (y) {
		default :
		case	0:	d = 0x80;				break;
		case	1:	d = 0xc0;				break;
		case	2:	d = 0x80 + LCDWidth;	break;
		case	3:	d = 0xC0 + LCDWidth;	break;
	}
	d += x > LCDWidth ? LCDWidth : x;

	return i2c_send (fd, LCD_CMD, LCDBL, &d, 1, 0);
}

//------------------------------------------------------------------------------
int lcd_printf (int fd, int x, int y, char *fmt, ...)
{
    char buf[LCDWidth +1], len;
    va_list va;

    memset(buf, 0x00, sizeof(buf));

    va_start(va, fmt);
    len = vsprintf(buf, fmt, va);
    va_end(va);

	lcd_goto_xy (fd, x, y);

	return i2c_send (fd, LCD_DAT, LCDBL,
		&buf[0], len > LCDWidth ? LCDWidth : len, 0);
}

//------------------------------------------------------------------------------
int lcd_clear (int fd, int line)
{
	byte_t d = 0x01;
    char buf[LCDWidth +1];

	/* clear all */
	if (line < 0)
		return i2c_send (fd, LCD_CMD, LCDBL, &d, 1, 2000);

	memset (buf, 0x20, LCDWidth);
	lcd_goto_xy (fd, 0, line > LCDHeight ? (LCDHeight - 1) : line);
	return i2c_send (fd, LCD_DAT, LCDBL, buf, LCDWidth, 0);
}

//------------------------------------------------------------------------------
int lcd_backlight (int fd, bool onoff)
{
	byte_t d = 0x00;

	LCDBL = onoff;
	return i2c_send (fd, LCD_CMD, LCDBL, &d, 1, 0);
}

//------------------------------------------------------------------------------
int lcd_disp_control (int fd, bool bl, bool disp, bool cursor, bool blink)
{
	byte_t d = 0x08 | (disp << 2) | (cursor << 1) | blink;

	LCDBL = bl;
	return i2c_send (fd, LCD_CMD, LCDBL, &d, 1, 0);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int lcd_init (int fd, int lcd_width, int lcd_height, bool lcd_bl)
{
	byte_t d, ret = 0;

	LCDWidth = lcd_width;	LCDHeight = lcd_height;		LCDBL = lcd_bl;

	// wait 15msec, Funcset (lcd startup init.)
	d = 0x30;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 0, 15000);

	// wait 4.1msec, Funcset (lcd startup init.)
	d = 0x30;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 0, 4100);

	// wait 100usec, Funcset (lcd startup init.)
	d = 0x30;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 0, 100);

	// wait 4.1msec, Funcset (lcd startup init. change funcset)
	d = 0x20;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 0, 4100);

	/* -------------------------------------------------------------------- *
	 * 4-bit mode initialization complete. Now configuring the function set *
	 * -------------------------------------------------------------------- */
	// Function set : D5 = 1, D3(N) = 1 (2 lune), D2(F) = 0 (5x8 font), 40usec
	d = 0x28;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 1, 100);

	/* -------------------------------------------------------------------- *
	 * Next turn display off                                                *
	 * -------------------------------------------------------------------- */
	// Display Control : D3=1, display_on = 0, cursor_on = 0, cursor_blink = 0
	d = 0x08;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 1, 100);

	/* -------------------------------------------------------------------- *
	 * Display clear, cursor home                                           *
	 * -------------------------------------------------------------------- */
	d = 0x01;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 1, 100);

	/* -------------------------------------------------------------------- *
	 * Set cursor direction                                                 *
	 * -------------------------------------------------------------------- */
	// Entry Mode : D2 = 1, I/D = 1, S = 0
	d = 0x06;
	ret += i2c_send(fd, LCD_CMD, LCD_BL_OFF, &d, 1, 100);

	/* -------------------------------------------------------------------- *
	 * Turn on the display                                                  *
	 * -------------------------------------------------------------------- */
	// Display Control : D3=1, display_on = 1, cursor_on = 0, cursor_blink = 0
	d = 0x0c;
	ret += i2c_send(fd, LCD_CMD, LCDBL, &d, 1, 100);

	/* -------------------------------------------------------------------- *
	 * LCD Initialize done                                                  *
	 * -------------------------------------------------------------------- */
	return ret < 9 ? false : true;
}

//------------------------------------------------------------------------------
void lcd_close(int fd)
{
	if (fd) {
		lcd_backlight (fd, false);
		close(fd);
	}
}

//------------------------------------------------------------------------------
int lcd_open (char *dev, byte_t id)
{
	int fd;
	// i2c open, chip address find & setup
	// return i2c - fd
	if((fd = open(dev, O_RDWR)) < 0) {
		err ("Error failed to open I2C bus [%s].\n", dev);
		return false;
	}
	// set the I2C slave address for all subsequent I2C device transfers
	if (ioctl(fd, I2C_SLAVE, id) < 0) {
		err("Error failed to set I2C address [0x%02x].\n", id);
		return false;
	}
	return fd ? fd : false;
}

//------------------------------------------------------------------------------
void lcd_test (void)
{
	int fd = lcd_open ("/dev/i2c-1", 0x3f);

	lcd_printf(fd, 0,0, "This is sample!"); 
	{
		uint16_t i;
		while (true) {
			if(!lcd_printf(fd, 0,1, "count = %d", i++))
				err ("i2c lcd error!\n");
			sleep(0.5);
			if (!i)
				lcd_clear(fd, 1);
		}
	}

}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
