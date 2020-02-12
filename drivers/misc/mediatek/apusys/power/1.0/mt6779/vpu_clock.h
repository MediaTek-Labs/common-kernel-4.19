/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MT6779_VPU_CLOCK_H_
#define _MT6779_VPU_CLOCK_H_

#include "spm_mtcmos_ctl.h"

#ifndef MTK_VPU_FPGA_PORTING
/* clock */
struct clk *clk_top_dsp_sel;
struct clk *clk_top_dsp1_sel;
struct clk *clk_top_dsp2_sel;
//struct clk *clk_top_dsp3_sel;
struct clk *clk_top_ipu_if_sel;
struct clk *clk_apu_core0_jtag_cg;
struct clk *clk_apu_core0_axi_m_cg;
struct clk *clk_apu_core0_apu_cg;
struct clk *clk_apu_core1_jtag_cg;
struct clk *clk_apu_core1_axi_m_cg;
struct clk *clk_apu_core1_apu_cg;

struct clk *clk_apu_conn_apu_cg;
struct clk *clk_apu_conn_ahb_cg;
struct clk *clk_apu_conn_axi_cg;
struct clk *clk_apu_conn_isp_cg;
struct clk *clk_apu_conn_cam_adl_cg;
struct clk *clk_apu_conn_img_adl_cg;
struct clk *clk_apu_conn_emi_26m_cg;
struct clk *clk_apu_conn_vpu_udi_cg;
struct clk *clk_apu_vcore_ahb_cg;
struct clk *clk_apu_vcore_axi_cg;
struct clk *clk_apu_vcore_adl_cg;
struct clk *clk_apu_vcore_qos_cg;

struct clk *clk_top_clk26m;
struct clk *clk_top_univpll_d3_d8;
struct clk *clk_top_univpll_d3_d4;
struct clk *clk_top_mainpll_d2_d4;
struct clk *clk_top_univpll_d3_d2;
struct clk *clk_top_mainpll_d2_d2;
struct clk *clk_top_univpll_d2_d2;
struct clk *clk_top_mainpll_d3;
struct clk *clk_top_univpll_d3;
struct clk *clk_top_mmpll_d7;
struct clk *clk_top_mmpll_d6;
struct clk *clk_top_adsppll_d5;
struct clk *clk_top_tvdpll_ck;
struct clk *clk_top_tvdpll_mainpll_d2_ck;
struct clk *clk_top_univpll_d2;
struct clk *clk_top_adsppll_d4;
struct clk *clk_top_mainpll_d2;
struct clk *clk_top_mmpll_d4;

#if CCF_MTCMOS_SUPPORT

/* mtcmos */
//struct clk *mtcmos_dis;

//struct clk *mtcmos_vpu_vcore_dormant;
struct clk *mtcmos_vpu_vcore_shutdown;
//struct clk *mtcmos_vpu_conn_dormant;
struct clk *mtcmos_vpu_conn_shutdown;

//static struct clk *mtcmos_vpu_core0_dormant;
struct clk *mtcmos_vpu_core0_shutdown;
//static struct clk *mtcmos_vpu_core1_dormant;
struct clk *mtcmos_vpu_core1_shutdown;
//static struct clk *mtcmos_vpu_core2_dormant;
struct clk *mtcmos_vpu_core2_shutdown;

#else

//int (*mtcmos_dis)(int) = spm_mtcmos_ctrl_dis;
int (*mtcmos_vpu_vcore_shutdown)(int) = spm_mtcmos_ctrl_vpu_vcore_shut_down;
int (*mtcmos_vpu_conn_shutdown)(int) = spm_mtcmos_ctrl_vpu_conn_shut_down;
int (*mtcmos_vpu_core0_shutdown)(int) = spm_mtcmos_ctrl_vpu_core0_shut_down;
int (*mtcmos_vpu_core1_shutdown)(int) = spm_mtcmos_ctrl_vpu_core1_shut_down;
int (*mtcmos_vpu_core2_shutdown)(int) = spm_mtcmos_ctrl_vpu_core2_shut_down;

#endif

/* smi */
struct clk *clk_mmsys_gals_ipu2mm;
struct clk *clk_mmsys_gals_ipu12mm;
struct clk *clk_mmsys_gals_comm0;
struct clk *clk_mmsys_gals_comm1;
struct clk *clk_mmsys_smi_common;
#endif

#endif
