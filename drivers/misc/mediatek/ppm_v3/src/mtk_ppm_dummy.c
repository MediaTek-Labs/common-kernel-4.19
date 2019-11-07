// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/cpufreq.h>
#include <linux/module.h>
#include "mtk_ppm_api.h"

void mt_ppm_set_dvfs_table(unsigned int cpu,
	struct cpufreq_frequency_table *tbl,
	unsigned int num, enum dvfs_table_type type)
{
}
EXPORT_SYMBOL(mt_ppm_set_dvfs_table);

void mt_ppm_register_client(enum ppm_client client,
	void (*limit)(struct ppm_client_req req))
{
}
EXPORT_SYMBOL(mt_ppm_register_client);


/* SYS boost policy */
void mt_ppm_sysboost_core(enum ppm_sysboost_user user, unsigned int core_num)
{
}
EXPORT_SYMBOL(mt_ppm_sysboost_core);

void mt_ppm_sysboost_freq(enum ppm_sysboost_user user, unsigned int freq)
{
}
EXPORT_SYMBOL(mt_ppm_sysboost_freq);

void mt_ppm_sysboost_set_core_limit(enum ppm_sysboost_user user,
	unsigned int cluster, int min_core, int max_core)
{
}
EXPORT_SYMBOL(mt_ppm_sysboost_set_core_limit);

void mt_ppm_sysboost_set_freq_limit(enum ppm_sysboost_user user,
	unsigned int cluster, int min_freq, int max_freq)
{
}
EXPORT_SYMBOL(mt_ppm_sysboost_set_freq_limit);

/* DLPT policy */
void mt_ppm_dlpt_set_limit_by_pbm(unsigned int limited_power)
{
}
EXPORT_SYMBOL(mt_ppm_dlpt_set_limit_by_pbm);

void mt_ppm_dlpt_kick_PBM(struct ppm_cluster_status *cluster_status,
	unsigned int cluster_num)
{
}
EXPORT_SYMBOL(mt_ppm_dlpt_kick_PBM);

/* Thermal policy */
void mt_ppm_cpu_thermal_protect(unsigned int limited_power)
{
}
EXPORT_SYMBOL(mt_ppm_cpu_thermal_protect);

unsigned int mt_ppm_thermal_get_min_power(void)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_thermal_get_min_power);

unsigned int mt_ppm_thermal_get_max_power(void)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_thermal_get_max_power);

unsigned int mt_ppm_thermal_get_cur_power(void)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_thermal_get_cur_power);

/* User limit policy */
unsigned int mt_ppm_userlimit_cpu_core(unsigned int cluster_num,
	struct ppm_limit_data *data)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_userlimit_cpu_core);

unsigned int mt_ppm_userlimit_cpu_freq(unsigned int cluster_num,
	struct ppm_limit_data *data)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_userlimit_cpu_freq);

unsigned int mt_ppm_userlimit_freq_limit_by_others(unsigned int cluster)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_userlimit_freq_limit_by_others);

void ppm_game_mode_change_cb(int is_game_mode)
{
}
EXPORT_SYMBOL(ppm_game_mode_change_cb);

/* Force limit policy */
unsigned int mt_ppm_forcelimit_cpu_core(unsigned int cluster_num,
	struct ppm_limit_data *data)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_forcelimit_cpu_core);

/* PTPOD policy */
void mt_ppm_ptpod_policy_activate(void)
{
}
EXPORT_SYMBOL(mt_ppm_ptpod_policy_activate);

void mt_ppm_ptpod_policy_deactivate(void)
{
}
EXPORT_SYMBOL(mt_ppm_ptpod_policy_deactivate);

unsigned int mt_ppm_get_leakage_mw(enum ppm_cluster_lkg cluster)
{
	return 0;
}
EXPORT_SYMBOL(mt_ppm_get_leakage_mw);

/* CPI */
unsigned int ppm_get_cluster_cpi(unsigned int cluster)
{
	return 0;
}
EXPORT_SYMBOL(ppm_get_cluster_cpi);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek PPM");
MODULE_AUTHOR("MediaTek Inc.");

