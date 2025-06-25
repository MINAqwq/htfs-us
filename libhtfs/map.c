#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <libhtfs/htfs.h>

uint64_t
findfreeblk(AllocMap *map)
{
	int secnd;
	uint64_t i, j;
	uint8_t *bp;

	/*
	 * Return 0 on failure, because this is always
	 * a reserved block
	 */
	if(map == NULL)
		return 0;
	
	/*
 	 * Only start search at the beginning
	 * if latest is the the last block in the map
	 */
	i = (map->latest != (map->size - 1))
		? map->latest / 8
		: 0;

	/*
	 * only start a second search if we are NOT at the beginning
	 */
	secnd = (i == 0) ? 1 : 0;

search:
	if(secnd == 2)
		return 0;

	for(; i != (map->size - 1); i++)
		for(j = 0; j != 8; j++)
			if((map->blks[i] & (0x80 >> j)) == 0)
				return (i * 8) + j;

	secnd++;
	i = 0;
	goto search;
}

int
allocblk(AllocMap *map, uint64_t blk)
{
	uint64_t ib;
	uint8_t *bp;

	/*
	 * blk is intended to be the result from a
	 * findfreeblk call, wich will return 0 if
	 * there is no free block left
	 */
	if(blk == 0)
		return Hdiskfull;

	bp = &map->blks[blk / 8];
	ib = (0x80 >> (blk % 8));

	if((*bp & ib) != 0)
		return Hnotfree;

	*bp |= ib;
	map->latest = blk;
	
	return Hok;
}

int
freeblk(AllocMap *map, uint64_t blk)
{
	uint64_t ib;
	uint8_t *bp;

	bp = &map->blks[blk / 8];
	ib = (0x80 >> (blk % 8));

	/*
	 * check if block is actually allocated
	 */
	if(*bp & ib){
		return Halreadyfree;
	}

	*bp ^= ib;

	return Hok;
}