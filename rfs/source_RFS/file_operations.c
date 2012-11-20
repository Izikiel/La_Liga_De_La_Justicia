/*
 * file_operations.c
 *
 *  Created on: 26/05/2012
 *      Author: utnso
 */

#include "../headers_RFS/file_operations.h"
#define EMPTY 0
#ifndef ROOT_INODE_NUMBER
#define ROOT_INODE_NUMBER 2
#endif



/*****************************************************
				START PRIVATE FUNCTIONS PROTOTYPES
******************************************************/

static uint32_t search_in_directory(s_Mapped_File_t* s_mapped_file , active_file_t** sub_paths, entry_t** old_entries);

static inode_t* inode_of_file(s_Mapped_File_t* s_mapped_file, const char* path, uint32_t* inode_number);

static entry_t** read_directory(s_Mapped_File_t* s_mapped_file, inode_t* inode);

static cache_key_t* generate_key(const char* path, uint32_t block_number);

static cache_fs_data_t* cache_fs_get_set(const params_t* params, uint32_t block_number, s_Mapped_File_t* s_Mapped_file, uint32_t* file_data);

static void cache_set(const params_t* params, uint32_t block_number, s_Mapped_File_t* s_Mapped_file, uint32_t* file_data);

static void cache_delete(const params_t* params, uint32_t block_number, s_Mapped_File_t* s_Mapped_file);

static void Inode_Indirect_Attach1 (s_Mapped_File_t* s_mapped_file, uint32_t indirect_block,uint32_t block_offset, uint32_t blocks_to_attach);

static int32_t Inode_Indirect_Attach2 (s_Mapped_File_t* s_mapped_file, uint32_t indirect_block, uint32_t blocks_to_attach );

static void check_and_set_Ind(uint32_t* indirection, s_Mapped_File_t* s_mapped_file);

static int32_t check_and_attach1(s_Mapped_File_t* s_mapped_file,uint32_t block_number, uint32_t blocks_to_attach);

static 	entry_t* create_entry(char* file_name, uint32_t inode_number);

static bool attach_entry(s_Mapped_File_t* s_mapped_file, inode_t* inode, entry_t* an_entry);

static void copy_entry(entry_t* to, entry_t* from);

static uint32_t  Attach_Blocks(s_Mapped_File_t* s_mapped_file, inode_t* inode,uint32_t  new_blocks_amount);

static void Remove_Blocks(s_Mapped_File_t* s_mapped_file,inode_t* inode, int64_t block_amount_to_remove, params_t* params);

static int32_t* find_block_coordinates (uint32_t block_index);

static void Remove_Block(s_Mapped_File_t* s_mapped_file,inode_t* inode, uint32_t block_index);

static void RemoveIndirect(s_Mapped_File_t* s_mapped_file,inode_t* inode, uint32_t block_index);

static bool deleteIfEmpty(s_Mapped_File_t* s_mapped_file, uint32_t block_index);

static uint32_t get_padded_size(entry_t* an_entry);

/*****************************************************
 				END   PRIVATE FUNCTIONS PROTOTYPES
******************************************************/





/*****************************************************
				START PRIVATE FUNCTIONS
******************************************************/

static uint32_t get_padded_size(entry_t* an_entry){
	uint32_t unpad_size = sizeof(entry_t)+ an_entry->name_len;

	return (unpad_size%4)>0 ? (unpad_size+4-unpad_size%4) : unpad_size;

}

static void delete_entry(s_Mapped_File_t* s_mapped_file, inode_t* inode, active_file_t* a_file){

	uint32_t* data_address = Inode_Get_Data_Adressess(s_mapped_file->file,inode, inode->blocks * 512);

	if(data_address == NULL){
		perror("Error consiguiendo data_address!");
		return ;
	}

	void* fs = s_mapped_file->file;
	char* file_name = active_file_get_file_name(a_file);

	bool deleted = false;

	for (int var = 0; data_address[var] != EMPTY && !deleted; ++var) {

		void* fs_data_offset = fs + data_address[var] * Super_constants.BLOCK_SIZE;
		void* iterate = fs_data_offset;
		entry_t* previous_entry = NULL;
		uint32_t acum_len = 0;
		entry_t* an_entry = (entry_t*)iterate;

		uint32_t padded_size = get_padded_size(an_entry);

		if((an_entry->name_len == strlen(file_name)) &&
		   (strncmp(an_entry->file_name,file_name,strlen(file_name))== 0 )){
			if(an_entry->entry_len == (Super_constants.BLOCK_SIZE-padded_size)){
				Remove_Block(s_mapped_file,inode,var);
				inode->size -= padded_size;
				inode->blocks -= Super_constants.BLOCK_SIZE/512;
				deleted = true;
			}
			else{
				entry_t* next_entry = iterate + an_entry->entry_len;
				next_entry->entry_len += an_entry->entry_len;
				copy_entry(an_entry,next_entry);
				deleted = true;
			}
		}
		else{
			while(iterate < fs_data_offset + Super_constants.BLOCK_SIZE){
				previous_entry = an_entry;

				acum_len += an_entry->entry_len;
				iterate += an_entry->entry_len;

				an_entry = (entry_t*)iterate;

				if((an_entry->name_len == strlen(file_name)) &&
				   (strncmp(an_entry->file_name,file_name,strlen(file_name))== 0 )){
						if(an_entry->entry_len == (Super_constants.BLOCK_SIZE-acum_len)){
							previous_entry->entry_len = previous_entry->entry_len + an_entry->entry_len;
							deleted = true;
							break;
						}
						else{
							entry_t* next_entry = iterate + an_entry->entry_len;
							next_entry->entry_len += an_entry->entry_len;
							copy_entry(an_entry,next_entry);
							deleted = true;
							break;
						}
				}
			}
		}
	}
	free(file_name);
	free(data_address);

	return ;
}

static 	entry_t* create_entry(char* file_name, uint32_t inode_number){
	entry_t* new_entry = calloc(1,sizeof(entry_t) + strlen(file_name));
	memcpy(new_entry->file_name,file_name,strlen(file_name));
	new_entry->name_len = strlen(file_name);
	new_entry->entry_len = sizeof(entry_t)+strlen(file_name);
	new_entry->inode_number = inode_number;
	free(file_name);

	return new_entry;
}

static void Inode_Indirect_Attach1 (s_Mapped_File_t* s_mapped_file, uint32_t indirect_block,uint32_t block_offset, uint32_t blocks_to_attach){

	uint32_t* fs_with_offset = (uint32_t*)(s_mapped_file->file +indirect_block*Super_constants.BLOCK_SIZE);

	uint32_t* new_blocks = Get_Free_Blocks(s_mapped_file,blocks_to_attach);

	if((new_blocks == NULL) && (errno==ENOSPC))
		return;

	memcpy(&fs_with_offset[block_offset], new_blocks,blocks_to_attach * sizeof(uint32_t));

	free(new_blocks);

	return;
}

