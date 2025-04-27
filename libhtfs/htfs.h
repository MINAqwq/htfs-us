typedef struct SuperBlk SuperBlk;
typedef struct HtfsCtx HtfsCtx;
typedef struct AllocMap AllocMap;

/* alloation map block offset */
#define OFF_MAP 1

enum {
	Hok = 0,
	Hnoctx,
	Hnofile,
	Hnull,
	Hdiskfull,
	Hnotfree,
	Hnohtfs,
	Hcorrupted,
};

/*
 * The filesystem header
 */
struct SuperBlk {
	char		magic[4];    /* 'HTFS' in ASCII */
	uint16_t	blksize;
	uint16_t	_resrvd;
	char		label[32];
};

/*
 * Allocation bitmap
 */
struct AllocMap {
	uint64_t	size;
	uint64_t	latest;
	uint8_t	blks[];
};

/*
 * Library context for operations
 */
struct HtfsCtx {
	SuperBlk	sblk;
	AllocMap	*map;

	/* superblock location */
	uint64_t	sblksec;
	FILE 		*drv;
};

#define FSSEEK(blk) fseek(ctx->drv, (blk) * ctx->sblk.blksize, SEEK_SET)

int htfsopen(HtfsCtx *ctx, char *path);

/*
 * writes in memory superblock and allocation map to disk
 */
int htfsclose(HtfsCtx *ctx);

int htfsrdblk(const HtfsCtx *ctx, uint64_t blk, uint8_t *buffer);

int htfswrtblk(const HtfsCtx *ctx, uint64_t blk, uint8_t *buffer);

uint64_t findfreeblk(AllocMap *map);

int allocblk(AllocMap *map, uint64_t blk);

int freeblk(AllocMap *map, uint64_t blk);
