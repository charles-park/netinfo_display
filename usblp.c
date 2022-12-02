//------------------------------------------------------------------------------
//
// 2022.12.01 USB Label printer control. (chalres-park)
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

#include "typedefs.h"
#include "usblp.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEXT_WIDTH	80

const int8_t USBLP_EPL_FORM[][TEXT_WIDTH] = {
	"I8,0,001\n",
	"Q78,16\n",
	"q240\n",
	"rN\n",
	"S4\n",
	"D15\n",
	"ZB\n",
	"JF\n",
	"O\n",
	"R304,10\n",
	"f100\n",
	"N\n",
	"A10,0,0,2,1,1,N,\"EPL Printer Test\"\n",
	"A16,32,0,2,1,1,N,\"00:1E:06:xx:xx:xx\"\n",
	"P1\n"
};

const int8_t USBLP_ZPL_INIT[TEXT_WIDTH] = {
	"~JC^XA^JUF^XZ\n"
};

const int8_t USBLP_ZPL_FORM[][TEXT_WIDTH] = {
	"^XA\n",
	"^CFC\n",
	"^LH0,0\n",
	"^FO310,25^FDZPL Printer Test^FS\n",
	"^FO316,55^FD00:1E:06:xx:xx:xx^FS\n",
	"^XZ\n"
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// check the usb label printer connection.
//------------------------------------------------------------------------------
static int32_t check_usblp_connection (void)
{
	FILE *fp;
	int8_t cmd_line[1024];

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "%s", "lsusb | grep Zebra 2<&1");

	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
			if (strstr (cmd_line, "Zebra") != NULL) {
				pclose (fp);
				return 1;
			}
			memset (cmd_line, 0x00, sizeof(cmd_line));
		}
		pclose(fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
// get the usb label printer info.
//------------------------------------------------------------------------------
static int32_t get_usblp_device (int8_t *lpname)
{
	FILE *fp;
	int8_t cmd_line[1024], *ptr;

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "%s", "lpinfo -v | grep usb 2<&1");

	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
			if ((ptr = strstr (cmd_line, "usb:")) != NULL) {
				strncpy (lpname, ptr, strlen(ptr));
				pclose (fp);
				return 1;
			}
			memset (cmd_line, 0x00, sizeof(cmd_line));
		}
		pclose(fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
// set the usb label printer info.
//------------------------------------------------------------------------------
static int32_t set_usblp_device (int8_t *lpname)
{
	FILE *fp;
	int8_t cmd_line[1024], s_lpname[512];

	memset (s_lpname, 0x00, sizeof(s_lpname));
	{
		int32_t i, pos;
		for (i = 0, pos = 0; i < strlen(lpname); i++, pos++) {
			if ((lpname[i] == '(') || (lpname[i] == ')'))
				s_lpname [pos++] = '\\';
			s_lpname [pos] = lpname[i];
		}
	}

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "lpadmin -p zebra -E -v %s 2<&1", s_lpname);

	if ((fp = popen(cmd_line, "w")) != NULL) {
		pclose(fp);
		return 1;
	}
	return 0;
}

//------------------------------------------------------------------------------
// confirm the usb label printer info.
//------------------------------------------------------------------------------
static int32_t confirm_usblp_device (int8_t *lpname)
{
	FILE *fp;
	int8_t cmd_line[1024], *ptr;

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "%s", "lpstat -v | grep usb 2<&1");

	if ((fp = popen(cmd_line, "r")) != NULL) {
		memset (cmd_line, 0x00, sizeof(cmd_line));
		while (fgets (cmd_line, sizeof(cmd_line), fp) != NULL) {
			if ((ptr = strstr (cmd_line, "usb:")) != NULL) {
				if (!strncmp (lpname, ptr, strlen(ptr)-1)) {
					pclose (fp);
					return 1;
				}
			}
			memset (cmd_line, 0x00, sizeof(cmd_line));
		}
		pclose(fp);
	}
	return 0;
}

//------------------------------------------------------------------------------
// Label printer test.
//------------------------------------------------------------------------------
static void test_usblp_device (int8_t *lpname)
{
	FILE *fp = fopen ("usblp.txt", "w");
	const int8_t *form;
	int8_t cmd_line[1024], *ptr, lines, i;

	if (fp == NULL) {
		fprintf (stdout, "%s : couuld not create file for usblp test. ", __func__);
		return;
	}

	if (strstr (lpname, "EPL") != NULL) {
		lines = sizeof (USBLP_EPL_FORM) / sizeof (USBLP_EPL_FORM[0]);
		form = USBLP_EPL_FORM[0];
	} else {
		lines = sizeof (USBLP_ZPL_FORM) / sizeof (USBLP_ZPL_FORM[0]);
		form = USBLP_ZPL_FORM[0];
	}

	for (i = 0; i < lines; i++, form += TEXT_WIDTH)
		fputs (form, fp);
	fclose(fp);

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "%s", "lpr usblp.txt -P zebra 2<&1");

	if ((fp = popen(cmd_line, "w")) != NULL) {
		pclose(fp);
	}
}

//------------------------------------------------------------------------------
static void init_zpl_device (void)
{
	FILE *fp = fopen ("zpl_factory.txt", "w");
	const int8_t *form;
	int8_t cmd_line[1024], *ptr, lines, i;

	if (fp == NULL) {
		fprintf (stdout, "%s : couuld not create file for usblp test. ", __func__);
		return;
	}
	fputs (USBLP_ZPL_INIT, fp);
	fclose(fp);

	memset (cmd_line, 0x00, sizeof(cmd_line));
	sprintf (cmd_line, "%s", "lpr zpl_factory.txt -P zebra 2<&1");

	if ((fp = popen(cmd_line, "w")) != NULL) {
		pclose(fp);
	}
}

//------------------------------------------------------------------------------
int32_t usblp_reconfig (void)
{
	static int8_t zpl_init = 0;
	int8_t usblp_device[512];

	memset (usblp_device, 0x00, sizeof(usblp_device));
	if (!check_usblp_connection ()) {
		fprintf (stdout, "Error : Zebra USB Label Printer not found\n");
		return 0;
	}

	if (!get_usblp_device (usblp_device)) {
		fprintf (stdout, "Error : Unable to get usblp infomation.\n");
		return 0;
	}
	
	if (!confirm_usblp_device (usblp_device)) {
		fprintf (stdout, "Error : The usblp information is different.\n");
		zpl_init = 0;
		if (!set_usblp_device (usblp_device)) {
			fprintf (stdout, "Error : Failed to configure usblp.\n");
			return 0;
		}
		if (!confirm_usblp_device (usblp_device)) {
			fprintf (stdout, "Error : The usblp settings have not been changed.\n");
			return 0;
		}
	}
	if (!zpl_init && strstr (usblp_device, "ZPL") != NULL) {
		// factory setup
		init_zpl_device ();
		zpl_init = 1;
	}
	fprintf (stdout, "*** USB Label Printer setup is complete. ***\n");
	fprintf (stdout, "*** Printer Device Name : %s\n", usblp_device);
	test_usblp_device (usblp_device);
	return 1;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
