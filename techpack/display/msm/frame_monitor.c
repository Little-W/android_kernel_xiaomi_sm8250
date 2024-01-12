// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Yusen <1405481963@qq.com>.
 */

#include <linux/frame_monitor.h>
#include <linux/uaccess.h>
#include <linux/sched/simple_fas.h>
#include <linux/moduleparam.h>
#include <drm/drm_notifier_mi.h>

fps_mon_info_t fps_info = {0};

static LIST_HEAD(receivers);
static DECLARE_RWSEM(receiver_lock);
int last_vrefresh,last_vrefresh_mul_10;
static int LONG_FRAME_TIME_LIMIT_US = 20 * FRAME_TIME_144HZ_US;

bool enabled = true;
static bool screen_on = false;
// static bool fps_force_update = false;
// u32 frame_time;
u16 fps;
module_param(fps, ushort, 0444);
module_param(enabled, bool, 0644);

void frame_time_calc(void)
{
	struct frame_mon_receiver *receiver;
	ktime_t cur_time = ktime_get();
	fps_info.frame_time = (u32)ktime_us_delta(cur_time, fps_info.last_commit_end_ts);
	// frame_time = fps_info.frame_time;
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
		fps_info.idle_state = false;
	}
	else
	{
		fps_info.fps = last_vrefresh_mul_10;
		fps = fps_info.fps;
		fps_info.idle_state = true;
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
	if(fps_info.idle_state)
	{
		fps_info.frame_count = 0;
		fps_info.fps_cnt_start_time = cur_time;
	}
	else
	{
		if(fps_info.frame_count < 300)
		fps_info.frame_count ++;
	
		diff_us = (u32)ktime_us_delta(cur_time, fps_info.fps_cnt_start_time);
		if(diff_us >= TIMER_INTERVAL_US)
		{
			fps_info.fps = mult_frac(fps_info.frame_count, 10000000L, diff_us);
			fps_info.frame_count = 0;
			fps_info.fps_cnt_start_time = cur_time;
			down_read(&receiver_lock);
			list_for_each_entry(receiver, &receivers, list) {
				receiver->callback_handler(fps_info.fps, fps_info.frame_time, cur_time, FPS_UPDATED);
			}
			up_read(&receiver_lock);
		}
		fps = fps_info.fps;
	}

}
void frame_stat(int event)
{
	if(enabled)
		switch (event)
		{
			case RENDER_COMPLETE_TS:
				frame_time_calc();
				break;
			case COMMIT_COMPLETE_TS:
				fps_calc();
				break;
		}
}
void vrefresh_stat(int vrefresh)
{
	static bool fas_need_update = true;
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
	if(fas_need_update && fas_ctl_by_vrefresh(vrefresh))
	{
		fas_need_update = false;
	}
}

// static inline int drm_notifier_callback(struct notifier_block *self,
// 				       unsigned long event, void *data)
// {

// 	struct mi_drm_notifier *evdata = data;
// 	int *blank;

// 	/* Parse framebuffer blank events as soon as they occur */
// 	if (event != MI_DRM_EARLY_EVENT_BLANK)
// 		return NOTIFY_OK;

// 	if (event != MI_DRM_EVENT_BLANK)
// 		return NOTIFY_OK;

// 	blank = evdata->data;
// 	switch (*blank) {
// 	case MI_DRM_BLANK_POWERDOWN:
// 		if (!screen_on)
// 			break;
// 		screen_on = false;
// 		break;
// 	case MI_DRM_BLANK_UNBLANK:
// 		if (screen_on)
// 			break;
// 		screen_on = true;
// 		break;
// 	}

// 	return NOTIFY_OK;
// }


// static struct notifier_block drm_notifier_block = {
// 	.notifier_call = drm_notifier_callback,
// };

// static int fps_mon_register_notifier(void)
// {
// 	int ret = 0;
// 	ret = mi_drm_register_client(&drm_notifier_block);
// 	return ret;
// }


// static void fps_mon_unregister_notifier(void)
// {
// 	mi_drm_unregister_client(&drm_notifier_block);
// }


// static int __init kms_fps_mon_init(void)
// {
// 	int ret = 0;

// 	ret = fps_mon_register_notifier();
// 	if (ret)
// 		pr_err("Failed to register notifier, err: %d\n", ret);

// 	return ret;
// }
// module_init(kms_fps_mon_init);

// static void __exit kp_exit(void)
// {
// 	fps_mon_unregister_notifier();
// }
// module_exit(kp_exit);

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


