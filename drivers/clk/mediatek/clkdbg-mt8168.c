// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#include <linux/clk-provider.h>
#include <linux/io.h>

#include "clkdbg.h"

#define DUMP_INIT_STATE		0
#define DEFAULT_CYCLE_COUNT	0x1FF
/*
 * clkdbg dump_regs
 */

enum {
	topckgen,
	infracfg,
	scpsys,
	apmixedsys,
	audiosys,
	mfgsys,
	mmsys,
	camsys,
	vdecsys,
	vencsys,
};

#define REGBASE_V(_phys, _id_name) { .phys = _phys, .name = #_id_name }

/*
 * checkpatch.pl ERROR:COMPLEX_MACRO
 *
 * #define REGBASE(_phys, _id_name) [_id_name] = REGBASE_V(_phys, _id_name)
 */

static struct regbase rb[] = {
	[topckgen] = REGBASE_V(0x10000000, topckgen),
	[infracfg] = REGBASE_V(0x10001000, infracfg),
	[scpsys]   = REGBASE_V(0x10006000, scpsys),
	[apmixedsys]  = REGBASE_V(0x1000c000, apmixedsys),
	[audiosys]    = REGBASE_V(0x11220000, audiosys),
	[mfgsys]   = REGBASE_V(0x13000000, mfgsys),
	[mmsys]    = REGBASE_V(0x14000000, mmsys),
	[camsys]   = REGBASE_V(0x15000000, camsys),
	[vdecsys]  = REGBASE_V(0x16000000, vdecsys),
	[vencsys]  = REGBASE_V(0x17000000, vencsys),
};