static int32_t Inode_Indirect_Attach2 (s_Mapped_File_t* s_mapped_file, uint32_t indirect_block, uint32_t blocks_to_attach ){

	uint32_t total_entries = Super_constants.BLOCK_SIZE/sizeof(uint32_t);

	uint32_t* this_block = (uint32_t*)(s_mapped_file->file + indirect_block * Super_constants.BLOCK_SIZE);

	uint32_t attached = 0;
	int32_t total_attached = 0;
	for (uint32_t entries = 0; entries<total_entries; entries++){

		check_and_set_Ind(&this_block[entries], s_mapped_file);

		attached = check_and_attach1(s_mapped_file,this_block[entries], blocks_to_attach);

		if(attached == -1)
			return total_attached;

		blocks_to_attach -= attached;
		total_attached += attached;

		if (blocks_to_attach == 0)
			break;
	}

	return total_attached;
}

static void check_and_set_Ind(uint32_t* indirection, s_Mapped_File_t* s_mapped_file){

	if(*indirection !=0)
		return;

	uint32_t* new_block = Get_Free_Blocks(s_mapped_file,1);
	*indirection = *new_block;
	free(new_block);

	return;


}

static int32_t check_and_attach1(s_Mapped_File_t* s_mapped_file,uint32_t block_number, uint32_t blocks_to_attach){

		uint32_t* entry_to_check = (uint32_t*)(s_mapped_file->file+ block_number * Super_constants.BLOCK_SIZE);

		uint32_t sub_entries = 0;
		for (; entry_to_check[sub_entries]!=0 && sub_entries < Super_constants.BLOCK_SIZE/sizeof(uint32_t); ++sub_entries);

		uint32_t slots_available = Super_constants.BLOCK_SIZE/sizeof(uint32_t) - sub_entries;
		uint32_t slots_to_copy = (slots_available > blocks_to_attach) ? blocks_to_attach : slots_available;

		if (slots_available > 0)
			Inode_Indirect_Attach1(s_mapped_file, block_number, sub_entries,slots_to_copy);

		if(errno == ENOSPC)
			return -1;

		return slots_to_copy;

}

static cache_fs_data_t* cache_fs_get_set(const params_t* params, uint32_t block_number, s_Mapped_File_t* s_Mapped_file, uint32_t* file_data){

	memcached_return return_status;
	uint32_t flags;

	uint32_t block_size;
	cache_key_t* my_key = generate_key(params->path, block_number);
	void* cache_data = memcached_get(params->memc, my_key->key, my_key->len , &block_size,&flags,&return_status);

	cache_fs_data_t* block_data = calloc(1,sizeof(cache_fs_data_t));
	block_data->from_cache = false;
	if (return_status == MEMCACHED_SUCCESS){
		block_data->data = cache_data;
		block_data->from_cache = true;
	}
	else{
		block_data->data = s_Mapped_file->file + file_data[block_number] * Super_constants.BLOCK_SIZE;
	    memcached_set(params->memc,my_key->key, my_key->len , block_data->data,Super_constants.BLOCK_SIZE, (time_t)0, (uint32_t)0);
	}
	free(my_key->key);
	free(my_key);
	return block_data;

}

static void cache_set(const params_t* params, uint32_t block_number, s_Mapped_File_t* s_Mapped_file, uint32_t* file_data){

	cache_key_t* my_key = generate_key(params->path, block_number);

	void* data = s_Mapped_file->file + file_data[block_number] * Super_constants.BLOCK_SIZE;

	memcached_return return_status = memcached_set(params->memc, my_key->key, my_key->len , data, Super_constants.BLOCK_SIZE, (time_t)0, (uint32_t)0);
    free(my_key->key);
    free(my_key);

	return;

}

static void cache_delete(const params_t* params, uint32_t block_number, s_Mapped_File_t* s_Mapped_file){
	cache_key_t* my_key = generate_key(params->path, block_number);

	memcached_delete(params->memc,my_key->key,my_key->len,0);

	free(my_key->key);
	free(my_key);

	return;
}

static cache_key_t* generate_key(const char* path, uint32_t block_number){

	cache_key_t* super_key = calloc(1, sizeof(cache_key_t));
	super_key->len = strlen(path) + sizeof(uint32_t);
	super_key->key = calloc(super_key->len, sizeof(char));
	memcpy(super_key->key,path, strlen(path));
	memcpy(&super_key->key[strlen(path)],&block_number, sizeof(uint32_t));
	return super_key;
}

static uint32_t search_in_directory(s_Mapped_File_t* s_mapped_file ,active_file_t** sub_paths, entry_t** old_entries){

	char* file_name = active_file_get_file_name(*sub_paths);

	for (uint32_t i = 0; old_entries[i] != NULL ; i++){

		if (*sub_paths == NULL){
			log_error(s_mapped_file->server_log,"%s %s","Archivo no encontrado", file_name);
			errno = ENOENT;
			free(file_name);
			free(old_entries);
			return NULL;
		}

		if(old_entries[i]->name_len == strlen(file_name)){
			if (strncmp(old_entries[i]->file_name, file_name, strlen(file_name))== 0){

				free(file_name);

				if(*(sub_paths+1) == NULL){
					uint32_t inode_number = old_entries[i]->inode_number;
					UNLOCK((*(sub_paths -1))->rwlock);
					UNLOCK((*sub_paths)->rwlock);
					free(old_entries);
					return inode_number;
				}

				inode_t* dir_to_list = Inode_Global_Read(s_mapped_file->file,old_entries[i]->inode_number);
				entry_t** new_entries = read_directory(s_mapped_file,dir_to_list);
				free(old_entries);
				UNLOCK((*(sub_paths -1))->rwlock);
				return search_in_directory(s_mapped_file, sub_paths + 1, new_entries);

			}
		}
	}

	for (int var = 0; sub_paths[var] != NULL; ++var)
		UNLOCK(sub_paths[var]->rwlock);
	UNLOCK((*(sub_paths -1))->rwlock);

	free(old_entries);

	log_error(s_mapped_file->server_log,"%s %s", "Archivo no encontrado",file_name);
	errno = ENOENT;

	free(file_name);

	return NULL;

}

