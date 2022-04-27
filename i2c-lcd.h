//------------------------------------------------------------------------------
//
// 2022.04.22 I2C-LCD Control Lib. (chalres-park)
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#ifndef __I2C_LCD_H__
#define __I2C_LCD_H__

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
typedef struct i2clcd__t {
	unsigned char	rs	: 1;
	unsigned char	rw	: 1;
	unsigned char	e	: 1;
	unsigned char	bl	: 1;
	unsigned char	dat	: 4;
}	i2clcd_t;

typedef union i2clcd__u {
	i2clcd_t	bits;
	byte_t		byte;
}	i2clcd_u;

//------------------------------------------------------------------------------
#define	DEFAULT_LCD_WIDTH	16
#define	DEFAULT_LCD_HEIGHT	2
#define DEFAULT_LCD_BL      1

#define	DEFAULT_I2C_DELAY	200	// usleep(100)
#define	LCD_CMD				0
#define	LCD_DAT				1
#define	LCD_BL_OFF			0
#define	LCD_BL_ON			1

//------------------------------------------------------------------------------
extern int  lcd_printf          (int fd, int x, int y, char *fmt, ...);
extern int  lcd_clear           (int fd, int line);
extern int  lcd_backlight       (int fd, bool onoff);
extern int  lcd_disp_control    (int fd, bool bl, bool disp, bool cursor, bool blink);
extern void lcd_close           (int fd);
extern int  lcd_init            (int fd, int lcd_width, int lcd_height, bool lcd_bl);
extern int  lcd_open 		    (char *dev, byte_t id);

//------------------------------------------------------------------------------

#endif  //  #define __I2C_LCD_H__
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