#define REGNAME(_base, _ofs, _name)	\
	{ .base = &rb[_base], .ofs = _ofs, .name = #_name }

static struct regname rn[] = {
	REGNAME(topckgen,  0x040, CLK_CFG_0),
	REGNAME(topckgen,  0x050, CLK_CFG_1),
	REGNAME(topckgen,  0x060, CLK_CFG_2),
	REGNAME(topckgen,  0x070, CLK_CFG_3),
	REGNAME(topckgen,  0x080, CLK_CFG_4),
	REGNAME(topckgen,  0x090, CLK_CFG_5),
	REGNAME(topckgen,  0x0a0, CLK_CFG_6),
	REGNAME(topckgen,  0x0b0, CLK_CFG_7),
	REGNAME(topckgen,  0x0c0, CLK_CFG_8),
	REGNAME(topckgen,  0x0d0, CLK_CFG_9),
	REGNAME(topckgen,  0x0e0, CLK_CFG_10),
	REGNAME(topckgen,  0x0ec, CLK_CFG_11),
	REGNAME(audiosys,  0x000, AUDIO_TOP_CON0),
	REGNAME(audiosys,  0x004, AUDIO_TOP_CON1),
	REGNAME(camsys,  0x000, CAMSYS_CG),
	REGNAME(infracfg,  0x090, MODULE_SW_CG_0),
	REGNAME(infracfg,  0x094, MODULE_SW_CG_1),
	REGNAME(infracfg,  0x0ac, MODULE_SW_CG_2),
	REGNAME(infracfg,  0x0c8, MODULE_SW_CG_3),
	REGNAME(infracfg,  0x0d8, MODULE_SW_CG_4),
	REGNAME(mfgsys,  0x000, MFG_CG),
	REGNAME(mmsys,	0x100, MMSYS_CG_CON0),
	REGNAME(mmsys,	0x110, MMSYS_CG_CON1),
	REGNAME(vencsys,  0x004, VENCSYS_CG),
	REGNAME(vdecsys,  0x000, VDEC_CKEN),
	REGNAME(vdecsys,  0x008, VDEC_LARB3_CKEN),
	REGNAME(apmixedsys,  0x30C, ARMPLL_CON0),
	REGNAME(apmixedsys,  0x310, ARMPLL_CON1),
	REGNAME(apmixedsys,  0x318, ARMPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x228, MAINPLL_CON0),
	REGNAME(apmixedsys,  0x22C, MAINPLL_CON1),
	REGNAME(apmixedsys,  0x234, MAINPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x208, UNIVPLL_CON0),
	REGNAME(apmixedsys,  0x20C, UNIVPLL_CON1),
	REGNAME(apmixedsys,  0x214, UNIVPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x218, MFGPLL_CON0),
	REGNAME(apmixedsys,  0x21C, MFGPLL_CON1),
	REGNAME(apmixedsys,  0x224, MFGPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x350, MSDCPLL_CON0),
	REGNAME(apmixedsys,  0x354, MSDCPLL_CON1),
	REGNAME(apmixedsys,  0x35C, MSDCPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x330, MMPLL_CON0),
	REGNAME(apmixedsys,  0x334, MMPLL_CON1),
	REGNAME(apmixedsys,  0x33C, MMPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x31C, APLL1_CON0),
	REGNAME(apmixedsys,  0x320, APLL1_CON1),
	REGNAME(apmixedsys,  0x32C, APLL1_PWR_CON0),
	REGNAME(apmixedsys,  0x360, APLL2_CON0),
	REGNAME(apmixedsys,  0x364, APLL2_CON1),
	REGNAME(apmixedsys,  0x370, APLL2_PWR_CON0),
	REGNAME(apmixedsys,  0x340, MPLL_CON0),
	REGNAME(apmixedsys,  0x344, MPLL_CON1),
	REGNAME(apmixedsys,  0x34C, MPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x374, LVDSPLL_CON0),
	REGNAME(apmixedsys,  0x378, LVDSPLL_CON1),
	REGNAME(apmixedsys,  0x380, LVDSPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x390, DSPPLL_CON0),
	REGNAME(apmixedsys,  0x394, DSPPLL_CON1),
	REGNAME(apmixedsys,  0x39C, DSPPLL_PWR_CON0),
	REGNAME(apmixedsys,  0x3A0, APUPLL_CON0),
	REGNAME(apmixedsys,  0x3A4, APUPLL_CON1),
	REGNAME(apmixedsys,  0x3AC, APUPLL_PWR_CON0),
	REGNAME(scpsys,  0x0180, PWR_STATUS),
	REGNAME(scpsys,  0x0184, PWR_STATUS_2ND),
	REGNAME(scpsys,  0x0338, MFG_PWR_CON),
	REGNAME(scpsys,  0x032C, CONN_PWR_CON),
	REGNAME(scpsys,  0x0314, AUD_PWR_CON),
	REGNAME(scpsys,  0x030C, DIS_PWR_CON),
	REGNAME(scpsys,  0x0344, CAM_PWR_CON),
	REGNAME(scpsys,  0x0300, VDEC_PWR_CON),
	REGNAME(scpsys,  0x0304, VENC_PWR_CON),
	REGNAME(scpsys,  0x0378, APU_PWR_CON),
	REGNAME(scpsys,  0x037C, DSP_PWR_CON),
	{}
};

static const struct regname *get_all_regnames(void)
{
	return rn;
}

static void __init init_regbase(void)
{
	size_t i;

	for (i = 0; i < ARRAY_SIZE(rb); i++)
		rb[i].virt = ioremap(rb[i].phys, PAGE_SIZE);
}

/*
 * clkdbg fmeter
 */

#include <linux/delay.h>

#define clk_readl(addr)		readl(addr)
#define clk_writel(addr, val)	\
	do { writel(val, addr); wmb(); } while (0) /* sync write */

#define FMCLK(_t, _i, _n) { .type = _t, .id = _i, .name = _n }
#define CKGEN_K1 0x3

static const struct fmeter_clk fclks[] = {
	FMCLK(CKGEN,  1, "axi_ck"),
	FMCLK(CKGEN,  2, "mem_ck"),
	FMCLK(CKGEN,  3, "mm_ck"),
	FMCLK(CKGEN,  4, "scp_ck"),
	FMCLK(CKGEN,  5, "mfg_ck"),
	FMCLK(CKGEN,  7, "camtg_ck"),
	FMCLK(CKGEN,  8, "camtg1_ck"),
	FMCLK(CKGEN,  9, "uart_ck"),
	FMCLK(CKGEN, 10, "f_fspi_ck"),
	FMCLK(CKGEN, 11, "msdc50_0_hclk_ck"),
	FMCLK(CKGEN, 12, "fmsdc2_2_hclk_ck"),
	FMCLK(CKGEN, 13, "msdc50_0_ck"),
	FMCLK(CKGEN, 14, "msdc50_2_ck"),
	FMCLK(CKGEN, 15, "msdc30_1_ck"),
	FMCLK(CKGEN, 16, "audio_ck"),
	FMCLK(CKGEN, 17, "aud_intbus_ck"),
	FMCLK(CKGEN, 18, "aud_1_ck"),
	FMCLK(CKGEN, 19, "aud_2_ck"),
	FMCLK(CKGEN, 20, "aud_engen1_ck"),
	FMCLK(CKGEN, 21, "aud_engen2_ck"),
	FMCLK(CKGEN, 22, "hf_faud_spdif_ck"),
	FMCLK(CKGEN, 23, "disp_pwm_ck"),
	FMCLK(CKGEN, 24, "sspm_ck"),
	FMCLK(CKGEN, 25, "dxcc_ck"),
	FMCLK(CKGEN, 26, "ssusb_sys_ck"),
	FMCLK(CKGEN, 27, "ssusb_xhci_ck"),
	FMCLK(CKGEN, 28, "spm_ck"),
	FMCLK(CKGEN, 29, "i2c_ck"),
	FMCLK(CKGEN, 30, "pwm_ck"),
	FMCLK(CKGEN, 31, "seninf_ck"),
	FMCLK(CKGEN, 32, "aes_fde_ck"),
	FMCLK(CKGEN, 33, "camtm_ck"),
	FMCLK(CKGEN, 34, "dpi0_ck"),
	FMCLK(CKGEN, 35, "dpi1_ck"),
	FMCLK(CKGEN, 36, "dsp_ck"),
	FMCLK(CKGEN, 37, "nfi2x_ck"),
	FMCLK(CKGEN, 38, "nfiecc_ck"),
	FMCLK(CKGEN, 39, "ecc_ck"),
	FMCLK(CKGEN, 40, "eth_ck"),
	FMCLK(CKGEN, 41, "gcpu_ck"),
	FMCLK(CKGEN, 42, "gcpu_cpm_ck"),
	FMCLK(CKGEN, 43, "apu_ck"),
	FMCLK(CKGEN, 44, "apu_if_ck"),
	FMCLK(CKGEN, 45, "mbist_diag_clk"),
	FMCLK(CKGEN, 48, "f_ufs_mp_sap_cfg_ck"),
	FMCLK(CKGEN, 49, "f_ufs_tick1us_ck"),
	FMCLK(CKGEN, 50, "hd_faxi_east_ck"),
	FMCLK(CKGEN, 51, "hd_faxi_west_ck"),
	FMCLK(CKGEN, 52, "hd_faxi_north_ck"),
	FMCLK(CKGEN, 53, "hd_faxi_south_ck"),
	FMCLK(CKGEN, 54, "hd_fmipicfg_tx_ck"),
	FMCLK(CKGEN, 55, "fmem_ck_bfe_dcm_ch0"),
	FMCLK(CKGEN, 56, "fmem_ck_aft_dcm_ch0"),
	FMCLK(CKGEN, 57, "fmem_ck_bfe_dcm_ch1"),
	FMCLK(CKGEN, 58, "fmem_ck_aft_dcm_ch1"),
	FMCLK(ABIST,  1, "AD_ARMPLL_CK"),
	FMCLK(ABIST,  2, "0"),
	FMCLK(ABIST,  3, "AD_MAINPLL_CK"),
	FMCLK(ABIST,  4, "AD_CSI0A_CDPHY_DELAYCAL_CK"),
	FMCLK(ABIST,  5, "AD_CSI0B_CDPHY_DELAYCAL_CK"),
	FMCLK(ABIST,  7, "AD_USB20_CLK480M"),
	FMCLK(ABIST,  8, "AD_USB20_CLK480M_1P"),
	FMCLK(ABIST,  9, "AD_MADADC_26MCKO"),
	FMCLK(ABIST, 10, "AD_MAINPLL_H546M_CK"),
	FMCLK(ABIST, 11, "AD_MAINPLL_H364M_CK"),
	FMCLK(ABIST, 12, "AD_MAINPLL_H218P4M_CK"),
	FMCLK(ABIST, 13, "AD_MAINPLL_H156M_CK"),
	FMCLK(ABIST, 14, "AD_UNIVPLL_1248M_CK"),
	FMCLK(ABIST, 15, "AD_USB20_192M_CK"),
	FMCLK(ABIST, 16, "AD_UNIVPLL_624M_CK"),
	FMCLK(ABIST, 17, "AD_UNIVPLL_416M_CK"),
	FMCLK(ABIST, 18, "AD_UNIVPLL_249P6M_CK"),
	FMCLK(ABIST, 19, "AD_UNIVPLL_178P3M_CK"),
	FMCLK(ABIST, 20, "AD_SYS_26M_CK"),
	FMCLK(ABIST, 21, "AD_CSI1A_DPHY_DELAYCAL_CK"),
	FMCLK(ABIST, 22, "AD_CSI1B_DPHY_DELAYCAL_CK"),
	FMCLK(ABIST, 23, "AD_CSI2A_DPHY_DELAYCAL_CK"),
	FMCLK(ABIST, 24, "AD_CSI2B_DPHY_DELAYCAL_CK"),
	FMCLK(ABIST, 25, "RTC32K"),
	FMCLK(ABIST, 26, "AD_MMPLL_CK"),
	FMCLK(ABIST, 27, "AD_MFGPLL_CK"),
	FMCLK(ABIST, 28, "AD_MSDCPLL_CK"),
	FMCLK(ABIST, 29, "AD_DSI0_LNTC_DSICLK"),
	FMCLK(ABIST, 30, "AD_DSI0_MPPLL_TST_CK"),
	FMCLK(ABIST, 31, "AD_APPLLGP_TST_CK"),
	FMCLK(ABIST, 32, "AD_APLL1_CK"),
	FMCLK(ABIST, 33, "AD_APLL2_CK"),
	FMCLK(ABIST, 34, "AD_MADCKO_TEST"),
	FMCLK(ABIST, 35, "AD_MPLL_208M_CK"),
	FMCLK(ABIST, 36, "Armpll_ll_mon_ck"),
	FMCLK(ABIST, 37, "vad_clk_i"),
	FMCLK(ABIST, 38, "msdc01_in_ck"),
	FMCLK(ABIST, 40, "msdc11_in_ck"),
	FMCLK(ABIST, 41, "msdc12_in_ck"),
	FMCLK(ABIST, 42, "AD_PLLGP_TST_CK"),
	FMCLK(ABIST, 43, "AD_LVDSTX_CLKDIG_CTS"),
	FMCLK(ABIST, 44, "AD_LVDSTX_CLKDIG"),
	FMCLK(ABIST, 45, "AD_VPLL_DPIX_CK"),
	FMCLK(ABIST, 46, "DA_USB20_48M_DIV_CK"),
	FMCLK(ABIST, 47, "DA_UNIV_48M_DIV_CK"),
	FMCLK(ABIST, 48, "DA_MPLL_104M_DIV_CK"),
	FMCLK(ABIST, 49, "DA_MPLL_52M_DIV_CK"),
	FMCLK(ABIST, 50, "DA_PLLGP_CPU_CK_MON"),
	FMCLK(ABIST, 51, "trng_freq_debug_out0"),
	FMCLK(ABIST, 52, "trng_freq_debug_out1"),
	FMCLK(ABIST, 53, "AD_LVDSTX_MONCLK"),
	FMCLK(ABIST, 54, "AD_VPLL_MONREF_CK"),
	FMCLK(ABIST, 55, "AD_VPLL_MONFBK_CK"),
	FMCLK(ABIST, 56, "AD_LVDSPLL_300M_CK"),
	FMCLK(ABIST, 57, "AD_DSPPLL_CK"),
	FMCLK(ABIST, 58, "AD_APUPLL_CK"),
	{}
};

#define CLK_MISC_CFG_0		(rb[topckgen].virt + 0x104)
#define CLK_DBG_CFG		(rb[topckgen].virt + 0x10C)
#define CLK26CALI_0		(rb[topckgen].virt + 0x220)
#define CLK26CALI_1		(rb[topckgen].virt + 0x224)

static unsigned int mt_get_ckgen_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int tmp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1, clk26cali_0;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFFFC0FC) | (ID << 8) | (0x1));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0, (clk_misc_cfg_0 & 0x00FFFFFF));

	clk26cali_0 = clk_readl(CLK26CALI_0);
	clk26cali_1 = clk_readl(CLK26CALI_1);
	clk_writel(CLK26CALI_1,
		  (clk26cali_1 & ~(0x3FF0000)) | (DEFAULT_CYCLE_COUNT << 16));
	mdelay(1);
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	tmp = clk_readl(CLK26CALI_1);
	output = ((tmp  & 0xFFFF) * 26000) / (DEFAULT_CYCLE_COUNT + 1);

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return output;

}

