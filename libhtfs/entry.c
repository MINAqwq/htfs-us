#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "htfs.h"

#include <string.h>

HtfsFileEntry *
entrycreate(HtfsCtx *ctx)
{
	HtfsFileEntry *file;

	file = (HtfsFileEntry*)mkbuffer(ctx);
	memset(file, 0, ctx->sblk.blksize);

	file->ctime = time(NULL);
	file->mtime = file->ctime;

	return file;
}

int
entryrename(HtfsCtx *ctx, HtfsFileEntry *file, char *name)
{
	long len;

	len = strlen(name);
	if(len == 0 || len >= (bpspace(ctx) - sizeof(*file)))
		return Hinvalidname;

	strncpy(file->name, name, len);

	return Hok;
}

int
entryput(HtfsCtx *ctx, uint64_t parent, HtfsFileEntry *file)
{
	int res;
	uint64_t blk;
	BptKey key;

	if(ctx == NULL || parent == 0 || file == NULL)
		return Hnull;

	printf("put %s to %d\n", file->name, parent);

	blk = findfreeblk(ctx->map);
	res = allocblk(ctx->map, blk);
	if(res != Hok)
		return res;

	res = htfswrtblk(ctx, blk, (uint8_t*)file);
	if(res != Hok)
		goto error;

	res = nametokey(file->name, key);
	if(res != Hok)
		goto error;

	res = bpinsert(ctx, parent, key, blk);
	if(res != Hok)
		goto error;

	return Hok;

error:
	freeblk(ctx->map, blk);
	return res;
}

int
entrydel(HtfsCtx *ctx, uint64_t parent, char *name)
{
	int res;
	BptKey key;
	BptValue data;

	if(ctx == NULL || parent == 0 || name == NULL)
		return Hnull;

	res = nametokey(name, key);
	if(res != Hok)
		return res;

	data = bpdelete(ctx, parent, key);
	if(data == 0)
		return Hnotfound;

	res = freeblk(ctx->map, data);
	if(res != Hok){
		/* allocate again on eror LMFAO */
		allocblk(ctx->map, data);
		return res;
	}

	return Hok;
}

HtfsFileEntry *
entryget(HtfsCtx *ctx, uint64_t parent, char *name)
{
	int res;
	BptKey key;
	BptValue data;
	HtfsFileEntry *file;

	if(ctx == NULL || parent == 0 || name == NULL)
		return NULL;

	res = nametokey(name, key);
	if(res != Hok)
		return NULL;

	data = bpsearch(ctx, parent, key);
	if(data == 0)
		return NULL;

	file = (HtfsFileEntry*)mkbuffer(ctx);
	res = htfsrdblk(ctx, data, (uint8_t*)file);
	if(res != Hok){
		free(file);
		return NULL;
	}

	return file;
}
