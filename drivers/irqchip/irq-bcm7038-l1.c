/*
 * Broadcom BCM7038 style Level 1 interrupt controller driver
 *
 * Copyright (C) 2014 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt)	KBUILD_MODNAME	": " fmt

#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kconfig.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irqdomain.h>
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/irqchip/chained_irq.h>

#include "irqchip.h"

#define IRQS_PER_WORD		32
#define REG_BYTES_PER_IRQ_WORD	(sizeof(u32) * 4)

struct bcm7038_l1_intc_data {
	unsigned int n_words;
	void __iomem *map_base;
	struct irq_domain *domain;
	bool can_wake;
};

/*
 * STATUS/MASK_STATUS/MASK_SET/MASK_CLEAR are packed one right after another, e.g.
 *
 * 7038:
 *   0x1000_1400: W0_STATUS
 *   0x1000_1404: W1_STATUS
 *   0x1000_1408: W0_MASK_STATUS
 *   0x1000_140c: W1_MASK_STATUS
 *   0x1000_1410: W0_MASK_SET
 *   0x1000_1414: W1_MASK_SET
 *   0x1000_1418: W0_MASK_CLEAR
 *   0x1000_141c: W1_MASK_CLEAR
 *
 * 7445:
 *   0xf03e_1500: W0_STATUS
 *   0xf03e_1504: W1_STATUS
 *   0xf03e_1508: W2_STATUS
 *   0xf03e_150c: W3_STATUS
 *   0xf03e_1510: W4_STATUS
 *   0xf03e_1514: W0_MASK_STATUS
 *   0xf03e_1518: W1_MASK_STATUS
 *   [...]
 */

static inline unsigned int reg_status(struct bcm7038_l1_intc_data *b,
				      unsigned int word)
{
	return (0 * b->n_words + word) * sizeof(u32);
}

static inline unsigned int reg_mask_status(struct bcm7038_l1_intc_data *b,
					   unsigned int word)
{
	return (1 * b->n_words + word) * sizeof(u32);
}

static inline unsigned int reg_mask_set(struct bcm7038_l1_intc_data *b,
					unsigned int word)
{
	return (2 * b->n_words + word) * sizeof(u32);
}

static inline unsigned int reg_mask_clear(struct bcm7038_l1_intc_data *b,
					  unsigned int word)
{
	return (3 * b->n_words + word) * sizeof(u32);
}

static void bcm7038_l1_intc_irq_handle(unsigned int irq, struct irq_desc *desc)
{
	struct bcm7038_l1_intc_data *b = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);
	unsigned int idx;

	chained_irq_enter(chip, desc);

	for (idx = 0; idx < b->n_words; idx++) {
		int base = idx * IRQS_PER_WORD;
		struct irq_chip_generic *gc =
			irq_get_domain_generic_chip(b->domain, base);
		unsigned long pending;
		int hwirq;

		irq_gc_lock(gc);
		pending = irq_reg_readl(gc, reg_status(b, idx)) &
					    gc->mask_cache;
		irq_gc_unlock(gc);

		for_each_set_bit(hwirq, &pending, IRQS_PER_WORD) {
			generic_handle_irq(irq_find_mapping(b->domain,
					   base + hwirq));
		}
	}

	chained_irq_exit(chip, desc);
}

static void bcm7038_l1_intc_suspend(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);
	struct bcm7038_l1_intc_data *b = gc->private;

	irq_gc_lock(gc);
	if (b->can_wake)
		irq_reg_writel(gc, gc->mask_cache | gc->wake_active,
			       ct->regs.enable);
	irq_gc_unlock(gc);
}

static void bcm7038_l1_intc_resume(struct irq_data *d)
{
	struct irq_chip_generic *gc = irq_data_get_irq_chip_data(d);
	struct irq_chip_type *ct = irq_data_get_chip_type(d);

	/* Restore the saved mask */
	irq_gc_lock(gc);
	irq_reg_writel(gc, gc->mask_cache, ct->regs.enable);
	irq_gc_unlock(gc);
}

