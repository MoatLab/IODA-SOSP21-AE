#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "tifa-syscall.h"

int main()
{
    int i;

    printf( "---------BIO STATISTICS---------\n"
            "BIO_TTL:%lu\n"
            "BIO_RET:%lu\n"
            "BIO_GCT:%lu\n"
            "BIO_RFW:%lu\n"
            "--------------------------------\n"
            "BIO_GCT_RET:%lu\n"
            "BIO_RFW_RET:%lu\n"
            "BIO_COM:%lu\n"
            "--------------------------------\n"
            "BIO_GCT_NOR:%lu\n"
            "BIO_GCT_EIO:%lu\n"
            "--------------------------------\n"
            "BIO_RFW_NOR:%lu\n"
            "BIO_RFW_EIO:%lu\n"
            "--------------------------------\n"
            "BIO_FULL_STRIPE:%lu\n"
            "---------DIO STATISTICS---------\n"
            "DIO_TTL:%lu\n"
            "DIO_GC:%lu\n"
            "--------------------------------\n",
            syscall(SYSCALL_TIFA_BIO_TTL, 1), syscall(SYSCALL_TIFA_BIO_RET, 1), syscall(SYSCALL_TIFA_BIO_GCT, 1), syscall(SYSCALL_TIFA_BIO_RFW, 1),
            syscall(SYSCALL_TIFA_BIO_GCT_RET, 1), syscall(SYSCALL_TIFA_BIO_RFW_RET, 1), syscall(SYSCALL_TIFA_BIO_COM, 1),
            syscall(SYSCALL_TIFA_BIO_GCT_NOR, 1), syscall(SYSCALL_TIFA_BIO_GCT_EIO, 1),
            syscall(SYSCALL_TIFA_BIO_RFW_NOR, 1), syscall(SYSCALL_TIFA_BIO_RFW_EIO, 1),
            syscall(SYSCALL_TIFA_BIO_STRIPE, 1),
            syscall(SYSCALL_TIFA_DIO_TTL, 1), syscall(SYSCALL_TIFA_DIO_GC, 1));

    for (i = 0; i < 5; i++) {
        printf("STRIPE_IN_GC_NUM-%d: %ld\n", i, syscall(SYSCALL_STRIPE_IN_GC_NUM, 1, i));
    }
    printf("--------------------------------\n");
    for (i = 0; i < 5; i++) {
        printf("DIO_IN_GC_NUM-%d: %ld\n", i, syscall(SYSCALL_TIFA_DIO_IN_GC_NUM, 1, i));
    }
    printf("---------END STATISTICS---------\n");

    return 0;
}
