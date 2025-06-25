typedef struct SuperBlk SuperBlk;
typedef struct HtfsCtx HtfsCtx;
typedef struct AllocMap AllocMap;
typedef struct BpTreeItem BpTreeItem;
typedef struct BpTreeNode BpTreeNode;
typedef struct BpTreeLeaf BpTreeLeaf;
typedef struct BpTreeRangeCtx BpTreeRangeCtx;
typedef struct BpTreeLeafResult BpTreeLeafResult;
typedef struct HtfsFileEntry HtfsFileEntry;
typedef struct HtfsFileCtx HtfsFileCtx;

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
	Halreadyfree,
	Hnotfound,
	Hinvalidname,
	Hinvalidpath
};

/*
 * The filesystem header
 */
struct SuperBlk {
	char		magic[4];    /* 'HTFS' in ASCII */
	uint16_t	blksize;
	uint16_t	_resrvd;
	char		label[32];
	uint64_t	root;
};

/*
 * Allocation bitmap
 */
struct AllocMap {
	uint64_t	size;
	uint64_t	latest;
	uint8_t		blks[];
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

enum {
	BptNode,
	BptLeaf,
};

typedef char BptKey[14];
typedef uint64_t BptValue;

/*
 * Base structure for tree items
 */
struct BpTreeItem {
	uint16_t	type;
	BptKey		hash;
};

struct BpTreeNode {
	BpTreeItem;

	uint64_t	blk;
	uint64_t	_unused;
};

struct BpTreeLeaf {
	BpTreeItem;

	BptValue	data;
	uint64_t	next; /* only set on last entry */
};

struct BpTreeRangeCtx {
	uint64_t	i;
	BpTreeLeaf	*buffer;
};

struct BpTreeLeafResult {
	uint64_t	depth;
	uint64_t	path[]; /* starts at root, leaf is path[depth - 1] */
};

/*
 * init a block as bptree root containing leafs
 */
int bpinit(const HtfsCtx *ctx, uint64_t blk);

/*
 * number of items a block can store
 */
uint64_t bpspace(const HtfsCtx *ctx);
BptValue bpsearch(const HtfsCtx *ctx, uint64_t root, BptKey key);

/*
 * init rctx and then you can just
 * for(v = bpscan(); v; v = bpscan())
 */
BpTreeLeaf *bpscan(const HtfsCtx *ctx, uint64_t root, BpTreeRangeCtx *rctx);
int	bpinsert(const HtfsCtx *ctx, uint64_t root, BptKey key, uint64_t data);

/* returns the data block or 0 */
BptValue bpdelete(const HtfsCtx *ctx, uint64_t root, BptKey key);

uint64_t bpsplit(const HtfsCtx *ctx, uint64_t root, uint64_t );

/* allocate enough space for a disk block */
uint8_t *mkbuffer(const HtfsCtx *ctx);

/* return 1 if key is empty */
int emptykey(BptKey key);

BpTreeLeafResult *listcreate();
BpTreeLeafResult *listappend(BpTreeLeafResult *list, uint64_t item);

enum {
	Fdir	= 1<<0,
	Fulink	= 1<<1,
	Fdlink	= 1<<2,
	Fsystem	= 1<<3,
};

enum {
	Pread	= 1<<0,
	Pwrite	= 1<<1,
	Pexec	= 1<<2,
	Pmod	= 1<<3,
};

struct HtfsFileEntry {
	uint32_t	user;
	uint32_t	group;
	uint8_t		uperms; /* user permissions */
	uint8_t		gperms; /* group */
	uint8_t		operms; /* other */
	uint8_t		attr;
	uint64_t	root;
	uint64_t	ctime;
	uint64_t	mtime;
	uint64_t	size;
	char		name[];
};

HtfsFileEntry *entrycreate(HtfsCtx *ctx);

int entryrename(HtfsCtx *ctx, HtfsFileEntry *file, char *name);

int entryput(HtfsCtx *ctx, uint64_t parent, HtfsFileEntry *file, uint64_t *blk);
int entrydel(HtfsCtx *ctx, uint64_t parent, char *name);
HtfsFileEntry *entryget(HtfsCtx *ctx, uint64_t parent, char *name, uint64_t *blk);
int entryupdate(HtfsCtx *ctx, uint64_t parent, HtfsFileEntry *file);

char *strsafeld(HtfsCtx *ctx, char *str);
int nametokey(char *name, BptKey key);

struct HtfsFileCtx {
	HtfsFileEntry *file;
	uint64_t off;
	uint64_t parent;
};

enum {
	Sabs, /* absolute */
	Scur  /* from current */
};

int filecreate(HtfsCtx *ctx, HtfsFileCtx *fctx, char *path, uint64_t root, uint8_t attr);
int fileopen(HtfsCtx *ctx, HtfsFileCtx *fctx, char *path, uint64_t root);
int fileupdate(HtfsCtx *ctx, HtfsFileCtx *fctx);

uint64_t filegetdata(HtfsCtx *ctx, HtfsFileCtx *file, BptKey key);
size_t filewrite(HtfsCtx *ctx, HtfsFileCtx *file, uint8_t *data, size_t len);
size_t fileread(HtfsCtx *ctx, HtfsFileCtx *file, uint8_t *data, size_t len);
void fileseek(HtfsFileCtx *file, int64_t where, int mode);

/*
 * Returns copy of string with '/' replaced by 0s
 */
char *pathparse(const char *path, size_t *depth);

/*
 * seek to (pos of next 0) + 1
 */
int strskip(char *str);