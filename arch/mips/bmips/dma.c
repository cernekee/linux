/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014 Broadcom Corporation
 * Author: Kevin Cernekee <cernekee@gmail.com>
 */

#include <linux/device.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <asm/bmips.h>
#include <asm/cpu-type.h>
#include <dma-coherence.h>

/*
 * BCM338x has configurable address translation windows which allow the
 * peripherals' DMA addresses to be different from the Zephyr-visible
 * physical addresses.  e.g. usb_dma_addr = zephyr_pa ^ 0x08000000
 *
 * If our DT "memory" node has a "dma-xor-mask" property we will enable this
 * translation using the provided offset.
 */
static u32 bcm338x_dma_xor_mask;
static u32 bcm338x_dma_xor_limit = 0xffffffff;

/*
 * PCI collapses the memory hole at 0x10000000 - 0x1fffffff on all
 * supported BMIPS based systems.
 *
 * On systems with a dma-xor-mask, this range is guaranteed to live above
 * the dma-xor-limit.
 */
#define BMIPS_MEM_HOLE_PA	0x10000000
#define BMIPS_MEM_HOLE_SIZE	0x10000000

#define FLUSH_RAC		0x100

static dma_addr_t bmips_phys_to_dma(struct device *dev, phys_addr_t pa)
{
	if (dev && dev_is_pci(dev) &&
	    pa >= (BMIPS_MEM_HOLE_PA + BMIPS_MEM_HOLE_SIZE))
		return pa - BMIPS_MEM_HOLE_SIZE;
	if (pa <= bcm338x_dma_xor_limit)
		return pa ^ bcm338x_dma_xor_mask;
	return pa;
}

dma_addr_t plat_map_dma_mem(struct device *dev, void *addr, size_t size)
{
	return bmips_phys_to_dma(dev, virt_to_phys(addr));
}

dma_addr_t plat_map_dma_mem_page(struct device *dev, struct page *page)
{
	return bmips_phys_to_dma(dev, page_to_phys(page));
}

unsigned long plat_dma_addr_to_phys(struct device *dev, dma_addr_t dma_addr)
{
	if (dev && dev_is_pci(dev) &&
	    dma_addr >= BMIPS_MEM_HOLE_PA)
		return dma_addr + BMIPS_MEM_HOLE_SIZE;
	if ((dma_addr ^ bcm338x_dma_xor_mask) <= bcm338x_dma_xor_limit)
		return dma_addr ^ bcm338x_dma_xor_mask;
	return dma_addr;
}

static int __init bcm338x_init_dma_xor(void)
{
	struct device_node *np = of_find_node_by_type(NULL, "memory");

	if (!np)
		return 0;

	of_property_read_u32(np, "dma-xor-mask", &bcm338x_dma_xor_mask);
	of_property_read_u32(np, "dma-xor-limit", &bcm338x_dma_xor_limit);

	of_node_put(np);
	return 0;
}
arch_initcall(bcm338x_init_dma_xor);

void plat_unmap_dma_mem(struct device *dev, dma_addr_t dma_addr,
	size_t size, enum dma_data_direction dir)
{
	if (dir == DMA_TO_DEVICE)
		return;

	switch (current_cpu_type()) {
	case CPU_BMIPS3300:
	case CPU_BMIPS4350:
	case CPU_BMIPS4380: {
		void __iomem *cbr = BMIPS_GET_CBR();

		/* Flush stale data out of the readahead cache */
		__raw_writel(FLUSH_RAC, cbr + BMIPS_RAC_CONFIG);
		__raw_readl(cbr + BMIPS_RAC_CONFIG);
		break;
	}
	}
}