static unsigned int mt_get_abist_freq(unsigned int ID)
{
	int output = 0, i = 0;
	unsigned int tmp, clk_dbg_cfg, clk_misc_cfg_0, clk26cali_1, clk26cali_0;

	clk_dbg_cfg = clk_readl(CLK_DBG_CFG);
	clk_writel(CLK_DBG_CFG, (clk_dbg_cfg & 0xFFC0FFFC) | (ID << 16));

	clk_misc_cfg_0 = clk_readl(CLK_MISC_CFG_0);
	clk_writel(CLK_MISC_CFG_0,
		  (clk_misc_cfg_0 & 0x00FFFFFF) | (CKGEN_K1 << 24));

	clk26cali_0 = clk_readl(CLK26CALI_0);
	clk26cali_1 = clk_readl(CLK26CALI_1);
	clk_writel(CLK26CALI_1,
		  (clk26cali_1 & ~(0x3FF0000)) | (DEFAULT_CYCLE_COUNT << 16));
	mdelay(1);
	clk_writel(CLK26CALI_0, 0x1000);
	clk_writel(CLK26CALI_0, 0x1010);

	while (clk_readl(CLK26CALI_0) & 0x10) {
		mdelay(10);
		i++;
		if (i > 10)
			break;
	}

	tmp = clk_readl(CLK26CALI_1);
	output = ((tmp  & 0xFFFF) * 26000) / (DEFAULT_CYCLE_COUNT + 1);

	clk_writel(CLK_DBG_CFG, clk_dbg_cfg);
	clk_writel(CLK_MISC_CFG_0, clk_misc_cfg_0);
	clk_writel(CLK26CALI_0, clk26cali_0);
	clk_writel(CLK26CALI_1, clk26cali_1);

	if (i > 10)
		return 0;
	else
		return (output * (CKGEN_K1 + 1));
}

