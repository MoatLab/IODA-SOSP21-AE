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

    syscall(SYSCALL_TIFA_BIO_TTL, 0);
    syscall(SYSCALL_TIFA_BIO_RET, 0);
    syscall(SYSCALL_TIFA_BIO_GCT, 0);
    syscall(SYSCALL_TIFA_BIO_RFW, 0);
    syscall(SYSCALL_TIFA_BIO_GCT_RET, 0);
    syscall(SYSCALL_TIFA_BIO_RFW_RET, 0);
    syscall(SYSCALL_TIFA_BIO_COM, 0);
    syscall(SYSCALL_TIFA_BIO_GCT_NOR, 0);
    syscall(SYSCALL_TIFA_BIO_GCT_EIO, 0);
    syscall(SYSCALL_TIFA_BIO_RFW_NOR, 0);
    syscall(SYSCALL_TIFA_BIO_RFW_EIO, 0);
    syscall(SYSCALL_TIFA_BIO_STRIPE, 0);
    syscall(SYSCALL_TIFA_DIO_TTL, 0);
    syscall(SYSCALL_TIFA_DIO_GC, 0);

    for (i = 0; i < 5; i++) {
        syscall(SYSCALL_STRIPE_IN_GC_NUM, 0, i);
        syscall(SYSCALL_TIFA_DIO_IN_GC_NUM, 0, i);
    }
    printf("Reset counters\n");
    return 0;
}
