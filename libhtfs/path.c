#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "htfs.h"

char *
pathparse(const char *path, size_t *depth)
{
	int i;
	long len;
	char *cpy;

	len = strlen(path);
	fprintf(stderr, "strp: %s(%d)\n", path, len);
	if(len < 2 || path[0] != '/' || len > 255)
		return NULL;

	*(depth) = 1;
	cpy = malloc(len);
	strncpy(cpy, path + 1, len - 1);

	for(i = 0; i < len; i++)
		if(cpy[i] == '/'){
			cpy[i] = 0;
			(*depth)++;
		}

	cpy[len - 1] = 0;
	return cpy;
}
