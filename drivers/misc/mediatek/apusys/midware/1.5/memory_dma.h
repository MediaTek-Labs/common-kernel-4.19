/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_MEMORY_DMA_H__
#define __APUSYS_MEMORY_DMA_H__

int dma_mem_alloc(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int dma_mem_free(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int dma_mem_init(struct apusys_mem_mgr *mem_mgr);
int dma_mem_destroy(struct apusys_mem_mgr *mem_mgr);

#endif
