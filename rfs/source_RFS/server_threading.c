/*
 * server_trheading.c
 *
 *  Created on: May 15, 2012
 *      Author: ezequieldariogambaccini
 */

#include "../headers_RFS/server_threading.h"

#define WRITES_TO_SYNC  (8*1024)

#define EVENT_SIZE        ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( EVENT_SIZE + 100 )

static active_file_t* active_file_create(char* path);

static void active_file_destroy(void*);

static void reset_counter(sync_counter_t* wcounter);
static uint64_t read_counter(sync_counter_t* wcounter);
static sync_counter_t* create_counter();
static void destroy_counter(sync_counter_t* wcounter);

/*****************************************************
				START WRITE_COUNTER_T FUNCTIONS
******************************************************/

void set_counter(sync_counter_t* wcounter, uint64_t new_value){
	WRLOCK(wcounter->rwlock);
	wcounter->value = new_value;
	UNLOCK(wcounter->rwlock);
}

void update_counter(sync_counter_t* wcounter){
	WRLOCK(wcounter->rwlock);
	wcounter->value++;
	UNLOCK(wcounter->rwlock);
}

static void reset_counter(sync_counter_t* wcounter){
	WRLOCK(wcounter->rwlock);
	wcounter->value = 0;
	UNLOCK(wcounter->rwlock);
}

static uint64_t read_counter(sync_counter_t* wcounter){
	uint64_t value = 0;

	WRLOCK(wcounter->rwlock);
	value = wcounter->value;
	UNLOCK(wcounter->rwlock);

	return value;
}

static sync_counter_t* create_counter(){
	sync_counter_t* wcounter = calloc(1,sizeof(sync_counter_t));
	wcounter->value = 0;
	pthread_rwlock_init(&(wcounter->rwlock), NULL);

	return wcounter;

}

static void destroy_counter(sync_counter_t* wcounter){
	pthread_rwlock_destroy(&(wcounter->rwlock));
	free(wcounter);
}


/*****************************************************
				END   WRITE_COUNTER_T Functions
******************************************************/



/*****************************************************
				START S_MAPPED_FILE FUNCTIONS
******************************************************/

s_Mapped_File_t*  s_Mapped_File_init(char* PATH, char* log_file_path){

	void* fs = Map_FS(PATH);

	SuperBlock_read(fs);

	s_Mapped_File_t* s_Mapped_File = calloc(1,sizeof(s_Mapped_File_t));

	s_Mapped_File->file = fs;


	pthread_mutex_init(&(s_Mapped_File->superblock_mutex),NULL);

	s_Mapped_File->wblocks_mutex = calloc(Super_constants.TOTAL_GROUPS,sizeof(pthread_mutex_t));
	s_Mapped_File->winodes_mutex = calloc(Super_constants.TOTAL_GROUPS,sizeof(pthread_mutex_t));
	s_Mapped_File->wdir_count_mutex  = calloc(Super_constants.TOTAL_GROUPS,sizeof(pthread_mutex_t));

	for (int var = 0; var < Super_constants.TOTAL_GROUPS; ++var) {
		pthread_mutex_init(&s_Mapped_File->wblocks_mutex[var],NULL);
		pthread_mutex_init(&s_Mapped_File->winodes_mutex[var],NULL);
		pthread_mutex_init(&s_Mapped_File->wdir_count_mutex[var],NULL);
	}

	s_Mapped_File->server_log = log_create(log_file_path,"Yanero_Server",true, LOG_LEVEL_DEBUG);

	s_Mapped_File->active_file_list = active_file_list_create();

	s_Mapped_File->writes_to_sync = create_counter();
	s_Mapped_File->fs_sleep = create_counter();

	return s_Mapped_File;
}

int s_Mapped_File_destroy(s_Mapped_File_t* s_Mapped_File){

	Unmap_FS(s_Mapped_File->file);
	destroy_counter(s_Mapped_File->writes_to_sync);
	destroy_counter(s_Mapped_File->fs_sleep);

	pthread_mutex_destroy(&(s_Mapped_File->superblock_mutex));

	for (int var = 0; var < Super_constants.TOTAL_GROUPS; ++var) {
		pthread_mutex_destroy(&s_Mapped_File->wblocks_mutex[var]);
		pthread_mutex_destroy(&s_Mapped_File->winodes_mutex[var]);
		pthread_mutex_destroy(&s_Mapped_File->wdir_count_mutex[var]);
	}

	free(s_Mapped_File->wblocks_mutex);
	free(s_Mapped_File->winodes_mutex);
	free(s_Mapped_File->wdir_count_mutex);

	active_file_list_destroy(s_Mapped_File->active_file_list);

	destroy_counter(s_Mapped_File->writes_to_sync);
	destroy_counter(s_Mapped_File->fs_sleep);

	log_destroy(s_Mapped_File->server_log);

	free(s_Mapped_File);

	return 1;

}

