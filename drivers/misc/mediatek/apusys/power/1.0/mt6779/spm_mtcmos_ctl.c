// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include "spm_mtcmos_ctl.h"

static int DBG_ID;
static int DBG_STA;
static int DBG_STEP;

static struct device_node *infracfg_node;
static struct device_node *spm_node;
static struct device_node *infra_node;
static struct device_node *ckgen_node;
static struct device_node *smi_common_node;
static struct device_node *clk_mmsys_config_node;
static struct device_node *clk_apu_conn_node;

void __iomem *infracfg_base;/*infracfg_ao*/
void __iomem *spm_base;
void __iomem *infra_base;/*infracfg*/
void __iomem *ckgen_base;/*ckgen*/
void __iomem *smi_common_base;
void __iomem *clk_mmsys_config_base;
void __iomem *clk_apu_conn_base;

int apusys_spm_mtcmos_init(void)
{
	infracfg_node = of_find_compatible_node(NULL, NULL,
					"mediatek,mt6779-infracfg_ao");
	if (infracfg_node) {
		infracfg_base = of_iomap(infracfg_node, 0);
		if (IS_ERR((void *)infracfg_base)) {
			pr_info("%s Unable to iomap infracfg_base\n", __func__);
			goto err_out;
		} else {
			pr_info("%s infracfg_base = 0x%x\n",
						__func__, infracfg_base);
		}
	}

	spm_node = of_find_compatible_node(NULL, NULL, "mediatek,spm");
	if (spm_node) {
		spm_base = of_iomap(spm_node, 0);
		if (IS_ERR((void *)spm_base)) {
			pr_info("%s Unable to iomap spm_base\n", __func__);
			goto err_out;
		} else {
			pr_info("%s spm_base = 0x%x\n", __func__, spm_base);
		}
	}

	infra_node = of_find_compatible_node(NULL, NULL,
					"mediatek,mt6779-infracfg");
	if (infra_node) {
		infra_base = of_iomap(infra_node, 0);
		if (IS_ERR((void *)infra_base)) {
			pr_info("%s Unable to iomap infra_base\n", __func__);
			goto err_out;
		} else {
			pr_info("%s infra_base = 0x%x\n", __func__, infra_base);
		}
	}

	ckgen_node = of_find_compatible_node(NULL, NULL, "topckgen");
	if (ckgen_node) {
		ckgen_base = of_iomap(ckgen_node, 0);
		if (IS_ERR((void *)ckgen_base)) {
			pr_info("%s Unable to iomap ckgen_base\n", __func__);
			goto err_out;
		} else {
			pr_info("%s ckgen_base = 0x%x\n", __func__, ckgen_base);
		}
	}

	smi_common_node = of_find_compatible_node(NULL, NULL,
				"mediatek,mt6779-smi-common");
	if (smi_common_node) {
		smi_common_base = of_iomap(smi_common_node, 0);
		if (IS_ERR((void *)smi_common_base)) {
			pr_info("%s Unable to iomap smi_common_base\n",
								__func__);
			goto err_out;
		} else {
			pr_info("%s smi_common_base = 0x%x\n",
					__func__, smi_common_base);
		}
	}

	clk_mmsys_config_node = of_find_compatible_node(NULL, NULL,
						"mediatek,mt6779-mmsys");
	if (clk_mmsys_config_node) {
		clk_mmsys_config_base = of_iomap(clk_mmsys_config_node, 0);
		if (IS_ERR((void *)clk_mmsys_config_base)) {
			pr_info("%s Unable to iomap clk_mmsys_config_base\n",
								__func__);
			goto err_out;
		} else {
			pr_info("%s clk_mmsys_config_base = 0x%x\n",
					__func__, clk_mmsys_config_base);
		}
	}

	clk_apu_conn_node = of_find_compatible_node(NULL, NULL,
						"mediatek,mt6779-apu_conn");
	if (clk_apu_conn_node) {
		clk_apu_conn_base = of_iomap(clk_apu_conn_node, 0);
		if (IS_ERR((void *)clk_apu_conn_base)) {
			pr_info("%s Unable to iomap clk_apu_conn_base\n",
								__func__);
			goto err_out;
		} else {
			pr_info("%s clk_apu_conn_base = 0x%x\n",
						__func__, clk_apu_conn_base);
		}
	}

	return 0;

err_out:
	return -1;
}

