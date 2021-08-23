#include <linux/kernel.h>
#include <linux/syscalls.h>

#include "tifa-utils.h"

asmlinkage void sys_start_stamp(void)
{
    printk("MikeT:customized starting stamp\n");
}

asmlinkage void sys_changeReadPolicy(int newval)
{
    readPolicy = newval;
    switch (readPolicy) {
    case 0:
        printk("Normal read\n");
        break;
    case 1:
        printk("EBUSY (Random under >=2GCs)\n");
        break;
    case 2:
        printk("GCT (BRT under >=2GCs)\n");
        break;
    case 3:
        printk("IOD3 (TW-only)\n");
        break;
    default:
        printk("Unknown Policy\n");
        break;
    }
}

asmlinkage unsigned long long sys_tifa_bio_ttl(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_ttl;
        break;
    default:
        tifa_bio_ttl = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_ret(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_ret;
        break;
    default:
        tifa_bio_ret = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_rfw(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_rfw;
        break;
    default:
        tifa_bio_rfw = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_gct(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_gct;
        break;
    default:
        tifa_bio_gct = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_gct_ret(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_gct_ret;
        break;
    default:
        tifa_bio_gct_ret = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_rfw_ret(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_rfw_ret;
        break;
    default:
        tifa_bio_rfw_ret = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_com(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_com;
        break;
    default:
        tifa_bio_com = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_gct_nor(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_gct_nor;
        break;
    default:
        tifa_bio_gct_nor = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_gct_eio(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_gct_eio;
        break;
    default:
        tifa_bio_gct_eio = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_rfw_nor(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_rfw_nor;
        break;
    default:
        tifa_bio_rfw_nor = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_rfw_eio(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_rfw_eio;
        break;
    default:
        tifa_bio_rfw_eio = 0;
        break;
    }

    return val;
}

asmlinkage unsigned long long sys_tifa_bio_stripe(int options)
{
    unsigned long long val = 0;

    switch (options) {
    case 1:
        val = tifa_bio_stripe;
        break;
    default:
        tifa_bio_stripe = 0;
        break;
    }

    return val;
}

asmlinkage int sys_tifa_dio_ttl(int options)
{
    int val = 0;

    switch (options) {
    case 1:
        val = atomic_read(&tifa_dio_ttl);
        break;
    default:
        atomic_set(&tifa_dio_ttl, 0);
        break;
    }

    return val;
}

asmlinkage int sys_tifa_dio_gc(int options)
{
    int val = 0;

    switch (options) {
    case 1:
        val = atomic_read(&tifa_dio_gc);
        break;
    default:
        atomic_set(&tifa_dio_gc, 0);
        break;
    }

    return val;
}

asmlinkage int sys_stripe_in_gc_num(int options, int idx)
{
    int val = 0;

    if (options == 1 && 0 <= idx && idx < 5) {
        val = atomic_read(&stripe_in_gc_num[idx]);
    } else if (options == 0 && 0 <= idx && idx < 5) {
        atomic_set(&stripe_in_gc_num[idx], 0);
        printk("Coperd,%s,%d-%d\n", __func__, idx, atomic_read(&stripe_in_gc_num[idx]));
    } else {
        WARN_ON(1);
    }

    return val;
}

asmlinkage int sys_tifa_dio_in_gc_num(int options, int idx)
{
    int val = 0;

    if (options == 1 && 0 <= idx && idx < 5) {
        val = atomic_read(&tifa_dio_in_gc_num[idx]);
    } else if (options == 0 && 0 <= idx && idx < 5) {
        atomic_set(&tifa_dio_in_gc_num[idx], 0);
        printk("Coperd,%s,%d-%d\n", __func__, idx, atomic_read(&tifa_dio_in_gc_num[idx]));
    } else {
        WARN_ON(1);
    }

    return val;
}

