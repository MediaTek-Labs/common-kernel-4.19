// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018-2019 MediaTek Inc.

/*
 * Driver for MediaTek Command-Queue DMA Controller
 *
 * Author: Shun-Chih Yu <shun-chih.yu@mediatek.com>
 *
 */

#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/iopoll.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_dma.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/preempt.h>
#include <linux/refcount.h>
#include <linux/slab.h>

#include "../virt-dma.h"

#define MTK_CQDMA_USEC_POLL		10
#define MTK_CQDMA_TIMEOUT_POLL		1000
#define MTK_CQDMA_DMA_BUSWIDTHS		BIT(DMA_SLAVE_BUSWIDTH_4_BYTES)
#define MTK_CQDMA_ALIGN_SIZE		1

/* The default number of virtual channel */
#define MTK_CQDMA_NR_VCHANS		32

/* The default number of physical channel */
#define MTK_CQDMA_NR_PCHANS		3

/* Registers for underlying dma manipulation */
#define MTK_CQDMA_INT_FLAG		0x0
#define MTK_CQDMA_INT_EN		0x4
#define MTK_CQDMA_EN			0x8
#define MTK_CQDMA_RESET			0xc
#define MTK_CQDMA_FLUSH			0x14
#define MTK_CQDMA_SRC			0x1c
#define MTK_CQDMA_DST			0x20
#define MTK_CQDMA_LEN1			0x24
#define MTK_CQDMA_SRC2			0x60
#define MTK_CQDMA_DST2			0x64

/* Registers setting */
#define MTK_CQDMA_EN_BIT		BIT(0)
#define MTK_CQDMA_INT_FLAG_BIT		BIT(0)
#define MTK_CQDMA_INT_EN_BIT		BIT(0)
#define MTK_CQDMA_FLUSH_BIT		BIT(0)

#define MTK_CQDMA_WARM_RST_BIT		BIT(0)
#define MTK_CQDMA_HARD_RST_BIT		BIT(1)

#define MTK_CQDMA_MAX_LEN		GENMASK(27, 0)
#define MTK_CQDMA_ADDR_LIMIT		GENMASK(31, 0)
#define MTK_CQDMA_ADDR2_SHFIT		(32)

/**
 * struct mtk_cqdma_vdesc - The struct holding info describing virtual
 *                         descriptor (CVD)
 * @vd:                    An instance for struct virt_dma_desc
 * @len:                   The total data size device wants to move
 * @dest:                  The destination address device wants to move to
 * @src:                   The source address device wants to move from
 * @ch:                    The pointer to the corresponding dma channel
 */
struct mtk_cqdma_vdesc {
	struct virt_dma_desc vd;
	size_t len;
	dma_addr_t dest;
	dma_addr_t src;
	struct dma_chan *ch;
};

/**
 * struct mtk_cqdma_pchan - The struct holding info describing physical
 *                         channel (PC)
 * @active_vdesc:          The pointer to the CVD which is under processing
 * @base:                  The mapped register I/O base of this PC
 * @irq:                   The IRQ that this PC are using
 * @refcnt:                Track how many VCs are using this PC
 * @lock:                  Lock protect agaisting multiple VCs access PC
 */
struct mtk_cqdma_pchan {
	struct mtk_cqdma_vdesc *active_vdesc;
	void __iomem *base;
	u32 irq;
	refcount_t refcnt;
	spinlock_t lock;
};

/**
 * struct mtk_cqdma_vchan - The struct holding info describing virtual
 *                         channel (VC)
 * @vc:                    An instance for struct virt_dma_chan
 * @pc:                    The pointer to the underlying PC
 */
struct mtk_cqdma_vchan {
	struct virt_dma_chan vc;
	struct mtk_cqdma_pchan *pc;
};

/**
 * struct mtk_cqdma_device - The struct holding info describing CQDMA
 *                          device
 * @ddev:                   An instance for struct dma_device
 * @clk:                    The clock that device internal is using
 * @dma_requests:           The number of VCs the device supports to
 * @dma_channels:           The number of PCs the device supports to
 * @dma_mask:               A mask for DMA capability
 * @vc:                     The pointer to all available VCs
 * @pc:                     The pointer to all the underlying PCs
 */