#ifndef IGNORE_MTCMOS_CHECK
static void ram_console_update(void)
{
	pr_notice("%s wait for mtcmos %d, %d, %d\n",
		__func__, DBG_ID, DBG_STA, DBG_STEP);
	udelay(50);
}
#endif

void enable_mm_clk(void)
{
	clk_writel(MM_CG_CLR0, 0x000003ff);
}

void enable_apu_vcore_clk(void)
{
	/*clk_writel(APU_VCORE_CG_CLR, 0x0000000F);*/
}

int spm_mtcmos_ctrl_vpu_vcore_shut_down(int state)
{
	int err = 0;

	DBG_ID = DBG_ID_VPU_VCORE_SHUTDOWN;
	DBG_STA = state;
	DBG_STEP = 0;

	/* TINFO="enable SPM register control" */

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VPU_VCORE" */
		/* TINFO="VPU_VCORE_SRAM_CON[16]=1"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) | (0x1 << 16));
		/* TINFO="VPU_VCORE_SRAM_CON[17]=1"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) | (0x1 << 17));
		/* TINFO="VPU_VCORE_SRAM_CON[18]=1"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) | (0x1 << 18));
		/* TINFO="VPU_VCORE_SRAM_CON[19]=1"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) | (0x1 << 19));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VPU_VCORE_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND)
			& VPU_VCORE_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Finish to turn off VPU_VCORE" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VPU_VCORE" */
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VPU_VCORE_PWR_STA_MASK)
			!= VPU_VCORE_PWR_STA_MASK)
			|| ((spm_read(PWR_STATUS_2ND)
			& VPU_VCORE_PWR_STA_MASK)
			!= VPU_VCORE_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VPU_VCORE_PWR_CON,
			spm_read(VPU_VCORE_PWR_CON) | PWR_RST_B);
		/* TINFO="VPU_VCORE_SRAM_CON[16]=0"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) & ~(0x1 << 16));
		/* TINFO="VPU_VCORE_SRAM_CON[17]=0"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) & ~(0x1 << 17));
		/* TINFO="VPU_VCORE_SRAM_CON[18]=0"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) & ~(0x1 << 18));
		/* TINFO="VPU_VCORE_SRAM_CON[19]=0"*/
		spm_write(VPU_VCORE_SRAM_CON,
			spm_read(VPU_VCORE_SRAM_CON) & ~(0x1 << 19));
		/* TINFO="Finish to turn on VPU_VCORE" */
		/*enable_apu_vcore_clk();*/
	}

	pr_info("%s %d ret : %d\n", __func__, state, err);
	return err;
}

void enable_apu_conn_clk(void)
{
	clk_writel(APU_CONN_CG_CLR, 0x0000FF);
}

