/*
 * ex2_block_types.c
 *
 *  Created on: Apr 26, 2012
 *      Author: ezequieldariogambaccini
 */

#include "../headers_RFS/ext2_block_types.h"
#include <math.h>
#include <stdint.h>

//agregar m en Project->Properties->Gcc linker -> libraries-> add library, sino no compila

#define OK 0
#define is ==

#define SUPERBLOCK_START 1024

struct Superblock_constants Super_constants;

/*****************************************************
				START SUPERBLOCK FUNCTIONS
******************************************************/

SuperBlock_t* SuperBlock_read(void* fs){

	SuperBlock_t* superblock = (SuperBlock_t*)(fs + SUPERBLOCK_START);
	static bool set = false;
	if(!set){
		Super_constants.BLOCKS_PER_GROUP = superblock->BLOCKS_PER_GROUP;
		Super_constants.INODES_PER_GROUP = superblock->INODES_PER_GROUP;
		Super_constants.TOTAL_NUMBER_BLOCKS_FS = superblock->TOTAL_NUMBER_BLOCKS_FS;
		Super_constants.TOTAL_NUMBER_INODES_FS = superblock->TOTAL_NUMBER_INODES_FS;
		Super_constants.BLOCK_SIZE = SuperBlock_getBlockSize(superblock);
		if (Super_constants.BLOCK_SIZE is 1024)
			Super_constants.GDT_START = SUPERBLOCK_START + Super_constants.BLOCK_SIZE;
		else
			Super_constants.GDT_START = Super_constants.BLOCK_SIZE;
		Super_constants.TOTAL_GROUPS = SuperBlock_getTotalGroups(superblock);

		set = true;
	}
	return superblock;
}

uint32_t SuperBlock_getBlockSize (SuperBlock_t *superblock){

	if(superblock is NULL)
		return 0;

	uint32_t base = 1024;
	uint32_t shift_left = superblock->SHIFT_NUMBER_BLOCK_SIZE;

	return (base << shift_left);
}

uint32_t SuperBlock_getTotalGroups(SuperBlock_t *superblock){
	//http://wiki.osdev.org/Ext2#Determining_the_Number_of_Block_Groups

	if(superblock is NULL)
		return 0;

	int32_t groups_through_blocks = lround( ((double)superblock->TOTAL_NUMBER_BLOCKS_FS)/(superblock->BLOCKS_PER_GROUP));
	int32_t groups_through_inodes = lround( ((double)superblock->TOTAL_NUMBER_INODES_FS)/(superblock->INODES_PER_GROUP));

	return ((groups_through_blocks < groups_through_inodes) ?  (groups_through_inodes) : (groups_through_blocks));

}

uint32_t SuperBlock_getGDTSize(){

	double size_in_bytes = sizeof(Group_Descriptor_t) * Super_constants.TOTAL_GROUPS;

	return Size_In_Blocks(size_in_bytes);

}

void SuperBlock_expose (SuperBlock_t* s){
	 printf("%s %d\n","Total number of inodes in file system" ,s->TOTAL_NUMBER_INODES_FS);
	 printf("%s %d\n","Total number of blocks in file system" ,s->TOTAL_NUMBER_BLOCKS_FS);
	 printf("%s %d\n","Number of blocks reserved for superuser" ,s->SUPERUSER_BLOCKS);
	 printf("%s %d\n","Total number of unallocated blocks" ,s->TOTAL_UNALLOCATED_BLOCKS);
	 printf("%s %d\n","Total number of unallocated inodes" ,s->TOTAL_UNALLOCATED_INODES);
	 printf("%s %d\n","Block number of the block containing the superblock" ,s->BLOCK_NUMBER_SUPERBLOCK);
	 printf("%s %x\n","Ext2 signature (0xef53), used to help confirm the presence of Ext2 on a volume is:" ,s->EXT2_SIGNATURE);
	 printf("%s %d\n","Number of blocks in each block group" ,s->BLOCKS_PER_GROUP);
	 printf("%s %d\n","Number of inodes in each block group" ,s->INODES_PER_GROUP);

}

/*****************************************************
				END   SUPERBLOCK FUNCTIONS
******************************************************/

/*****************************************************
				START GROUP DESCRIPTOR FUNCTIONS
******************************************************/
static bool is_Sparse(uint32_t gnumber){
	return ((gnumber is 0) || (gnumber is 1) || is_Power(gnumber,3) || is_Power(gnumber,5) || is_Power(gnumber,7));
}

