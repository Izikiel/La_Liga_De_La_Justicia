/*
 * server_threading.h
 *
 *  Created on: May 11, 2012
 *      Author: izikiel
 */

#ifndef SERVER_THREADING_H_
#define SERVER_THREADING_H_

#define RDLOCK(lock) pthread_rwlock_rdlock(&lock)
#define WRLOCK(lock) pthread_rwlock_wrlock(&lock)
#define UNLOCK(lock) pthread_rwlock_unlock(&lock)

#include "../headers_RFS/ext2_block_types.h"

#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/stat.h>


#include <linux/inotify.h>


#include <libmemcached/memcached.h>

#include "../headers_RFS/manage_fs.h"
#include "../commons/collections/queue.h"
#include "../commons/collections/list.h"
#include "../commons/string.h"
#include "../commons/log.h"
#include "../commons/config.h"



/*****************************************************
				START FILE ACCESS SINC STRUCTS
******************************************************/

typedef struct {
	pthread_rwlock_t rwlock;
	uint32_t is_working; //Para saber cuantas operaciones lo estan usando
	char* path; //PATH COMPLETO! Para conseguir el file name usar la funcion active_file_get_file_name
}active_file_t;

typedef struct {
	t_list* file_list; //Lista de active_file_t*
	pthread_mutex_t access;
} active_file_list_t;

/*****************************************************
				END   FILE ACCESS SINC STRUCTS
******************************************************/

/*****************************************************
				START THREADING STRUCTS
******************************************************/

typedef struct{
	uint64_t value;
	pthread_rwlock_t rwlock;
}sync_counter_t;

typedef struct {
	void* file;
	active_file_list_t* active_file_list;
	pthread_mutex_t superblock_mutex; // Cuando se quiera modificar superbloque, bloquear y desbloquear (Free_Blocks, Free_Inodes)
	sync_counter_t* fs_sleep;
	sync_counter_t* writes_to_sync;
	pthread_mutex_t* wblocks_mutex; //Para cuando se quiera llamar a Free_Blocks
	pthread_mutex_t* winodes_mutex; //Para cuando se quiera llamar a Free_Inodes
	pthread_mutex_t* wdir_count_mutex; //Para cuando se quiera modificar el USED_DIR_COUNT de los descriptores
	t_log* server_log;
}s_Mapped_File_t;

typedef struct {
  char* path;
  sync_counter_t* sleep_counter;
  t_log* logger;
}inotify_params;

typedef struct{
	char* path;
	mode_t mode;
	void* write_buffer;
	bool file;
	uint32_t offset;
	uint32_t bytes_to_read;
	uint32_t bytes_to_write;
	uint64_t size;
	int32_t socket;
	memcached_st* memc;
}params_t;

typedef struct{
	char* path;
	void* data;
	uint32_t read_bytes;
	uint32_t error;
}operation_result_t;

typedef struct{
	operation_result_t* (*run) (params_t*, s_Mapped_File_t*);
	params_t* params;
	void (*deliver_response) (const operation_result_t* const result, int32_t socket, s_Mapped_File_t*);

}task_t;

typedef struct {
	t_queue* queue;
	sem_t available_elements;
	pthread_mutex_t access;
}s_queue_t;

typedef struct {
	s_Mapped_File_t* filesystem;
	s_queue_t* task_queue;
	char* cache_server;
	uint32_t cache_port;
}Worker_params_t;

/*****************************************************
				END   THREADING STRUCTS
******************************************************/

/*****************************************************
				START S_MAPPED_FILE CONSTRUCTOR/DESTRUCTOR
******************************************************/

s_Mapped_File_t* s_Mapped_File_init(char* PATH, char* log_file_path);
int 			 s_Mapped_File_destroy(s_Mapped_File_t* mapped_file);

/*****************************************************
				END   S_MAPPED_FILE CONSTRUCTOR/DESTRUCTOR
******************************************************/

/*****************************************************
				START PUBLIC COUNTER FUNCTIONS
******************************************************/

void update_counter(sync_counter_t* wcounter);

/*****************************************************
				END   PUBLIC COUNTER FUNCTIONS
******************************************************/

/*****************************************************
				START S_QUEUE FUNCTIONS
******************************************************/

s_queue_t* s_queue_create();
void s_queue_push(s_queue_t *, void *element);
void*s_queue_pop(s_queue_t *);
void s_queue_destroy(s_queue_t *);

/*****************************************************
				END   S_QUEUE FUNCTIONS
******************************************************/

/*****************************************************
				START ACTIVE LIST FUNCTIONS
******************************************************/

active_file_list_t* active_file_list_create();
active_file_t* active_file_list_find(active_file_list_t*, char* );
void active_file_list_remove_and_destroy_element(active_file_list_t*,active_file_t*);
void active_file_list_destroy(active_file_list_t*);

/*****************************************************
				END   ACTIVE LIST FUNCTIONS
******************************************************/


/*****************************************************
				START ACTIVE FILE FUNCTIONS
******************************************************/

void active_file_finished_using(active_file_list_t*,active_file_t*);
char* active_file_get_file_name(active_file_t* a_file);

/*****************************************************
				END   ACTIVE FILE FUNCTIONS
******************************************************/

/*****************************************************
				START POOL WORKER
******************************************************/

void worker(void* worker_params );

void syncer(void* params);

void free_result(operation_result_t* result);

void th_inotify(void* params);

/*****************************************************
				END   POOL WORKER
******************************************************/



/*****************************************************
				START GET SET FREE BLOCKS/INODES
******************************************************/

uint32_t* Get_Free_Blocks(s_Mapped_File_t* s_mapped_file, uint32_t how_many);
uint32_t* Get_Free_Inodes(s_Mapped_File_t* s_mapped_file, uint32_t how_many);
void setFreeBlockBitmap(s_Mapped_File_t* s_mapped_file,uint32_t block_number);
void setFreeInodeBitmap(s_Mapped_File_t* s_mapped_file,uint32_t inode_number);

/*****************************************************
				END   GET SET FREE BLOCKS/INODES
******************************************************/

void thread_sleep(sync_counter_t* sleep_value);

#endif /* SERVER_THREADING_H_ */