static u32 fmeter_freq_op(const struct fmeter_clk *fclk)
{
	if (fclk->type == ABIST)
		return mt_get_abist_freq(fclk->id);
	else if (fclk->type == CKGEN)
		return mt_get_ckgen_freq(fclk->id);
	return 0;
}

static const struct fmeter_clk *get_all_fmeter_clks(void)
{
	return fclks;
}

/*
 * clkdbg dump_state
 */

static const char * const *get_all_clk_names(void)
{
	static const char * const clks[] = {

		"i2s0_bck",
		"dsi0_lntc_dsick",
		"vpll_dpix",
		"lvdstx_dig_cts",
		"mfgpll_ck",
		"syspll_d2",
		"syspll1_d2",
		"syspll1_d4",
		"syspll1_d8",
		"syspll1_d16",
		"syspll_d3",
		"syspll2_d2",
		"syspll2_d4",
		"syspll2_d8",
		"syspll_d5",
		"syspll3_d2",
		"syspll3_d4",
		"syspll_d7",
		"syspll4_d2",
		"syspll4_d4",
		"univpll_d2",
		"univpll1_d2",
		"univpll1_d4",
		"univpll_d3",
		"univpll2_d2",
		"univpll2_d4",
		"univpll2_d8",
		"univpll2_d32",
		"univpll_d5",
		"univpll3_d2",
		"univpll3_d4",
		"mmpll_ck",
		"mmpll_d2",
		"lvdspll_d2",
		"lvdspll_d4",
		"lvdspll_d8",
		"lvdspll_d16",
		"usb20_192m_ck",
		"usb20_192m_d4",
		"usb20_192m_d8",
		"usb20_192m_d16",
		"usb20_192m_d32",
		"apll1_ck",
		"apll1_d2",
		"apll1_d4",
		"apll1_d8",
		"apll2_ck",
		"apll2_d2",
		"apll2_d4",
		"apll2_d8",
		"clk26m_ck",
		"sys_26m_d2",
		"msdcpll_ck",
		"msdcpll_d2",
		"dsppll_ck",
		"dsppll_d2",
		"dsppll_d4",
		"dsppll_d8",
		"apupll_ck",
		"mpll_d2",
		"mpll_d4",
		"clk26m_d52",
		"axi_sel",
		"mem_sel",
		"mm_sel",
		"scp_sel",
		"mfg_sel",
		"atb_sel",
		"camtg_sel",
		"camtg1_sel",
		"uart_sel",
		"spi_sel",
		"msdc50_0_hc_sel",
		"msdc2_2_hc_sel",
		"msdc50_0_sel",
		"msdc50_2_sel",
		"msdc30_1_sel",
		"audio_sel",
		"aud_intbus_sel",
		"aud_1_sel",
		"aud_2_sel",
		"aud_engen1_sel",
		"aud_engen2_sel",
		"aud_spdif_sel",
		"disp_pwm_sel",
		"dxcc_sel",
		"ssusb_sys_sel",
		"ssusb_xhci_sel",
		"spm_sel",
		"i2c_sel",
		"pwm_sel",
		"senif_sel",
		"aes_fde_sel",
		"camtm_sel",
		"dpi0_sel",
		"dpi1_sel",
		"dsp_sel",
		"nfi2x_sel",
		"nfiecc_sel",
		"ecc_sel",
		"eth_sel",
		"gcpu_sel",
		"gcpu_cpm_sel",
		"apu_sel",
		"apu_if_sel",
		"mbist_diag_sel",
		"apll_i2s0_sel",
		"apll_i2s1_sel",
		"apll_i2s2_sel",
		"apll_i2s3_sel",
		"apll_tdmout_sel",
		"apll_tdmin_sel",
		"apll_spdif_sel",
		"apll12_ck_div0",
		"apll12_ck_div1",
		"apll12_ck_div2",
		"apll12_ck_div3",
		"apll12_ck_div4",
		"apll12_ck_div4b",
		"apll12_ck_div5",
		"apll12_ck_div5b",
		"apll12_ck_div6",
		"aud_i2s0_m_ck",
		"aud_i2s1_m_ck",
		"aud_i2s2_m_ck",
		"aud_i2s3_m_ck",
		"aud_tdmout_m_ck",
		"aud_tdmout_b_ck",
		"aud_tdmin_m_ck",
		"aud_tdmin_b_ck",
		"aud_spdif_m_ck",
		"usb20_48m_en",
		"univpll_48m_en",
		"lvdstx_dig_en",
		"vpll_dpix_en",
		"ssusb_top_ck_en",
		"ssusb_phy_ck_en",
		"conn_32k",
		"conn_26m",
		"dsp_32k",
		"dsp_26m",
		"ifr_pmic_tmr",
		"ifr_pmic_ap",
		"ifr_pmic_md",
		"ifr_pmic_conn",
		"ifr_sej",
		"ifr_apxgpt",
		"ifr_icusb",
		"ifr_gce",
		"ifr_therm",
		"ifr_pwm_hclk",
		"ifr_pwm1",
		"ifr_pwm2",
		"ifr_pwm3",
		"ifr_pwm4",
		"ifr_pwm5",
		"ifr_pwm",
		"ifr_uart0",
		"ifr_uart1",
		"ifr_gce_26m",
		"ifr_cq_dma_fpc",
		"ifr_btif",
		"ifr_spi0",
		"ifr_msdc",
		"ifr_msdc1",
		"ifr_dvfsrc",
		"ifr_gcpu",
		"ifr_trng",
		"ifr_auxadc",
		"ifr_auxadc_md",
		"ifr_ap_dma",
		"ifr_device_apc",
		"ifr_debugsys",
		"ifr_audio",
		"ifr_dramc_f26m",
		"ifr_pwm_fbclk6",
		"ifr_disp_pwm",
		"ifr_aud_26m_bk",
		"ifr_cq_dma",
		"ifr_msdc0_sf",
		"ifr_msdc1_sf",
		"ifr_msdc2_sf",
		"ifr_ap_msdc0",
		"ifr_md_msdc0",
		"ifr_msdc0_src",
		"ifr_msdc1_src",
		"ifr_msdc2_src",
		"ifr_pwrap_tmr",
		"ifr_pwrap_spi",
		"ifr_pwrap_sys",
		"ifr_sej_f13m",
		"ifr_mcu_pm_bk",
		"ifr_irrx_26m",
		"ifr_irrx_32k",
		"ifr_i2c0_axi",
		"ifr_i2c1_axi",
		"ifr_i2c2_axi",
		"ifr_i2c3_axi",
		"ifr_nic_axi",
		"ifr_nic_slv_axi",
		"ifr_apu_axi",
		"ifr_nfiecc",
		"ifr_nfiecc_bk",
		"ifr_nfi1x_bk",
		"ifr_nfi_bk",
		"ifr_msdc2_ap_bk",
		"ifr_msdc2_md_bk",
		"ifr_msdc2_bk",
		"ifr_susb_133_bk",
		"ifr_susb_66_bk",
		"ifr_ssusb_sys",
		"ifr_ssusb_ref",
		"ifr_ssusb_xhci",
		"armpll",
		"mainpll",
		"univpll",
		"mfgpll",
		"msdcpll",
		"mmpll",
		"apll1",
		"apll2",
		"mpll",
		"lvdspll",
		"dsppll",
		"apupll",
		"mfg_bg3d",
		"mfg_mbist_diag",
		"mm_mdp_rdma0",
		"mm_mdp_ccorr0",
		"mm_mdp_rsz0",
		"mm_mdp_rsz1",
		"mm_mdp_tdshp0",
		"mm_mdp_wrot0",
		"mm_mdp_wdma0",
		"mm_disp_ovl0",
		"mm_disp_ovl0_21",
		"mm_disp_rsz0",
		"mm_disp_rdma0",
		"mm_disp_wdma0",
		"mm_disp_color0",
		"mm_disp_ccorr0",
		"mm_disp_aal0",
		"mm_disp_gamma0",
		"mm_disp_dither0",
		"mm_dsi0",
		"mm_disp_rdma1",
		"mm_mdp_rdma1",
		"mm_dpi0_dpi0",
		"mm_fake",
		"mm_smi_common",
		"mm_smi_larb0",
		"mm_smi_comm0",
		"mm_smi_comm1",
		"mm_cam_mdp",
		"mm_smi_img",
		"mm_smi_cam",
		"mm_dl_relay",
		"mm_dl_async_top",
		"mm_dsi0_dig_dsi",
		"mm_f26m_hrtwt",
		"mm_dpi0",
		"mm_flvdstx_pxl",
		"mm_flvdstx_cts",
		"cam_larb2",
		"cam",
		"camtg",
		"cam_senif",
		"camsv0",
		"camsv1",
		"cam_fdvt",
		"cam_wpe",
		"vdec_fvdec_ck",
		"vdec_flarb1_ck",
		"venc_fvenc_ck",
		"venc_jpgenc_ck",
		/* end */
		NULL
	};

	return clks;
}