int spm_mtcmos_ctrl_vpu_conn_shut_down(int state)
{
	int err = 0;

	DBG_ID = DBG_ID_VPU_CONN_SHUTDOWN;
	DBG_STA = state;
	DBG_STEP = 0;

	/* TINFO="enable SPM register control" */
	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VPU_CONN" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET,
			VPU_CONN_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1_2)
			& VPU_CONN_PROT_STEP1_0_ACK_MASK)
			!= VPU_CONN_PROT_STEP1_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step1 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MM_SET,
			VPU_CONN_PROT_STEP1_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MM_STA1)
			& VPU_CONN_PROT_STEP1_1_ACK_MASK)
			!= VPU_CONN_PROT_STEP1_1_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step1 : 2" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CONN_PROT_STEP1_2_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CONN_PROT_STEP1_2_ACK_MASK)
			!= VPU_CONN_PROT_STEP1_2_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_SET,
			VPU_CONN_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_STA1_2)
			& VPU_CONN_PROT_STEP2_0_ACK_MASK)
			!= VPU_CONN_PROT_STEP2_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step2 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MM_SET,
			VPU_CONN_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MM_STA1)
			& VPU_CONN_PROT_STEP2_1_ACK_MASK)
			!= VPU_CONN_PROT_STEP2_1_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step3 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CONN_PROT_STEP3_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CONN_PROT_STEP3_0_ACK_MASK)
			!= VPU_CONN_PROT_STEP3_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CONN_SRAM_CON[16]=1"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) | (0x1 << 16));
		/* TINFO="VPU_CONN_SRAM_CON[17]=1"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) | (0x1 << 17));
		/* TINFO="VPU_CONN_SRAM_CON[18]=1"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) | (0x1 << 18));
		/* TINFO="VPU_CONN_SRAM_CON[19]=1"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) | (0x1 << 19));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VPU_CONN_PWR_STA_MASK)
			|| (spm_read(PWR_STATUS_2ND)
			& VPU_CONN_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="EXT_BUCK_ISO[2]=1"*/
		spm_write(EXT_BUCK_ISO,
			spm_read(EXT_BUCK_ISO) | (0x1 << 2));
		/* TINFO="Finish to turn off VPU_CONN" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VPU_CONN" */
		/* TINFO="EXT_BUCK_ISO[2]=0"*/
		spm_write(EXT_BUCK_ISO,
			spm_read(EXT_BUCK_ISO) & ~(0x1 << 2));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VPU_CONN_PWR_STA_MASK)
			!= VPU_CONN_PWR_STA_MASK)
			|| ((spm_read(PWR_STATUS_2ND)
			& VPU_CONN_PWR_STA_MASK)
			!= VPU_CONN_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VPU_CONN_PWR_CON,
			spm_read(VPU_CONN_PWR_CON) | PWR_RST_B);
		/* TINFO="VPU_CONN_SRAM_CON[16]=0"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) & ~(0x1 << 16));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CONN_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(VPU_CONN_SRAM_CON)
			& VPU_CONN_SRAM_PDN_ACK_BIT0) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CONN_SRAM_CON[17]=0"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) & ~(0x1 << 17));
		/* TINFO="VPU_CONN_SRAM_CON[18]=0"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) & ~(0x1 << 18));
		/* TINFO="VPU_CONN_SRAM_CON[19]=0"*/
		spm_write(VPU_CONN_SRAM_CON,
			spm_read(VPU_CONN_SRAM_CON) & ~(0x1 << 19));
		/* TINFO="Release bus protect - step3 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CONN_PROT_STEP3_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR,
			VPU_CONN_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step2 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MM_CLR,
			VPU_CONN_PROT_STEP2_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_2_CLR,
			VPU_CONN_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step1 : 1" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MM_CLR,
			VPU_CONN_PROT_STEP1_1_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step1 : 2" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CONN_PROT_STEP1_2_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Finish to turn on VPU_CONN" */
		enable_apu_vcore_clk();
		enable_apu_conn_clk();
	}

	pr_info("%s %d ret : %d\n", __func__, state, err);
	return err;
}