/*****************************************************
				END   S_MAPPED_FILE FUNCTIONS
******************************************************/



/*****************************************************
				START ACTIVE FILE LIST FUNCTIONS
******************************************************/

active_file_list_t* active_file_list_create(){
	active_file_list_t* a_f_l = calloc(1,sizeof(active_file_list_t));
	a_f_l->file_list = list_create();
	pthread_mutex_init((&(a_f_l->access)),NULL);
	return a_f_l;
}


active_file_t* active_file_list_find(active_file_list_t* a_f_l, char* path){

	bool cmp_str (void* elem){
		active_file_t* a_file = (active_file_t*) elem;
		if(strlen(path)==strlen(a_file->path)){
			if(strncmp(path, a_file->path,strlen(path)) == 0)
				return true;
		}
		return false;
	}

	active_file_t* found;
	pthread_mutex_lock(&(a_f_l->access));
	if((found = list_find(a_f_l->file_list, cmp_str)) == NULL){ //Si no lo encuentro, lo creo y agrego a la lista
		found = active_file_create(path);
		list_add(a_f_l->file_list,(void*) found);
	}
	else{
		pthread_rwlock_wrlock(&(found->rwlock)); //Si lo encuentro, le sumo uno
		found->is_working++;
		pthread_rwlock_unlock(&(found->rwlock));
	}
	pthread_mutex_unlock(&(a_f_l->access));

	return found;
}

void active_file_list_remove_and_destroy_element(active_file_list_t* a_f_l, active_file_t* a_file){

	bool cmp_str (void* elem){
		active_file_t* list_elem = (active_file_t*) elem;
		if(strncmp(a_file->path, list_elem->path,strlen(a_file->path)+1) == 0)
			return true;
		return false;
	}

	list_remove_and_destroy_by_condition(a_f_l->file_list,cmp_str,active_file_destroy );
}

void active_file_list_destroy(active_file_list_t* a_f_l){
	pthread_mutex_lock(&(a_f_l->access));
	list_destroy_and_destroy_elements(a_f_l->file_list, active_file_destroy);
	pthread_mutex_unlock(&(a_f_l->access));
	pthread_mutex_destroy(&(a_f_l->access));
	free(a_f_l);
}

/*****************************************************
				END   ACTIVE FILE LIST FUNCTIONS
******************************************************/


/*****************************************************
				START ACTIVE FILE FUNCTIONS
******************************************************/

static active_file_t* active_file_create(char* path	){
	active_file_t* a_file = calloc(1, sizeof(active_file_t));
	a_file->is_working = 1;
	pthread_rwlock_init(&(a_file->rwlock), NULL);
	a_file->path = strdup(path);
	return a_file;
}


static void active_file_destroy(void* data){
	active_file_t* a_file = (active_file_t*) data;
	free(a_file->path);
	pthread_rwlock_destroy(&(a_file->rwlock));
	free(a_file);
}


char* active_file_get_file_name(active_file_t* a_file){

	if(strlen(a_file->path) == 1)//Es root
		return strdup(a_file->path);
	else{
		uint32_t last = 0;
		char** splitted_path = string_split(a_file->path,"/");
		for(; splitted_path[last] != NULL ; last++);

		char* file_name = strdup(splitted_path[last-1]);

		for (int var = 0; var < last; ++var)
			free(splitted_path[var]);
		free(splitted_path);

		return file_name;
	}
}


void active_file_finished_using(active_file_list_t* a_f_l,active_file_t* a_file){
	pthread_mutex_lock(&(a_f_l->access));

	WRLOCK(a_file->rwlock);
	a_file->is_working--;
	UNLOCK(a_file->rwlock);

	if(a_file->is_working == 0)
		active_file_list_remove_and_destroy_element(a_f_l, a_file);


	pthread_mutex_unlock(&(a_f_l->access));
}

/*****************************************************
				END   ACTIVE FILE FUNCTIONS
******************************************************/

/*****************************************************
				START S_QUEUE OPERATIONS
******************************************************/

s_queue_t * s_queue_create(){
	s_queue_t * s_queue = calloc(1, sizeof(s_queue_t));
	s_queue->queue = queue_create();
	sem_init(&(s_queue->available_elements),0,0);
	pthread_mutex_init((&(s_queue->access)),NULL);
	return s_queue;
}