static inode_t* inode_of_file(s_Mapped_File_t* s_mapped_file, const char* path, uint32_t* inode_number){

	char** splitted_path  = split_path(path);

	if(splitted_path == NULL )
		return NULL;

	uint32_t count = 0;

	for (; splitted_path[count] != NULL; count++);

	active_file_t** sub_paths = calloc(count+1, sizeof(active_file_t*));

	for (uint32_t var = 0;  var < count; ++var){
		sub_paths[var] = active_file_list_find(s_mapped_file->active_file_list, splitted_path[var]);
		RDLOCK(sub_paths[var]->rwlock);
	}

	for (int var = 0; var < count; ++var)
		free(splitted_path[var]);
	free(splitted_path);

	if(count == 1){
		*inode_number = ROOT_INODE_NUMBER;
		inode_t* found_file = Inode_Global_Read(s_mapped_file->file,ROOT_INODE_NUMBER);
		UNLOCK(sub_paths[0]->rwlock);
		active_file_finished_using(s_mapped_file->active_file_list, sub_paths[0]);
		free(sub_paths);
		return found_file;
	}

	entry_t** root_entries = read_directory(s_mapped_file,Inode_Global_Read(s_mapped_file->file,ROOT_INODE_NUMBER));

	*inode_number = search_in_directory(s_mapped_file, sub_paths + 1,root_entries); //El + 1 es para saltear root
	inode_t* found_file = NULL;

	if(*inode_number >0)
		 found_file = Inode_Global_Read(s_mapped_file->file, *inode_number);

	for (uint32_t var = 0;  var < count; ++var){
		active_file_finished_using(s_mapped_file->active_file_list, sub_paths[var]);
	}

	free(sub_paths);

	return found_file;
}

static entry_t** read_directory(s_Mapped_File_t* s_mapped_file, inode_t* inode){
	uint32_t* data_address = Inode_Get_Data_Adressess(s_mapped_file->file,inode, inode->size);

	if(data_address == NULL){
		perror("Error consiguiendo data_address!");
		return NULL;
	}

	void* fs = s_mapped_file->file;
	uint32_t number_of_entries =0;

	for (int var = 0; data_address[var] != EMPTY; ++var) {

		void* fs_data_offset = fs + data_address[var] * Super_constants.BLOCK_SIZE;
		void* iterate = fs_data_offset;

		while(iterate < fs_data_offset + Super_constants.BLOCK_SIZE){
			entry_t* an_entry = (entry_t*)iterate;
			if(an_entry->inode_number != 0)
				number_of_entries++;
			iterate += an_entry->entry_len;
		}
	}

	entry_t** entries = calloc(number_of_entries+1, sizeof(entry_t*));
	uint32_t entry_number = 0;
	for (int var = 0; data_address[var] != EMPTY; ++var) {

		void* fs_data_offset = fs + data_address[var] * Super_constants.BLOCK_SIZE;
		void* iterate = fs_data_offset;

		while(iterate < fs_data_offset + Super_constants.BLOCK_SIZE){
			entry_t* an_entry = (entry_t*)iterate;

			if(an_entry->inode_number != 0){
				entries[entry_number] = an_entry;
				entry_number++;
			}

			iterate += an_entry->entry_len;
		}
	}

	free(data_address);

	return entries;
}

static void copy_entry(entry_t* to, entry_t* from) {
	to->inode_number = from->inode_number;
	to->entry_len = from->entry_len;
	to->name_len = from->name_len;
	to->file_type = from->file_type;
	memcpy(to->file_name, from->file_name, from->name_len);
}

static bool attach_entry(s_Mapped_File_t* s_mapped_file, inode_t* inode, entry_t* new_entry){

	uint32_t* data_address = Inode_Get_Data_Adressess(s_mapped_file->file,inode, inode->blocks*512);

	if(data_address == NULL){
		perror("Error consiguiendo data_address!");
		return false;
	}

	void* fs = s_mapped_file->file;
	bool attached = false;

	for (int var = 0; data_address[var] != EMPTY; ++var) {

		void* fs_data_offset = fs + data_address[var] * Super_constants.BLOCK_SIZE;
		void* iterate = fs_data_offset;
		uint32_t acum_len = 0;

		while((iterate < fs_data_offset + Super_constants.BLOCK_SIZE) && !attached){
			entry_t* an_entry = (entry_t*)iterate;

			uint32_t padded_size = get_padded_size(an_entry);

			acum_len += padded_size;
			if(an_entry->entry_len > padded_size ){
				uint32_t free_space = an_entry->entry_len - padded_size;
				if( free_space> new_entry->entry_len){

					if(padded_size>8){
						an_entry->entry_len = padded_size;
						an_entry = (entry_t*)(iterate + padded_size);
						new_entry->entry_len = free_space;
					}
					else
						new_entry->entry_len = an_entry->entry_len;

					copy_entry(an_entry,new_entry);
					attached = true;
				}
			}
			iterate += an_entry->entry_len;
		}

		if(attached)
			break;

	}

	if(!attached){
		Attach_Blocks(s_mapped_file,inode,1);

		if(errno == ENOSPC)
			return false;
		inode->blocks += Super_constants.BLOCK_SIZE/512;
		inode->size += Super_constants.BLOCK_SIZE;
		if(!attach_entry(s_mapped_file,inode,new_entry))
			errno = ECANCELED;
		else
			attached = true;
	}

	free(data_address);

	return attached;
}

static void setFileInode(inode_t* an_inode,mode_t mode ){
	an_inode->blocks = 0;
	an_inode->links_count = 1;
	an_inode->mode = mode | EXT2_IFREG;
	an_inode->size = 0;

	for (int var = 0; var < 12; ++var)
		an_inode->pointer_data_0[var] = 0;

	an_inode->pointer_data_1 = 0;
	an_inode->pointer_data_2 = 0;
	an_inode->pointer_data_3 = 0;

}

static void setDirInode(inode_t* an_inode,mode_t mode, s_Mapped_File_t* s_mapped_file, uint32_t inode_number){

	uint32_t group_number = inode_number/Super_constants.INODES_PER_GROUP;

	Group_Descriptor_t* g_desc = Group_Descriptor_read(s_mapped_file->file,group_number);

	pthread_mutex_lock(&s_mapped_file->wdir_count_mutex[group_number]);
	g_desc->NUMBER_DIRECTORIES_GROUP++;
	pthread_mutex_unlock(&s_mapped_file->wdir_count_mutex[group_number]);

	an_inode->blocks = Super_constants.BLOCK_SIZE/512;
	an_inode->links_count = 1;
	an_inode->mode = mode | EXT2_IFDIR;
	an_inode->size = Super_constants.BLOCK_SIZE;

	for (int var = 0; var < 12; ++var)
		an_inode->pointer_data_0[var] = 0;

	an_inode->pointer_data_1 = 0;
	an_inode->pointer_data_2 = 0;
	an_inode->pointer_data_3 = 0;

}

