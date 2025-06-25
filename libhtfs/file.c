#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "htfs.h"

#include <string.h>

int
filecreate(HtfsCtx *ctx, HtfsFileCtx *fctx, char *path, uint64_t root, uint8_t attr)
{
	char *dirlistp;
	char *dirlist;
	HtfsFileEntry *parent;
	HtfsFileEntry *entry;
	size_t depth;
	uint64_t data;

	entry = NULL;
	parent = NULL;
	depth = 0;

	dirlist = pathparse(path, &depth);
	dirlistp = dirlist;

	if(dirlist == NULL || depth == 0)
		return Hnull;

	if(depth != 1){
		while((depth--) != 1){
			parent = entryget(ctx, root, dirlist, &data);
			if(parent == NULL)
				goto error;

			if((parent->attr & Fdir) == 0)
				goto error;

			root = parent->root;
			free(parent);
			parent = NULL;
			strskip(path);
		}
	}

	/* fprintf(stderr, "creating entry\n"); */
	entry = entrycreate(ctx);
	if(entry == NULL){
		free(dirlistp);
		return Hdiskfull;
	}

	entry->attr = attr;

	/* fprintf(stderr, "renaming entry to %s\n", dirlist); */
	if(entryrename(ctx, entry, dirlist) != Hok)
		goto error;

	free(dirlistp);

	/* fprintf(stderr, "put entry\n", dirlist); */
	if(entryput(ctx, root, entry, &data) != Hok)
		goto error;

	fctx->file = entry;
	fctx->off = 0;
	fctx->parent = data;

	return Hok;

error:
	if(entry != NULL)
		free(entry);

	free(dirlistp);

	return Hdiskfull;
}

int
fileopen(HtfsCtx *ctx, HtfsFileCtx *fctx, char *path, uint64_t root)
{
	char *dirlist;
	char *dirlistp;
	HtfsFileEntry *entry;
	size_t depth;
	uint64_t data;

	entry = NULL;
	depth = 0;

	dirlist = pathparse(path, &depth);
	dirlistp = dirlist;

	if(dirlist == NULL || depth == 0)
		return Hnull;

	while(depth--){
		entry = entryget(ctx, root, dirlist, &data);
		if(entry == NULL)
			goto error;

		if(depth != 0){
			root = entry->root;
			free(entry);
			entry = NULL;
			strskip(dirlist);
			continue;
		}

		if((entry->attr & Fdir) == 1)
			goto error;
	}

	free(dirlistp);

	fctx->file = entry;
	fctx->off = 0;
	fctx->parent = data;
	return Hok;

error:
	if(entry != NULL)
		free(entry);

	free(dirlistp);

	return Hnotfound;
}

int
fileupdate(HtfsCtx *ctx, HtfsFileCtx *fctx)
{
	return entryupdate(ctx, fctx->parent, fctx->file);
}


uint64_t
filegetdata(HtfsCtx *ctx, HtfsFileCtx *file, BptKey key)
{
	int res;
	uint64_t blk;
	BptValue data;

	data = bpsearch(ctx, file->file->root, key);
	if(data != 0)
		return data;

	blk = findfreeblk(ctx->map);
	res = allocblk(ctx->map, blk);
	if(res != Hok)
		return 0;

	res = bpinsert(ctx, file->file->root, key, blk);
	if(res != Hok){
		freeblk(ctx->map, blk);
		return 0;
	}
		
	return blk;
}

size_t
filewrite(HtfsCtx *ctx, HtfsFileCtx *file, uint8_t *data, size_t len)
{
	uint64_t amount;
	size_t bytes;
	BptKey rkey;
	uint64_t *key;
	uint64_t blk;
	uint8_t *buffer;

	if(len==0){
		file->file->size = 0;
		return 0;
	}

	key = (uint64_t*)&rkey;

	bytes = 0;
	buffer = mkbuffer(ctx);

	for(*key = (file->off / ctx->sblk.blksize) + 1; len != 0; (*key)++){
		blk = filegetdata(ctx, file, rkey);
		if(blk == 0)
			return bytes;

		amount = (len >= ctx->sblk.blksize) ? ctx->sblk.blksize : len;

		memset(buffer, 0, ctx->sblk.blksize);
		memcpy(buffer, data, amount);

		if(htfswrtblk(ctx, blk, buffer)!=Hok)
			break;
		
		bytes += amount;
		file->file->size = bytes;

		len -= amount;
	}

	free(buffer);

	/* TODO: shrink if needed */
	return bytes;
}

size_t
fileread(HtfsCtx *ctx, HtfsFileCtx *file, uint8_t *data, size_t len)
{
	
}

void
fileseek(HtfsFileCtx *file, int64_t where, int mode)
{
	int64_t pos;

	switch(mode){
		case Sabs:
			pos = where;
			break;
		case Scur:
			pos = file->off + where;
			break;
		default:
			return;
	}

	file->off = pos;
}

