/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * Partially based on arch/mips/ralink/irq.c
 *
 * Copyright (C) 2009 Gabor Juhos <juhosg@openwrt.org>
 * Copyright (C) 2013 John Crispin <blogic@openwrt.org>
 * Copyright (C) 2014 Kevin Cernekee <cernekee@gmail.com>
 */

#include <linux/of.h>
#include <linux/irqchip.h>

#include <asm/bmips.h>
#include <asm/irq.h>
#include <asm/irq_cpu.h>
#include <asm/time.h>

unsigned int get_c0_compare_int(void)
{
	return CP0_LEGACY_COMPARE_IRQ;
}

asmlinkage void plat_irq_dispatch(void)
{
	mips_cpu_intc_dispatch();
}

void __init arch_init_irq(void)
{
	bmips_tp1_irqs = 0;
	irqchip_init();
}

OF_DECLARE_2(irqchip, mips_cpu_intc, "mti,cpu-interrupt-controller",
	     mips_cpu_intc_init);
