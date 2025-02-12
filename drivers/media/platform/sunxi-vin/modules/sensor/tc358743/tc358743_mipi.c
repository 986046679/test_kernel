/*
 * A V4L2 driver for imx386 Raw cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors: Zheng Wanyu <zhengwanyu@allwinnertrch>
 * Authors: Li Huiyu <lihuiyu@allwinnertrch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-dv-timings.h>
#include <uapi/linux/v4l2-dv-timings.h>
#include <linux/hdmi.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/kernel.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "tc358743_reg.h"

#define MCLK              (27 * 1000 * 1000) /*tc358743 not need mclk*/
#define V4L2_IDENT_SENSOR 0x0000

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The imx386 i2c address
 */
#define I2C_ADDR 0x1e

#define SENSOR_NUM 0x2
#define SENSOR_NAME "tc358743_mipi"
#define SENSOR_NAME_2 "tc358743_mipi_2"

#define EDID_NUM_BLOCKS_MAX 8
#define EDID_BLOCK_SIZE 128

static unsigned int tc_debug = 1;

#define TC_INF(...) \
	do { \
		if (tc_debug) { \
			printk("[TC358743]"__VA_ARGS__); \
		} \
	} while (0);

#define TC_INF2(...) \
	do { \
		if (tc_debug >= 2) { \
			printk("[TC358743]"__VA_ARGS__); \
		} \
	} while (0);

#define TC_ERR(...) \
	do { \
		printk("[TC358743 ERROR]"__VA_ARGS__); \
	} while (0);

enum tc_ddc5v_delays {
	DDC5V_DELAY_0_MS,
	DDC5V_DELAY_50_MS,
	DDC5V_DELAY_100_MS,
	DDC5V_DELAY_200_MS,
};

enum tc_hdmi_detection_delay {
	HDMI_MODE_DELAY_0_MS,
	HDMI_MODE_DELAY_25_MS,
	HDMI_MODE_DELAY_50_MS,
	HDMI_MODE_DELAY_100_MS,
};

struct tc_audio_param {
	bool present; //if there is audio data received
	unsigned int fs; //sampling frequency
};

struct hdmi_rx_tc358743 {
	struct sensor_info info;
	struct i2c_client *client;
	struct v4l2_ctrl_handler hdl;
	struct task_struct   *tc_task;

	unsigned char status;
	unsigned char wait4get_timing; //wait user space to get timing

	struct v4l2_ctrl *tmds_signal_ctrl;
	bool tmds_signal;
	bool signal_event_subscribe;

	struct delayed_work source_change_work;
	struct v4l2_dv_timings timing;
	struct tc_audio_param audio;
	bool ddc_5v_present;

	unsigned int refclk_hz;
	enum tc_ddc5v_delays ddc5v_delays;
	unsigned int hdcp_enable;

	unsigned char colorbar_enable;
	unsigned char edid[1024];
};

#define V4L2_EVENT_SRC_CH_HPD_IN             (1 << 1)
#define V4L2_EVENT_SRC_CH_HPD_OUT            (1 << 2)

struct hdmi_rx_tc358743 tc358743[SENSOR_NUM];

 const struct v4l2_dv_timings_cap tc358743_timings_cap = {
	.type = V4L2_DV_BT_656_1120,
	.reserved = { 0 },

	//min_width,max_width,min_height,max_height,min_pixelclock,max_pixelclock,
	//standards,capabilities
	V4L2_INIT_BT_TIMINGS(640, 1920, 350, 1200, 13000000, 165000000,
		V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
		V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT,
		V4L2_DV_BT_CAP_PROGRESSIVE |
		V4L2_DV_BT_CAP_REDUCED_BLANKING |
		V4L2_DV_BT_CAP_CUSTOM)
};

extern bool v4l2_match_dv_timings(const struct v4l2_dv_timings *t1,
			const struct v4l2_dv_timings *t2,
			unsigned pclock_delta, bool match_reduced_fps);
extern void v4l2_print_dv_timings(const char *dev_prefix, const char *prefix,
			const struct v4l2_dv_timings *t, bool detailed);

extern int v4l2_enum_dv_timings_cap(struct v4l2_enum_dv_timings *t,
				const struct v4l2_dv_timings_cap *cap,
				v4l2_check_dv_timings_fnc fnc,
				void *fnc_handle);

static int tc_get_detected_timings(struct v4l2_subdev *sd,
			struct v4l2_dv_timings *timings);

static void tc_msleep(unsigned int ms)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(ms));
}

void set_edid(struct v4l2_subdev *sd,
			unsigned char *edid_data,
			unsigned int blocks)
{
	int i = 0;

	tc_write8(sd, EDID_MODE, 0x02);
	tc_write8(sd, EDID_LEN1, 0x00);
	tc_write8(sd, EDID_LEN2, 0x01);

	for (i = 0; i < 0x100; i++) {
		tc_write8(sd, 0x8C00 + i, edid_data[i]);
	}
}