void s_queue_push(s_queue_t *s_queue, void *element){
	pthread_mutex_lock(&(s_queue->access));
	queue_push((s_queue->queue),element);
	pthread_mutex_unlock(&(s_queue->access));
	sem_post(&(s_queue->available_elements));
}

void *s_queue_pop(s_queue_t *s_queue){

	sem_wait(&(s_queue->available_elements));
	pthread_mutex_lock(&(s_queue->access));
	void* elem =  queue_pop(s_queue->queue);
	pthread_mutex_unlock(&(s_queue->access));
	return elem;
}

void s_queue_destroy(s_queue_t * s_queue){

	queue_destroy(s_queue->queue);
	sem_destroy(&(s_queue->available_elements));
	pthread_mutex_destroy(&(s_queue->access));
	free(s_queue);
}

/*****************************************************
				END   S_QUEUE OPERATIONS
******************************************************/


/*****************************************************
				START POOL WORKER
******************************************************/


void syncer(void* params){

	s_Mapped_File_t* s_mapped_file = (s_Mapped_File_t*) params;

	while(true){
		sleep(10);
		if (read_counter(s_mapped_file->writes_to_sync) >= WRITES_TO_SYNC){
			log_info(s_mapped_file->server_log, "%s","Syncing filesystem");
			msync(s_mapped_file->file,FS_SIZE,MS_SYNC);
			log_info(s_mapped_file->server_log,"%s","Sync finished");
			reset_counter(s_mapped_file->writes_to_sync);
		}
	}
}


void th_inotify(void* params){

  int length, offset = 0;
  int inotify_descriptor;
  int watched_descriptor;
  char buffer[EVENT_BUF_LEN];

  static uint64_t old_value = 0;

  inotify_params* iparams = (inotify_params*) params;


  inotify_descriptor = inotify_init();

  if ( inotify_descriptor < 0 )
    perror( "inotify_init" );

  char** file_tree = split_path(iparams->path);

  int i = 0;
  for(;file_tree[i+1] != NULL ; i++ );

  watched_descriptor = inotify_add_watch( inotify_descriptor, file_tree[i-1], IN_MODIFY );

  for (int var = 0; var < i; ++var)
	  free(file_tree[var]);
  free(file_tree);

  while(true){

    length = read( inotify_descriptor, buffer, EVENT_BUF_LEN );

    if ( length < 0 )
      perror( "read" );

    offset = 0;

    while(offset < length){

		struct inotify_event *event = ( struct inotify_event * ) &buffer[offset];

		if ( event->mask & IN_MODIFY ) {

		  t_config* config = config_create(iparams->path);
		  uint64_t new_value = config_get_int_value(config, "SLEEP_VALUE");
		  config_destroy(config);

		  if(new_value != old_value){
			  old_value = new_value;
			  log_info(iparams->logger,"Updating Sleep Value");
			  log_info(iparams->logger,"%s %ld ms","New Sleep Value: ",new_value);
	          set_counter(iparams->sleep_counter,new_value);
		  }

		}


		offset += sizeof (struct inotify_event) + event->len;
    }


  }
     inotify_rm_watch( inotify_descriptor, watched_descriptor);

     close( inotify_descriptor );
}


void worker(void* worker_params ){

	Worker_params_t* w_params = (Worker_params_t*) worker_params;
	s_queue_t* task_queue = w_params->task_queue;

	s_Mapped_File_t* s_mapped_file = w_params->filesystem;

	memcached_st* memc = memcached_create(NULL);
	memcached_behavior_set(memc,MEMCACHED_BEHAVIOR_BINARY_PROTOCOL,true);
	memcached_server_add(memc,w_params->cache_server,w_params->cache_port);

	while(true){

		task_t* task = (task_t*) s_queue_pop(task_queue);

		task->params->memc = memc;

		thread_sleep(s_mapped_file->fs_sleep);

		operation_result_t* solved;

		if(strlen(task->params->path)<= 40){
			solved = task->run(task->params, w_params->filesystem);
		}
		else{
			solved = calloc(1,sizeof(operation_result_t));
			solved->path = strdup(task->params->path);
			solved->data = NULL;
			errno = ENAMETOOLONG;
		}

		solved->error = errno;
		errno = 0;

		task->deliver_response(solved, task->params->socket, s_mapped_file);

		free_result(solved);

		free(task->params->path);
		free(task->params);
		free(task);

	}

	memcached_free(memc);

	pthread_exit(NULL);

}

void free_result(operation_result_t* result){
	free(result->data);
	free(result->path);
	free(result);
}

