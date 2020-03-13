// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/seq_file.h>

#include "vpu_cmn.h"
#include "mdla_dvfs.h"

void apu_power_dump_opp_table(struct seq_file *s)
{
	vpu_dump_opp_table(s);
	seq_puts(s, "\n");
	mdla_dump_opp_table(s);
	seq_puts(s, "\n");
}

int apu_power_dump_curr_status(struct seq_file *s, int oneline_str)
{
	vpu_dump_power(s);
	seq_puts(s, "\n");
	mdla_dump_power(s);
	seq_puts(s, "\n");

	return 0;
}

int apusys_power_fail_show(struct seq_file *s, void *unused)
{
	return 0;
}