void colorbar_1024x768(struct v4l2_subdev *sd)
{
	/*REM Debug Control
	REM refclk=27MHz, mipi 594Mbps 4 lane
	REM color bar for 1024x768 YUV422*/
	tc_write16(sd, 0x7080, 0x0000);
	//REM Software Reset
	tc_write16(sd, 0x0002, 0x0F00);
	tc_write16(sd, 0x0002, 0x0000);
	//REM Data Format
	tc_write16(sd, 0x0004, 0x0084);
	tc_write16(sd, 0x0010, 0x001E);
	//REM PLL Setting
	tc_write16(sd, 0x0020, 0x3057);
	tc_write16(sd, 0x0022, 0x0203);
	msleep(1);
	tc_write16(sd, 0x0022, 0x0213);
	//REM CSI Lane Enable
	tc_write32(sd, 0x0140, 0x00000000);
	tc_write32(sd, 0x0144, 0x00000000);
	tc_write32(sd, 0x0148, 0x00000000);
	tc_write32(sd, 0x014C, 0x00000000);
	tc_write32(sd, 0x0150, 0x00000000);
	//REM CSI Transition Timing
	tc_write32(sd, 0x0210, 0x00000FA0);
	tc_write32(sd, 0x0214, 0x00000005);
	tc_write32(sd, 0x0218, 0x00001505);
	tc_write32(sd, 0x021C, 0x00000001);
	tc_write32(sd, 0x0220, 0x00000305);
	tc_write32(sd, 0x0224, 0x00003A98);
	tc_write32(sd, 0x0228, 0x00000008);
	tc_write32(sd, 0x022C, 0x00000002);
	tc_write32(sd, 0x0230, 0x00000005);
	tc_write32(sd, 0x0234, 0x0000001F);
	tc_write32(sd, 0x0238, 0x00000000);
	tc_write32(sd, 0x0204, 0x00000001);
	tc_write32(sd, 0x0518, 0x00000001);
	tc_write32(sd, 0x0500, 0xA3008086);
	//REM Color Bar Setting
	tc_write16(sd, 0x000A, 0x0800);
	tc_write16(sd, 0x7080, 0x0082);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF7F);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xFF00);
	tc_write16(sd, 0x7000, 0xFFFF);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0xC0FF);
	tc_write16(sd, 0x7000, 0xC000);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7F00);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x7FFF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x00FF);
	tc_write16(sd, 0x7000, 0x0000);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	tc_write16(sd, 0x7000, 0x007F);
	//REM Color Bar Setting
	tc_write16(sd, 0x7090, 0x02FF);
	tc_write16(sd, 0x7092, 0x060B);
	tc_write16(sd, 0x7094, 0x0013);
	tc_write16(sd, 0x7080, 0x0083);

	tc_write8(sd, 0x8544, 0x11); // HPD_CTL
}