static int __init bcm7038_l1_intc_map_regs(struct device_node *dn,
					   struct bcm7038_l1_intc_data *data)
{
	struct resource res;
	resource_size_t sz;

	if (of_address_to_resource(dn, 0, &res))
		return -EINVAL;
	sz = resource_size(&res);
	data->map_base = ioremap(res.start, sz);
	if (!data->map_base)
		return -ENOMEM;

	data->n_words = sz / REG_BYTES_PER_IRQ_WORD;
	if (data->n_words == 0)
		return -EINVAL;

	return 0;
}

int __init bcm7038_l1_intc_of_init(struct device_node *dn,
					struct device_node *parent)
{
	unsigned int clr = IRQ_NOREQUEST | IRQ_NOPROBE | IRQ_NOAUTOEN;
	struct bcm7038_l1_intc_data *data;
	struct irq_chip_generic *gc;
	struct irq_chip_type *ct;
	int parent_irq, ret;
	unsigned int idx, irq, flags;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	ret = bcm7038_l1_intc_map_regs(dn, data);
	if (ret < 0) {
		pr_err("failed to remap intc L1 registers\n");
		goto out_free;
	}

	for (idx = 0; idx < data->n_words; idx++) {
		__raw_writel(0xffffffff,
			     data->map_base + reg_mask_set(data, idx));
	}

	parent_irq = irq_of_parse_and_map(dn, 0);
	if (parent_irq < 0) {
		pr_err("failed to map parent interrupt %d\n", parent_irq);
		return parent_irq;
	}
	irq_set_handler_data(parent_irq, data);
	irq_set_chained_handler(parent_irq, bcm7038_l1_intc_irq_handle);

	data->domain = irq_domain_add_linear(dn, IRQS_PER_WORD * data->n_words,
					     &irq_generic_chip_ops, NULL);
	if (!data->domain) {
		ret = -ENOMEM;
		goto out_unmap;
	}

	/* MIPS chips strapped for BE will automagically configure the
	 * peripheral registers for CPU-native byte order.
	 */
	flags = 0;
	if (IS_ENABLED(CONFIG_MIPS) && IS_ENABLED(CONFIG_CPU_BIG_ENDIAN))
		flags |= IRQ_GC_BE_IO;

	ret = irq_alloc_domain_generic_chips(data->domain, IRQS_PER_WORD, 1,
				dn->full_name, handle_level_irq, clr, 0, flags);
	if (ret) {
		pr_err("failed to allocate generic irq chip\n");
		goto out_free_domain;
	}

	if (of_property_read_bool(dn, "brcm,irq-can-wake"))
		data->can_wake = true;

	for (idx = 0; idx < data->n_words; idx++) {
		irq = idx * IRQS_PER_WORD;
		gc = irq_get_domain_generic_chip(data->domain, irq);

		gc->private = data;
		ct = gc->chip_types;

		gc->reg_base = data->map_base;
		ct->regs.disable = reg_mask_set(data, idx);
		ct->regs.enable = reg_mask_clear(data, idx);

		ct->chip.irq_mask = irq_gc_mask_disable_reg;
		ct->chip.irq_unmask = irq_gc_unmask_enable_reg;
		ct->chip.irq_ack = irq_gc_noop;
		ct->chip.irq_suspend = bcm7038_l1_intc_suspend;
		ct->chip.irq_resume = bcm7038_l1_intc_resume;

		if (data->can_wake) {
			/* This IRQ chip can wake the system, set all
			 * relevant child interupts in wake_enabled mask
			 */
			gc->wake_enabled = 0xffffffff;
			ct->chip.irq_set_wake = irq_gc_set_wake;
		}
	}

	pr_info("registered BCM7038 L1 intc (mem: 0x%p, parent IRQ: %d)\n",
		data->map_base, parent_irq);

	return 0;

out_free_domain:
	irq_domain_remove(data->domain);
out_unmap:
	iounmap(data->map_base);
out_free:
	kfree(data);
	return ret;
}
IRQCHIP_DECLARE(bcm7038_l1_intc, "brcm,bcm7038-l1-intc",
		bcm7038_l1_intc_of_init);