static void Remove_Blocks(s_Mapped_File_t* s_mapped_file,inode_t* inode, int64_t block_amount_to_remove, params_t* params){

	uint32_t start_count = Size_In_Blocks(inode->size);

	block_amount_to_remove = (block_amount_to_remove>0)? -block_amount_to_remove : block_amount_to_remove;

	uint32_t start_remotion_index = start_count + block_amount_to_remove;//block_amount_to_remove es negativa

	for(;start_remotion_index < start_count;start_remotion_index++){
		Remove_Block(s_mapped_file,inode,start_remotion_index);
		cache_delete(params, start_remotion_index, s_mapped_file);
	}

	return;
}

static uint32_t  Attach_Blocks(s_Mapped_File_t* s_mapped_file, inode_t* inode,uint32_t  new_blocks_amount){//Agrega bloques a un inodo dado en el lugar que encuentre libre

	uint32_t total_entries_per_block = Super_constants.BLOCK_SIZE/sizeof(uint32_t);

	uint32_t level_0_limit = 12;

	///////LEVEL 0 /////////
	uint32_t sub_entries =0;
	for (; inode->pointer_data_0[sub_entries]!=0 && sub_entries < level_0_limit; ++sub_entries);

	uint32_t slots_available = level_0_limit - sub_entries;
	uint32_t slots_to_copy = (slots_available > new_blocks_amount) ? new_blocks_amount : slots_available;

	if (slots_available > 0){
		uint32_t* new_entries = Get_Free_Blocks(s_mapped_file,slots_to_copy);

		if(new_entries == NULL)
			return slots_available;

		memcpy(&inode->pointer_data_0[sub_entries], new_entries, slots_to_copy * sizeof(uint32_t));
		free(new_entries);
	}

	new_blocks_amount -= slots_to_copy;

	if (new_blocks_amount == 0)
		return 0;

	//////////LEVEL 1///////////////////
	check_and_set_Ind(&inode->pointer_data_1,s_mapped_file);

	int32_t attached = check_and_attach1(s_mapped_file,inode->pointer_data_1,new_blocks_amount);

	if(attached<0)
		return new_blocks_amount;

	new_blocks_amount -= attached;

	if (new_blocks_amount == 0)
		return 0;


	/////////LEVEL 2//////////////////////

	check_and_set_Ind(&inode->pointer_data_2,s_mapped_file);

	attached = Inode_Indirect_Attach2(s_mapped_file, inode->pointer_data_2,new_blocks_amount);

	if(errno == ENOSPC)
		return attached;

	new_blocks_amount -= attached;

	if (new_blocks_amount == 0)
		return 0;



	/////////LEVEL 3////////////////////

	check_and_set_Ind(&inode->pointer_data_3,s_mapped_file);
	uint32_t* entry_to_check = (uint32_t*)(s_mapped_file->file+ inode->pointer_data_3 * Super_constants.BLOCK_SIZE);

	for (int i = 0; i < total_entries_per_block && new_blocks_amount >0; ++i){

		attached = 0;

		check_and_set_Ind(&entry_to_check[i],s_mapped_file);

		if(errno == ENOSPC)
			return new_blocks_amount;

		attached = Inode_Indirect_Attach2(s_mapped_file, entry_to_check[i],new_blocks_amount);

		new_blocks_amount -= attached;

		if(errno == ENOSPC)
			return new_blocks_amount;

		if (new_blocks_amount == 0)
			return 0;
	}

	return new_blocks_amount;
}

static int32_t* find_block_coordinates (uint32_t block_index){

	uint64_t pointers_per_block = Super_constants.BLOCK_SIZE / sizeof(uint32_t);
	uint64_t squared_pointers_per_block = pointers_per_block * pointers_per_block;

	uint32_t bounds[4][2] = {{0,11},{0,0},{0,0},{0,0}};

	for (int i = 1; i < 4; ++i)
	{
		bounds[i][0] = bounds[i-1][1] +1;
		bounds[i][1] = bounds[i-1][1] + pointers_per_block;
		pointers_per_block *= Super_constants.BLOCK_SIZE / sizeof(uint32_t);
	}

	pointers_per_block = Super_constants.BLOCK_SIZE / sizeof(uint32_t);

	int32_t* block_coordinates = calloc(4, sizeof(int32_t));
	block_coordinates[0] = -1;
	block_coordinates[1] = -1;
	block_coordinates[2] = -1;
	block_coordinates[3] = -1;

	for (int level = 0; level < 4; ++level){
		if((bounds[level][0]<= block_index) && (block_index  <= bounds[level][1])){

			switch(level){
				case 0:
					block_coordinates[0] = block_index;
					break;

				case 1:
					block_coordinates[0] = 0;
					block_coordinates[1] = (block_index - bounds[level][0])%pointers_per_block; //Devuelve la cantidad de bloques, hay que restarle 1 para tener el indice al ultimo
					break;

				case 2:
					block_coordinates[2] = (block_index - bounds[level][0])/pointers_per_block;
					block_coordinates[1] = (block_index - bounds[level][0])%pointers_per_block;
					block_coordinates[0] = 0;
					break;

				case 3:
					block_coordinates[3] = (block_index - bounds[level][0])/squared_pointers_per_block;
					block_coordinates[2] = ((block_index - bounds[level][0])%squared_pointers_per_block)/pointers_per_block;
					block_coordinates[1] = ((block_index - bounds[level][0])%squared_pointers_per_block)%pointers_per_block;
					block_coordinates[0] = 0;
					break;
				}

			break;
		}
	}
	return block_coordinates;
}

static bool deleteIfEmpty(s_Mapped_File_t* s_mapped_file, uint32_t block_index){
	void* a_block = (s_mapped_file->file + block_index * Super_constants.BLOCK_SIZE);
	void* empty_block = calloc(Super_constants.BLOCK_SIZE,sizeof(uint8_t));

	bool empty = (memcmp(a_block,empty_block,Super_constants.BLOCK_SIZE) == 0);

	if(empty)
		setFreeBlockBitmap(s_mapped_file,block_index);

	free(empty_block);

	return empty;
}

static void RemoveIndirect(s_Mapped_File_t* s_mapped_file,inode_t* inode, uint32_t block_index){

	int32_t* coordinates = find_block_coordinates(block_index);

	uint32_t level=0;
	for(;level<4;level++){
		if(coordinates[level] == -1)
			break;
	}
	level--;


	switch(level){

		case 1:
			if(deleteIfEmpty(s_mapped_file, inode->pointer_data_1))
				inode->pointer_data_1 =0;
			break;

		case 2:{

				uint32_t* ind2 = (uint32_t*) (s_mapped_file->file + inode->pointer_data_2 * Super_constants.BLOCK_SIZE);
				if(deleteIfEmpty(s_mapped_file,ind2[coordinates[level]])){
					ind2[coordinates[level]] = 0;

					if(deleteIfEmpty(s_mapped_file,inode->pointer_data_2))
						inode->pointer_data_2 = 0;
				}
			}
			break;

		case 3:{
				uint32_t* ind3 = (uint32_t*) (s_mapped_file->file + inode->pointer_data_3 * Super_constants.BLOCK_SIZE);

				uint32_t* ind2 = (uint32_t*) (s_mapped_file->file + ind3[coordinates[level]] * Super_constants.BLOCK_SIZE);

				if(deleteIfEmpty(s_mapped_file,ind2[coordinates[level-1]])){
					ind2[coordinates[level-1]] = 0;

					if(deleteIfEmpty(s_mapped_file,ind3[coordinates[level]])){
						ind3[coordinates[level]] = 0;
						if(deleteIfEmpty(s_mapped_file, inode->pointer_data_3))
							inode->pointer_data_3 = 0;
					}
				}
			}
			break;
	}
	free(coordinates);
	return;
}

