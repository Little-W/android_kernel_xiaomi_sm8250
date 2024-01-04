// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Yusen <1405481963@qq.com>.
 */

#include <linux/frame_monitor.h>
#include <linux/uaccess.h>

fps_mon_info_t fps_info = {0};

static LIST_HEAD(receivers);
static DECLARE_RWSEM(receiver_lock);
int last_vrefresh,last_vrefresh_mul_10;
static int LONG_FRAME_TIME_LIMIT_US = 20 * FRAME_TIME_144HZ_US;

void frame_time_calc(void)
{
	struct frame_mon_receiver *receiver;
	ktime_t cur_time = ktime_get();

	fps_info.frame_time = (u32)ktime_us_delta(cur_time, fps_info.last_commit_end_ts);

	if(!(fps_info.frame_time > LONG_FRAME_TIME_LIMIT_US))
	{
		down_read(&receiver_lock);
		list_for_each_entry(receiver, &receivers, list) {
			if (fps_info.frame_time >= receiver->jank_frame_time)
				receiver->callback_handler(fps_info.fps, fps_info.frame_time, cur_time, IS_JANK);
			else
				receiver->callback_handler(fps_info.fps, fps_info.frame_time, cur_time, IS_FLUID);
		}
		up_read(&receiver_lock);
	}
	else
	{
		fps_info.fps = last_vrefresh_mul_10;
		down_read(&receiver_lock);
		list_for_each_entry(receiver, &receivers, list) {
			receiver->callback_handler(fps_info.fps, fps_info.frame_time, cur_time, IDLE);
		}
		up_read(&receiver_lock);
	}
}
void fps_calc(void)
{
	u32 diff_us;
	struct frame_mon_receiver *receiver;
	ktime_t cur_time = ktime_get();

	fps_info.last_commit_end_ts = cur_time;
	if(fps_info.frame_count < 65535)
		fps_info.frame_count ++;

	if(fps_info.start) 
	{
		diff_us = (u32)ktime_us_delta(cur_time, fps_info.fps_cnt_start_time);
		if(diff_us >= TIMER_INTERVAL_US)
		{
			fps_info.fps = mult_frac(fps_info.frame_count, 10000000, diff_us);
			fps_info.frame_count = 0;
			fps_info.start = false;
			down_read(&receiver_lock);
			list_for_each_entry(receiver, &receivers, list) {
				receiver->callback_handler(fps_info.fps, fps_info.frame_time, cur_time, FPS_UPDATED);
			}
			up_read(&receiver_lock);
		}
	}
	else
	{
		fps_info.start = true;
		fps_info.fps_cnt_start_time = cur_time;
	}
	return;
}
void frame_stat(int event)
{
	switch (event)
	{
		case COMMIT_RENDER_COMPLETE_TS:
			frame_time_calc();
			break;
		case COMMIT_END_TS:
			fps_calc();
			break;
	}
}
void vrefresh_stat(int vrefresh)
{
	if(last_vrefresh != vrefresh)
	{
		last_vrefresh = vrefresh;
		last_vrefresh_mul_10 = vrefresh * 10;
		fas_need_update = true;
		switch(vrefresh)
		{
			case 144:
				LONG_FRAME_TIME_LIMIT_US = vrefresh / 5 * FRAME_TIME_144HZ_US;
				break;
			case 120:
				LONG_FRAME_TIME_LIMIT_US = vrefresh / 5 * FRAME_TIME_120HZ_US;
				break;
			case 90:
				LONG_FRAME_TIME_LIMIT_US = vrefresh / 5 * FRAME_TIME_90HZ_US;
				break;
			case 60:
				LONG_FRAME_TIME_LIMIT_US = vrefresh / 5 * FRAME_TIME_60HZ_US;
				break;
			default:
				LONG_FRAME_TIME_LIMIT_US = vrefresh / 5 * FRAME_TIME_144HZ_US;
				break;
		}
	}
}
int register_fps_mon_handler(struct frame_mon_receiver *receiver)
{
	down_write(&receiver_lock);
	list_add(&receiver->list, &receivers);
	up_write(&receiver_lock);
	return 0;
}

int unregister_fps_mon_handler(struct frame_mon_receiver *receiver)
{
	down_write(&receiver_lock);
	list_del(&receiver->list);
	up_write(&receiver_lock);
	return 0;
}