/*****************************************************
				END   POOL WORKER
******************************************************/


/*****************************************************
				START GET SET FREE BLOCKS/INODES
******************************************************/

static void CleanBlocks(s_Mapped_File_t* s_mapped_file,uint32_t* blocks_indexes){

	for (int count = 0; blocks_indexes[count+1] != 0; count++){

		void* block = s_mapped_file->file + blocks_indexes[count] * Super_constants.BLOCK_SIZE;

		memset(block,0,Super_constants.BLOCK_SIZE);
	}

	return;
}


uint32_t* Get_Free_Blocks(s_Mapped_File_t* s_mapped_file, uint32_t how_many){
	//Ahora es safe
	void* fs = s_mapped_file->file;

	SuperBlock_t* superblock = SuperBlock_read(fs);

	pthread_mutex_lock(&(s_mapped_file->superblock_mutex));

	if(how_many > superblock->TOTAL_UNALLOCATED_BLOCKS){
		log_error(s_mapped_file->server_log,"All blocks allocated!");
		errno = ENOSPC;
		pthread_mutex_unlock(&(s_mapped_file->superblock_mutex));
		return NULL;
	}
	else
		superblock->TOTAL_UNALLOCATED_BLOCKS -= how_many;

	pthread_mutex_unlock(&(s_mapped_file->superblock_mutex));

	uint32_t* blocks_to_return = calloc(how_many,sizeof(uint32_t));

	uint32_t mem_index = 0;

	for (int group_number = 0; (group_number < Super_constants.TOTAL_GROUPS) &&( how_many > 0); ++group_number) {
		pthread_mutex_lock(&(s_mapped_file->wblocks_mutex[group_number]));

		Group_Descriptor_t* group_descriptor = Group_Descriptor_read(fs,group_number);

		if(group_descriptor->UNALLOCATED_BLOCKS == 0){
			pthread_mutex_unlock(&(s_mapped_file->wblocks_mutex[group_number]));
			continue;
		}

		if(how_many <= group_descriptor->UNALLOCATED_BLOCKS){

			group_descriptor->UNALLOCATED_BLOCKS -= how_many;

			uint32_t* free_blocks = Get_Free_Blocks_In_Group(fs,how_many,group_descriptor);

			if(free_blocks == NULL){
				pthread_mutex_unlock(&(s_mapped_file->wblocks_mutex[group_number]));
				return NULL;
			}

			memcpy(&blocks_to_return[mem_index],free_blocks,how_many * sizeof(uint32_t));
			free(free_blocks);
			how_many = 0;
		}
		else{

			how_many -= group_descriptor->UNALLOCATED_BLOCKS;

			uint32_t* free_blocks = Get_Free_Blocks_In_Group(fs,group_descriptor->UNALLOCATED_BLOCKS, group_descriptor);

			if(free_blocks == NULL){
				pthread_mutex_unlock(&(s_mapped_file->wblocks_mutex[group_number]));
				return NULL;
			}

			memcpy(&blocks_to_return[mem_index],free_blocks,group_descriptor->UNALLOCATED_BLOCKS * sizeof(uint32_t));
			free(free_blocks);

			mem_index += group_descriptor->UNALLOCATED_BLOCKS;

			group_descriptor->UNALLOCATED_BLOCKS = 0;
		}

		pthread_mutex_unlock(&(s_mapped_file->wblocks_mutex[group_number]));

	}


	CleanBlocks(s_mapped_file,blocks_to_return);

	return blocks_to_return;

}