int spm_mtcmos_ctrl_vpu_core0_shut_down(int state)
{
	int err = 0;

	DBG_ID = DBG_ID_VPU_CORE0_SHUTDOWN;
	DBG_STA = state;
	DBG_STEP = 0;

	/* TINFO="enable SPM register control" */

	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VPU_CORE0" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CORE0_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CORE0_PROT_STEP1_0_ACK_MASK)
			!= VPU_CORE0_PROT_STEP1_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CORE0_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CORE0_PROT_STEP2_0_ACK_MASK)
			!= VPU_CORE0_PROT_STEP2_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE0_SRAM_CON[16]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 16));
		/* TINFO="VPU_CORE0_SRAM_CON[17]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 17));
		/* TINFO="VPU_CORE0_SRAM_CON[18]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 18));
		/* TINFO="VPU_CORE0_SRAM_CON[19]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 19));
		/* TINFO="VPU_CORE0_SRAM_CON[20]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 20));
		/* TINFO="VPU_CORE0_SRAM_CON[21]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 21));
		/* TINFO="VPU_CORE0_SRAM_CON[22]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 22));
		/* TINFO="VPU_CORE0_SRAM_CON[23]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 23));
		/* TINFO="VPU_CORE0_SRAM_CON[24]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 24));
		/* TINFO="VPU_CORE0_SRAM_CON[25]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 25));
		/* TINFO="VPU_CORE0_SRAM_CON[26]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 26));
		/* TINFO="VPU_CORE0_SRAM_CON[27]=1"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) | (0x1 << 27));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VPU_CORE0_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & VPU_CORE0_PWR_STA_MASK)) {
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="EXT_BUCK_ISO[5]=1"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) | (0x1 << 5));
		/* TINFO="Finish to turn off VPU_CORE0" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VPU_CORE0" */
		/* TINFO="EXT_BUCK_ISO[5]=0"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) & ~(0x1 << 5));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VPU_CORE0_PWR_STA_MASK)
			!= VPU_CORE0_PWR_STA_MASK)
			|| ((spm_read(PWR_STATUS_2ND)
			& VPU_CORE0_PWR_STA_MASK)
			!= VPU_CORE0_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VPU_CORE0_PWR_CON,
			spm_read(VPU_CORE0_PWR_CON) | PWR_RST_B);
		/* TINFO="VPU_CORE0_SRAM_CON[16]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 16));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CORE0_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(VPU_CORE0_SRAM_STA)
			& VPU_CORE0_SRAM_PDN_ACK_BIT0) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE0_SRAM_CON[17]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 17));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CORE0_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(VPU_CORE0_SRAM_STA)
			& VPU_CORE0_SRAM_PDN_ACK_BIT1) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE0_SRAM_CON[18]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 18));
		/* TINFO="VPU_CORE0_SRAM_CON[19]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 19));
		/* TINFO="VPU_CORE0_SRAM_CON[20]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 20));
		/* TINFO="VPU_CORE0_SRAM_CON[21]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 21));
		/* TINFO="VPU_CORE0_SRAM_CON[22]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 22));
		/* TINFO="VPU_CORE0_SRAM_CON[23]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 23));
		/* TINFO="VPU_CORE0_SRAM_CON[24]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 24));
		/* TINFO="VPU_CORE0_SRAM_CON[25]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 25));
		/* TINFO="VPU_CORE0_SRAM_CON[26]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 26));
		/* TINFO="VPU_CORE0_SRAM_CON[27]=0"*/
		spm_write(VPU_CORE0_SRAM_CON,
			spm_read(VPU_CORE0_SRAM_CON) & ~(0x1 << 27));
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CORE0_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CORE0_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Finish to turn on VPU_CORE0" */
	}

	pr_info("%s %d ret : %d\n", __func__, state, err);
	return err;
}

