#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

#include "tifa-syscall.h"

void help()
{
	printf(
		"\nreadPolicy controlls the policy we use in RAID\n"
		"\nUse:"
		"\t./changeReadPolicy (int)\n"
		"Bit:"
		"\t0\tNormal Read (default)\n"
		"\t1\tEBUSY (Random under >=2GCs)\n"
		"\t2\tGCT (BRT under >=2GCs)\n"
        "\t3\tIOD3 (TW-only)\n"
		"\t-\tReactive read (TODO)\n"
		"\t-\tProactive Read (TODO)\n"
		"\t-\tAdaptive read (TODO)\n"
		"\t-\tFake GCT (TODO)\n");
}

int main(int argc, char *argv[])
{
	unsigned long i;

	if (argc == 1) {
		help();
		return 0;
	}

	i = strtol(argv[1], NULL, 10);

	syscall(SYSCALL_TIFA_CHANGE_READ_POLICY, i);

    return 0;
}
