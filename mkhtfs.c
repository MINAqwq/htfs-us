#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <libhtfs/htfs.h>

static uint16_t blksize;
static const char *label;
static const char *file;

_Noreturn void
usage(void)
{
	fputs("usage: mkhtfs blksize label file\n", stderr);
	exit(EXIT_FAILURE);
}

_Noreturn void
fatal(const char *s)
{
	fprintf(stderr, "Fatal: %s\n", s);
	exit(EXIT_FAILURE);
}

_Noreturn void
pexit(const char *s)
{
	perror(s);
	exit(EXIT_FAILURE);
}

void
main(int argc, char **argv)
{
	size_t fsize, mapsize;
	uint64_t blks, i, blkstart;
	HtfsCtx ctx;

	if(argc != 4)
		usage();

	blksize = strtoul(argv[1], NULL, 10);
	if(blksize == 0)
		usage();

	label = argv[2];
	if(label[0] == 0)
		usage();

	file = argv[3];
	if(file[0] == 0)
		usage();

	ctx.drv = fopen(file, "rb+");
	if(ctx.drv == 0)
		pexit(label);

	ctx.sblksec = 1;
	ctx.sblk.magic[0] = 'H';
	ctx.sblk.magic[1] = 'T';
	ctx.sblk.magic[2] = 'F';
	ctx.sblk.magic[3] = 'S';
	ctx.sblk.blksize = blksize;
	(strlen(label) == sizeof(ctx.sblk.label))
		? memcpy(ctx.sblk.label, label, sizeof(ctx.sblk.label))
		: strncpy(ctx.sblk.label, label, sizeof(ctx.sblk.label));

	/* obtain file size in a portable way */
	fseek(ctx.drv, 0, SEEK_END);
	fsize = ftell(ctx.drv);

	blks = fsize / 512;
	if(blks < 4)
		fatal("Your disk needs at least 4 blocks");

	/* Add 0.999 to always round up */
	mapsize = ((blks / 8) + 0.999);
	ctx.map = calloc(1, sizeof(*ctx.map) + mapsize);
	if(ctx.map == NULL)
		fatal("calloc fail");

	ctx.map->size = mapsize;
	fprintf(stderr, "mapsize: 0x%lx\n", mapsize);

	/* boot, super and first map block */
	ctx.map->blks[0] = 0b11100000;
	ctx.map->latest = 2;

	blkstart = ctx.map->latest;
	for(i = 0; i < ((mapsize / 512)); i++)
		allocblk(ctx.map, ctx.map->latest + 1);

	htfsclose(&ctx);
	fputs("finished\n", stderr);
	exit(EXIT_SUCCESS);
}
