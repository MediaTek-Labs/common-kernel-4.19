// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019 MediaTek Inc.
 */

#include <linux/clk-provider.h>
#include <linux/platform_device.h>

#include "clk-mtk.h"
#include "clk-gate.h"

#include <dt-bindings/clock/mt8168-clk.h>

static const struct mtk_gate_regs img_cg_regs = {
	.set_ofs = 0x4,
	.clr_ofs = 0x8,
	.sta_ofs = 0x0,
};

#define GATE_IMG(_id, _name, _parent, _shift) {	\
		.id = _id,				\
		.name = _name,				\
		.parent_name = _parent,			\
		.regs = &img_cg_regs,			\
		.shift = _shift,			\
		.ops = &mtk_clk_gate_ops_setclr,	\
	}

static const struct mtk_gate img_clks[] = {
	GATE_IMG(CLK_IMG_LARB2, "img_larb2", "mm_sel", 0),
	GATE_IMG(CLK_IMG_CAM, "img_cam", "mm_sel", 6),
	GATE_IMG(CLK_IMG_CAMTG, "img_camtg", "mm_sel", 7),
	GATE_IMG(CLK_IMG_SENIF, "img_senif", "mm_sel", 8),
	GATE_IMG(CLK_IMG_CAMSV0, "img_camsv0", "mm_sel", 9),
	GATE_IMG(CLK_IMG_CAMSV1, "img_camsv1", "mm_sel", 10),
	GATE_IMG(CLK_IMG_FDVT, "img_fdvt", "mm_sel", 11),
	GATE_IMG(CLK_IMG_CAM_WPE, "img_cam_wpe", "mm_sel", 12),
};

static int clk_mt8168_img_probe(struct platform_device *pdev)
{
	struct clk_onecell_data *clk_data;
	int r;
	struct device_node *node = pdev->dev.of_node;

	clk_data = mtk_alloc_clk_data(CLK_IMG_NR_CLK);

	mtk_clk_register_gates(node, img_clks, ARRAY_SIZE(img_clks), clk_data);

	r = of_clk_add_provider(node, of_clk_src_onecell_get, clk_data);

	if (r)
		pr_err("%s(): could not register clock provider: %d\n",
			__func__, r);

	return r;
}

static const struct of_device_id of_match_clk_mt8168_img[] = {
	{ .compatible = "mediatek,mt8168-imgsys", },
	{}
};

static struct platform_driver clk_mt8168_img_drv = {
	.probe = clk_mt8168_img_probe,
	.driver = {
		.name = "clk-mt8168-img",
		.of_match_table = of_match_clk_mt8168_img,
	},
};

builtin_platform_driver(clk_mt8168_img_drv);