Group_Descriptor_t* Group_Descriptor_read(void* fs, uint32_t gd_position){

	if(gd_position < Super_constants.TOTAL_GROUPS){

		Group_Descriptor_t* group_descriptor = (Group_Descriptor_t*)(fs + Super_constants.GDT_START);
		return &group_descriptor[gd_position];
	}
	return NULL;

}

uint32_t Group_Number (Group_Descriptor_t* group_desc){

	return (group_desc->BITMAP_BLOCK_ADDRESS - 1)/Super_constants.BLOCKS_PER_GROUP;
}


uint32_t Data_Blocks_In_Group (Group_Descriptor_t* group_desc){
	uint32_t data_blocks = Super_constants.BLOCKS_PER_GROUP - 2 - Inode_TableSize();//1 x cada bitmap
	uint32_t gnumber = Group_Number(group_desc);

	if(gnumber is 0)
		return 8109; //Hardcodeado !! No se pq :(

	if( is_Sparse(gnumber))
		data_blocks -= (1 + SuperBlock_getGDTSize() ); //1 = Bloque SuperBloque


	return data_blocks;
}



uint32_t* Get_Free_Inodes_In_Group (void* fs, uint32_t how_many, Group_Descriptor_t* group_desc,uint32_t group_number ){
	uint32_t* free_inodes = calloc(how_many,sizeof(uint32_t));

	if(free_inodes is NULL)
		return NULL;

	uint32_t bitmap_addr = group_desc->BITMAP_INODE_ADDRESS * Super_constants.BLOCK_SIZE;

	uint32_t got = 0;

	t_bitarray* bitmap = bitarray_create(((char*) (fs + bitmap_addr)), Super_constants.BLOCK_SIZE);

	for (int var = 0; (got < how_many && var <= Super_constants.INODES_PER_GROUP); var++) {

		if(!bitarray_test_bit(bitmap,var)){
			bitarray_set_bit(bitmap, var);
			free_inodes[got] = group_number* Super_constants.INODES_PER_GROUP + var + 1;//Inodos arrancan de 1 //Devolver direcciones absolutas!
			got++;
		}
	}

	bitarray_destroy(bitmap);

	return free_inodes;
}

uint32_t* Get_Free_Blocks_In_Group (void* fs, uint32_t how_many, Group_Descriptor_t* group_desc ){
	uint32_t* free_blocks = calloc(how_many,sizeof(uint32_t));

	if(free_blocks is NULL)
		return NULL;

	uint32_t bitmap_addr = group_desc->BITMAP_BLOCK_ADDRESS * Super_constants.BLOCK_SIZE;

	uint32_t got = 0;

	t_bitarray* bitmap =  bitarray_create(((char*) (fs + bitmap_addr)), Super_constants.BLOCK_SIZE);

//	uint32_t data_blocks_size = Data_Blocks_In_Group(group_desc);
	uint32_t group_offset = Group_Number(group_desc) * Super_constants.BLOCKS_PER_GROUP + 1;
//	uint32_t offset = group_desc->START_INODE_TABLE + Inode_TableSize() - group_offset;

	//printf("Bloques de datos por grupo: %d\n", data_blocks_size);

	for (int var = 0; (got < how_many) && (var< Super_constants.BLOCKS_PER_GROUP); var++) { //8bits = 1 byte

		if(!bitarray_test_bit(bitmap,var)){
			bitarray_set_bit(bitmap, var);
			free_blocks[got] =  var + group_offset;
			got++;
		}
	}

	bitarray_destroy(bitmap);

	return free_blocks;

}


/*****************************************************
				END   GROUP DESCRIPTOR FUNCTIONS
******************************************************/

/*****************************************************
				START AUXILIARY FUNCTIONS
******************************************************/

int64_t Size_In_Blocks(uint32_t bytes){
	int64_t blocks = bytes / Super_constants.BLOCK_SIZE;
	if(bytes%Super_constants.BLOCK_SIZE > 0)
		blocks++;
	return blocks;
}

bool is_Power(uint32_t number, uint64_t base){

	while (base < number)
		base *= base;
	return (base is number);

}

/*****************************************************
				END   AUXILIARY FUNCTIONS
******************************************************/

/*****************************************************
				START INODE FUNCTIONS
******************************************************/

inode_t* Inode_readTable(void* fs, uint32_t group_number){
	Group_Descriptor_t* gd = Group_Descriptor_read(fs, group_number);

	uint32_t inode_table_start = gd->START_INODE_TABLE * Super_constants.BLOCK_SIZE;

	inode_t* inode_table = (inode_t*)(fs + inode_table_start);

	return (inode_table);
}