int spm_mtcmos_ctrl_vpu_core1_shut_down(int state)
{
	int err = 0;

	DBG_ID = DBG_ID_VPU_CORE1_SHUTDOWN;
	DBG_STA = state;
	DBG_STEP = 0;

	/* TINFO="enable SPM register control" */


	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VPU_CORE1" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CORE1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CORE1_PROT_STEP1_0_ACK_MASK)
			!= VPU_CORE1_PROT_STEP1_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CORE1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CORE1_PROT_STEP2_0_ACK_MASK)
			!= VPU_CORE1_PROT_STEP2_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE1_SRAM_CON[16]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 16));
		/* TINFO="VPU_CORE1_SRAM_CON[17]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 17));
		/* TINFO="VPU_CORE1_SRAM_CON[18]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 18));
		/* TINFO="VPU_CORE1_SRAM_CON[19]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 19));
		/* TINFO="VPU_CORE1_SRAM_CON[20]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 20));
		/* TINFO="VPU_CORE1_SRAM_CON[21]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 21));
		/* TINFO="VPU_CORE1_SRAM_CON[22]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 22));
		/* TINFO="VPU_CORE1_SRAM_CON[23]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 23));
		/* TINFO="VPU_CORE1_SRAM_CON[24]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 24));
		/* TINFO="VPU_CORE1_SRAM_CON[25]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 25));
		/* TINFO="VPU_CORE1_SRAM_CON[26]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 26));
		/* TINFO="VPU_CORE1_SRAM_CON[27]=1"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) | (0x1 << 27));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VPU_CORE1_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & VPU_CORE1_PWR_STA_MASK)) {
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="EXT_BUCK_ISO[6]=1"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) | (0x1 << 6));
		/* TINFO="Finish to turn off VPU_CORE1" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VPU_CORE1" */
		/* TINFO="EXT_BUCK_ISO[6]=0"*/
		spm_write(EXT_BUCK_ISO,
			spm_read(EXT_BUCK_ISO) & ~(0x1 << 6));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VPU_CORE1_PWR_STA_MASK)
			!= VPU_CORE1_PWR_STA_MASK)
			|| ((spm_read(PWR_STATUS_2ND)
			& VPU_CORE1_PWR_STA_MASK)
			!= VPU_CORE1_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VPU_CORE1_PWR_CON,
			spm_read(VPU_CORE1_PWR_CON) | PWR_RST_B);
		/* TINFO="VPU_CORE1_SRAM_CON[16]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 16));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CORE1_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(VPU_CORE1_SRAM_STA)
			& VPU_CORE1_SRAM_PDN_ACK_BIT0) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE1_SRAM_CON[17]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 17));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CORE1_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(VPU_CORE1_SRAM_STA)
			& VPU_CORE1_SRAM_PDN_ACK_BIT1) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE1_SRAM_CON[18]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 18));
		/* TINFO="VPU_CORE1_SRAM_CON[19]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 19));
		/* TINFO="VPU_CORE1_SRAM_CON[20]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 20));
		/* TINFO="VPU_CORE1_SRAM_CON[21]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 21));
		/* TINFO="VPU_CORE1_SRAM_CON[22]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 22));
		/* TINFO="VPU_CORE1_SRAM_CON[23]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 23));
		/* TINFO="VPU_CORE1_SRAM_CON[24]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 24));
		/* TINFO="VPU_CORE1_SRAM_CON[25]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 25));
		/* TINFO="VPU_CORE1_SRAM_CON[26]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 26));
		/* TINFO="VPU_CORE1_SRAM_CON[27]=0"*/
		spm_write(VPU_CORE1_SRAM_CON,
			spm_read(VPU_CORE1_SRAM_CON) & ~(0x1 << 27));
		/* TINFO="Release bus protect - step2 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CORE1_PROT_STEP2_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CORE1_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Finish to turn on VPU_CORE1" */
	}

	pr_info("%s %d ret : %d\n", __func__, state, err);
	return err;
}