/*
 * clkdbg pwr_status
 */

static const char * const *get_pwr_names(void)
{
	static const char * const pwr_names[] = {
		[0]  = "",
		[1]  = "CONN",
		[2]  = "DDRPHY",
		[3]  = "DISP",
		[4]  = "MFG",
		[5]  = "",
		[6]  = "INFRA",
		[7]  = "",
		[8]  = "MP0_CPUTOP",
		[9]  = "MP0_CPU0",
		[10] = "MP0_CPU1",
		[11] = "MP0_CPU2",
		[12] = "MP0_CPU3",
		[13] = "",
		[14] = "MCUSYS",
		[15] = "",
		[16] = "APU",
		[17] = "DSP",
		[18] = "",
		[19] = "",
		[20] = "",
		[21] = "VENC",
		[22] = "",
		[23] = "",
		[24] = "AUDIO",
		[25] = "CAM",
		[26] = "",
		[27] = "",
		[28] = "",
		[29] = "",
		[30] = "",
		[31] = "VDEC",
	};

	return pwr_names;
}

/*
 * clkdbg dump_clks
 */

static void setup_provider_clk(struct provider_clk *pvdck)
{
	static const struct {
		const char *pvdname;
		u32 pwr_mask;
	} pvd_pwr_mask[] = {
	};

	size_t i;
	const char *pvdname = pvdck->provider_name;

	if (pvdname == NULL)
		return;

	for (i = 0; i < ARRAY_SIZE(pvd_pwr_mask); i++) {
		if (strcmp(pvdname, pvd_pwr_mask[i].pvdname) == 0) {
			pvdck->pwr_mask = pvd_pwr_mask[i].pwr_mask;
			return;
		}
	}
}

/*
 * init functions
 */

static struct clkdbg_ops clkdbg_mt8168_ops = {
	.get_all_fmeter_clks = get_all_fmeter_clks,
	.fmeter_freq = fmeter_freq_op,
	.get_all_regnames = get_all_regnames,
	.get_all_clk_names = get_all_clk_names,
	.get_pwr_names = get_pwr_names,
	.setup_provider_clk = setup_provider_clk,
};

static void __init init_custom_cmds(void)
{
	static const struct cmd_fn cmds[] = {
		{}
	};

	set_custom_cmds(cmds);
}

static int __init clkdbg_mt8168_init(void)
{
	if (of_machine_is_compatible("mediatek,mt8168") == 0)
		return -ENODEV;

	init_regbase();

	init_custom_cmds();
	set_clkdbg_ops(&clkdbg_mt8168_ops);

#if DUMP_INIT_STATE
	print_regs();
	print_fmeter_all();
#endif /* DUMP_INIT_STATE */

	return 0;
}
device_initcall(clkdbg_mt8168_init);
