/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MT6779_SPM_MTCMOS_CTL_H_
#define _MT6779_SPM_MTCMOS_CTL_H_

#include <linux/io.h>

//#define IGNORE_MTCMOS_CHECK

#ifdef CONFIG_ARM64
#define IOMEM(a)        ((void __force __iomem *)((a)))
#endif

#define mt_reg_sync_writel(v, a) \
	do { \
		__raw_writel((v), IOMEM(a)); \
		/* sync up */ \
		mb(); } \
		while (0)

#define spm_read(addr)                  __raw_readl(IOMEM(addr))
#define spm_write(addr, val)            mt_reg_sync_writel(val, addr)

#define clk_writel(addr, val)   \
	mt_reg_sync_writel(val, addr)

#define clk_readl(addr)                 __raw_readl(IOMEM(addr))

extern void __iomem *infracfg_base;/*infracfg_ao*/
extern void __iomem *spm_base;
extern void __iomem *infra_base;/*infracfg*/
extern void __iomem *ckgen_base;/*ckgen*/
extern void __iomem *smi_common_base;
extern void __iomem *clk_mmsys_config_base;
extern void __iomem *clk_apu_conn_base;

#define INFRACFG_REG(offset)	(infracfg_base + offset)
#define SPM_REG(offset)		(spm_base + offset)
#define INFRA_REG(offset)	(infra_base + offset)
#define CKGEN_REG(offset)	(ckgen_base + offset)
#define SMI_COMMON_REG(offset)	(smi_common_base + offset)

/* Define MTCMOS power control */
#define PWR_RST_B                        (0x1 << 0)
#define PWR_ISO                          (0x1 << 1)
#define PWR_ON                           (0x1 << 2)
#define PWR_ON_2ND                       (0x1 << 3)
#define PWR_CLK_DIS                      (0x1 << 4)

#define DBG_ID_DIS			2
#define DBG_ID_VPU_VCORE_SHUTDOWN	14
#define DBG_ID_VPU_CONN_SHUTDOWN	16
#define DBG_ID_VPU_CORE0_SHUTDOWN	18
#define DBG_ID_VPU_CORE1_SHUTDOWN	20
#define DBG_ID_VPU_CORE2_SHUTDOWN	22

#define ID_MADK   0xFF000000
#define STA_MASK  0x00F00000
#define STEP_MASK 0x000000FF

#define INCREASE_STEPS \
	do { DBG_STEP++; } while (0)

#define STA_POWER_DOWN	0
#define STA_POWER_ON	1
#define SUBSYS_PWR_DOWN	0
#define SUBSYS_PWR_ON	1

/*
 * SPM related reg
 */
#define POWERON_CONFIG_EN       SPM_REG(0x0000)
#define PWR_STATUS              SPM_REG(0x0160)
#define PWR_STATUS_2ND          SPM_REG(0x0164)

#define DIS_PWR_CON		SPM_REG(0x30C)
#define VPU_VCORE_PWR_CON       SPM_REG(0x33C)
#define VPU_CONN_PWR_CON        SPM_REG(0x340)
#define VPU_CORE0_PWR_CON       SPM_REG(0x344)
#define VPU_CORE1_PWR_CON       SPM_REG(0x348)
#define VPU_CORE2_PWR_CON       SPM_REG(0x34C)

#define VPU_VCORE_SRAM_CON      SPM_REG(0x384)
#define VPU_CONN_SRAM_CON       SPM_REG(0x388)
#define VPU_CORE0_SRAM_CON      SPM_REG(0x38C)
#define VPU_CORE1_SRAM_CON      SPM_REG(0x390)
#define VPU_CORE2_SRAM_CON      SPM_REG(0x394)

#define VPU_CORE0_SRAM_STA      SPM_REG(0x398)
#define VPU_CORE1_SRAM_STA      SPM_REG(0x39C)
#define VPU_CORE2_SRAM_STA      SPM_REG(0x3A0)

#define EXT_BUCK_ISO            SPM_REG(0x3B4)

/*
 * VPU PWR MASK
 */
#define VPU_VCORE_PWR_STA_MASK           (0x1 << 26)
#define VPU_CONN_PWR_STA_MASK            (0x1 << 27)
#define VPU_CORE0_PWR_STA_MASK           (0x1 << 28)
#define VPU_CORE1_PWR_STA_MASK           (0x1 << 29)
#define VPU_CORE2_PWR_STA_MASK           (0x1 << 30)

#define VPU_CORE0_SRAM_PDN_ACK_BIT0      (0x1 << 16)
#define VPU_CORE1_SRAM_PDN_ACK_BIT0      (0x1 << 16)
#define VPU_CORE2_SRAM_PDN_ACK_BIT0      (0x1 << 16)
#define VPU_CORE0_SRAM_PDN_ACK_BIT1      (0x1 << 17)
#define VPU_CORE2_SRAM_PDN_ACK_BIT1      (0x1 << 17)
#define VPU_CORE1_SRAM_PDN_ACK_BIT1      (0x1 << 17)
#define VPU_CONN_SRAM_PDN_ACK_BIT0       (0x1 << 28)