int spm_mtcmos_ctrl_vpu_core2_shut_down(int state)
{
	int err = 0;

	DBG_ID = DBG_ID_VPU_CORE2_SHUTDOWN;
	DBG_STA = state;
	DBG_STEP = 0;

	/* TINFO="enable SPM register control" */


	if (state == STA_POWER_DOWN) {
		/* TINFO="Start to turn off VPU_CORE2" */
		/* TINFO="Set bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_SET,
			VPU_CORE2_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
		while ((spm_read(INFRA_TOPAXI_PROTECTEN_MCU_STA1)
			& VPU_CORE2_PROT_STEP1_0_ACK_MASK)
			!= VPU_CORE2_PROT_STEP1_0_ACK_MASK)
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE2_SRAM_CON[16]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 16));
		/* TINFO="VPU_CORE2_SRAM_CON[17]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 17));
		/* TINFO="VPU_CORE2_SRAM_CON[18]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 18));
		/* TINFO="VPU_CORE2_SRAM_CON[19]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 19));
		/* TINFO="VPU_CORE2_SRAM_CON[20]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 20));
		/* TINFO="VPU_CORE2_SRAM_CON[21]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 21));
		/* TINFO="VPU_CORE2_SRAM_CON[22]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 22));
		/* TINFO="VPU_CORE2_SRAM_CON[23]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 23));
		/* TINFO="VPU_CORE2_SRAM_CON[24]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 24));
		/* TINFO="VPU_CORE2_SRAM_CON[25]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 25));
		/* TINFO="VPU_CORE2_SRAM_CON[26]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 26));
		/* TINFO="VPU_CORE2_SRAM_CON[27]=1"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) | (0x1 << 27));
		/* TINFO="Set PWR_ISO = 1" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) | PWR_ISO);
		/* TINFO="Set PWR_CLK_DIS = 1" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) | PWR_CLK_DIS);
		/* TINFO="Set PWR_RST_B = 0" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) & ~PWR_RST_B);
		/* TINFO="Set PWR_ON = 0" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) & ~PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 0" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) & ~PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 0 and PWR_STATUS_2ND = 0" */
		while ((spm_read(PWR_STATUS) & VPU_CORE2_PWR_STA_MASK)
		       || (spm_read(PWR_STATUS_2ND) & VPU_CORE2_PWR_STA_MASK)) {
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="EXT_BUCK_ISO[7]=1"*/
		spm_write(EXT_BUCK_ISO, spm_read(EXT_BUCK_ISO) | (0x1 << 7));
		/* TINFO="Finish to turn off VPU_CORE2" */
	} else {    /* STA_POWER_ON */
		/* TINFO="Start to turn on VPU_CORE2" */
		/* TINFO="EXT_BUCK_ISO[7]=0"*/
		spm_write(EXT_BUCK_ISO,
			spm_read(EXT_BUCK_ISO) & ~(0x1 << 7));
		/* TINFO="Set PWR_ON = 1" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) | PWR_ON);
		/* TINFO="Set PWR_ON_2ND = 1" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) | PWR_ON_2ND);
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until PWR_STATUS = 1 and PWR_STATUS_2ND = 1" */
		while (((spm_read(PWR_STATUS) & VPU_CORE2_PWR_STA_MASK)
			!= VPU_CORE2_PWR_STA_MASK)
			|| ((spm_read(PWR_STATUS_2ND)
			& VPU_CORE2_PWR_STA_MASK)
			!= VPU_CORE2_PWR_STA_MASK))
			ram_console_update();
		INCREASE_STEPS;
#endif
		/* TINFO="Set PWR_CLK_DIS = 0" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) & ~PWR_CLK_DIS);
		/* TINFO="Set PWR_ISO = 0" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) & ~PWR_ISO);
		/* TINFO="Set PWR_RST_B = 1" */
		spm_write(VPU_CORE2_PWR_CON,
			spm_read(VPU_CORE2_PWR_CON) | PWR_RST_B);
		/* TINFO="VPU_CORE2_SRAM_CON[16]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 16));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CORE2_SRAM_PDN_ACK_BIT0 = 0" */
		while (spm_read(VPU_CORE2_SRAM_STA)
			& VPU_CORE2_SRAM_PDN_ACK_BIT0) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE2_SRAM_CON[17]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 17));
#ifndef IGNORE_MTCMOS_CHECK
		/* TINFO="Wait until VPU_CORE2_SRAM_PDN_ACK_BIT1 = 0" */
		while (spm_read(VPU_CORE2_SRAM_STA)
			& VPU_CORE2_SRAM_PDN_ACK_BIT1) {
		/* Need f_fsmi_ck for SRAM PDN delay IP */
			ram_console_update();
		}
		INCREASE_STEPS;
#endif
		/* TINFO="VPU_CORE2_SRAM_CON[18]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 18));
		/* TINFO="VPU_CORE2_SRAM_CON[19]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 19));
		/* TINFO="VPU_CORE2_SRAM_CON[20]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 20));
		/* TINFO="VPU_CORE2_SRAM_CON[21]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 21));
		/* TINFO="VPU_CORE2_SRAM_CON[22]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 22));
		/* TINFO="VPU_CORE2_SRAM_CON[23]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 23));
		/* TINFO="VPU_CORE2_SRAM_CON[24]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 24));
		/* TINFO="VPU_CORE2_SRAM_CON[25]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 25));
		/* TINFO="VPU_CORE2_SRAM_CON[26]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 26));
		/* TINFO="VPU_CORE2_SRAM_CON[27]=0"*/
		spm_write(VPU_CORE2_SRAM_CON,
			spm_read(VPU_CORE2_SRAM_CON) & ~(0x1 << 27));
		/* TINFO="Release bus protect - step1 : 0" */
		spm_write(INFRA_TOPAXI_PROTECTEN_MCU_CLR,
			VPU_CORE2_PROT_STEP1_0_MASK);
#ifndef IGNORE_MTCMOS_CHECK
#endif
		/* TINFO="Finish to turn on VPU_CORE2" */
	}

	pr_info("%s %d ret : %d\n", __func__, state, err);
	return err;
}