struct mtk_cqdma_device {
	struct dma_device ddev;
	struct clk *clk;

	u32 dma_requests;
	u32 dma_channels;
	u32 dma_mask;
	struct mtk_cqdma_vchan *vc;
	struct mtk_cqdma_pchan **pc;
};

static struct mtk_cqdma_device *to_cqdma_dev(struct dma_chan *chan)
{
	return container_of(chan->device, struct mtk_cqdma_device, ddev);
}

static struct mtk_cqdma_vchan *to_cqdma_vchan(struct dma_chan *chan)
{
	return container_of(chan, struct mtk_cqdma_vchan, vc.chan);
}

static struct mtk_cqdma_vdesc *to_cqdma_vdesc(struct virt_dma_desc *vd)
{
	return container_of(vd, struct mtk_cqdma_vdesc, vd);
}

static struct device *cqdma2dev(struct mtk_cqdma_device *cqdma)
{
	return cqdma->ddev.dev;
}

static u32 mtk_dma_read(struct mtk_cqdma_pchan *pc, u32 reg)
{
	return readl_relaxed(pc->base + reg);
}

static void mtk_dma_write(struct mtk_cqdma_pchan *pc, u32 reg, u32 val)
{
	writel_relaxed(val, pc->base + reg);
}

static void mtk_dma_rmw(struct mtk_cqdma_pchan *pc, u32 reg,
			u32 mask, u32 set)
{
	u32 val;

	val = mtk_dma_read(pc, reg);
	val &= ~mask;
	val |= set;
	mtk_dma_write(pc, reg, val);
}

static void mtk_dma_set(struct mtk_cqdma_pchan *pc, u32 reg, u32 val)
{
	mtk_dma_rmw(pc, reg, 0, val);
}

static void mtk_dma_clr(struct mtk_cqdma_pchan *pc, u32 reg, u32 val)
{
	mtk_dma_rmw(pc, reg, val, 0);
}

static void mtk_cqdma_vdesc_free(struct virt_dma_desc *vd)
{
	kfree(to_cqdma_vdesc(vd));
}

static int mtk_cqdma_poll_engine_done(struct mtk_cqdma_pchan *pc)
{
	u32 status = 0;

	if (!in_task())
		return readl_poll_timeout(pc->base + MTK_CQDMA_EN,
					  status,
					  !(status & MTK_CQDMA_EN_BIT),
					  MTK_CQDMA_USEC_POLL,
					  MTK_CQDMA_TIMEOUT_POLL);
	else
		return readl_poll_timeout_atomic(pc->base + MTK_CQDMA_EN,
						 status,
						 !(status & MTK_CQDMA_EN_BIT),
						 MTK_CQDMA_USEC_POLL,
						 MTK_CQDMA_TIMEOUT_POLL);
}

static int mtk_cqdma_hard_reset(struct mtk_cqdma_pchan *pc)
{
	mtk_dma_set(pc, MTK_CQDMA_RESET, MTK_CQDMA_HARD_RST_BIT);
	mtk_dma_clr(pc, MTK_CQDMA_RESET, MTK_CQDMA_HARD_RST_BIT);

	return mtk_cqdma_poll_engine_done(pc);
}

static void mtk_cqdma_start(struct mtk_cqdma_pchan *pc,
			    struct mtk_cqdma_vdesc *cvd)
{
	/* warm reset the dma engine for the new transaction */
	mtk_dma_set(pc, MTK_CQDMA_RESET, MTK_CQDMA_WARM_RST_BIT);
	if (mtk_cqdma_poll_engine_done(pc) < 0)
		dev_err(cqdma2dev(to_cqdma_dev(cvd->ch)),
			"cqdma warm reset timeout\n");

