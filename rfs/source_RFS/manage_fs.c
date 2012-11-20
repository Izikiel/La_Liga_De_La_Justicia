/*
 * manage_fs.c
 *
 *  Created on: Apr 30, 2012
 *      Author: ezequieldariogambaccini
 */

#include "../headers_RFS/manage_fs.h"
#include <sys/stat.h>
#define is ==
#define BAD_MAP (void*)(-1)

#ifndef O_RDWR
#define O_RDWR 2
#endif

void* Map_FS(char* path){

	int32_t fs;

	if((fs = open(path,O_RDWR)) is -1)
		perror("Bad open file");

	struct stat stats_fs;

	stat(path,&stats_fs);
	FS_SIZE = stats_fs.st_size;
	//Para probar con valgrind, usar un tamaño más chico (200 megas anda), o cambiar el stack de valgrind al tamaño del FS
	void* mapped_fs = mmap(NULL,FS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fs, 0);

	if(mapped_fs is BAD_MAP){
		perror("BAD MAP!");//Meter un log aca!!!
		exit(1);
	}

	posix_madvise(mapped_fs,stats_fs.st_size,MADV_RANDOM);

	return mapped_fs;

}

int32_t Unmap_FS(void* mapped_fs){
	if(munmap(mapped_fs,FS_SIZE) != -1)
		return 0;
	else
		return -1;
}