void set_capture1(struct v4l2_subdev *sd,
			unsigned char *edid,
			unsigned int blocks)
{
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	// Software Reset
	tc_write16(sd, 0x0002, 0x0F00); // SysCtl
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	//REM Data Format
	tc_write16(sd, 0x0004, 0x0000);
	tc_write16(sd, 0x0010, 0x0000);

	// FIFO Delay Setting
	tc_write16(sd, 0x0006, 0x0154); // FIFO Ctl
	// PLL Setting
	tc_write16(sd, 0x0020, 0x305F); // PLLCtl0
	tc_write16(sd, 0x0022, 0x0203); // PLLCtl1
	msleep(1);
	tc_write16(sd, 0x0022, 0x0213); // PLLCtl1
	// Interrupt Control
	tc_write16(sd, 0x0014, 0x0000); // IntStatus
	tc_write16(sd, 0x0016, 0x05FF); // IntMask
	// CSI Lane Enable
	tc_write32(sd, 0x0140, 0x00000000); // CLW_CNTRL
	tc_write32(sd, 0x0144, 0x00000000); // D0W_CNTRL
	tc_write32(sd, 0x0148, 0x00000000); // D1W_CNTRL
	tc_write32(sd, 0x014C, 0x00000000); // D2W_CNTRL
	tc_write32(sd, 0x0150, 0x00000000); // D3W_CNTRL
	// CSI Transition Timing
	tc_write32(sd, 0x0210, 0x00000FA0); // LINEINITCNT
	tc_write32(sd, 0x0214, 0x00000004); // LPTXTIMECNT
	tc_write32(sd, 0x0218, 0x00001503); // TCLK_HEADERCNT
	tc_write32(sd, 0x021C, 0x00000001); // TCLK_TRAILCNT
	tc_write32(sd, 0x0220, 0x00000103); // THS_HEADERCNT
	tc_write32(sd, 0x0224, 0x00003A98); // TWAKEUP
	tc_write32(sd, 0x0228, 0x00000008); // TCLK_POSTCNT
	tc_write32(sd, 0x022C, 0x00000002); // THS_TRAILCNT
	tc_write32(sd, 0x0230, 0x00000005); // HSTXVREGCNT
	tc_write32(sd, 0x0234, 0x0000001F); // HSTXVREGEN
	tc_write32(sd, 0x0238, 0x00000000); // TXOPTIONACNTRL
	tc_write32(sd, 0x0204, 0x00000001); // STARTCNTRL
	tc_write32(sd, 0x0518, 0x00000001); // CSI_START
	tc_write32(sd, 0x0500, 0xA3008086); // CSI_CONFW
	// HDMI Interrupt Mask
	tc_write8(sd, 0x8502, 0x01); // SYS_INT
	tc_write8(sd, 0x8512, 0xFE); // SYS_INTM
	tc_write8(sd, 0x8514, 0x00); // PACKET_INTM
	tc_write8(sd, 0x8515, 0x00); // AUDIO_IMNTM
	tc_write8(sd, 0x8516, 0x00); // ABUF_INTM
	// HDMI Audio REFCLK
	tc_write8(sd, 0x8531, 0x01); // PHY_CTL0
	tc_write8(sd, 0x8540, 0x8C); // SYS_FREQ0
	tc_write8(sd, 0x8541, 0x0A); // SYS_FREQ1
	tc_write8(sd, 0x8630, 0xB0); // LOCKDET_REF0
	tc_write8(sd, 0x8631, 0x1E); // LOCKDET_REF1
	tc_write8(sd, 0x8632, 0x04); // LOCKDET_REF2
	tc_write8(sd, 0x8670, 0x01); // NCO_F0_MOD
	// NCO_48F0A
	// NCO_48F0B
	// NCO_48F0C
	// NCO_48F0D
	// NCO_44F0A
	// NCO_44F0B
	// NCO_44F0C
	// NCO_44F0D
	// HDMI PHY
	tc_write8(sd, 0x8532, 0x80); // PHY CTL1
	tc_write8(sd, 0x8536, 0x40); // PHY_BIAS
	tc_write8(sd, 0x853F, 0x0A); // PHY_CSQ
	// HDMI SYSTEM
	tc_write8(sd, 0x8543, 0x32); // DDC_CTL
	tc_write8(sd, 0x8544, 0x10); // HPD_CTL
	tc_write8(sd, 0x8545, 0x31); // ANA_CTL
	tc_write8(sd, 0x8546, 0x2D); // AVM_CTL

	tc_write8_mask(sd, HPD_CTL, MASK_HPD_OUT0, 0);
	if (edid && blocks) {
		set_edid(sd, edid, blocks);
		memcpy(tc->edid, edid, 0x80 * blocks);
	} else {
		set_edid(sd, tc->edid, 2);
	}

	// HDCP Setting
	tc_write8(sd, 0x85D1, 0x01); //
	tc_write8(sd, 0x8560, 0x24); // HDCP_MODE
	tc_write8(sd, 0x8563, 0x11); //
	tc_write8(sd, 0x8564, 0x0F); //
	// Video Setting
	tc_write8(sd, 0x8573, 0x81); // VOUT_SET2
	// HDMI Audio Setting
	tc_write8(sd, 0x8600, 0x00); // AUD_Auto_Mute
	tc_write8(sd, 0x8602, 0xF3); // Auto_CMD0
	tc_write8(sd, 0x8603, 0x02); // Auto_CMD1
	tc_write8(sd, 0x8604, 0x0C); // Auto_CMD2
	tc_write8(sd, 0x8606, 0x05); // BUFINIT_START
	tc_write8(sd, 0x8607, 0x00); // FS_MUTE
	tc_write8(sd, 0x8620, 0x22); // FS_IMODE
	tc_write8(sd, 0x8640, 0x01); // ACR_MODE
	tc_write8(sd, 0x8641, 0x65); // ACR_MDF0
	tc_write8(sd, 0x8642, 0x07); // ACR_MDF1
	tc_write8(sd, 0x8652, 0x02); // SDO_MODE1
	tc_write8(sd, 0x8665, 0x10); // DIV_MODE
	// Info Frame Extraction
	tc_write8(sd, 0x8709, 0xFF); // PK_INT_MODE
	tc_write8(sd, 0x870B, 0x2C); // NO_PKT_LIMIT
	tc_write8(sd, 0x870C, 0x53); // NO_PKT_CLR
	tc_write8(sd, 0x870D, 0x01); // ERR_PK_LIMIT
	tc_write8(sd, 0x870E, 0x30); // NO_PKT_LIMIT2
	tc_write8(sd, 0x9007, 0x10); // NO_GDB_LIMIT
	tc_write8(sd, 0x854A, 0x01); // INIT_END
	tc_write16(sd, 0x0004, 0x0CD7); // ConfCtl
	tc_write8_mask(sd, HPD_CTL, MASK_HPD_OUT0, 1);
}

