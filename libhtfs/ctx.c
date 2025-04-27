#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libhtfs/htfs.h>

int
htfsopen(HtfsCtx *ctx, char *path)
{
	FILE *fp;
	AllocMap *map;
	uint64_t mapsize;

	if(ctx == NULL)
		return Hnoctx;

	fp = fopen(path, "rb+");
	if(fp == NULL)
		return Hnofile;

	fseek(fp, ctx->sblksec * ctx->sblk.blksize , SEEK_SET);
	fread(&ctx->sblk, sizeof(ctx->sblk), 1, fp);

	if(memcmp(&ctx->sblk.magic[0], "HTFS", sizeof(ctx->sblk.magic)) != 0){
		fclose(fp);
		return Hnohtfs;
	}
	ctx->drv = fp;

	FSSEEK(ctx->sblksec + OFF_MAP);

	/* read alloc map size in byte */
	fread(&mapsize, sizeof(mapsize), 1, fp);

	if(mapsize == 0){
		fclose(fp);
		return Hcorrupted;
	}

	map = malloc(mapsize);
	if(map == NULL){
		fclose(fp);
		return Hnull;
	}

	FSSEEK(ctx->sblksec + OFF_MAP);
	fread(map, mapsize, 1, fp);

	ctx->map = map;

	return Hok;
}

int
htfsclose(HtfsCtx *ctx)
{
	uint64_t i;
	AllocMap *end, *backup;
	uint8_t *sblkbuf;

	if(ctx == NULL)
		return Hnoctx;

	sblkbuf = calloc(1, ctx->sblk.blksize);

	/* TODO: exit/fatal? */
	if(sblkbuf == NULL)
		return Hnull;

	/*
	 * copy superblock to a blocksize aligned filled with zeros to avoid
	 * writing stack memory to the disk
	 */
	memcpy(sblkbuf, &ctx->sblk, sizeof(ctx->sblk));

	htfswrtblk(ctx, ctx->sblksec, sblkbuf);

	backup = ctx->map;
	for(i =0, end = ctx->map + ctx->map->size ;  ctx->map < end; ctx->map += ctx->sblk.blksize, i++)
		htfswrtblk(ctx, ctx->sblksec + OFF_MAP + i, (uint8_t*)ctx->map);

	fclose(ctx->drv);
	free(backup);
	memset(ctx, 0, sizeof(*ctx));

	return Hok;
}

int
htfsrdblk(const HtfsCtx *ctx, uint64_t blk, uint8_t *buffer)
{
	if(ctx == NULL)
		return Hnoctx;

	if(buffer == NULL)
		return Hnull;

	FSSEEK(blk);
	fread(buffer, ctx->sblk.blksize, 1, ctx->drv);

	return Hok;
}

int
htfswrtblk(const HtfsCtx *ctx, uint64_t blk, uint8_t *buffer)
{
	if(ctx == NULL)
		return Hnoctx;
	
	if(buffer == NULL)
		return Hnull;

	FSSEEK(blk);
	fwrite(buffer, ctx->sblk.blksize, 1, ctx->drv);
	return Hok;
}
