/*
 * vpmu.h: PMU virtualization for HVM domain.
 *
 * Copyright (c) 2007, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307 USA.
 *
 * Author: Haitao Shan <haitao.shan@intel.com>
 */

#ifndef __ASM_X86_HVM_VPMU_H_
#define __ASM_X86_HVM_VPMU_H_

/*
 * Flag bits given as a string on the hypervisor boot parameter 'vpmu'.
 * See arch/x86/hvm/vpmu.c.
 */
#define VPMU_BOOT_ENABLED 0x1    /* vpmu generally enabled. */
#define VPMU_BOOT_BTS     0x2    /* Intel BTS feature wanted. */


#define msraddr_to_bitpos(x) (((x)&0xffff) + ((x)>>31)*0x2000)
#define vcpu_vpmu(vcpu)   (&((vcpu)->arch.hvm_vcpu.vpmu))
#define vpmu_vcpu(vpmu)   (container_of((vpmu), struct vcpu, \
                                          arch.hvm_vcpu.vpmu))
#define vpmu_domain(vpmu) (vpmu_vcpu(vpmu)->domain)

#define MSR_TYPE_COUNTER            0
#define MSR_TYPE_CTRL               1
#define MSR_TYPE_GLOBAL             2
#define MSR_TYPE_ARCH_COUNTER       3
#define MSR_TYPE_ARCH_CTRL          4


/*<VT> add*
 *debug store structure
 */
/* DEBUGCTLMSR bits (others vary by model): */
#define DEBUGCTLMSR_LBR			(1UL <<  0) /* last branch recording */
#define DEBUGCTLMSR_BTF			(1UL <<  1) /* single-step on branches */
#define DEBUGCTLMSR_TR			(1UL <<  6)
#define DEBUGCTLMSR_BTS			(1UL <<  7)
#define DEBUGCTLMSR_BTINT		(1UL <<  8)
#define DEBUGCTLMSR_BTS_OFF_OS		(1UL <<  9)
#define DEBUGCTLMSR_BTS_OFF_USR		(1UL << 10)
#define DEBUGCTLMSR_FREEZE_LBRS_ON_PMI	(1UL << 11)
#define DEBUGCTLMSR_FREEZE_PERFMON_ON_PMI	(1UL << 12)
#define DEBUGCTLMSR_UNCRE_PMI_EN	(1UL << 13)
#define DEBUGCTLMSR_FREEZE_WHILE_SMM_EN	(1UL << 14)

/* Arch specific operations shared by all vpmus */
struct arch_vpmu_ops {
    int (*do_wrmsr)(unsigned int msr, uint64_t msr_content);
    int (*do_rdmsr)(unsigned int msr, uint64_t *msr_content);
    int (*do_interrupt)(struct cpu_user_regs *regs);
    void (*do_cpuid)(unsigned int input,
                     unsigned int *eax, unsigned int *ebx,
                     unsigned int *ecx, unsigned int *edx);
    void (*arch_vpmu_destroy)(struct vcpu *v);
    void (*arch_vpmu_save)(struct vcpu *v);
    void (*arch_vpmu_load)(struct vcpu *v);
};

int vmx_vpmu_initialise(struct vcpu *, unsigned int flags);
int svm_vpmu_initialise(struct vcpu *, unsigned int flags);

struct vpmu_struct {
    u32 flags;
    void *context;
    struct arch_vpmu_ops *arch_vpmu_ops;

	/*<VT> add*/
	int host_domID;
	unsigned long host_cr3;
	unsigned long guest_ds_addr;
	unsigned long host_ds_addr;
	unsigned long guest_bts_base;
	unsigned long host_bts_base;
	u32 bts_size_order;
	u32 bts_enable;
};

#define VPMU_CONTEXT_ALLOCATED              0x1
#define VPMU_CONTEXT_LOADED                 0x2
#define VPMU_RUNNING                        0x4
#define VPMU_PASSIVE_DOMAIN_ALLOCATED       0x8
#define VPMU_CPU_HAS_DS                     0x10 /* Has Debug Store */
#define VPMU_CPU_HAS_BTS                    0x20 /* Has Branch Trace Store */


#define vpmu_set(_vpmu, _x)    ((_vpmu)->flags |= (_x))
#define vpmu_reset(_vpmu, _x)  ((_vpmu)->flags &= ~(_x))
#define vpmu_is_set(_vpmu, _x) ((_vpmu)->flags & (_x))
#define vpmu_clear(_vpmu)      ((_vpmu)->flags = 0)

int vpmu_do_wrmsr(unsigned int msr, uint64_t msr_content);
int vpmu_do_rdmsr(unsigned int msr, uint64_t *msr_content);
int vpmu_do_interrupt(struct cpu_user_regs *regs);
void vpmu_do_cpuid(unsigned int input, unsigned int *eax, unsigned int *ebx,
                                       unsigned int *ecx, unsigned int *edx);
void vpmu_initialise(struct vcpu *v);
void vpmu_destroy(struct vcpu *v);
void vpmu_save(struct vcpu *v);
void vpmu_load(struct vcpu *v);

extern int acquire_pmu_ownership(int pmu_ownership);
extern void release_pmu_ownership(int pmu_ownership);

#endif /* __ASM_X86_HVM_VPMU_H_*/