void set_capture2(struct v4l2_subdev *sd,
			unsigned char *edid,
			unsigned int blocks)
{
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	// Initialization for Stand-by (RS1)
	// Software Reset
	tc_write16(sd, 0x0002, 0x0F00); // SysCtl
	tc_write16(sd, 0x0002, 0x0000); // SysCtl

	//REM Data Format
	tc_write16(sd, 0x0004, 0x0000);
	tc_write16(sd, 0x0010, 0x0000);

	// PLL Setting
	tc_write16(sd, 0x0020, 0x305F); // PLLCtl0
	tc_write16(sd, 0x0022, 0x0203); // PLLCtl1
	udelay(10);
	tc_write16(sd, 0x0022, 0x0213); // PLLCtl1
	// HDMI Interrupt Control
	tc_write16(sd, 0x0016, 0x073F); // TOP_INTM
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	tc_write8(sd, 0x8512, 0xFE); // SYS_INTM
	tc_write8(sd, 0x851B, 0x1D); // MISC_INTM
	// HDMI PHY
	tc_write8(sd, 0x8532, 0x80); // PHY CTL1
	tc_write8(sd, 0x8536, 0x40); // PHY_BIAS
	tc_write8(sd, 0x853F, 0x0A); // PHY_CSQ
	tc_write8(sd, 0x8537, 0x02); // PHY_EQ
	// HDMI SYSTEM
	tc_write8(sd, 0x8543, 0x32); // DDC_CTL
	tc_write8(sd, 0x8544, 0x10); // HPD_CTL
	tc_write8(sd, 0x8545, 0x31); // ANA_CTL
	tc_write8(sd, 0x8546, 0x2D); // AVM_CTL
	// HDCP Setting
	tc_write8(sd, 0x85D1, 0x01); //
	tc_write8(sd, 0x8560, 0x24); // HDCP_MODE
	tc_write8(sd, 0x8563, 0x11); //
	tc_write8(sd, 0x8564, 0x0F); //
	// HDMI Audio REFCLK
	tc_write8(sd, 0x8531, 0x01); // PHY_CTL0
	tc_write8(sd, 0x8532, 0x80); // PHY_CTL1
	tc_write8(sd, 0x8540, 0x8C); // SYS_FREQ0
	tc_write8(sd, 0x8541, 0x0A); // SYS_FREQ1
	tc_write8(sd, 0x8630, 0xB0); // LOCKDET_REF0
	tc_write8(sd, 0x8631, 0x1E); // LOCKDET_REF1
	tc_write8(sd, 0x8632, 0x04); // LOCKDET_REF2
	tc_write8(sd, 0x8670, 0x01); // NCO_F0_MOD
	// HDMI Audio Setting
	tc_write8(sd, 0x8600, 0x00); // AUD_Auto_Mute
	tc_write8(sd, 0x8602, 0xF3); // Auto_CMD0
	tc_write8(sd, 0x8603, 0x02); // Auto_CMD1
	tc_write8(sd, 0x8604, 0x0C); // Auto_CMD2
	tc_write8(sd, 0x8606, 0x05); // BUFINIT_START
	tc_write8(sd, 0x8607, 0x00); // FS_MUTE
	tc_write8(sd, 0x8620, 0x22); // FS_IMODE
	tc_write8(sd, 0x8640, 0x01); // ACR_MODE
	tc_write8(sd, 0x8641, 0x65); // ACR_MDF0
	tc_write8(sd, 0x8642, 0x07); // ACR_MDF1
	tc_write8(sd, 0x8652, 0x02); // SDO_MODE1
	tc_write8(sd, 0x85AA, 0x50); // FH_MIN0
	tc_write8(sd, 0x85AF, 0xC6); // HV_RST
	tc_write8(sd, 0x85AB, 0x00); // FH_MIN1
	tc_write8(sd, 0x8665, 0x10); // DIV_MODE
	// Info Frame Extraction
	tc_write8(sd, 0x8709, 0xFF); // PK_INT_MODE
	tc_write8(sd, 0x870B, 0x2C); // NO_PKT_LIMIT
	tc_write8(sd, 0x870C, 0x53); // NO_PKT_CLR
	tc_write8(sd, 0x870D, 0x01); // ERR_PK_LIMIT
	tc_write8(sd, 0x870E, 0x30); // NO_PKT_LIMIT2
	tc_write8(sd, 0x9007, 0x10); // NO_GDB_LIMIT

	//EDID
	if (edid && blocks) {
		set_edid(sd, edid, blocks);
		memcpy(tc->edid, edid, 0x80 * blocks);
	} else {
		set_edid(sd, tc->edid, 2);
	}

	// Enable Interrupt
	tc_write16(sd, 0x0016, 0x053F); // TOP_INTM
	// Enter Sleep
	tc_write16(sd, 0x0002, 0x0001); // SysCtl
	// Interrupt Service Routine(RS_Int)
	// Exit from Sleep
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	udelay(10);
	// Check Interrupt
	tc_write16(sd, 0x0016, 0x073F); // TOP_INTM
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	// Initialization for Ready (RS2)
	// Enable Interrupt
	tc_write16(sd, 0x0016, 0x053F); // TOP_INTM
	// Set HPDO to "H"
	tc_write8(sd, 0x854A, 0x01); // INIT_END
	// Interrupt Service Routine(RS_Int)
	// Exit from Sleep
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	udelay(10);
	// Check Interrupt
	tc_write16(sd, 0x0016, 0x073F); // TOP_INTM
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	// MIPI Output Enable(RS3)

	// MIPI Output Setting
	// Stop Video and Audio
	tc_write16(sd, 0x0004, 0x0CD4); // ConfCtl
	// Reset CSI-TX Block, Enter Sleep mode
	tc_write16(sd, 0x0002, 0x0200); // SysCtl
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	// PLL Setting in Sleep mode, Int clear
	tc_write16(sd, 0x0002, 0x0001); // SysCtl
	tc_write16(sd, 0x0020, 0x305F); // PLLCtl0
	tc_write16(sd, 0x0022, 0x0203); // PLLCtl1
	udelay(10);
	tc_write16(sd, 0x0022, 0x0213); // PLLCtl1
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	udelay(10);
	// Video Setting
	tc_write8(sd, 0x8573, 0xC1); // VOUT_SET2
	tc_write8(sd, 0x8574, 0x08); // VOUT_SET3
	tc_write8(sd, 0x8576, 0xA0); // VI_REP
	// FIFO Delay Setting
	tc_write16(sd, 0x0006, 0x015E); // FIFO Ctl
	// Special Data ID Setting.
	// CSI Lane Enable
	tc_write32(sd, 0x0140, 0x00000000); // CLW_CNTRL
	tc_write32(sd, 0x0144, 0x00000000); // D0W_CNTRL
	tc_write32(sd, 0x0148, 0x00000000); // D1W_CNTRL
	tc_write32(sd, 0x014C, 0x00000000); // D2W_CNTRL
	tc_write32(sd, 0x0150, 0x00000000); // D3W_CNTRL
	// CSI Transition Timing
	tc_write32(sd, 0x0210, 0x00001004); // LINEINITCNT
	tc_write32(sd, 0x0214, 0x00000004); // LPTXTIMECNT
	tc_write32(sd, 0x0218, 0x00001603); // TCLK_HEADERCNT
	tc_write32(sd, 0x021C, 0x00000001); // TCLK_TRAILCNT
	tc_write32(sd, 0x0220, 0x00000204); // THS_HEADERCNT
	tc_write32(sd, 0x0224, 0x00004268); // TWAKEUP
	tc_write32(sd, 0x0228, 0x00000008); // TCLK_POSTCNT
	tc_write32(sd, 0x022C, 0x00000003); // THS_TRAILCNT
	tc_write32(sd, 0x0230, 0x00000005); // HSTXVREGCNT
	tc_write32(sd, 0x0234, 0x0000001F); // HSTXVREGEN
	tc_write32(sd, 0x0238, 0x00000000); // TXOPTIONACNTRL
	tc_write32(sd, 0x0204, 0x00000001); // STARTCNTRL
	tc_write32(sd, 0x0518, 0x00000001); // CSI_START
	tc_write32(sd, 0x0500, 0xA3008087); // CSI_CONFW
	// Enable Interrupt
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x8503, 0x7F); // CLK_INTS_C
	tc_write8(sd, 0x8504, 0xFF); // PACKET_INTS_C
	tc_write8(sd, 0x8505, 0xFF); // AUDIO_INTS_C
	tc_write8(sd, 0x8506, 0xFF); // ABUF_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	tc_write16(sd, 0x0016, 0x053F); // TOP_INTM
	// Start CSI output
	tc_write16(sd, 0x0004, 0x0CD7); // ConfCtl
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	TC_INF("%s\n", __func__);
	switch (on) {
	case STBY_ON:
		TC_INF("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			TC_ERR("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		TC_INF("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			TC_ERR("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(1000, 1200);
		vin_gpio_set_status(sd, RESET, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	TC_INF("%s\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret, i = 0;
	int rdval = 0;

	ret = tc_reg_read(sd, 0x0000, &rdval, 4, TC_FLIP_EN);
	while ((V4L2_IDENT_SENSOR != rdval)) {
		i++;
		ret = tc_reg_read(sd, 0x0000, &rdval, 4, TC_FLIP_EN);
		if (i > 4) {
			TC_INF("warning:chip_id(%d) is NOT equal to %d\n",
					rdval, V4L2_IDENT_SENSOR);
			break;
		}
	}
	sensor_print("sensor id = 0x%04x\n", rdval);
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	struct sensor_info *info = to_state(sd);

	TC_INF("sensor_init\n");

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 4000;
	info->height = 3000;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	TC_INF("%s cmd:%d\n", __func__, cmd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
				sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			TC_ERR("empty wins!\n");
			ret = -1;
		}
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
		.desc = "mipi",
		.mbus_code = MEDIA_BUS_FMT_VYUY8_2X8,
		.regs = NULL,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	 {
	.width         = ALIGN(1024, 32),
	.height        = ALIGN(768, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },

	{
	.width         = ALIGN(720, 32),
	.height        = ALIGN(480, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	{
	.width         = ALIGN(720, 32),
	.height        = ALIGN(576, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
	{
	.width         = ALIGN(1280, 32),
	.height        = ALIGN(720, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	{
	.width         = ALIGN(1280, 32),
	.height        = ALIGN(720, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
	 {
	.width         = ALIGN(1920, 32),
	.height        = ALIGN(1080, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	{
	.width         = ALIGN(1920, 32),
	.height        = ALIGN(1080, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
	{
	.width         = ALIGN(1920, 32),
	.height        = ALIGN(544, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	 {
	.width         = ALIGN(1920, 32),
	.height        = ALIGN(544, 16),
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	TC_INF("%s\n", __func__);
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int tc_enum_dv_timings(struct v4l2_subdev *sd,
			struct v4l2_enum_dv_timings *timings)
{
	printk("subdev feekback function:%s\n", __func__);
	if (timings->pad != 0)
		return -EINVAL;

	return v4l2_enum_dv_timings_cap(timings,
		&tc358743_timings_cap, NULL, NULL);
}

static int tc_query_dv_timings(struct v4l2_subdev *sd,
       struct v4l2_dv_timings *timings)
{
	int ret;

	printk("subdev feekback function:%s\n", __func__);
	ret = tc_get_detected_timings(sd, timings);
	if (ret)
		return ret;

	if (!v4l2_valid_dv_timings(timings,
			&tc358743_timings_cap, NULL, NULL)) {
		TC_ERR("%s: timings out of range\n", __func__);
		return -ERANGE;
	}


	v4l2_print_dv_timings(sd->name, "tc358743 format detected: ",
				timings, true);

	return 0;
}

static int tc_get_dv_timings(struct v4l2_subdev *sd,
       struct v4l2_dv_timings *timings)
{
	int ret, cnt = 0;
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	//read the timings until to get a valid timing,
	//e.g. until the tc358743 to get the hdmi signal stably
	while (cnt < 15) { //over 3s, then timeout!
		ret = tc_get_detected_timings(sd, timings);
		if (ret)
			return ret;

		if (v4l2_valid_dv_timings(timings,
			&tc358743_timings_cap, NULL, NULL)) {
			tc->wait4get_timing = 0;

			//store the valid timings
			memcpy(&tc->timing, timings, sizeof(*timings));

			v4l2_print_dv_timings(sd->name,
				"tc358743 format detected: ",
				timings, true);
			break;
		}

		msleep(200);
		cnt++;

		TC_INF2("%s: invalid timings\n", __func__);
	}

	return 0;
}

static int tc_dv_timings_cap(struct v4l2_subdev *sd,
		struct v4l2_dv_timings_cap *cap)
{
	printk("subdev feekback function:%s\n", __func__);
	if (cap->pad != 0)
		return -EINVAL;

	*cap = tc358743_timings_cap;

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	//struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	printk("subdev feekback function:%s  enable:%d\n", __func__,  enable);
	TC_INF("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		info->current_wins->width, info->current_wins->height,
		info->current_wins->fps_fixed, info->fmt->mbus_code);

	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

static int tc_set_edid(struct v4l2_subdev *sd,
		struct v4l2_subdev_edid *edid)
{
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	printk("subdev feekback function:%s\n", __func__);
	TC_INF("%s, pad %d, start block %d, blocks %d\n",
	    __func__, edid->pad, edid->start_block, edid->blocks);

	if (tc->colorbar_enable)
		colorbar_1024x768(sd);
	else
		set_capture2(sd, edid->edid, edid->blocks);

	printk("%s finished, colorbar_enable:%d!\n",
			__func__, tc->colorbar_enable);

	return 0;
}

static int tc_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				struct v4l2_event_subscription *sub)
{
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;
	printk("subdev feekback function:%s\n", __func__);

	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		printk("V4L2_EVENT_SOURCE_CHANGE\n");
		tc->signal_event_subscribe = 1;
		return v4l2_src_change_event_subdev_subscribe(sd, fh, sub);
	case V4L2_EVENT_CTRL:
		printk("V4L2_EVENT_CTRL\n");
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	default:
		return -EINVAL;
	}
}

static int tc_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
					struct v4l2_event_subscription *sub)
{
	   struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	   switch (sub->type) {
	   case V4L2_EVENT_SOURCE_CHANGE:
			   tc->signal_event_subscribe = 0;
			   break;
	   case V4L2_EVENT_CTRL:
			   break;
	   default:
			   return -EINVAL;
	   }
	   return v4l2_event_subdev_unsubscribe(sd, fh, sub);
}

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.subscribe_event = tc_subscribe_event,
	.unsubscribe_event = tc_unsubscribe_event,
	.ioctl = sensor_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_dv_timings = tc_get_dv_timings,
	.query_dv_timings = tc_query_dv_timings,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_dv_timings = tc_enum_dv_timings,
	.dv_timings_cap = tc_dv_timings_cap,
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.set_edid = tc_set_edid,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

static ssize_t tc_reg_dump_all_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	unsigned short i = 0;

	n += sprintf(buf + n, "\nGlobal Control Register:\n");
	for (i = 0; i < 0x0100; i += 2) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%04x ", tc_read16(sd, i));
	}

	n += sprintf(buf + n, "\nCSI2 TX PHY/PPI  Register:\n");
	for (i = 0x0100; i < 0x023c; i += 4) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%08x ", tc_read32(sd, i));
	}

	n += sprintf(buf + n, "\nCSI2 TX Control  Register:\n");
	for (i = 0x040c; i < 0x051c; i += 4) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%08x ", tc_read32(sd, i));
	}

	n += sprintf(buf + n, "\nHDMIRX  Register:\n");
	for (i = 0x8500; i < 0x8844; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", tc_read8(sd, i));
	}
	n += sprintf(buf + n, "\n");

	return n;
}


static u32 tc_reg_read_start, tc_reg_read_end;
static ssize_t tc_read_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	u32 reg_index;

	if ((tc_reg_read_start > tc_reg_read_end)
		|| (tc_reg_read_end - tc_reg_read_start > 0x4ff)
		|| (tc_reg_read_end > 0xffff)) {
		n += sprintf(buf + n, "invalid reg addr input:(0x%x, 0x%x)\n",
				tc_reg_read_start, tc_reg_read_end);
		return n;
	}

	for (reg_index = tc_reg_read_start; reg_index <= tc_reg_read_end;) {
		if (reg_index < 0x0100) { //Global Control Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%04x ", tc_read16(sd, reg_index));
			reg_index += 2;
		} else if (reg_index < 0x051c) { //CSI2 TX PHY/PPI  Register and CSI2 TX Control  Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%08x ", tc_read32(sd, reg_index));
			reg_index += 4;
		} else if ((reg_index >= 0x8500) && (reg_index < 0xffff)) { // HDMI RX Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%02x ", tc_read8(sd, reg_index));
			reg_index++;
		}
	}

	n += sprintf(buf + n, "\n");

	return n;
}

static ssize_t tc_read_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	u8 *separator;

	separator = strchr(buf, ',');
	if (separator != NULL) {
		tc_reg_read_start = simple_strtoul(buf, NULL, 0);
		tc_reg_read_end = simple_strtoul(separator + 1, NULL, 0);
	} else {
		TC_ERR("Invalid input!must add a comma as separator\n");
	}

	return count;
}

static u32 tc_reg_write_addr,  tc_reg_write_value;
static ssize_t tc_write_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n,  "echo [addr] [value] > tc_write\n");
	return n;
}

static ssize_t tc_write_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;

	u8 *separator;

	separator = strchr(buf, ' ');
	if (separator != NULL) {
		tc_reg_write_addr = simple_strtoul(buf, NULL, 0);
		tc_reg_write_value = simple_strtoul(separator + 1, NULL, 0);
	} else {
		TC_ERR("Invalid input!must add a space as separator\n");
	}

	if (tc_reg_write_addr < 0x0100)
		tc_write16(sd, tc_reg_write_addr, tc_reg_write_value);
	else if (tc_reg_write_addr >= 0x0100 && tc_reg_write_addr < 0x051c)
		tc_write32(sd, tc_reg_write_addr, tc_reg_write_value);
	else if (tc_reg_write_addr >= 0x051c && tc_reg_write_addr <= 0xffff)
		tc_write8(sd, tc_reg_write_addr, tc_reg_write_value);
	return count;
}

static ssize_t tc_debug_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n, "tc_debug:%d\n", tc_debug);
	return n;
}

static ssize_t tc_debug_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	if (strncmp(buf, "0", 1) == 0)
		tc_debug = 0;
	else
		tc_debug = 1;

	return count;
}

static ssize_t tc_colorbar_enable_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	n += sprintf(buf + n, "colorbar enable:%u\n", tc->colorbar_enable);
	return n;
}

static ssize_t tc_colorbar_enable_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;

	if (strncmp(buf, "0", 1) == 0) {
		colorbar_1024x768(sd);
		tc->colorbar_enable = 1;
	} else {
		tc->colorbar_enable = 0;
		set_capture2(sd, 0, 0);
	}

	return count;
}

static struct device_attribute tc_device_attrs[] = {
	__ATTR(tc_read, S_IWUSR | S_IRUGO, tc_read_show, tc_read_store),
	__ATTR(tc_write, S_IWUSR | S_IRUGO, tc_write_show, tc_write_store),
	__ATTR(tc_debug, S_IWUSR | S_IRUGO, tc_debug_show, tc_debug_store),
	__ATTR(colorbar_enable, S_IWUSR | S_IRUGO, tc_colorbar_enable_show,
				tc_colorbar_enable_store),
	__ATTR(reg_dump_all, S_IRUGO, tc_reg_dump_all_show, NULL),
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv[] = {
	{
		.name = SENSOR_NAME,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_16,
	}, {
		.name = SENSOR_NAME_2,
		.addr_width = CCI_BITS_16,
		.data_width = CCI_BITS_16,
	}
};

static bool tc_signal(struct v4l2_subdev *sd)
{
	return (tc_read8(sd, SYS_STATUS) & MASK_S_TMDS);
}

static int tc_get_detected_timings(struct v4l2_subdev *sd,
			struct v4l2_dv_timings *timings)
{
	struct v4l2_bt_timings *bt = &timings->bt;
	unsigned width, height, frame_width, frame_height, frame_interval, fps;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));

	if (!tc_signal(sd)) {
		TC_INF2("%s: no valid signal\n", __func__);
		return -ENOLINK;
	}

	timings->type = V4L2_DV_BT_656_1120;
	bt->interlaced = tc_read8(sd, VI_STATUS1) & MASK_S_V_INTERLACE ?
	V4L2_DV_INTERLACED : V4L2_DV_PROGRESSIVE;

	width = ((tc_read8(sd, DE_WIDTH_H_HI) & 0x1f) << 8) +
		tc_read8(sd, DE_WIDTH_H_LO);
	height = ((tc_read8(sd, DE_WIDTH_V_HI) & 0x1f) << 8) +
		tc_read8(sd, DE_WIDTH_V_LO);
	frame_width = ((tc_read8(sd, H_SIZE_HI) & 0x1f) << 8) +
		tc_read8(sd, H_SIZE_LO);
	frame_height = (((tc_read8(sd, V_SIZE_HI) & 0x3f) << 8) +
		tc_read8(sd, V_SIZE_LO)) / 2;
	/* frame interval in milliseconds * 10
	Require SYS_FREQ0 and SYS_FREQ1 are precisely set */
	frame_interval = ((tc_read8(sd, FV_CNT_HI) & 0x3) << 8) +
		 tc_read8(sd, FV_CNT_LO);
	fps = (frame_interval > 0) ?
		 DIV_ROUND_CLOSEST(10000, frame_interval) : 0;

	bt->width = width;
	bt->height = height;
	bt->vsync = frame_height - height;
	bt->hsync = frame_width - width;
	bt->pixelclock = frame_width * frame_height * fps;
	if (bt->interlaced == V4L2_DV_INTERLACED) {
		bt->height *= 2;
		bt->il_vsync = bt->vsync + 1;
		bt->pixelclock /= 2;
	}

	return 0;
}

static int tc_run_thread(void *parg)
{
	bool signal = 0;
	struct v4l2_event tc_ev_fmt;
	struct v4l2_subdev *sd = (struct v4l2_subdev *)parg;
	struct hdmi_rx_tc358743 *tc = (struct hdmi_rx_tc358743 *)sd;
	struct v4l2_dv_timings timing;
	unsigned int change = 0;

	while (1) {
		if (kthread_should_stop())
			break;

		change = 0;
		signal = tc_signal(sd);
		if (tc->tmds_signal != signal) {
			/*for anti signal-shake*/
			msleep(100);
			if (signal != tc_signal(sd))
				continue;

			TC_INF("tmds signal:%d\n", signal);
			tc->tmds_signal = signal;

			change |= tc->tmds_signal ?
				V4L2_EVENT_SRC_CH_HPD_IN :
				V4L2_EVENT_SRC_CH_HPD_OUT;
		}

		if (!tc->wait4get_timing) {
			memset(&timing, 0, sizeof(timing));
			tc_get_detected_timings(sd, &timing);
			if (!v4l2_match_dv_timings(&tc->timing, &timing, 0, false)) {
				TC_INF("tc resolution change!\n");

				tc->wait4get_timing = 1;
				change |= V4L2_EVENT_SRC_CH_RESOLUTION;

			}
		}

		//Notify user-space the resolution_change event
		if (change) {
			if (v4l2_ctrl_s_ctrl(tc->tmds_signal_ctrl,
				tc->tmds_signal) < 0) {
				TC_ERR("v4l2_ctrl_s_ctrl for tmds signal failed!\n");
			}

			memset(&tc_ev_fmt, 0, sizeof(tc_ev_fmt));
			tc_ev_fmt.type = V4L2_EVENT_SOURCE_CHANGE;
			tc_ev_fmt.u.src_change.changes = change;
			if (tc->signal_event_subscribe)
				v4l2_subdev_notify_event(sd, &tc_ev_fmt);
		}

		tc_msleep(100);
	}

	return 0;
}

#define TC358743_CID_TMDS_SIGNAL   (V4L2_CID_USER_TC358743_BASE + 3)
static const struct v4l2_ctrl_config tc358743_ctrl_tmds_signal_present = {
	.id = TC358743_CID_TMDS_SIGNAL,
	.name = "tmds signal",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
	.flags = V4L2_CTRL_FLAG_READ_ONLY,
};

static int sensor_dev_id;
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct hdmi_rx_tc358743 *tc;
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	struct cci_driver *tc_cci_drv;
	int i, ret;

	TC_INF("sensor_probe start!\n");

	if (!client) {
		TC_ERR("i2c_client is NULL, do NOT find any i2c master driver for tc358743 sensor!\n");
		return -1;
	}

	for (i = 0; i < SENSOR_NUM; i++) {
		if (!strcmp(cci_drv[i].name, client->name))
			break;
	}

	if (i >= SENSOR_NUM) {
		TC_ERR("NOT find any matched cci driver for client(%s)\n", client->name);
		return -1;
	}
	TC_INF("find the matched cci driver for client(%s)\n", client->name);

	tc = &tc358743[i];
	info = &tc->info;
	sd = &info->sd;
	tc_cci_drv =  &cci_drv[i];

	tc->refclk_hz = 27000000;
	tc->ddc5v_delays = DDC5V_DELAY_100_MS;

	tc->client = client;

	cci_dev_probe_helper(sd, client, &sensor_ops, tc_cci_drv);
	for (i = 0; i < ARRAY_SIZE(tc_device_attrs); i++) {
		ret = device_create_file(&tc_cci_drv->cci_device,
					&tc_device_attrs[i]);
		if (ret) {
			TC_ERR("device_create_file failed, index:%d\n", i);
			continue;
		}
	}

	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;

	ret = v4l2_async_register_subdev(sd);
	if (ret < 0) {
		TC_ERR("v4l2_async_register_subdev failed\n");
	}

	v4l2_ctrl_handler_init(&tc->hdl, 1);
	tc->tmds_signal_ctrl = v4l2_ctrl_new_custom(&tc->hdl,
			&tc358743_ctrl_tmds_signal_present, NULL);
	ret = v4l2_ctrl_handler_setup(&tc->hdl);
	if (ret) {
		TC_ERR("v4l2_ctrl_handler_setup!\n");
		return -1;
	}
	sd->ctrl_handler = &tc->hdl;

	pr_info("v4l2_dev:%p\n", sd->v4l2_dev);

	tc_write16(sd, 0x0002, 0x0F00);
	ret = sensor_detect(sd);
	if (ret) {
		TC_ERR("chip found is not an target chip.\n");
		return ret;
	}

	//Create hdmi thread to poll hpd and hdcp status and handle hdcp and hpd event
	tc->tc_task = kthread_create(tc_run_thread, (void *)sd, "tc358743_proc");
	if (IS_ERR(tc->tc_task)) {
			 TC_INF("Unable to start kernel thread  tc358743_proc\n");
			tc->tc_task = NULL;
			 return -1;
	}
	wake_up_process(tc->tc_task);

	mutex_init(&info->lock);
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	TC_INF("tc358743 chipid:0x%x\n", tc_read16(sd, CHIPID));

	TC_INF("sensor_probe end!\n");
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	int i;
	struct hdmi_rx_tc358743 *tc;

	if (client) {
		for (i = 0; i < SENSOR_NUM; i++) {
			if (!strcmp(cci_drv[i].name, client->name))
				break;
		}

		tc = &tc358743[i];
		sd = (struct v4l2_subdev *)tc;

		kthread_stop(tc->tc_task);
		msleep(10);

		sd = cci_dev_remove_helper(client, &cci_drv[i]);
		v4l2_async_unregister_subdev(sd);
	} else {
		sd = cci_dev_remove_helper(client, &cci_drv[sensor_dev_id++]);
	}

	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

static const struct i2c_device_id sensor_id_2[] = {
	{SENSOR_NAME_2, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);
MODULE_DEVICE_TABLE(i2c, sensor_id_2);

static struct i2c_driver sensor_driver[] = {
	{
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id,
	}, {
		.driver = {
			   .owner = THIS_MODULE,
			   .name = SENSOR_NAME_2,
			   },
		.probe = sensor_probe,
		.remove = sensor_remove,
		.id_table = sensor_id_2,
	},
};
static __init int init_sensor(void)
{
	int i, ret = 0;
	sensor_dev_id = 0;

	TC_INF("init_sensor!\n");
	for (i = 0; i < SENSOR_NUM; i++) {
		ret = cci_dev_init_helper(&sensor_driver[i]);
		if (ret < 0)
			TC_ERR("cci_dev_init_helper failed, sen\n");
	}

	return ret;
}

static __exit void exit_sensor(void)
{
	int i;

	sensor_dev_id = 0;

	for (i = 0; i < SENSOR_NUM; i++)
		cci_dev_exit_helper(&sensor_driver[i]);
}

#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
subsys_initcall_sync(init_sensor);
#else
module_init(init_sensor);
#endif
module_exit(exit_sensor);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("zhengwanyu@allwinnertech.com");
MODULE_AUTHOR("lihuiyu@allwinnertrch");
MODULE_DESCRIPTION("TC358743 HDMI RX module driver");
MODULE_VERSION("1.0");