	/* setup the source */
	mtk_dma_set(pc, MTK_CQDMA_SRC, cvd->src & MTK_CQDMA_ADDR_LIMIT);
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	mtk_dma_set(pc, MTK_CQDMA_SRC2, cvd->src >> MTK_CQDMA_ADDR2_SHFIT);
#else
	mtk_dma_set(pc, MTK_CQDMA_SRC2, 0);
#endif

	/* setup the destination */
	mtk_dma_set(pc, MTK_CQDMA_DST, cvd->dest & MTK_CQDMA_ADDR_LIMIT);
#ifdef CONFIG_ARCH_DMA_ADDR_T_64BIT
	mtk_dma_set(pc, MTK_CQDMA_DST2, cvd->dest >> MTK_CQDMA_ADDR2_SHFIT);
#else
	mtk_dma_set(pc, MTK_CQDMA_DST2, 0);
#endif

	/* setup the length */
	mtk_dma_set(pc, MTK_CQDMA_LEN1, (cvd->len < MTK_CQDMA_MAX_LEN) ?
		    cvd->len : MTK_CQDMA_MAX_LEN);

	/* start dma engine */
	mtk_dma_set(pc, MTK_CQDMA_EN, MTK_CQDMA_EN_BIT);
}

static void mtk_cqdma_issue_vchan_pending(struct mtk_cqdma_vchan *cvc)
{
	struct virt_dma_desc *vd;
	struct mtk_cqdma_pchan *pc = cvc->pc;

	lockdep_assert_held(&cvc->vc.lock);
	lockdep_assert_held(&pc->lock);

	vd = vchan_next_desc(&cvc->vc);

	if (vd && !pc->active_vdesc) {
		pc->active_vdesc = to_cqdma_vdesc(vd);
		mtk_cqdma_start(pc, pc->active_vdesc);
	}
}

/*
 * return true if this VC is active,
 * meaning that there are VDs under processing by the PC
 */
static bool mtk_cqdma_is_vchan_active(struct mtk_cqdma_vchan *cvc)
{
	return (!cvc->pc->active_vdesc) ? false :
	       (cvc == to_cqdma_vchan(cvc->pc->active_vdesc->ch));
}

static void mtk_cqdma_complete_vdesc(struct mtk_cqdma_pchan *pc)
{
	struct mtk_cqdma_vchan *cvc;
	struct mtk_cqdma_vdesc *cvd;
	struct virt_dma_desc *vd;
	size_t tlen;

	cvd = pc->active_vdesc;
	cvc = to_cqdma_vchan(cvd->ch);

	tlen = (cvd->len < MTK_CQDMA_MAX_LEN) ? cvd->len : MTK_CQDMA_MAX_LEN;
	cvd->len -= tlen;
	cvd->src += tlen;
	cvd->dest += tlen;

	spin_lock(&cvc->vc.lock);

	/* check whether the VD completed */
	if (!cvd->len) {
		/* delete VD from desc_issued */
		list_del(&cvd->vd.node);

		/* add the VD into list desc_completed */
		vchan_cookie_complete(&cvd->vd);

		/* get the next active VD */
		vd = vchan_next_desc(&cvc->vc);
		pc->active_vdesc = (!vd) ? NULL : to_cqdma_vdesc(vd);
	}

	/* start the next transaction */
	if (pc->active_vdesc)
		mtk_cqdma_start(pc, pc->active_vdesc);

	spin_unlock(&cvc->vc.lock);
}

static irqreturn_t mtk_cqdma_irq(int irq, void *devid)
{
	struct mtk_cqdma_device *cqdma = devid;
	irqreturn_t ret = IRQ_NONE;
	u32 i;

	/* clear interrupt flags for each PC */
	for (i = 0; i < cqdma->dma_channels; ++i) {
		spin_lock(&cqdma->pc[i]->lock);
		if (mtk_dma_read(cqdma->pc[i],
				 MTK_CQDMA_INT_FLAG) & MTK_CQDMA_INT_FLAG_BIT) {
			/* clear interrupt */
			mtk_dma_clr(cqdma->pc[i], MTK_CQDMA_INT_FLAG,
				    MTK_CQDMA_INT_FLAG_BIT);

			mtk_cqdma_complete_vdesc(cqdma->pc[i]);

			ret = IRQ_HANDLED;
		}
		spin_unlock(&cqdma->pc[i]->lock);
	}

	return ret;
}

