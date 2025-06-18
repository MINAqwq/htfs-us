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
	BpTreeLeaf *v;
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

		(int)sizeof(ctx.sblk.label),
		ctx.sblk.label,
		ctx.map->size * 8
		);

		BpTreeRangeCtx rctx;
		rctx.i = 0;
		rctx.buffer = NULL;


		for(v = bpscan(&ctx, ctx.sblk.root, &rctx);
			v != NULL;
			v = bpscan(&ctx, ctx.sblk.root, &rctx)){
				if(v->data == 0)
					continue;

				fprintf(stderr, "Entry %02lX -> %lu\n", rctx.i, v->data);
		}

	exit(EXIT_SUCCESS);
}
