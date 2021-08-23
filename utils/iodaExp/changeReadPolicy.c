#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

void help()
{
	printf(
		"\nreadPolicy controlls the policy we use in RAID\n"
		"\nUse:"
		"\t./changeReadPolicy (int)\n"
		"Bit:"
		"\t0\tNormal Read\n"
		"\t1\tReactive read\n"
		"\t2\tProactive Read\n"
		"\t3\tAdaptive read\n"
		"\t4\tEBUSY\n"
		"\t5\tGCT\n"
		"\t6\tFake GCT\n"
		"\t7\tDefault 10ms\n");
}

int main(int argc, char *argv[])
{
	unsigned long i;
	if(argc == 1)
	{
		help();
		return 0;
	}
	i = strtol(argv[1], NULL, 10);
	syscall(334, i);
    return 0;
}