static enum dma_status mtk_cqdma_tx_status(struct dma_chan *c,
					   dma_cookie_t cookie,
					   struct dma_tx_state *txstate)
{
	return dma_cookie_status(c, cookie, txstate);
}

static void mtk_cqdma_issue_pending(struct dma_chan *c)
{
	struct mtk_cqdma_vchan *cvc = to_cqdma_vchan(c);
	unsigned long pc_flags;
	unsigned long vc_flags;

	/* acquire PC's lock before VC's lock for lock dependency in ISR */
	spin_lock_irqsave(&cvc->pc->lock, pc_flags);
	spin_lock_irqsave(&cvc->vc.lock, vc_flags);

	if (vchan_issue_pending(&cvc->vc))
		mtk_cqdma_issue_vchan_pending(cvc);

	spin_unlock_irqrestore(&cvc->vc.lock, vc_flags);
	spin_unlock_irqrestore(&cvc->pc->lock, pc_flags);
}

static struct dma_async_tx_descriptor *
mtk_cqdma_prep_dma_memcpy(struct dma_chan *c, dma_addr_t dest,
			  dma_addr_t src, size_t len, unsigned long flags)
{
	struct mtk_cqdma_vdesc *cvd;

	cvd = kzalloc(sizeof(*cvd), GFP_NOWAIT);
	if (!cvd)
		return NULL;

	/* setup dma channel */
	cvd->ch = c;

	/* setup sourece, destination, and length */
	cvd->len = len;
	cvd->src = src;
	cvd->dest = dest;

	return vchan_tx_prep(to_virt_chan(c), &cvd->vd, flags);
}

static int mtk_cqdma_terminate_all(struct dma_chan *c)
{
	struct mtk_cqdma_vchan *cvc = to_cqdma_vchan(c);
	struct virt_dma_chan *vc = to_virt_chan(c);
	unsigned long pc_flags;
	unsigned long vc_flags;
	LIST_HEAD(head);

	do {
		/* acquire PC's lock first due to lock dependency in dma ISR */
		spin_lock_irqsave(&cvc->pc->lock, pc_flags);
		spin_lock_irqsave(&cvc->vc.lock, vc_flags);

		/* wait for the VC to be inactive  */
		if (mtk_cqdma_is_vchan_active(cvc)) {
			spin_unlock_irqrestore(&cvc->vc.lock, vc_flags);
			spin_unlock_irqrestore(&cvc->pc->lock, pc_flags);
			continue;
		}

		/* get VDs from lists */
		vchan_get_all_descriptors(vc, &head);

		/* free all the VDs */
		vchan_dma_desc_free_list(vc, &head);

		spin_unlock_irqrestore(&cvc->vc.lock, vc_flags);
		spin_unlock_irqrestore(&cvc->pc->lock, pc_flags);

		break;
	} while (1);

	vchan_synchronize(&cvc->vc);

	return 0;
}

static int mtk_cqdma_alloc_chan_resources(struct dma_chan *c)
{
	struct mtk_cqdma_device *cqdma = to_cqdma_dev(c);
	struct mtk_cqdma_vchan *vc = to_cqdma_vchan(c);
	struct mtk_cqdma_pchan *pc = NULL;
	u32 i, min_refcnt = U32_MAX, refcnt;
	unsigned long flags;

	/* allocate PC with the minimum refcount */
	for (i = 0; i < cqdma->dma_channels; ++i) {
		refcnt = refcount_read(&cqdma->pc[i]->refcnt);
		if (refcnt < min_refcnt) {
			pc = cqdma->pc[i];
			min_refcnt = refcnt;
		}
	}

	if (!pc)
		return -ENOSPC;

	spin_lock_irqsave(&pc->lock, flags);

	if (!refcount_read(&pc->refcnt)) {
		/* allocate PC when the refcount is zero */
		mtk_cqdma_hard_reset(pc);

		/* enable interrupt for this PC */
		mtk_dma_set(pc, MTK_CQDMA_INT_EN, MTK_CQDMA_INT_EN_BIT);

		/*
		 * refcount_inc would complain increment on 0; use-after-free.
		 * Thus, we need to explicitly set it as 1 initially.
		 */
		refcount_set(&pc->refcnt, 1);
	} else {
		refcount_inc(&pc->refcnt);
	}

	spin_unlock_irqrestore(&pc->lock, flags);

	vc->pc = pc;

	return 0;
}