#define APU_VCORE_BASE (0x19020000)
#define APU_VCORE_REG_SIZE (0x1000)
#define VPU0_BASE (0x19180000)
#define VPU0_REG_SIZE (0x1000)
#define VPU1_BASE (0x19280000)
#define VPU1_REG_SIZE (0x1000)
#define MDLA_BASE (0x19380000)
#define MDLA_REG_SIZE (0x1000)
void debug_reg(void)
{
	void __iomem *apu_vcore = NULL;
	void __iomem *vpu0 = NULL;
	void __iomem *vpu1 = NULL;
	void __iomem *mdla = NULL;
	uint32_t reg_val = 0x0;

	apu_vcore = ioremap_nocache(APU_VCORE_BASE, APU_VCORE_REG_SIZE);
	vpu0 = ioremap_nocache(VPU0_BASE, VPU0_REG_SIZE);
	vpu1 = ioremap_nocache(VPU1_BASE, VPU1_REG_SIZE);
	mdla = ioremap_nocache(MDLA_BASE, MDLA_REG_SIZE);

	/* apu_vcore read */
	reg_val = ioread32(apu_vcore + 0x44);
	pr_info("%s APUSYS_VCORE_GSM_APC_STS3 = 0x%08x\n", __func__, reg_val);

	reg_val = ioread32(apu_vcore + 0x48);
	pr_info("%s APUSYS_VCORE_MDOM_CTRL = 0x%08x\n", __func__, reg_val);

	/* vpu core0 write */
	reg_val = ioread32(vpu0 + 0x1F8);
	pr_info("%s CORE_XTENSA_ALTRESETVEC = 0x%08x\n", __func__, reg_val);
	iowrite32(0x1234, vpu0 + 0x1F8);
	reg_val = ioread32(vpu0 + 0x1F8);
	pr_info("%s CORE_XTENSA_ALTRESETVEC = 0x%08x\n", __func__, reg_val);

	/* vpu core0 read */
	reg_val = ioread32(vpu0 + 0x40);
	pr_info("%s vpu0 CORE_SRAM_DELSEL_0 = 0x%08x\n", __func__, reg_val);

	/* vpu core1 read */
	reg_val = ioread32(vpu1 + 0x44);
	pr_info("%s vpu1 CORE_SRAM_DELSEL_1 = 0x%08x\n", __func__, reg_val);

	/* mdla read */
	reg_val = ioread32(mdla + 0x60);
	pr_info("%s mdla MDLA_SRAM_DELSEL0 = 0x%08x\n", __func__, reg_val);

	iounmap(apu_vcore);
	iounmap(vpu0);
	iounmap(vpu1);
	iounmap(mdla);
}