static void Remove_Block(s_Mapped_File_t* s_mapped_file,inode_t* inode, uint32_t block_index){

	int32_t* coordinates = find_block_coordinates(block_index);

	uint32_t level=0;
	for(;level<4;level++){
		if(coordinates[level] == -1)
			break;
	}
	level--;

	switch(level){

		case 0:{
				uint32_t block_number = inode->pointer_data_0[coordinates[level]];
				setFreeBlockBitmap(s_mapped_file,block_number);
				inode->pointer_data_0[coordinates[level]] = 0;
			}
			break;

		case 1:{
				uint32_t* ind1 = (uint32_t*) (s_mapped_file->file + inode->pointer_data_1 * Super_constants.BLOCK_SIZE);
				uint32_t block_number = ind1[coordinates[level]];
				setFreeBlockBitmap(s_mapped_file,block_number);
				ind1[coordinates[level]] = 0;
				RemoveIndirect(s_mapped_file,inode,block_index);
			}
			break;

		case 2:{
				uint32_t* ind2 = (uint32_t*) (s_mapped_file->file + inode->pointer_data_2 * Super_constants.BLOCK_SIZE);
				uint32_t* ind1 = (uint32_t*) (s_mapped_file->file + ind2[coordinates[level]] * Super_constants.BLOCK_SIZE);
				uint32_t block_number = ind1[coordinates[level-1]];
				setFreeBlockBitmap(s_mapped_file,block_number);
				ind1[coordinates[level-1]] = 0;
				RemoveIndirect(s_mapped_file,inode,block_index);
			}
			break;

		case 3:{
				uint32_t* ind3 = (uint32_t*) (s_mapped_file->file + inode->pointer_data_3 * Super_constants.BLOCK_SIZE);
				uint32_t* ind2 = (uint32_t*) (s_mapped_file->file + ind3[coordinates[level]] * Super_constants.BLOCK_SIZE);
				uint32_t* ind1 = (uint32_t*) (s_mapped_file->file + ind2[coordinates[level-1]] * Super_constants.BLOCK_SIZE);
				uint32_t block_number = ind1[coordinates[level-2]];
				setFreeBlockBitmap(s_mapped_file,block_number);
				ind1[coordinates[level-2]] = 0;
				RemoveIndirect(s_mapped_file,inode,block_index);
			}
			break;

	}
	free(coordinates);
	return;
}

static void Clear_Inode(inode_t* an_inode){
	an_inode->blocks = 0;
	an_inode->links_count = 0;
	an_inode->mode = 0;
	an_inode->size = 0;

	for (int var = 0; var < 12; ++var)
		an_inode->pointer_data_0[var] = 0;

	an_inode->pointer_data_1 = 0;
	an_inode->pointer_data_2 = 0;
	an_inode->pointer_data_3 = 0;

}
/*****************************************************
				END   PRIVATE FUNCTIONS
******************************************************/


/*****************************************************
				START PUBLIC FUNCTIONS
******************************************************/

char** split_path(const char* path){

	char separator = '/';
	int ocurrences = 0;

	if (strncmp(path, &separator,1) != 0){
		perror("Mal formato de entrada, no empieza con /");
		return NULL;
	}

	char* pointer_to_separator = strchr(path,separator);

	for(;pointer_to_separator != '\0';ocurrences++)
		pointer_to_separator = strchr(pointer_to_separator +1, separator);

	char** file_tree = calloc(ocurrences +2,sizeof(char*));
	pointer_to_separator = strchr(path+1,separator);


	file_tree[0] = strdup("/");

	if (strlen(path) == 1) //Es root dir
		return file_tree;

	if(ocurrences == 1){
		file_tree[1] = calloc(strlen(path)+1, sizeof(char));
		strncpy(file_tree[1], path, strlen(path));
		return file_tree;
	}

	file_tree[1] = calloc(pointer_to_separator - path +1, sizeof(char));
	strncpy(file_tree[1], path, pointer_to_separator - path);

	int i = 2;

	for(i = 2; i < ocurrences; i++){
		pointer_to_separator = strchr(pointer_to_separator + 1,separator);
		file_tree[i] = calloc(pointer_to_separator - path + 2, sizeof(char));
		strncpy(file_tree[i],path, pointer_to_separator-path +1);
	}

	if ((strncmp(path, file_tree[i-1], strlen(path))) != 0)
		file_tree[i] = strdup(path);

	return file_tree;
}

