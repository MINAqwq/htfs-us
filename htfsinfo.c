#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <libhtfs/htfs.h>

static uint64_t blksize;

_Noreturn void
fatal(const char *s)
{
	fprintf(stderr, "fatal: %s\n", s);
	exit(EXIT_FAILURE);
}

_Noreturn void
usage(void)
{
	fputs("usage: htfsinfo file [blksize]\n", stderr);
	exit(EXIT_FAILURE);
}

void
main(int argc, char *argv[])
{
	HtfsCtx ctx;

	if(argc != 3 && argc != 2)
		usage();

	ctx.sblksec = 1;
	ctx.sblk.blksize = (argc != 3)
		? 512
		: strtoull(argv[2], NULL, 10);

	if(htfsopen(&ctx, argv[1]) != Hok)
		fatal("Failed to open HTFS disk");

	fprintf(stderr,
		"Label: %.*s\n"
		"Total Blocks: %ld\n",

		sizeof(ctx.sblk.label),
		ctx.sblk.label,
		ctx.map->size * 8
		);

	exit(EXIT_SUCCESS);
}
