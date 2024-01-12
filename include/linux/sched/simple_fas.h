// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 Yusen <1405481963@qq.com>.
 */
#ifndef _SIMPLE_FAS_H_
#define _SIMPLE_FAS_H_
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/frame_monitor.h>
#define DEFAULT_FAS_NORMAL_MODE_FREQ_BOOST_DURATION_FRAME 10
#define DEFAULT_FAS_PERF_MODE_FREQ_BOOST_DURATION_FRAME 20
#define DEFAULT_FAS_NORMAL_MODE_FREQ_BOOST_DURATION_TIME_US 90000L
#define DEFAULT_FAS_PERF_MODE_FREQ_BOOST_DURATION_TIME_US 180000L

#define USE_HWUI_MON 1
#define USE_KMS_MON 2
#define GAME_MODE 3

#define FPS_UPDATED 1
#define IS_JANK 2
#define IS_FLUID 3
#define IDLE 4

//0.5s
#define HWUI_MON_CHECKER_DURATION_NS  500000000L

struct fas_info {
	int   vrefresh;
	int   vrefresh_mul_10;
	int   hwui_last_pid;
	int   proc_max_freq_index;
	int   min_boost_freq_index;
	int   smooth_frame_count;
	u32   last_frame_time;
	u32	  hwui_target_frame_time;
	u32   kms_target_frame_time;
	u32	  hwui_normal_aggressiveness;
	u32   kms_normal_aggressiveness;
	u32   hwui_perf_aggressiveness;
	u32   kms_perf_aggressiveness;
	bool  freq_need_update;
	u32   freq;
	u32   freq_index;
	u16   freq_boost;
	u16   freq_boost_frame_count;
	u64   perf_mode_end_time;
	u64   perf_duration_ns;
	u64   perf_freq_boost_duration_ns;
	u64   normal_freq_boost_duration_ns;
};

extern unsigned int adrenoboost;
inline void trace_top_app(struct task_struct *p);
inline bool fas_ctl_by_vrefresh(int vrefresh);
int __init sugov_init_fas(void);
#endif /* _SIMPLE_FAS_H_ */