operation_result_t* delete_dir(params_t* params, s_Mapped_File_t* s_mapped_file){
	active_file_t* dir = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t dir_inode_number = 0;
	inode_t* dir_inode = inode_of_file(s_mapped_file, params->path, &dir_inode_number);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->data = calloc(1,sizeof(uint32_t));
	result->path = strdup(params->path);

	if(dir_inode == NULL){
		uint32_t* err = calloc(1,sizeof(uint32_t));
		*err = 1;
		memcpy(result->data,err,sizeof(uint32_t));
		free(err);
		active_file_finished_using(s_mapped_file->active_file_list,dir);
		return result;
	}

	if(dir_inode->blocks > (Super_constants.BLOCK_SIZE/512)){
		uint32_t* err = calloc(1,sizeof(uint32_t));
		*err = 1;
		memcpy(result->data,err,sizeof(uint32_t));
		free(err);
		errno = ENOTEMPTY;
		active_file_finished_using(s_mapped_file->active_file_list,dir);
		return result;
	}

	char** file_tree = split_path(params->path);

	uint32_t parent_index= 0;
	for(;strncmp(file_tree[parent_index],params->path,strlen(params->path))!=0  ;parent_index++);
	parent_index--;

	active_file_t* parent_dir = active_file_list_find(s_mapped_file->active_file_list,file_tree[parent_index]);

	for (int var = 0; file_tree[var]!= NULL; ++var)
		free(file_tree[var]);
	free(file_tree);

	uint32_t p_inode_number = 0;
	inode_t* parent_inode = inode_of_file(s_mapped_file, parent_dir->path, &p_inode_number);

	WRLOCK(parent_dir->rwlock);

	WRLOCK(dir->rwlock);

	uint32_t* data_address = Inode_Get_Data_Adressess(s_mapped_file->file,dir_inode, dir_inode->size);

	if(data_address == NULL){
		uint32_t* err = calloc(1,sizeof(uint32_t));
		*err = 1;
		memcpy(result->data,err,sizeof(uint32_t));
		free(err);
		UNLOCK(dir->rwlock);
		UNLOCK(parent_dir->rwlock);
		active_file_finished_using(s_mapped_file->active_file_list,dir);
		active_file_finished_using(s_mapped_file->active_file_list,parent_dir);
		return result;
	}
	delete_entry(s_mapped_file,parent_inode,dir);

	void* iterate = s_mapped_file->file + data_address[0] * Super_constants.BLOCK_SIZE;

	free(data_address);

	entry_t* an_entry = (entry_t*)iterate;
	uint32_t first_len = get_padded_size(an_entry);
	iterate += an_entry->entry_len;
	an_entry = (entry_t*)iterate;

	if(an_entry->entry_len != Super_constants.BLOCK_SIZE-first_len){
		uint32_t* err = calloc(1,sizeof(uint32_t));
		*err = 1;
		memcpy(result->data,err,sizeof(uint32_t));
		free(err);
		errno = ENOTEMPTY;
		UNLOCK(dir->rwlock);
		UNLOCK(parent_dir->rwlock);
		active_file_finished_using(s_mapped_file->active_file_list,dir);
		active_file_finished_using(s_mapped_file->active_file_list,parent_dir);
		return result;
	}


	Remove_Block(s_mapped_file,dir_inode,0);
	Clear_Inode(dir_inode);

	uint32_t group_number = Inode_locateGroup(dir_inode_number);
	Group_Descriptor_t* g_desc = Group_Descriptor_read(s_mapped_file->file,group_number);

	pthread_mutex_lock(&s_mapped_file->wdir_count_mutex[group_number]);
	g_desc->NUMBER_DIRECTORIES_GROUP--;
	pthread_mutex_unlock(&s_mapped_file->wdir_count_mutex[group_number]);

	setFreeInodeBitmap(s_mapped_file,dir_inode_number);

	UNLOCK(dir->rwlock);

	UNLOCK(parent_dir->rwlock);

	active_file_finished_using(s_mapped_file->active_file_list,dir);
	active_file_finished_using(s_mapped_file->active_file_list,parent_dir);

	return result;


}

operation_result_t* delete_file(params_t* params, s_Mapped_File_t* s_mapped_file){
	active_file_t* file = active_file_list_find(s_mapped_file->active_file_list,params->path);
	uint32_t file_inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file,params->path,&file_inode_number);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->data = calloc(1,sizeof(uint32_t));
	result->path = strdup(params->path);

	if(file_inode == NULL){
		uint32_t* err = calloc(1,sizeof(uint32_t));
		*err = 1;
		memcpy(result->data,err,sizeof(uint32_t));
		free(err);
		active_file_finished_using(s_mapped_file->active_file_list,file);
		return result;
	}

	char** file_tree = split_path(params->path);

	uint32_t parent_index= 0;
	for(;strncmp(file_tree[parent_index],params->path,strlen(params->path))!=0  ;parent_index++);
	parent_index--;

	active_file_t* parent_dir = active_file_list_find(s_mapped_file->active_file_list,file_tree[parent_index]);

	for (int var = 0; file_tree[var]!= NULL; ++var)
		free(file_tree[var]);
	free(file_tree);

	uint32_t inode_number = 0;
	inode_t* parent_inode = inode_of_file(s_mapped_file, parent_dir->path, &inode_number);

	WRLOCK(parent_dir->rwlock);
	delete_entry(s_mapped_file,parent_inode,file);


	WRLOCK(file->rwlock);

	Remove_Blocks(s_mapped_file,file_inode,Size_In_Blocks(file_inode->size),params);

	Clear_Inode(file_inode);

	setFreeInodeBitmap(s_mapped_file,file_inode_number);

	UNLOCK(file->rwlock);
	UNLOCK(parent_dir->rwlock);

	active_file_finished_using(s_mapped_file->active_file_list,file);
	active_file_finished_using(s_mapped_file->active_file_list,parent_dir);

	return result;

}

operation_result_t* create_file_dir(params_t* params, s_Mapped_File_t* s_mapped_file){

	active_file_t* new_file = active_file_list_find(s_mapped_file->active_file_list,params->path);
	char** file_tree = split_path(params->path);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->path = strdup(params->path);

	uint32_t err = 1;
	result->data = calloc(1,sizeof(uint32_t));
	memcpy(result->data,&err, sizeof(uint32_t));

	//Checkeo si el archivo existe
	uint32_t file_inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file,params->path,&file_inode_number);

	if(file_inode != NULL){
		for (int var = 0; file_tree[var]!= NULL; ++var)
			free(file_tree[var]);
		free(file_tree);
		active_file_finished_using(s_mapped_file->active_file_list,new_file);
		errno = EEXIST;
		return result;
	}

	errno = 0;

	uint32_t parent_index= 0;
	for(;strncmp(file_tree[parent_index],params->path,strlen(params->path))!=0  ;parent_index++);
	parent_index--;

	active_file_t* parent_dir = active_file_list_find(s_mapped_file->active_file_list,file_tree[parent_index]);

	for (int var = 0; file_tree[var]!= NULL; ++var)
		free(file_tree[var]);
	free(file_tree);


	uint32_t parent_inode_number = 0;
	inode_t* parent_inode = inode_of_file(s_mapped_file, parent_dir->path, &parent_inode_number);

	uint32_t* new_inode_number = Get_Free_Inodes(s_mapped_file,1);

	if(new_inode_number == NULL){
    	active_file_finished_using(s_mapped_file->active_file_list,new_file);
    	active_file_finished_using(s_mapped_file->active_file_list,parent_dir);

    	return result;
	}

	entry_t* new_entry = create_entry(active_file_get_file_name(new_file),*new_inode_number);

	WRLOCK(parent_dir->rwlock);

    if(!attach_entry(s_mapped_file,parent_inode,new_entry)){
    	free(new_entry);
    	setFreeInodeBitmap(s_mapped_file, *new_inode_number);
    	free(new_inode_number);
    	UNLOCK(parent_dir->rwlock);
    	active_file_finished_using(s_mapped_file->active_file_list,new_file);
    	active_file_finished_using(s_mapped_file->active_file_list,parent_dir);

    	return result;
    }
    free(new_entry);

	file_inode = Inode_Global_Read(s_mapped_file->file,*new_inode_number);

	WRLOCK(new_file->rwlock);

	if(params->file)
		setFileInode(file_inode,params->mode);
	else{

		setDirInode(file_inode,params->mode,s_mapped_file,*new_inode_number);

		Attach_Blocks(s_mapped_file,file_inode,1);

		if(errno == ENOSPC){
			setFreeInodeBitmap(s_mapped_file, *new_inode_number);
			delete_entry(s_mapped_file,parent_inode,new_file);
			Clear_Inode(file_inode);
			free(new_inode_number);

			UNLOCK(new_file->rwlock);
			UNLOCK(parent_dir->rwlock);

			active_file_finished_using(s_mapped_file->active_file_list,new_file);
			active_file_finished_using(s_mapped_file->active_file_list,parent_dir);

			return result;
		}

		entry_t* base_entry = (entry_t*) (s_mapped_file->file + Super_constants.BLOCK_SIZE * file_inode->pointer_data_0[0]);

		entry_t* self = create_entry(strdup("."),*new_inode_number);
		self->entry_len = Super_constants.BLOCK_SIZE;
		copy_entry(base_entry,self);
		free(self);

		entry_t* father_entry = create_entry(strdup(".."),parent_inode_number);
		attach_entry(s_mapped_file,file_inode,father_entry);
		free(father_entry);
	}

	free(new_inode_number);

	UNLOCK(new_file->rwlock);
	UNLOCK(parent_dir->rwlock);

	active_file_finished_using(s_mapped_file->active_file_list,new_file);
	active_file_finished_using(s_mapped_file->active_file_list,parent_dir);

	uint32_t ok = 0;

	memcpy(result->data,&ok,sizeof(uint32_t));
	errno = 0;

	return result;

}

