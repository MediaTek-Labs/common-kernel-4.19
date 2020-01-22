/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef __APUSYS_MEMORY_ION_H__
#define __APUSYS_MEMORY_ION_H__

#include "apusys_device.h"

#define APUSYS_ION_PAGE_SIZE (0x1000)

int ion_mem_alloc(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_free(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_import(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_unimport(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_init(struct apusys_mem_mgr *mem_mgr);
int ion_mem_destroy(struct apusys_mem_mgr *mem_mgr);
int ion_mem_map_iova(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_map_kva(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_unmap_iova(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_unmap_kva(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_ctl(struct apusys_mem_mgr *mem_mgr,
		struct apusys_mem_ctl *ctl_data, struct apusys_kmem *mem);
int ion_mem_flush(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
int ion_mem_invalidate(struct apusys_mem_mgr *mem_mgr, struct apusys_kmem *mem);
#endif

