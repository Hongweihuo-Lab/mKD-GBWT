#include <stdio.h>

#include "sbtmpfile.h"
#include "sbt_util.h"

sbtmpfile_t*
sbtmpfile_read_from_file(FILE* f){

	sbtmpfile_t* stf = (sbtmpfile_t*) sb_malloc(sizeof(sbtmpfile_t));
	stf->f = f;

	if(!f){
		fprintf(stderr, "error opening existing file.\n");
		exit(EXIT_FAILURE);
	} else {
		fseek(f, 0, SEEK_SET); /* seek to the start */
		stf->mode = READLATER;
	}

	return stf;
}

/* create a tmp file */
sbtmpfile_t*
sbtmpfile_create_write(){
	sbtmpfile_t * stf = (sbtmpfile_t*) sb_malloc(sizeof(sbtmpfile_t));
	stf->f = tmpfile();
	if(!stf->f){
		fprintf(stderr, "error opening existing file.\n");
		exit(EXIT_FAILURE);
	} else {
		stf->mode = WRITEONLY;
	}

	return stf;
}

/* finish the tempfile */
void
sbtmpfile_finish(sbtmpfile_t* stf) {

	if(stf->mode == WRITEONLY){
		stf->mode = READLATER;
	} else if(stf->mode == READONLY) {
		stf->mode = READLATER;
	}

}


/* delete the tmp file */
void 
sbtmpfile_delete(sbtmpfile_t* stf){

	if(!stf->f) {
		fprintf(stderr, "error deleting file.\n");
		exit(EXIT_FAILURE);
	} else {
		fclose(stf->f);
		free(stf);  /* forget to free memory. */
	}
}


void
sbtmpfile_open_read(sbtmpfile_t* stf){

	if(stf->mode == READLATER){
		stf->mode = READONLY;
		fseek(stf->f, 0, SEEK_SET);
	} else {
		fprintf(stderr, "error open read sbtmpfile\n");
		exit(EXIT_FAILURE);
	}

}

uint64_t sbtmpfile_elem_num(sbtmpfile_t* stf){

	fseek(stf->f, 0, SEEK_END);
	uint64_t bytes = ftell(stf->f);
	fseek(stf->f, 0, SEEK_SET);
	return (bytes / 8);
}

int sbtmpfile_read_block(sbtmpfile_t* stf, uint64_t* buf, int max){
	
	if(stf->f && stf->mode == READONLY) {
		int count = fread(buf, sizeof(uint64_t), max, stf->f);
		return count;
	} else {
		fprintf(stderr, "error reading block\n");
		exit(EXIT_FAILURE);
	}
}

/* write n*sizeof(uint64_t) into tmp file */
int sbtmpfile_write_block(sbtmpfile_t* stf, uint64_t* buf, int n) {

	if(stf->f && stf->mode == WRITEONLY){

		if(fwrite(buf, sizeof(uint64_t), n, stf->f) != n) {
			fprintf(stderr, "error writing block.\n");
			exit(EXIT_FAILURE);
		}

		return n*sizeof(uint64_t);
	} else {
		fprintf(stderr, "error writing block.\n");
		exit(EXIT_FAILURE);
	}


}	