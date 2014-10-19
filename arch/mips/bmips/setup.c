/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2014 Broadcom Corporation
 * Author: Kevin Cernekee <cernekee@gmail.com>
 */

#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/bootmem.h>
#include <linux/clk-provider.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_platform.h>
#include <linux/smp.h>
#include <asm/addrspace.h>
#include <asm/bmips.h>
#include <asm/bootinfo.h>
#include <asm/cpu-type.h>
#include <asm/mipsregs.h>
#include <asm/prom.h>
#include <asm/smp-ops.h>
#include <asm/time.h>
#include <asm/traps.h>
#include <asm/fw/cfe/cfe_api.h>

#define CMT_LOCAL_TPID		BIT(31)
#define RELO_NORMAL_VEC		BIT(18)

#define REG_DSL_CHIP_ID		((void __iomem *)CKSEG1ADDR(0x10000000))
#define REG_STB_CHIP_ID		((void __iomem *)CKSEG1ADDR(0x10404000))

static const unsigned long kbase = VMLINUX_LOAD_ADDRESS & 0xfff00000;

struct bmips_board_list {
	void			*dtb;
	u32			chip_id;
	const char		*boardname;
};

extern char __dtb_bcm9ejtagprb_begin;
extern char __dtb_bcm97346dbsmb_begin;
extern char __dtb_bcm97425svmb_begin;

static const struct bmips_board_list bmips_board_list[] = {
	{ &__dtb_bcm9ejtagprb_begin,	0x6328, NULL },
	{ &__dtb_bcm97346dbsmb_begin,	0x7346, "BCM97346DBSMB" },
	{ &__dtb_bcm97425svmb_begin,	0x7425, "BCM97425SVMB" },
	{ },
};

static void kbase_setup(void)
{
	__raw_writel(kbase | RELO_NORMAL_VEC,
		     BMIPS_GET_CBR() + BMIPS_RELO_VECTOR_CONTROL_1);
	ebase = kbase;
}

void __init prom_init(void)
{
	register_bmips_smp_ops();

	/*
	 * Some experimental CM boxes are set up to let CM own the Viper TP0
	 * and let Linux own TP1.  This requires moving the kernel
	 * load address to a non-conflicting region (e.g. via
	 * CONFIG_PHYSICAL_START) and supplying an alternate DTB.
	 * If we detect this condition, we need to move the MIPS exception
	 * vectors up to an area that we own.
	 *
	 * This is distinct from the OTHER special case mentioned in
	 * smp-bmips.c (boot on TP1, but enable SMP, then TP0 becomes our
	 * logical CPU#1).  For the Viper TP1 case, SMP is off limits.
	 *
	 * Also note that many BMIPS435x CPUs do not have a
	 * BMIPS_RELO_VECTOR_CONTROL_1 register, so it isn't safe to just
	 * write VMLINUX_LOAD_ADDRESS into that register on every SoC.
	 */
	if (current_cpu_type() == CPU_BMIPS4350 &&
	    kbase != CKSEG0 &&
	    read_c0_brcm_cmt_local() & CMT_LOCAL_TPID) {
		board_ebase_setup = &kbase_setup;
		bmips_smp_enabled = 0;
	}
}

void __init prom_free_prom_memory(void)
{
}

const char *get_system_type(void)
{
	return "BMIPS multiplatform kernel";
}

void __init plat_time_init(void)
{
	struct device_node *np;
	u32 freq;

	np = of_find_node_by_name(NULL, "cpus");
	if (!np)
		panic("missing 'cpus' DT node");
	if (of_property_read_u32(np, "mips-hpt-frequency", &freq) < 0)
		panic("missing 'mips-hpt-frequency' property");
	of_node_put(np);

	mips_hpt_frequency = freq;
}

static void __init *find_dtb(void)
{
	u32 chip_id;
	char boardname[64] = "";
	const struct bmips_board_list *b;

	/* Intended to somewhat resemble ARM; see Documentation/arm/Booting */
	if (fw_arg0 == 0 && fw_arg1 == 0xffffffff)
		return phys_to_virt(fw_arg2);

	if (fw_arg1 != 0 || fw_arg3 != CFE_EPTSEAL ||
	    (fw_arg2 & 0xf0000000) != CKSEG0)
		panic("cannot identify chip");

	/*
	 * Unfortunately the CFE API doesn't seem to provide chip
	 * identification, but we can check the entry point to see whether
	 * the current platform is a DSL chip or STB chip.  On STB,
	 * CAE_STKSIZE = _regidx(13) = 13*8 = 104, so the first instruction is:
	 * 0:	23bdff98	addi	sp,sp,-104
	 */
	if (__raw_readl((void *)fw_arg2) == 0x23bdff98) {
		chip_id = __raw_readl(REG_STB_CHIP_ID);
		cfe_init(fw_arg0, fw_arg2);
		cfe_getenv("CFE_BOARDNAME", boardname, sizeof(boardname));
	} else {
		/*
		 * This works on most modern chips, but will break on older
		 * ones like 6358
		 */
		chip_id = __raw_readl(REG_DSL_CHIP_ID);
	}

	/* 4-digit parts use bits [31:16]; 5-digit parts use [27:8] */
	if (chip_id & 0xf0000000)
		chip_id >>= 16;
	else
		chip_id >>= 8;

	for (b = bmips_board_list; b->dtb; b++) {
		if (b->chip_id != chip_id)
			continue;
		if (b->boardname && strcmp(b->boardname, boardname))
			continue;
		return b->dtb;
	}

	panic("no dtb found for current board");
}

void __init plat_mem_setup(void)
{
	set_io_port_base(0);
	ioport_resource.start = 0;
	ioport_resource.end = ~0;

	__dt_setup_arch(find_dtb());
	strlcpy(arcs_cmdline, boot_command_line, COMMAND_LINE_SIZE);
}

void __init device_tree_init(void)
{
	struct device_node *np;

	unflatten_and_copy_device_tree();

	/* Disable SMP boot unless both CPUs are listed in DT and !disabled */
	np = of_find_node_by_name(NULL, "cpus");
	if (np && of_get_available_child_count(np) <= 1)
		bmips_smp_enabled = 0;
	of_node_put(np);
}

int __init plat_of_setup(void)
{
	return __dt_register_buses("brcm,bmips", "simple-bus");
}

arch_initcall(plat_of_setup);

static int __init plat_dev_init(void)
{
	of_clk_init(NULL);
	return 0;
}

device_initcall(plat_dev_init);