operation_result_t* list_directory(params_t* params, s_Mapped_File_t* s_mapped_file){

	/*
	 Estas 3 lineas las tienen que hacer todas las funciones para conseguir el inodo de su archivo.
	 */

	active_file_t* dir = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t inode_number = 0;
	inode_t* dir_inode = inode_of_file(s_mapped_file, params->path, &inode_number);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));

	result->path = strdup(params->path);

	RDLOCK(dir->rwlock);

	if (dir_inode == NULL)
		result->data = NULL;
	else{
		if(dir_inode->mode & EXT2_IFDIR)
			result->data = read_directory(s_mapped_file, dir_inode);
		else
			errno = ENOTDIR;
	}

	UNLOCK(dir->rwlock);

	active_file_finished_using(s_mapped_file->active_file_list, dir);
	return result;


}

operation_result_t* read_file(params_t* params,s_Mapped_File_t* s_mapped_file){

	active_file_t* file = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file, params->path, &inode_number);


	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->path = strdup(params->path);

	if(file_inode == NULL){
		active_file_finished_using(s_mapped_file->active_file_list, file);
		return result;
	}

	uint32_t offset = params->offset;
	int64_t bytes_to_read = params->bytes_to_read;

	uint32_t start_block = offset/Super_constants.BLOCK_SIZE;
	uint32_t start_byte_in_start_block =  offset%Super_constants.BLOCK_SIZE;

	uint8_t* read_data = calloc(bytes_to_read, sizeof(uint8_t));

	if(file_inode->mode & EXT2_IFREG){

		RDLOCK(file->rwlock);

		uint32_t* file_data = Inode_Get_Data_Adressess(s_mapped_file->file, file_inode, file_inode->size);

		if( file_inode->size <= offset + bytes_to_read)
			bytes_to_read = file_inode->size - offset;

		result->read_bytes = bytes_to_read;

		uint32_t read_so_far = 0;
		uint32_t remaining_bytes_in_start_block = Super_constants.BLOCK_SIZE - start_byte_in_start_block;

		uint32_t bytes_to_copy = bytes_to_read >remaining_bytes_in_start_block ? remaining_bytes_in_start_block: bytes_to_read;

		//Para leer el primer bloque

		cache_fs_data_t* cache_fs_data = cache_fs_get_set(params,start_block,s_mapped_file,file_data);

		log_info(s_mapped_file->server_log,"%s %d","Bloque leido del archivo: ",start_block);

		memcpy(&read_data[read_so_far], (cache_fs_data->data + start_byte_in_start_block) , bytes_to_copy);
		bytes_to_read -= bytes_to_copy;
		read_so_far += bytes_to_copy;

		if(cache_fs_data->from_cache)
			free(cache_fs_data->data);
		free(cache_fs_data);

		if (bytes_to_read > remaining_bytes_in_start_block ) {

			for (uint32_t block_number = start_block+1; bytes_to_read > 0; ++block_number) {
				bytes_to_copy = bytes_to_read >Super_constants.BLOCK_SIZE ? Super_constants.BLOCK_SIZE: bytes_to_read;

				cache_fs_data_t* cache_fs_data = cache_fs_get_set(params,block_number,s_mapped_file,file_data);

				log_info(s_mapped_file->server_log,"%s %d","Bloque leido del archivo: ",block_number);

				memcpy(&read_data[read_so_far], cache_fs_data->data ,bytes_to_copy);

				if(cache_fs_data->from_cache)
					free(cache_fs_data->data);
				else
					thread_sleep(s_mapped_file->fs_sleep);

				free(cache_fs_data);

				read_so_far += bytes_to_copy;
				bytes_to_read -= bytes_to_copy;

			} // Fin del for
		}

		result->data = read_data;
		free(file_data);
		UNLOCK(file->rwlock);
	}
	else
		errno = EPERM;

	active_file_finished_using(s_mapped_file->active_file_list, file);
	return result;
}

operation_result_t* getattr(params_t* params, s_Mapped_File_t* s_mapped_file){

	active_file_t* file = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file, params->path, &inode_number);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->path = strdup(params->path);

	RDLOCK(file->rwlock);

	if(file_inode == NULL)
		result->data = NULL;
	else{
		reduced_stat_t* stats = calloc(1,sizeof(reduced_stat_t));
		stats->inode_number = inode_number;
		stats->mode = file_inode->mode;
		stats->links_count = file_inode->links_count;
		stats->size = file_inode->size;
		result->data = stats;
	}

	UNLOCK(file->rwlock);

	active_file_finished_using(s_mapped_file->active_file_list, file);
	return result;

}