static void mtk_cqdma_free_chan_resources(struct dma_chan *c)
{
	struct mtk_cqdma_vchan *cvc = to_cqdma_vchan(c);
	unsigned long flags;

	/* free all descriptors in all lists on the VC */
	mtk_cqdma_terminate_all(c);

	spin_lock_irqsave(&cvc->pc->lock, flags);

	/* PC is not freed until there is no VC mapped to it */
	if (refcount_dec_and_test(&cvc->pc->refcnt)) {
		/* start the flush operation and stop the engine */
		mtk_dma_set(cvc->pc, MTK_CQDMA_FLUSH, MTK_CQDMA_FLUSH_BIT);

		/* wait for the completion of flush operation */
		if (mtk_cqdma_poll_engine_done(cvc->pc) < 0)
			dev_err(cqdma2dev(to_cqdma_dev(c)),
				"cqdma flush timeout\n");

		/* clear the flush bit and interrupt flag */
		mtk_dma_clr(cvc->pc, MTK_CQDMA_FLUSH, MTK_CQDMA_FLUSH_BIT);
		mtk_dma_clr(cvc->pc, MTK_CQDMA_INT_FLAG,
			    MTK_CQDMA_INT_FLAG_BIT);

		/* disable interrupt for this PC */
		mtk_dma_clr(cvc->pc, MTK_CQDMA_INT_EN, MTK_CQDMA_INT_EN_BIT);
	}

	spin_unlock_irqrestore(&cvc->pc->lock, flags);
}

static int mtk_cqdma_hw_init(struct mtk_cqdma_device *cqdma)
{
	unsigned long flags;
	int err;
	u32 i;

	pm_runtime_enable(cqdma2dev(cqdma));
	pm_runtime_get_sync(cqdma2dev(cqdma));

	err = clk_prepare_enable(cqdma->clk);

	if (err) {
		pm_runtime_put_sync(cqdma2dev(cqdma));
		pm_runtime_disable(cqdma2dev(cqdma));
		return err;
	}

	/* reset all PCs */
	for (i = 0; i < cqdma->dma_channels; ++i) {
		spin_lock_irqsave(&cqdma->pc[i]->lock, flags);
		if (mtk_cqdma_hard_reset(cqdma->pc[i]) < 0) {
			dev_err(cqdma2dev(cqdma), "cqdma hard reset timeout\n");
			spin_unlock_irqrestore(&cqdma->pc[i]->lock, flags);

			clk_disable_unprepare(cqdma->clk);
			pm_runtime_put_sync(cqdma2dev(cqdma));
			pm_runtime_disable(cqdma2dev(cqdma));
			return -EINVAL;
		}
		spin_unlock_irqrestore(&cqdma->pc[i]->lock, flags);
	}

	return 0;
}

static void mtk_cqdma_hw_deinit(struct mtk_cqdma_device *cqdma)
{
	unsigned long flags;
	u32 i;

	/* reset all PCs */
	for (i = 0; i < cqdma->dma_channels; ++i) {
		spin_lock_irqsave(&cqdma->pc[i]->lock, flags);
		if (mtk_cqdma_hard_reset(cqdma->pc[i]) < 0)
			dev_err(cqdma2dev(cqdma), "cqdma hard reset timeout\n");
		spin_unlock_irqrestore(&cqdma->pc[i]->lock, flags);
	}

	clk_disable_unprepare(cqdma->clk);

	pm_runtime_put_sync(cqdma2dev(cqdma));
	pm_runtime_disable(cqdma2dev(cqdma));
}

