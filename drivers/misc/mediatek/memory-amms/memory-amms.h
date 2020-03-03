/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _MEMORY_AMMS_H_
#define _MEMORY_AMMS_H_
int free_reserved_memory(phys_addr_t start_phys,
		phys_addr_t end_phys);
void *vmap_reserved_mem(phys_addr_t start, phys_addr_t size, pgprot_t prot);
#endif /* end of _MEMORY_AMMS_H_ */
