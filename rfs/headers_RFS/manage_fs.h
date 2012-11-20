/*
 * manage_fs.h
 *
 *  Created on: Apr 30, 2012
 *      Author: ezequieldariogambaccini
 */

#ifndef MANAGE_FS_H_
#define MANAGE_FS_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
//#include <fcntl.h>

#include <stdint.h>
#include <stdio.h>

uint32_t FS_SIZE;

	void* Map_FS(char* path);
	int32_t Unmap_FS(void* mapped_fs);


#endif /* MANAGE_FS_H_ */