#define VPU_CONN_PROT_STEP1_0_MASK       ((0x1 << 8))
#define VPU_CONN_PROT_STEP1_0_ACK_MASK   ((0x1 << 8))
#define VPU_CONN_PROT_STEP1_1_MASK       ((0x1 << 8) \
		|(0x1 << 9) \
		|(0x1 << 12))
#define VPU_CONN_PROT_STEP1_1_ACK_MASK   ((0x1 << 8) \
		|(0x1 << 9) \
		|(0x1 << 12))
#define VPU_CONN_PROT_STEP1_2_MASK       ((0x1 << 12) \
		|(0x1 << 13))
#define VPU_CONN_PROT_STEP1_2_ACK_MASK   ((0x1 << 12) \
		|(0x1 << 13))
#define VPU_CONN_PROT_STEP2_0_MASK       ((0x1 << 0) \
		|(0x1 << 1) \
		|(0x1 << 6))
#define VPU_CONN_PROT_STEP2_0_ACK_MASK   ((0x1 << 0) \
		|(0x1 << 1) \
		|(0x1 << 6))
#define VPU_CONN_PROT_STEP2_1_MASK       ((0x1 << 10) \
		|(0x1 << 11))
#define VPU_CONN_PROT_STEP2_1_ACK_MASK   ((0x1 << 10) \
		|(0x1 << 11))
#define VPU_CONN_PROT_STEP3_0_MASK       ((0x1 << 11))
#define VPU_CONN_PROT_STEP3_0_ACK_MASK   ((0x1 << 11))
#define VPU_CORE0_PROT_STEP1_0_MASK      ((0x1 << 6))
#define VPU_CORE0_PROT_STEP1_0_ACK_MASK   ((0x1 << 6))
#define VPU_CORE0_PROT_STEP2_0_MASK      ((0x1 << 0) \
		|(0x1 << 2) \
		|(0x1 << 4) \
		|(0x1 << 14))
#define VPU_CORE0_PROT_STEP2_0_ACK_MASK   ((0x1 << 0) \
		|(0x1 << 2) \
		|(0x1 << 4) \
		|(0x1 << 14))
#define VPU_CORE1_PROT_STEP1_0_MASK      ((0x1 << 7))
#define VPU_CORE1_PROT_STEP1_0_ACK_MASK   ((0x1 << 7))
#define VPU_CORE1_PROT_STEP2_0_MASK      ((0x1 << 1) \
		|(0x1 << 3) \
		|(0x1 << 5) \
		|(0x1 << 15))
#define VPU_CORE1_PROT_STEP2_0_ACK_MASK   ((0x1 << 1) \
		|(0x1 << 3) \
		|(0x1 << 5) \
		|(0x1 << 15))
#define VPU_CORE2_PROT_STEP1_0_MASK      ((0x1 << 8) \
		|(0x1 << 9) \
		|(0x1 << 10))
#define VPU_CORE2_PROT_STEP1_0_ACK_MASK   ((0x1 << 8) \
		|(0x1 << 9) \
		|(0x1 << 10))

/*
 * Non-CPU SRAM Mask
 */
#define DIS_SRAM_PDN                     (0x1 << 8)
#define DIS_SRAM_PDN_ACK                 (0x1 << 12)
#define DIS_SRAM_PDN_ACK_BIT0            (0x1 << 12)

/*
 * DIS related mask
 */
#define DIS_PWR_STA_MASK                 (0x1 << 3)
#define DIS_PROT_STEP1_0_MASK            ((0x1 << 0) \
		|(0x1 << 1) \
		|(0x1 << 3) \
		|(0x1 << 4) \
		|(0x1 << 5) \
		|(0x1 << 6))
#define DIS_PROT_STEP1_0_ACK_MASK        ((0x1 << 0) \
		|(0x1 << 1) \
		|(0x1 << 3) \
		|(0x1 << 4) \
		|(0x1 << 5) \
		|(0x1 << 6))
#define DIS_PROT_STEP2_0_MASK            ((0x1 << 0) \
		|(0x1 << 1) \
		|(0x1 << 2) \
		|(0x1 << 3) \
		|(0x1 << 4) \
		|(0x1 << 5) \
		|(0x1 << 6) \
		|(0x1 << 7))
#define DIS_PROT_STEP2_0_ACK_MASK        ((0x1 << 0) \
		|(0x1 << 1) \
		|(0x1 << 2) \
		|(0x1 << 3) \
		|(0x1 << 4) \
		|(0x1 << 5) \
		|(0x1 << 6) \
		|(0x1 << 7))
#define DIS_PROT_STEP3_0_MASK            ((0x1 << 16) \
		|(0x1 << 17))
#define DIS_PROT_STEP3_0_ACK_MASK        ((0x1 << 16) \
		|(0x1 << 17))
