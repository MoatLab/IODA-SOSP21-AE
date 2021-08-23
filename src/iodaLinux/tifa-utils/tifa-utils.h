#include <linux/kernel.h>
#include <linux/ktime.h>

int readPolicy = 0;
EXPORT_SYMBOL(readPolicy);

unsigned long long tifa_bio_ttl; //Total number of bios sent in RAID
EXPORT_SYMBOL(tifa_bio_ttl);
unsigned long long tifa_bio_ret; //Number of retry bios sent in RAID
EXPORT_SYMBOL(tifa_bio_ret);
unsigned long long tifa_bio_rfw; //Number of bios read for write
EXPORT_SYMBOL(tifa_bio_rfw);
unsigned long long tifa_bio_gct; //Number of gct bios sent in RAID
EXPORT_SYMBOL(tifa_bio_gct);
unsigned long long tifa_bio_gct_ret; //Among retry bios, number of retry for gct
EXPORT_SYMBOL(tifa_bio_gct_ret);
unsigned long long tifa_bio_rfw_ret; //Among retry bios, number of retry for rfw
EXPORT_SYMBOL(tifa_bio_rfw_ret);
unsigned long long tifa_bio_com; //Number of bios computed
EXPORT_SYMBOL(tifa_bio_com);
unsigned long long tifa_bio_gct_nor; //Number of bios gct normal finished
EXPORT_SYMBOL(tifa_bio_gct_nor);
unsigned long long tifa_bio_gct_eio; //Number of gct returned with eio
EXPORT_SYMBOL(tifa_bio_gct_eio);
unsigned long long tifa_bio_rfw_nor; //Number of bios rfw normal finished
EXPORT_SYMBOL(tifa_bio_rfw_nor);
unsigned long long tifa_bio_rfw_eio; //Number of rfw returned with eio
EXPORT_SYMBOL(tifa_bio_rfw_eio);
unsigned long long tifa_bio_stripe; //Number of full stripe
EXPORT_SYMBOL(tifa_bio_stripe);

atomic_t tifa_dio_ttl;
EXPORT_SYMBOL(tifa_dio_ttl);
atomic_t tifa_dio_gc;
EXPORT_SYMBOL(tifa_dio_gc);
atomic_t stripe_in_gc_num[5];
EXPORT_SYMBOL(stripe_in_gc_num);
atomic_t tifa_dio_in_gc_num[5];
EXPORT_SYMBOL(tifa_dio_in_gc_num);

u64 *rdlat_array = NULL;
EXPORT_SYMBOL(rdlat_array);
atomic_t rdlat_idx;
EXPORT_SYMBOL(rdlat_idx);

asmlinkage void sys_start_stamp(void);
asmlinkage void sys_changeReadPolicy(int newval);
asmlinkage unsigned long long sys_tifa_bio_ttl(int options);
asmlinkage unsigned long long sys_tifa_bio_ret(int options);
asmlinkage unsigned long long sys_tifa_bio_rfw(int options);
asmlinkage unsigned long long sys_tifa_bio_gct(int options);
asmlinkage unsigned long long sys_tifa_bio_gct_ret(int options);
asmlinkage unsigned long long sys_tifa_bio_rfw_ret(int options);
asmlinkage unsigned long long sys_tifa_bio_com(int options);
asmlinkage unsigned long long sys_tifa_bio_gct_nor(int options);
asmlinkage unsigned long long sys_tifa_bio_gct_eio(int options);
asmlinkage unsigned long long sys_tifa_bio_rfw_nor(int options);
asmlinkage unsigned long long sys_tifa_bio_rfw_eio(int options);
asmlinkage unsigned long long sys_tifa_bio_stripe(int options);
asmlinkage int sys_tifa_dio_ttl(int options);
asmlinkage int sys_tifa_dio_gc(int options);
asmlinkage int sys_stripe_in_gc_num(int options, int idx);
asmlinkage int sys_tifa_dio_in_gc_num(int options, int idx);
