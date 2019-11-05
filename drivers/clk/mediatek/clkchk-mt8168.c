// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#include <linux/clk-provider.h>
#include <linux/syscore_ops.h>
#include <linux/version.h>
#define WARN_ON_CHECK_FAIL		0
#define CLKDBG_CCF_API_4_4		1
#define TAG	"[clkchk] "
#define clk_warn(fmt, args...)	pr_notice(TAG fmt, ##args)
#if !CLKDBG_CCF_API_4_4
/* backward compatible */
static const char *clk_hw_get_name(const struct clk_hw *hw)
{
	return __clk_get_name(hw->clk);
}
static bool clk_hw_is_prepared(const struct clk_hw *hw)
{
	return __clk_is_prepared(hw->clk);
}
static bool clk_hw_is_enabled(const struct clk_hw *hw)
{
	return __clk_is_enabled(hw->clk);
}
#endif /* !CLKDBG_CCF_API_4_4 */

void print_enabled_clks(void);

static const char * const *get_all_clk_names(void)
{
	static const char * const clks[] = {
		/* PLLs */
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
		/* TOP */
		"i2s0_bck",
		"dsi0_lntc_dsick",
		"vpll_dpix",
		"lvdstx_dig_cts",
		"mfgpll_ck",
		"syspll_d2",
		"syspll1_d4",
		"syspll1_d16",
		"syspll_d3",
		"syspll2_d2",
		"syspll2_d4",
		"syspll2_d8",
		"syspll_d5",
		"syspll3_d2",
		"syspll3_d4",
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
		"mem_sel",
		"mm_sel",
		"scp_sel",
		"mfg_sel",
		"atb_sel",
		"camtg_sel",
		"camtg1_sel",
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
		"ssusb_sys_sel",
		"ssusb_xhci_sel",
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
		/* INFRACFG */
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
		"ifr_uart1",
		"ifr_gce_26m",
		"ifr_cq_dma_fpc",
		"ifr_btif",
		"ifr_spi0",
		"ifr_msdc",
		"ifr_msdc1",
		"ifr_gcpu",
		"ifr_trng",
		"ifr_auxadc",
		"ifr_auxadc_md",
		"ifr_ap_dma",
		"ifr_device_apc",
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
		"ifr_sej_f13m",
		"ifr_mcu_pm_bk",
		"ifr_irrx_26m",
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
		/* AUDIO */
		"aud_afe",
		"aud_22m",
		"aud_24m",
		"aud_apll_tuner",
		"aud_adc",
		"aud_dac",
		"aud_dac_predis",
		"aud_tml",
		"aud_i2s1_bclk",
		"aud_i2s2_bclk",
		"aud_i2s3_bclk",
		"aud_i2s4_bclk",
		/* CAM */
		"cam_larb2",
		"cam",
		"camtg",
		"cam_senif",
		"camsv0",
		"camsv1",
		"cam_fdvt",
		"cam_wpe",
		/* IMG */
		"img_larb2",
		"img_dip",
		"img_fdvt",
		"img_dpe",
		"img_rsc",
		/* MFG */
		"mfg_bg3d",
		"mfg_mbist_diag",
		/* MM */
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
		/* VENC */
		"venc_fvenc_ck",
		"venc_jpgenc_ck",
		/* VDEC */
		"vdec_fvdec_ck",
		"vdec_flarb1_ck",
		/* end */
		NULL
	};
	return clks;
}

static const char *ccf_state(struct clk_hw *hw)
{
	if (__clk_get_enable_count(hw->clk))
		return "enabled";
	if (clk_hw_is_prepared(hw))
		return "prepared";
	return "disabled";
}

void print_enabled_clks(void)
{
	const char * const *cn = get_all_clk_names();

	clk_warn("enabled clks:\n");
	for (; *cn; cn++) {
		struct clk *c = __clk_lookup(*cn);
		struct clk_hw *c_hw = __clk_get_hw(c);
		struct clk_hw *p_hw;

		if (IS_ERR_OR_NULL(c) || !c_hw)
			continue;
		p_hw = clk_hw_get_parent(c_hw);
		if (!p_hw)
			continue;
		if (!clk_hw_is_prepared(c_hw) && !__clk_get_enable_count(c))
			continue;
		clk_warn("[%-17s: %8s, %3d, %3d, %10ld, %17s]\n",
			clk_hw_get_name(c_hw),
			ccf_state(c_hw),
			clk_hw_is_prepared(c_hw),
			__clk_get_enable_count(c),
			clk_hw_get_rate(c_hw),
			p_hw ? clk_hw_get_name(p_hw) : "- ");
	}
}

static void check_pll_off(void)
{
	static const char * const off_pll_names[] = {
		"univpll",
		"mfgpll",
		"apll1",
		"apll2",
		"mmpll",
		"msdcpll",
		"lvdspll",
		"dsppll",
		"apupll",
		NULL
	};
	static struct clk *off_plls[ARRAY_SIZE(off_pll_names)];
	struct clk **c;
	int invalid = 0;
	char buf[128] = {0};
	int n = 0;

	if (!off_plls[0]) {
		const char * const *pn;

		for (pn = off_pll_names, c = off_plls; *pn; pn++, c++)
			*c = __clk_lookup(*pn);
	}
	for (c = off_plls; *c; c++) {
		struct clk_hw *c_hw = __clk_get_hw(*c);

		if (!c_hw)
			continue;
		if (!clk_hw_is_enabled(c_hw))
			continue;
		n += snprintf(buf + n, sizeof(buf) - n, "%s ",
				clk_hw_get_name(c_hw));
		invalid++;
	}
	if (invalid) {
		clk_warn("suspend warning: unexpected unclosed PLL: %s\n", buf);
		print_enabled_clks();
#if WARN_ON_CHECK_FAIL
		WARN_ON(1);
#endif
	}

}

void print_enabled_clks_once(void)
{
	static bool first_flag = true;

	if (first_flag) {
		first_flag = false;
		print_enabled_clks();
	}
}

static int clkchk_syscore_suspend(void)
{
	check_pll_off();
	return 0;
}

static void clkchk_syscore_resume(void)
{
}

static struct syscore_ops clkchk_syscore_ops = {
	.suspend = clkchk_syscore_suspend,
	.resume = clkchk_syscore_resume,
};

static int __init clkchk_init(void)
{
	if (!of_machine_is_compatible("mediatek,mt8168"))
		return -ENODEV;
	register_syscore_ops(&clkchk_syscore_ops);
	return 0;
}
subsys_initcall(clkchk_init);
