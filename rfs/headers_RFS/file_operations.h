/*
 * file_operations.h
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#ifndef FILE_OPERATIONS_H_
#define FILE_OPERATIONS_H_

#include "../commons/string.h"
#include "ext2_block_types.h"
#include "server_threading.h"
#include "ext2_def.h"
#include <string.h>
#include <libmemcached/memcached.h>

/*****************************************************
				START FILE STRUCTS
******************************************************/

typedef struct{
	uint32_t inode_number;
	uint16_t entry_len;
	uint8_t name_len;
	uint8_t file_type;
	char  file_name[];
} entry_t;

typedef struct{
	uint32_t inode_number;
	uint16_t mode;
	int32_t size;
	uint16_t links_count;
} reduced_stat_t;

typedef struct{
	uint32_t len;
	char* key;
}cache_key_t;


typedef struct{
	void* data;
	bool from_cache;
}cache_fs_data_t ;
/*****************************************************
				END   FILE STRUCTS
******************************************************/

// TODAS LAS OPERACIONES HAY QUE HACERLAS THREAD SAFE !!!

/*****************************************************
				START FUNCTIONS PROTOTYPES
******************************************************/
char** split_path(const char* path);

operation_result_t* list_directory(params_t*,s_Mapped_File_t*);
operation_result_t* read_file(params_t*,s_Mapped_File_t*);
operation_result_t* getattr(params_t*,s_Mapped_File_t*);
operation_result_t* write_file(params_t* params, s_Mapped_File_t* s_mapped_file);
operation_result_t* truncate_file(params_t* params, s_Mapped_File_t* s_mapped_file);
operation_result_t* create_file_dir(params_t* params, s_Mapped_File_t* s_mapped_file);
operation_result_t* delete_file(params_t* params, s_Mapped_File_t* s_mapped_file);
operation_result_t* delete_dir(params_t* params, s_Mapped_File_t* s_mapped_file);
operation_result_t* open_file(params_t* params, s_Mapped_File_t* s_mapped_file);

//operation_result_t* attach(params_t* params, s_Mapped_File_t* s_mapped_file); // NO USAR, SOLO PARA TESTING!!!!!
/*****************************************************
				END   FUNCTIONS PROTOTYPES
******************************************************/



#endif /* FILE_OPERATIONS_H_ */
