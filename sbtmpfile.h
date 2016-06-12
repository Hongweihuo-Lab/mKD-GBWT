#ifndef SBTMPFIEL_H
#define SBTMPFIEL_H

#include <stdint.h>

typedef struct sbtmpfile{

	FILE* f;
	uint64_t mode;

}sbtmpfile_t;


#define READONLY 0
#define WRITEONLY 1
#define READLATER 2



/* file I/O */

/* A sbtmpfile will be create for write and write until read and be deleted at last.
*/


/* Create sbtmpfile_t from FILE* f */
sbtmpfile_t* sbtmpfile_read_from_file(FILE* f);

/* Create a temp file */
sbtmpfile_t* sbtmpfile_create_write();

/* close the tmp file */
void sbtmpfile_finish(sbtmpfile_t* stf);

/* delete the tmp file */
void sbtmpfile_delete(sbtmpfile_t* stf);	

int sbtmpfile_read_block(sbtmpfile_t * stf, uint64_t* buf, int max);

int sbtmpfile_write_block(sbtmpfile_t* stf, uint64_t* buf, int n);

void sbtmpfile_open_read(sbtmpfile_t* stf);

/* the number of elements of 64bit */
uint64_t sbtmpfile_elem_num(sbtmpfile_t* stf);

#endif //SBTMPFIEL_H 