#define DIS_PROT_STEP4_0_MASK            ((0x1 << 10) \
		|(0x1 << 11))
#define DIS_PROT_STEP4_0_ACK_MASK        ((0x1 << 10) \
		|(0x1 << 11))
#define DIS_PROT_STEP5_0_MASK            ((0x1 << 6))
#define DIS_PROT_STEP5_0_ACK_MASK        ((0x1 << 6))

/*
 * INFRACFG related reg
 */
#define INFRA_TOPAXI_SI2_CTL		INFRACFG_REG(0x0234)
#define INFRA_TOPAXI_SI0_CTL            INFRACFG_REG(0x0200)
#define INFRA_TOPAXI_SI0_CTL_SET        INFRACFG_REG(0x03B8)
#define INFRA_TOPAXI_SI0_CTL_CLR        INFRACFG_REG(0x03BC)

#define INFRA_TOPAXI_PROTECTEN_MCU_SET  INFRACFG_REG(0x02C4)
#define INFRA_TOPAXI_PROTECTEN_MCU_CLR  INFRACFG_REG(0x02C8)
#define INFRA_TOPAXI_PROTECTEN_MCU_STA0 INFRACFG_REG(0x02E0)
#define INFRA_TOPAXI_PROTECTEN_MCU_STA1 INFRACFG_REG(0x02E4)

#define INFRA_TOPAXI_PROTECTEN          INFRACFG_REG(0x0220)
#define INFRA_TOPAXI_PROTECTEN_STA0     INFRACFG_REG(0x0224)
#define INFRA_TOPAXI_PROTECTEN_STA1     INFRACFG_REG(0x0228)
#define INFRA_TOPAXI_PROTECTEN_STA1_1	INFRACFG_REG(0x0258)

#define INFRA_TOPAXI_PROTECTEN_SET      INFRACFG_REG(0x02A0)
#define INFRA_TOPAXI_PROTECTEN_CLR      INFRACFG_REG(0x02A4)
#define INFRA_TOPAXI_PROTECTEN_1_SET    INFRACFG_REG(0x02A8)
#define INFRA_TOPAXI_PROTECTEN_1_CLR    INFRACFG_REG(0x02AC)

#define INFRA_TOPAXI_PROTECTEN_2_SET    INFRACFG_REG(0x0714)
#define INFRA_TOPAXI_PROTECTEN_2_CLR    INFRACFG_REG(0x0718)
#define INFRA_TOPAXI_PROTECTEN_STA1_2   INFRACFG_REG(0x0724)

#define INFRA_TOPAXI_PROTECTEN_MM       INFRACFG_REG(0x02D0)
#define INFRA_TOPAXI_PROTECTEN_MM_SET   INFRACFG_REG(0x02D4)
#define INFRA_TOPAXI_PROTECTEN_MM_CLR   INFRACFG_REG(0x02D8)
#define INFRA_TOPAXI_PROTECTEN_MM_STA0  INFRACFG_REG(0x02E8)
#define INFRA_TOPAXI_PROTECTEN_MM_STA1  INFRACFG_REG(0x02EC)

/*
 * INFRA related reg
 */
#define INFRA_TOPAXI_SI0_STA		INFRA_REG(0x0000)
#define INFRA_TOPAXI_SI2_STA		INFRA_REG(0x0028)

/*
 * SMI related reg
 */
#define SMI_COMMON_SMI_CLAMP		SMI_COMMON_REG(0x03C0)
#define SMI_COMMON_SMI_CLAMP_SET        SMI_COMMON_REG(0x03C4)
#define SMI_COMMON_SMI_CLAMP_CLR        SMI_COMMON_REG(0x03C8)

/*
 * MM CG
 */
#define MM_CG_CLR0 (clk_mmsys_config_base + 0x108)

/*
 * APU CG
 */
#define APU_CONN_CG_CON		(clk_apu_conn_base)
#define APU_CONN_CG_CLR		(clk_apu_conn_base + 0x0008)

enum apusys_mtcmos_id {
	VPU_VCORE = 0,
	VPU_CONN,
	VPU_CORE0,
	VPU_CORE1,
	VPU_CORE2, // mdla
	APUSYS_MTCMOS_NR,
};

enum apusys_cg_id {
	VPU_VCORE_CG = 0,
	APUSYS_CG_NR,
};

int apusys_spm_mtcmos_init(void);
int atf_vcore_cg_ctl(int state);
int spm_mtcmos_ctrl_vpu_vcore_shut_down(int state);
int spm_mtcmos_ctrl_vpu_conn_shut_down(int state);
int spm_mtcmos_ctrl_vpu_core0_shut_down(int state);
int spm_mtcmos_ctrl_vpu_core1_shut_down(int state);
int spm_mtcmos_ctrl_vpu_core2_shut_down(int state);
void debug_reg(void);

extern int mm_dis_cnt;

#endif //_MT6779_SPM_MTCMOS_CTL_H_
