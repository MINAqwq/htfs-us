#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <libhtfs/htfs.h>

static const char *src;
static const char *dst;
static const char *disk;
static uint16_t blksize;

_Noreturn void
usage(void)
{
	fputs("usage: htfscp src dst (blksize)\n\nstart a path with a colon to indicate htfs\n:/testfile <- htfs\n/testfile <- host\ndefault blksize is 512\n", stderr);
	exit(EXIT_FAILURE);
}

_Noreturn void
fatal(const char *s)
{
	fprintf(stderr, "Fatal: %s\n", s);
	exit(EXIT_FAILURE);
}

uint8_t *
loadhost(size_t *fsize, const char *path)
{
	FILE *fp;
	size_t size;
	uint8_t *buffer;

	fp = fopen(path, "rb");
	if(fp == NULL)
		return NULL;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);

	if(size < 1)
		goto err;

	buffer = malloc(size);
	fread(buffer, size, 1, fp);

	*fsize = size;
	return buffer;
	
errfree:
	free(buffer);
err:
	fclose(fp);
	return NULL;
}

uint8_t *
loadhtfs(size_t *fsize, const char *path, const char *disk)
{
	/* TODO */
	HtfsCtx ctx;
	HtfsFileCtx fctx;
	uint8_t *buffer;

	ctx.sblksec = 1;
	ctx.sblk.blksize = blksize;
	if(htfsopen(&ctx, disk) != Hok)
		return NULL;

	if(fileopen(&ctx, &fctx, path, ctx.sblk.root) != Hok)
		goto err;

	buffer = malloc(fctx.file->size);
	if(fileread(&ctx, &fctx, buffer, fctx.file->size) != Hok)
		goto errfree;

	htfsclose(&ctx);
	return buffer;

errfree:
	free(buffer);
err:
	htfsclose(&ctx);
	return NULL;
}

int
storehost(uint8_t *data, size_t size, const char *path)
{
	fatal("toto\n");
}

int
storehtfs(uint8_t *data, size_t size, const char *path, const char *disk)
{
	HtfsCtx ctx;
	size_t wrote;
	HtfsFileCtx fctx;

	wrote = 0;

	ctx.sblksec = 1;
	ctx.sblk.blksize = blksize;

	if(htfsopen(&ctx, disk) != Hok)
		return 0;

	if(filecreate(&ctx, &fctx, path, ctx.sblk.root, 0) != Hok)
		goto close;

	wrote = filewrite(&ctx, &fctx, data, size);

	fileupdate(&ctx, &fctx);

close:
	htfsclose(&ctx);
	return wrote;
}

void
main(int argc, char *argv[])
{
	int res;
	uint8_t *srcdata;
	size_t srcsize;

	if(argc != 4 && argc != 5)
		usage();

	src = argv[1];
	dst = argv[2];
	disk = argv[3];

	blksize = (argc == 5)
		? strtoull(argv[4], NULL, 10)
		: 512;

	fprintf(stderr, "htfs: %s\n", disk);

	srcdata = (src[0] == ':')
		? loadhtfs(&srcsize, &src[1], disk)
		: loadhost(&srcsize, src);

	if(srcdata == NULL)
		fatal("error reading from src\n");

	res = (dst[0] == ':')
		? storehtfs(srcdata, srcsize, &dst[1], disk)
		: storehost(srcdata, srcsize, dst);
}
