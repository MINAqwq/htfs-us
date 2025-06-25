#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libhtfs/htfs.h>

int
bpinit(const HtfsCtx *ctx, uint64_t blk) {
	int i, res;
	BpTreeLeaf *root;
	BpTreeLeaf *leaf;

	root = (BpTreeLeaf *)mkbuffer(ctx);
	if(root == NULL)
		return Hnull;

	for(i = 0; i < bpspace(ctx); i++){
		/* printf("BLK: %lu\tSetup Leaf %d\n", blk, i); */
		leaf = &root[i];

		leaf->type = BptLeaf;
		memset(leaf->hash, 0, sizeof(leaf->hash));
		leaf->data = 0;
		leaf->next = 0;
	}

	res = htfswrtblk(ctx, blk, (uint8_t *)root);
	free(root);
	return res;
}

uint64_t
bpspace(const HtfsCtx *ctx)
{
	return ctx->sblk.blksize / sizeof(BpTreeLeaf);
}

BptValue
bpsearch(const HtfsCtx *ctx, uint64_t node, BptKey key)
{
	size_t i;
	uint8_t *buffer;
	BpTreeItem *items;
	BpTreeLeaf *leafs;
	BpTreeNode *nodes;

	buffer = NULL;

nextlvl:
	if(buffer)
		free(buffer);

	buffer = mkbuffer(ctx);

	if(htfsrdblk(ctx, node, buffer) != Hok)
		return 0;

	items = (BpTreeItem *)buffer;

	if(items[0].type == BptLeaf){
		leafs = (BpTreeLeaf *)items;
		for(i = 0; i < bpspace(ctx); i++){
			if(emptykey(items[i].hash))
				break;

			if(memcmp(items[i].hash, key, sizeof(BptKey)) == 0){
				return leafs[i].data;
			}
		}

		return 0;
	}

	nodes = (BpTreeNode*)buffer;
	for(i = 0; i < bpspace(ctx); i++){
		if(emptykey(items[i].hash))
			break;

		if(memcmp(items[i].hash, key, sizeof(BptKey)) <= 0){
			node = nodes[i].blk;
			goto nextlvl;
		}		
	}

	return 0;
}

BpTreeLeaf *
bpscan(const HtfsCtx *ctx, uint64_t root, BpTreeRangeCtx *rctx)
{
	size_t i;
	uint64_t blk;
	uint8_t *entries;
	BpTreeItem *item;
	BpTreeLeaf *leaf;

	/* find first leaf and fill buffer */
	if(rctx->buffer == NULL){
		entries = mkbuffer(ctx);

		if(htfsrdblk(ctx, root, entries) != Hok)
			return NULL;

nextlvl:
		for(i = 0; i < bpspace(ctx); i++){
			item = &((BpTreeItem *)entries)[i];
			if(item->type == BptLeaf){
				rctx->buffer = (BpTreeLeaf *)entries;
				break;
			}

			blk = ((BpTreeNode *)item)->blk;
			if(htfsrdblk(ctx, blk, entries) != Hok){
				free(entries);
				return NULL;
			}

			fprintf(stderr, "next level\n");
			goto nextlvl;
		}
	}

	if(rctx->i != bpspace(ctx))
		return &rctx->buffer[rctx->i++];

	leaf = &rctx->buffer[rctx->i - 1];
	if(leaf->next == 0)
		return NULL;

	if(htfsrdblk(ctx, leaf->next, (uint8_t *)rctx->buffer) == Hok)
		return &rctx->buffer[rctx->i++];

	free(rctx->buffer);
	return NULL;
}

BpTreeLeafResult *
bpfindleaf(const HtfsCtx *ctx, uint64_t node, BptKey key)
{
	size_t i;
	uint8_t *buffer;
	BpTreeItem *items;
	BpTreeLeaf *leafs;
	BpTreeNode *nodes;
	BpTreeLeafResult *res;

	buffer = NULL;
	res = listcreate();

nextlvl:
	if(buffer)
		free(buffer);

	buffer = mkbuffer(ctx);

	if(htfsrdblk(ctx, node, buffer) != Hok)
		return res;

	listappend(res, node);

	items = (BpTreeItem *)buffer;

	if(items[0].type == BptLeaf)
		return res;

	nodes = (BpTreeNode*)buffer;
	for(i = 0; i < bpspace(ctx); i++){
		if(emptykey(items[i].hash))
			break;

		if(memcmp(items[i].hash, key, sizeof(BptKey)) <= 0){
			node = nodes[i].blk;
			goto nextlvl;
		}
	}

	return res;
}

/* TODO:
 * currently a write is only possible on 1 leaf
 * (split and merge are not implemented)
 */
int
bpinsert(const HtfsCtx *ctx, uint64_t root, BptKey key, uint64_t data)
{
	int i;
	BpTreeLeafResult *res;
	BpTreeLeaf *leaf;

	fprintf(stderr, "searching leaf for root[%02X/%02X] at %lu\n", (uint8_t)key[0], (uint8_t)key[1], root);
	res = bpfindleaf(ctx, root, key);

	if(res->depth == 0)
		return Hdiskfull;

	fprintf(stderr, "result: %d | leaf: %d\n", res->depth, res->path[res->depth - 1]);

	leaf = (BpTreeLeaf*)mkbuffer(ctx);
	if(htfsrdblk(ctx, res->path[res->depth - 1], (uint8_t*)leaf) != Hok){
		free(leaf);
		return Hnull;
	}

	for(i = 0; i < bpspace(ctx); i++){
		if(!emptykey(leaf[i].hash))
			continue;

		memcpy(leaf[i].hash, key, sizeof(leaf[i].hash));
		leaf[i].data = data;
		return htfswrtblk(ctx, res->path[res->depth - 1], (uint8_t*)leaf);
	}

	printf("disk full???\n");
	return Hdiskfull;
}

/* TODO */
BptValue
bpdelete(const HtfsCtx *ctx, uint64_t root, BptKey key)
{
	return 0;
}

/* TODO */
uint64_t
bpsplit(const HtfsCtx *ctx, uint64_t root, uint64_t )
{
	
}
