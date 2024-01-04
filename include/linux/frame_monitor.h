// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Yusen <1405481963@qq.com>.
 */
#ifndef _DIS_FPS_MON_H_
#define _DIS_FPS_MON_H_

#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/timekeeping.h>
#include <linux/list.h>

#define TIMER_INTERVAL_US 333333

#define FRAME_TIME_144HZ_US 6944
#define FRAME_TIME_120HZ_US 8333
#define FRAME_TIME_90HZ_US 11111
#define FRAME_TIME_60HZ_US 16666

#define COMMIT_START_TS 1
#define COMMIT_END_TS 2
#define INPUT_FENCE_TS 3
#define COMMIT_RENDER_COMPLETE_TS 4

#define FPS_UPDATED 1
#define IS_JANK 2
#define IS_FLUID 3
#define IDLE 4

typedef void (*frame_time_handler)(u16 fps, u32 frame_generation_time,
				   ktime_t cur_time, u8 mode);

struct frame_mon_receiver {
	struct list_head list;
	unsigned int jank_frame_time;
	frame_time_handler callback_handler;
};

typedef struct fps_mon_info {
    bool start;
    u16 frame_count;
    u16 fps;
    int cur_vrefresh;
    ktime_t fps_cnt_start_time;
    ktime_t render_end_ts;
    ktime_t last_commit_end_ts;
    u32 frame_time;
} fps_mon_info_t;

extern fps_mon_info_t fps_info;
extern int last_vrefresh,last_vrefresh_mul_10;
int register_fps_mon_handler(struct frame_mon_receiver *receiver);
int unregister_fps_mon_handler(struct frame_mon_receiver *receiver);
void frame_stat(int event);
void vrefresh_stat(int vrefresh);
#endif /* _DIS_FPS_MON_H_ */