uint32_t Inode_locateGroup(uint32_t anInodeNumber){
	uint32_t numberBlock = (anInodeNumber-1) / Super_constants.INODES_PER_GROUP;
	return numberBlock;
}

uint32_t Inode_getLocalIndex(uint32_t anInodeNumber) {
	uint32_t localNumber_inode = (anInodeNumber-1) % Super_constants.INODES_PER_GROUP;
	return localNumber_inode;
}

inode_t* Inode_Local_Read(void* fs,uint32_t aGroupBlock,uint32_t anInodeIndex){
	inode_t* tablaDeInodos = Inode_readTable(fs,aGroupBlock);
	inode_t* anInode = &tablaDeInodos[anInodeIndex];
	return (anInode);
}

inode_t* Inode_Global_Read(void* fs,uint32_t anInodeNumber){
	uint32_t grupo,indice;
	grupo = Inode_locateGroup(anInodeNumber);
	indice = Inode_getLocalIndex(anInodeNumber);
	inode_t* anInode = Inode_Local_Read(fs,grupo,indice);
	return (anInode);
}

uint32_t Inode_TableSize(){
	uint32_t size_in_bytes = sizeof(inode_t) * Super_constants.INODES_PER_GROUP ;
	return Size_In_Blocks(size_in_bytes);
}

void Inode_expose(inode_t* anInode) {
	printf("###############################################\n");
	printf("Tamaño del archivo al que apunta: %d\n",anInode->size);
	printf("Cantidad de bloques: %d\n",(anInode->blocks)/2); //Lo dividi por 2 ya que este campo te devuelve la cantidad de bloques pero en tamaños de 512Bytes, y los tamaños de los bloques son de 1024Bytes
	printf("Permisos: %d\n",anInode->mode);
	printf("File ACL: %d - (Deberia ser 0)\n",anInode->file_acl);
	printf("Dir ACL: %d - (Deberia ser 0)\n",anInode->file_acl);
	printf("Locacion del archivo : %d\n",anInode->faddr);
	printf("###############################################\n");
	printf("Las direcciones de los Bloques de datos son:\n");
	for (int i=0;i<12;i++){
		printf("Bloque numero %2d tiene la direccion %d\n",i,Inode_Block_Read(anInode,i));
	}
	printf("###############################################\n");

}

/*****************************************************
				START LEER PUNTEROS DE DATOS DEL INODO
******************************************************/

uint32_t Inode_Block_Read(inode_t* anInode, uint32_t aBlockNumber){
	return (anInode->pointer_data_0[aBlockNumber]);
}


uint32_t* Inode_Indirect_Read1 (void* fs, uint32_t indirect_block,uint32_t level, uint32_t blocks_to_read){

	uint32_t* fs_with_offset = (uint32_t*)(fs+indirect_block*Super_constants.BLOCK_SIZE);

	if(blocks_to_read > Super_constants.BLOCK_SIZE/sizeof(uint32_t))
		return NULL;

	uint32_t* block_address = calloc(blocks_to_read, sizeof(uint32_t));

	memcpy(block_address,fs_with_offset,blocks_to_read * sizeof(uint32_t));

	return block_address;

}

uint32_t* Inode_Indirect_Read23 (void* fs, uint32_t indirect_block,uint32_t level, uint32_t blocks_to_read ){

	uint32_t max_blocks_to_read_level = pow((Super_constants.BLOCK_SIZE/sizeof(uint32_t)),level);

	uint32_t* (*Indirect_Read)(void*,uint32_t,uint32_t,uint32_t) = Inode_Indirect_Read23;

	if ((level-1) == 1)
		Indirect_Read = Inode_Indirect_Read1;

	if (blocks_to_read > max_blocks_to_read_level)
		return NULL;

	uint32_t* block_address_to_return = calloc(blocks_to_read,sizeof(uint32_t));

	if(block_address_to_return == NULL)
		return NULL;

	uint32_t blocks_per_full_entry = pow((Super_constants.BLOCK_SIZE/sizeof(uint32_t)),level-1);
	uint32_t full_read_entries = blocks_to_read/blocks_per_full_entry;
	uint32_t remaining_blocks_to_read = blocks_to_read%blocks_per_full_entry;

	uint32_t bytes_to_copy_full_entry = blocks_per_full_entry * sizeof(uint32_t);

	uint32_t i;
	uint32_t offset;
	uint32_t mem_index = 0;
	uint32_t* indirect_addresses = (uint32_t*)(fs+ indirect_block * Super_constants.BLOCK_SIZE);

	for (i = 0; i < full_read_entries ; ++i)
	{
		offset = indirect_addresses[i];
		uint32_t* some_block_addresses = Indirect_Read(fs, offset,level-1, blocks_per_full_entry);

		if(some_block_addresses == NULL)
			return NULL;

		memcpy(&block_address_to_return[mem_index],some_block_addresses,bytes_to_copy_full_entry);
		free(some_block_addresses);

		mem_index += blocks_per_full_entry;

	}

	if (remaining_blocks_to_read >0)
	{
		offset = indirect_addresses[i];
		uint32_t* some_block_addresses = Indirect_Read(fs,offset,level-1, remaining_blocks_to_read);

		if(some_block_addresses == NULL)
			return NULL;

		memcpy(&block_address_to_return[mem_index],some_block_addresses,remaining_blocks_to_read * sizeof(uint32_t));
		free(some_block_addresses);

	}

	return block_address_to_return;

}

