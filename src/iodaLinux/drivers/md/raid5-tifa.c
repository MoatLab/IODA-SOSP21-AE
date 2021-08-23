#include "md.h"
#include "raid5.h"

ssize_t tifa_show_rdlat(struct mddev  *mddev, char *page)
{
    u64 nr = atomic_read(&rdlat_idx);
    u64 i = 0;

    printk("Coperd,rdlat_idx=%llu\n", nr);
    for (i = 0; i < nr; i++) {
        printk("Coperd,i:%llu,%llu\n", i, rdlat_array[i]);
    }

    return 0;
}

ssize_t tifa_store_rdlat(struct mddev  *mddev, const char *page, size_t len)
{
#define TSZ 5000000
    unsigned long new;

	if (len >= PAGE_SIZE)
		return -EINVAL;

	if (kstrtoul(page, 10, &new))
		return -EINVAL;

    if (new == 0) {
        if (rdlat_array) {
            printk("Coperd,%s,rdlat_array already initialized ..\n", __func__);
            return 0;
        } else {
            printk("Coperd,%s,initialize rdlat array\n", __func__);
            rdlat_array = vmalloc(TSZ * sizeof(u64));
            atomic_set(&rdlat_idx, 0);
        }
    } else if (new == 1) {
        // reset rdlat_idx
        atomic_set(&rdlat_idx, 0);
        if (rdlat_array) {
            memset(rdlat_array, 0, TSZ * sizeof(u64));
        } else {
            printk("Coperd,rdlat_array=NULL not initialized yet !!\n");
        }
        printk("Coperd,%s,rdlat_idx reset to %d\n", __func__, atomic_read(&rdlat_idx));
    } else if (new >= 1000000) {
        printk("Coperd,re-allocate rdlat_array to be have %ld entries\n", new);
        if (rdlat_array) {
            vfree(rdlat_array);
        }
        rdlat_array = vmalloc(new * sizeof(u64));
        atomic_set(&rdlat_idx, 0);

    } else {
        printk("Coperd, not supported !\n");
    }

	return len;
}

struct md_sysfs_entry tifa_rdlat = __ATTR(tifa_rdlat, S_IRUGO | S_IWUSR,
				tifa_show_rdlat, tifa_store_rdlat);