static const struct of_device_id mtk_cqdma_match[] = {
	{ .compatible = "mediatek,cqdma" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mtk_cqdma_match);

static u64 cqdma_dmamask;
static int mtk_cqdma_probe(struct platform_device *pdev)
{
	struct mtk_cqdma_device *cqdma;
	struct mtk_cqdma_vchan *vc;
	struct dma_device *dd;
	struct resource *res;
	int err;
	u32 i;

	cqdma = devm_kzalloc(&pdev->dev, sizeof(*cqdma), GFP_KERNEL);
	if (!cqdma)
		return -ENOMEM;

	dd = &cqdma->ddev;

	cqdma->clk = devm_clk_get(&pdev->dev, "cqdma");
	if (IS_ERR(cqdma->clk)) {
		dev_err(&pdev->dev, "No clock for %s\n",
			dev_name(&pdev->dev));
		return PTR_ERR(cqdma->clk);
	}

	dma_cap_set(DMA_MEMCPY, dd->cap_mask);

	dd->copy_align = MTK_CQDMA_ALIGN_SIZE;
	dd->device_alloc_chan_resources = mtk_cqdma_alloc_chan_resources;
	dd->device_free_chan_resources = mtk_cqdma_free_chan_resources;
	dd->device_tx_status = mtk_cqdma_tx_status;
	dd->device_issue_pending = mtk_cqdma_issue_pending;
	dd->device_prep_dma_memcpy = mtk_cqdma_prep_dma_memcpy;
	dd->device_terminate_all = mtk_cqdma_terminate_all;
	dd->src_addr_widths = MTK_CQDMA_DMA_BUSWIDTHS;
	dd->dst_addr_widths = MTK_CQDMA_DMA_BUSWIDTHS;
	dd->directions = BIT(DMA_MEM_TO_MEM);
	dd->residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;
	dd->dev = &pdev->dev;
	INIT_LIST_HEAD(&dd->channels);

	if (pdev->dev.of_node && of_property_read_u32(pdev->dev.of_node,
						      "dma-requests",
						      &cqdma->dma_requests)) {
		dev_dbg(&pdev->dev,
			"Using %u as missing dma-requests property\n",
			MTK_CQDMA_NR_VCHANS);

		cqdma->dma_requests = MTK_CQDMA_NR_VCHANS;
	}

	if (pdev->dev.of_node && of_property_read_u32(pdev->dev.of_node,
						      "dma-channels",
						      &cqdma->dma_channels)) {
		dev_dbg(&pdev->dev,
			"Using %u as missing dma-channels property\n",
			MTK_CQDMA_NR_PCHANS);

		cqdma->dma_channels = MTK_CQDMA_NR_PCHANS;
	}

	if (pdev->dev.of_node && of_property_read_u32(pdev->dev.of_node,
						      "dma-mask",
						      &cqdma->dma_mask)) {
		dev_dbg(&pdev->dev,
			"Using 0 as missing dma-mask property\n");
	} else {
		cqdma_dmamask = DMA_BIT_MASK(cqdma->dma_mask);
		pdev->dev.dma_mask = &cqdma_dmamask;
	}

	cqdma->pc = devm_kcalloc(&pdev->dev, cqdma->dma_channels,
				 sizeof(*cqdma->pc), GFP_KERNEL);
	if (!cqdma->pc)
		return -ENOMEM;

	/* initialization for PCs */
	for (i = 0; i < cqdma->dma_channels; ++i) {
		cqdma->pc[i] = devm_kcalloc(&pdev->dev, 1,
					    sizeof(**cqdma->pc), GFP_KERNEL);
		if (!cqdma->pc[i])
			return -ENOMEM;

		cqdma->pc[i]->active_vdesc = NULL;
		spin_lock_init(&cqdma->pc[i]->lock);
		refcount_set(&cqdma->pc[i]->refcnt, 0);

		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (!res) {
			dev_err(&pdev->dev, "No mem resource for %s\n",
				dev_name(&pdev->dev));
			return -EINVAL;
		}

		cqdma->pc[i]->base = devm_ioremap_resource(&pdev->dev, res);
		if (IS_ERR(cqdma->pc[i]->base))
			return PTR_ERR(cqdma->pc[i]->base);

		/* allocate IRQ resource */
		res = platform_get_resource(pdev, IORESOURCE_IRQ, i);
		if (!res) {
			dev_err(&pdev->dev, "No irq resource for %s\n",
				dev_name(&pdev->dev));
			return -EINVAL;
		}
		cqdma->pc[i]->irq = res->start;

		err = devm_request_irq(&pdev->dev, cqdma->pc[i]->irq,
				       mtk_cqdma_irq, 0, dev_name(&pdev->dev),
				       cqdma);
		if (err) {
			dev_err(&pdev->dev,
				"request_irq failed with err %d\n", err);
			return -EINVAL;
		}
	}

	/* allocate resource for VCs */
	cqdma->vc = devm_kcalloc(&pdev->dev, cqdma->dma_requests,
				 sizeof(*cqdma->vc), GFP_KERNEL);
	if (!cqdma->vc)
		return -ENOMEM;

	for (i = 0; i < cqdma->dma_requests; i++) {
		vc = &cqdma->vc[i];
		vc->vc.desc_free = mtk_cqdma_vdesc_free;
		vchan_init(&vc->vc, dd);
	}

	err = dma_async_device_register(dd);
	if (err)
		return err;

	err = of_dma_controller_register(pdev->dev.of_node,
					 of_dma_xlate_by_chan_id, cqdma);
	if (err) {
		dev_err(&pdev->dev,
			"MediaTek CQDMA OF registration failed %d\n", err);
		goto err_unregister;
	}

	err = mtk_cqdma_hw_init(cqdma);
	if (err) {
		dev_err(&pdev->dev,
			"MediaTek CQDMA HW initialization failed %d\n", err);
		goto err_unregister;
	}

	platform_set_drvdata(pdev, cqdma);

	dev_dbg(&pdev->dev, "MediaTek CQDMA driver registered\n");

	return 0;

err_unregister:
	dma_async_device_unregister(dd);

	return err;
}

static int mtk_cqdma_remove(struct platform_device *pdev)
{
	struct mtk_cqdma_device *cqdma = platform_get_drvdata(pdev);
	struct mtk_cqdma_vchan *vc;
	unsigned long flags;
	int i;

	/* kill VC task */
	for (i = 0; i < cqdma->dma_requests; i++) {
		vc = &cqdma->vc[i];

		list_del(&vc->vc.chan.device_node);
		tasklet_kill(&vc->vc.task);
	}

	/* disable interrupt */
	for (i = 0; i < cqdma->dma_channels; i++) {
		spin_lock_irqsave(&cqdma->pc[i]->lock, flags);
		mtk_dma_clr(cqdma->pc[i], MTK_CQDMA_INT_EN,
			    MTK_CQDMA_INT_EN_BIT);
		spin_unlock_irqrestore(&cqdma->pc[i]->lock, flags);

		/* Waits for any pending IRQ handlers to complete */
		synchronize_irq(cqdma->pc[i]->irq);
	}

	/* disable hardware */
	mtk_cqdma_hw_deinit(cqdma);

	dma_async_device_unregister(&cqdma->ddev);
	of_dma_controller_free(pdev->dev.of_node);

	return 0;
}

static struct platform_driver mtk_cqdma_driver = {
	.probe = mtk_cqdma_probe,
	.remove = mtk_cqdma_remove,
	.driver = {
		.name           = KBUILD_MODNAME,
		.of_match_table = mtk_cqdma_match,
	},
};
module_platform_driver(mtk_cqdma_driver);

MODULE_DESCRIPTION("MediaTek CQDMA Controller Driver");
MODULE_AUTHOR("Shun-Chih Yu <shun-chih.yu@mediatek.com>");
MODULE_LICENSE("GPL v2");
