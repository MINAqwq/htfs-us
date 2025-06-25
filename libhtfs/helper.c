#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <blake3.h>

#include "htfs.h"

uint8_t *
mkbuffer(const HtfsCtx *ctx)
{
	return malloc(ctx->sblk.blksize);
}

int
emptykey(BptKey key)
{
	int i;

	for(i = 0; i < sizeof(BptKey); i++)
		if(key[i] != 0)
			return 0;


	return 1;
}

BpTreeLeafResult *
listcreate()
{
	BpTreeLeafResult *lf;

	lf = malloc(sizeof(lf->depth) + sizeof(*lf->path));
	lf->depth = 0;

	return lf;
}

BpTreeLeafResult *
listappend(BpTreeLeafResult *list, uint64_t item)
{
	list = realloc(list, sizeof(list->depth) + (sizeof(*list->path) * (list->depth + 1)));
	list->path[list->depth++] = item;
	return list;
}

char *
strsafeld(HtfsCtx *ctx, char *str)
{
	size_t size;
	char *buffer;

	size = ctx->sblk.blksize - sizeof(HtfsFileEntry) + 1;
	buffer = malloc(size);

	buffer[size - 1] = 0;

	strncpy(buffer, str, size);

	return buffer;
}

int
nametokey(char *name, BptKey key)
{
	long len;
	blake3_hasher hasher;

	len = strlen(name);
	if(len == 0)
		return Hnull;

	blake3_hasher_init(&hasher);
	blake3_hasher_update(&hasher, name, len);
	blake3_hasher_finalize(&hasher, key, sizeof(BptKey));

	return Hok;
}

int
strskip(char *str)
{
	if(!str)
		return Hnull;

	while(*str)
		str++;

	str++;
	return Hok;
}