operation_result_t* truncate_file(params_t* params, s_Mapped_File_t* s_mapped_file){

	active_file_t* file = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file, params->path, &inode_number);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->path = strdup(params->path);
	result->data = calloc(1,sizeof(uint32_t));


	if(file_inode == NULL){
		active_file_finished_using(s_mapped_file->active_file_list, file);
		uint32_t err = 1;
		memcpy(result->data,&err, sizeof(uint32_t));
		return result;
	}

	uint32_t new_size = params->size;

	if(file_inode->mode & EXT2_IFREG){

			WRLOCK(file->rwlock);

			int64_t diff = Size_In_Blocks(new_size) - Size_In_Blocks(file_inode->size);

			uint32_t not_attached = 0;

			if(diff >0)
				not_attached = Attach_Blocks(s_mapped_file,file_inode,diff);
			else if(diff<0)
				Remove_Blocks(s_mapped_file,file_inode,diff, params);

			if(not_attached > 0){
				file_inode->size += (diff-not_attached)*Super_constants.BLOCK_SIZE;
				file_inode->blocks = Size_In_Blocks(file_inode->size)/512;
				UNLOCK(file->rwlock);
			}

			file_inode->size = new_size;
			file_inode->blocks = Size_In_Blocks(new_size)/512;
			UNLOCK(file->rwlock);
	}
	else{
		uint32_t err = 1;
		memcpy(result->data,&err, sizeof(uint32_t));
		errno = EPERM;
	}

	active_file_finished_using(s_mapped_file->active_file_list, file);
	return result;

}

operation_result_t* write_file(params_t* params, s_Mapped_File_t* s_mapped_file){

	active_file_t* file = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file, params->path, &inode_number);

	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->path = strdup(params->path);


	uint32_t start_block = params->offset/Super_constants.BLOCK_SIZE;
	uint32_t start_byte_in_start_block =  params->offset%Super_constants.BLOCK_SIZE;

	int64_t bytes_to_write = params->bytes_to_write;

	if(file_inode == NULL){
		free(params->write_buffer);
		active_file_finished_using(s_mapped_file->active_file_list, file);
		return result;
	}

	if(file_inode->mode & EXT2_IFREG){

		WRLOCK(file->rwlock);

		int64_t diff = params->offset + bytes_to_write - file_inode->size;


		if( (params->offset > file_inode->size) &&  (file_inode->size % Super_constants.BLOCK_SIZE > 0) ){
			uint32_t* file_data = Inode_Get_Data_Adressess(s_mapped_file->file, file_inode, file_inode->size);

			void* last_block = s_mapped_file->file + Super_constants.BLOCK_SIZE * file_data[Size_In_Blocks(file_inode->size) - 1];

			uint32_t bytes_to_set = Super_constants.BLOCK_SIZE - file_inode->size % Super_constants.BLOCK_SIZE;

			memset(&last_block[file_inode->size % Super_constants.BLOCK_SIZE],0, bytes_to_set);

			free(file_data);
		}


		if( Size_In_Blocks(params->offset + bytes_to_write) > Size_In_Blocks(file_inode->size) ){
			Attach_Blocks(s_mapped_file,file_inode,Size_In_Blocks(diff));
		}

		uint32_t* file_data = Inode_Get_Data_Adressess(s_mapped_file->file, file_inode, file_inode->size + diff);

		uint32_t count = Size_In_Blocks(params->offset + bytes_to_write);

		if(start_block > count){
			result->data = calloc(1,sizeof(uint32_t));
			free(params->write_buffer);
			active_file_finished_using(s_mapped_file->active_file_list, file);
			return result;
		}

		if (file_data[count-1] == 0){
			result->data = calloc(1,sizeof(uint32_t));
			free(params->write_buffer);
			active_file_finished_using(s_mapped_file->active_file_list, file);
			return result;
		}


		//Escritura primer bloque!

		void* first_block = s_mapped_file->file + file_data[start_block] * Super_constants.BLOCK_SIZE;

		uint32_t remaining_space = Super_constants.BLOCK_SIZE - start_byte_in_start_block;

		uint32_t bytes_to_copy = (remaining_space > bytes_to_write)? bytes_to_write : remaining_space;

		memcpy(&first_block[start_byte_in_start_block],params->write_buffer,bytes_to_copy);

		log_info(s_mapped_file->server_log,"%s %s %s %d","Archivo: ",params->path, "Bloque escrito: ",file_data[start_block]);

		cache_set(params,start_block,s_mapped_file,file_data);

		bytes_to_write -= bytes_to_copy;

		uint32_t bytes_written = bytes_to_copy;

		if (bytes_to_write > bytes_written){

			for(uint32_t block_number = start_block+1; bytes_to_write > 0; block_number++ ){

				if(block_number > count){
					result->data = calloc(1,sizeof(uint32_t));
					memcpy(result->data,&bytes_written,sizeof(uint32_t));
					free(params->write_buffer);
					active_file_finished_using(s_mapped_file->active_file_list, file);
					return result;
				}

				bytes_to_copy = bytes_to_write > Super_constants.BLOCK_SIZE ? Super_constants.BLOCK_SIZE : bytes_to_write;

				void* block_to_overwrite = s_mapped_file->file + file_data[block_number]* Super_constants.BLOCK_SIZE;

				memcpy(block_to_overwrite,&params->write_buffer[bytes_written], bytes_to_copy);

				log_info(s_mapped_file->server_log,"%s %s %s %d","Archivo: ",params->path, "Bloque escrito: ",file_data[block_number]);

				cache_set(params,block_number,s_mapped_file,file_data);

				bytes_to_write -= bytes_to_copy;

				bytes_written += bytes_to_copy;

			}
		}

		result->data = calloc(1,sizeof(uint32_t));
		memcpy(result->data,&bytes_written,sizeof(uint32_t));

		if(params->offset + bytes_written > file_inode->size){
			uint32_t diff = params->offset + bytes_written - file_inode->size;
			file_inode->size += diff;
			file_inode->blocks = Size_In_Blocks(file_inode->size)*(Super_constants.BLOCK_SIZE/512); //blocks es de bloques de 512B
		}
		update_counter(s_mapped_file->writes_to_sync);//EN TESTING VER DONDE ES MAS OPTIMO QUE ESTE!

		free(file_data);
	}
	else
		errno = EPERM;

	UNLOCK(file->rwlock);
	free(params->write_buffer);
	active_file_finished_using(s_mapped_file->active_file_list, file);

	return result;

}

operation_result_t* open_file(params_t* params, s_Mapped_File_t* s_mapped_file){
	active_file_t* file = active_file_list_find(s_mapped_file->active_file_list, params->path);
	uint32_t inode_number = 0;
	inode_t* file_inode = inode_of_file(s_mapped_file, params->path, &inode_number);


	operation_result_t* result = calloc(1, sizeof(operation_result_t));
	result->path = strdup(params->path);

	result->data = calloc(1,sizeof(uint32_t));

	if(file_inode == NULL){
		uint32_t err = 1;
		memcpy(result->data,&err, sizeof(uint32_t));
	}

	active_file_finished_using(s_mapped_file->active_file_list,file);
	return result;
}


/*****************************************************
				END   PUBLIC FUNCTIONS
******************************************************/