bool Indirect_Read(void* fs,uint32_t* address_to_copy, uint32_t indirect_address, uint32_t level, uint32_t cantidad){


	uint32_t* (*Read_Indirection) (void*, uint32_t,uint32_t,uint32_t);

	if (level > 1)
		Read_Indirection = Inode_Indirect_Read23;
	else
		Read_Indirection = Inode_Indirect_Read1;

	uint32_t* indirect_pointer = Read_Indirection(fs,indirect_address,level,cantidad);

	if (indirect_pointer == NULL)
		return false;

	memcpy(address_to_copy,indirect_pointer,cantidad * sizeof(uint32_t));
	free(indirect_pointer);

	return true;
}


uint32_t* Inode_Get_Data_Adressess(void* fs,inode_t* anInode, uint32_t file_size) {


	uint32_t cantidadDeBloques = Size_In_Blocks(file_size)>1 ? Size_In_Blocks(file_size): 1;
	uint32_t cantidadPunterosABloques = Super_constants.BLOCK_SIZE/sizeof(uint32_t);
	uint32_t bloquesDirectos = 12;
	uint32_t bloquesIndirectos = 0;
	uint32_t level;
	uint32_t* punteroDirecto = anInode->pointer_data_0;
	uint32_t* listaDeBloques = calloc(cantidadDeBloques+1,sizeof(uint32_t));
	int ubicacion = 0;

	if (cantidadDeBloques>12){
		bloquesDirectos = 12;
		cantidadDeBloques -= bloquesDirectos;
		memcpy(&listaDeBloques[0],punteroDirecto,sizeof(uint32_t)*bloquesDirectos);
		ubicacion += bloquesDirectos;
	}
	else{
		memcpy(&listaDeBloques[0],punteroDirecto,sizeof(uint32_t)* cantidadDeBloques);
		return listaDeBloques;
	}


	if (cantidadDeBloques > cantidadPunterosABloques) {
		bloquesIndirectos = cantidadPunterosABloques;
		cantidadDeBloques -= bloquesIndirectos;
		level = 1;

		if (Indirect_Read(fs, &listaDeBloques[ubicacion], anInode->pointer_data_1,level,bloquesIndirectos))
			ubicacion += bloquesIndirectos;
		else
			return NULL;


	}
	else{
		bloquesIndirectos =cantidadDeBloques;
		level = 1;
		if (Indirect_Read(fs, &listaDeBloques[ubicacion], anInode->pointer_data_1,level,bloquesIndirectos))
			return listaDeBloques;
		else
			return NULL;
	}


	if (cantidadDeBloques > pow(cantidadPunterosABloques,2)){

		bloquesIndirectos = pow(cantidadPunterosABloques,2);

		cantidadDeBloques -= bloquesIndirectos;
		level = 2;

		if (Indirect_Read(fs, &listaDeBloques[ubicacion], anInode->pointer_data_2,level,bloquesIndirectos))
			ubicacion += bloquesIndirectos;
		else
			return NULL;

	}
	else{
		bloquesIndirectos =cantidadDeBloques;
		level = 2;
		if (Indirect_Read(fs, &listaDeBloques[ubicacion], anInode->pointer_data_2,level,bloquesIndirectos))
			return listaDeBloques;
		else
			return NULL;

	}

	if (cantidadDeBloques <= pow(cantidadPunterosABloques,3)) {
		bloquesIndirectos = cantidadDeBloques;

		level = 3;

		if (Indirect_Read(fs, &listaDeBloques[ubicacion], anInode->pointer_data_3,level,bloquesIndirectos))
			return listaDeBloques;


		else
			return NULL;
	}

	return NULL;
}

/*****************************************************
				END   LEER PUNTEROS DE DATOS DEL INODO
******************************************************/

/*****************************************************
				END   INODE FUNCTIONS
******************************************************/