uint32_t* Get_Free_Inodes(s_Mapped_File_t* s_mapped_file, uint32_t how_many){

	void* fs = s_mapped_file->file;

	SuperBlock_t* superblock = SuperBlock_read(fs);

	pthread_mutex_lock(&(s_mapped_file->superblock_mutex));

	if(how_many > superblock->TOTAL_UNALLOCATED_INODES){
		log_error(s_mapped_file->server_log,"All inodes allocated!");
		errno = ENOSPC;
		pthread_mutex_unlock(&(s_mapped_file->superblock_mutex));
		return NULL;
	}
	else
		superblock->TOTAL_UNALLOCATED_INODES -= how_many;
	pthread_mutex_unlock(&(s_mapped_file->superblock_mutex));

	uint32_t* inodes_to_return = calloc(how_many,sizeof(uint32_t));

	uint32_t mem_index = 0;

	for (int group_number = 0; (group_number < Super_constants.TOTAL_GROUPS) &&( how_many > 0); group_number++) {

		pthread_mutex_lock(&(s_mapped_file->winodes_mutex[group_number]));

		Group_Descriptor_t* group_descriptor = Group_Descriptor_read(fs,group_number);

		if (how_many <= group_descriptor->UNALLOCATED_INODES) {

			group_descriptor->UNALLOCATED_INODES -= how_many;

			uint32_t* free_inodes = Get_Free_Inodes_In_Group(fs,how_many,group_descriptor,group_number);

			if(free_inodes == NULL){
				pthread_mutex_unlock(&(s_mapped_file->winodes_mutex[group_number]));
				return NULL;
			}

			memcpy(&inodes_to_return[mem_index],free_inodes,how_many*sizeof(uint32_t));
			free(free_inodes);
			how_many = 0;

		} else {

			if(group_descriptor->UNALLOCATED_INODES == 0){
				pthread_mutex_unlock(&(s_mapped_file->winodes_mutex[group_number]));
				continue;
			}

			how_many -= group_descriptor->UNALLOCATED_INODES;

			group_descriptor->UNALLOCATED_INODES = 0;

			uint32_t* free_inodes = Get_Free_Inodes_In_Group(fs,how_many,group_descriptor,group_number);

			if(free_inodes == NULL){
				pthread_mutex_unlock(&(s_mapped_file->winodes_mutex[group_number]));
				return NULL;
			}

			memcpy(&inodes_to_return[mem_index],free_inodes,group_descriptor->UNALLOCATED_INODES);
			free(free_inodes);

			mem_index += group_descriptor->UNALLOCATED_INODES;

		}

		pthread_mutex_unlock(&(s_mapped_file->winodes_mutex[group_number]));

	}

	return inodes_to_return;

}

void setFreeBlockBitmap(s_Mapped_File_t* s_mapped_file,uint32_t block_number){

	uint32_t group_number = (block_number-1)/Super_constants.BLOCKS_PER_GROUP;

	pthread_mutex_lock(&s_mapped_file->wblocks_mutex[group_number]);

	Group_Descriptor_t* group_desc = Group_Descriptor_read(s_mapped_file->file,group_number);

	uint32_t bitmap_addr = group_desc->BITMAP_BLOCK_ADDRESS * Super_constants.BLOCK_SIZE;

	uint32_t local_index =  (block_number-1)%(Super_constants.BLOCKS_PER_GROUP);

	t_bitarray* bitmap =  bitarray_create(((char*) (s_mapped_file->file + bitmap_addr)), Super_constants.BLOCK_SIZE);

	bitarray_clean_bit(bitmap,local_index);

	bitarray_destroy(bitmap);

	group_desc->UNALLOCATED_BLOCKS+=1;

	pthread_mutex_lock(&s_mapped_file->superblock_mutex);

	SuperBlock_t* super = SuperBlock_read(s_mapped_file->file);

	super->TOTAL_UNALLOCATED_BLOCKS+=1;

	pthread_mutex_unlock(&s_mapped_file->superblock_mutex);

	pthread_mutex_unlock(&s_mapped_file->wblocks_mutex[group_number]);

	return;

}

void setFreeInodeBitmap(s_Mapped_File_t* s_mapped_file,uint32_t inode_number){

	uint32_t group_number = Inode_locateGroup(inode_number);

	pthread_mutex_lock(&s_mapped_file->winodes_mutex[group_number]);

	Group_Descriptor_t* group_desc = Group_Descriptor_read(s_mapped_file->file,group_number);

	uint32_t bitmap_addr = group_desc->BITMAP_INODE_ADDRESS * Super_constants.BLOCK_SIZE;

	uint32_t local_index =  Inode_getLocalIndex(inode_number);

	t_bitarray* bitmap =  bitarray_create(((char*) (s_mapped_file->file + bitmap_addr)), Super_constants.BLOCK_SIZE);

	bitarray_clean_bit(bitmap,local_index);

	bitarray_destroy(bitmap);

	group_desc->UNALLOCATED_INODES+=1;

	pthread_mutex_lock(&s_mapped_file->superblock_mutex);

	SuperBlock_t* super = SuperBlock_read(s_mapped_file->file);

	super->TOTAL_UNALLOCATED_INODES+=1;

	pthread_mutex_unlock(&s_mapped_file->superblock_mutex);

	pthread_mutex_unlock(&s_mapped_file->winodes_mutex[group_number]);

	return;
}


/*****************************************************
				END   GET SET FREE BLOCKS/INODES
******************************************************/

void thread_sleep(sync_counter_t* sleep_value){
	uint64_t value = read_counter(sleep_value);

	if(value)
		usleep(value*1000);//para que haga sleep en milisegundos
}













