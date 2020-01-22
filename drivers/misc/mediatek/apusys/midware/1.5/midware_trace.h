/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/string.h>
#include <linux/kernel.h>
#include "apusys_trace.h"

#ifdef CONFIG_FTRACE
extern u8 cfg_apusys_trace;
#ifdef midware_trace_begin
#undef midware_trace_begin
#endif
#define midware_trace_begin(format, args...) \
	{ \
		char buf[256]; \
		int len; \
		if (cfg_apusys_trace) { \
			len = snprintf(buf, sizeof(buf), \
				       format, args); \
			trace_async_tag(1, buf); \
		} \
	}
#ifdef midware_trace_end
#undef midware_trace_end
#endif
#define midware_trace_end(format, args...) \
	{ \
		char buf[256]; \
		int len; \
		if (cfg_apusys_trace) { \
			len = snprintf(buf, sizeof(buf), \
				       format, args); \
			trace_async_tag(0, buf); \
		} \
	}
#else
#define midware_trace_begin(...)
#define midware_trace_end(...)
#endif /* CONFIG_FTRACE */
