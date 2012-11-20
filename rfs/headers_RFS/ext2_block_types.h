/*
 * ext2_block_types.h
 *
 *  Created on: Apr 25, 2012
 *      Authors: Izikiel, LuksenbergLM
 *      Documentation: http://www.nongnu.org/ext2-doc/ext2.pdf
 *      			   http://wiki.osdev.org/Ext2
 */

#ifndef EXT2_BLOCK_TYPES_H_
#define EXT2_BLOCK_TYPES_H_

	#include <stdint.h>
	#include <stdio.h>
	#include <errno.h>
	#include <error.h>
	#include <stdbool.h>
	#include <stdlib.h>
	#include <string.h>
	#include "../commons/bitarray.h"
	#include "../headers_RFS/server_threading.h"


	/*****************************************************
					START EXT2 STRUCTS
	******************************************************/

	//Superbloque!
	typedef struct {
		uint32_t TOTAL_NUMBER_INODES_FS;  //Total number of inodes in file system
		uint32_t TOTAL_NUMBER_BLOCKS_FS;  //Total number of blocks in file system
		uint32_t SUPERUSER_BLOCKS;        //Number of blocks reserved for superuser
		uint32_t TOTAL_UNALLOCATED_BLOCKS;//Total number of unallocated blocks
		uint32_t TOTAL_UNALLOCATED_INODES;//Total number of unallocated inodes
		uint32_t BLOCK_NUMBER_SUPERBLOCK; //Block number of the block containing the superblock
		uint32_t SHIFT_NUMBER_BLOCK_SIZE; //log2 (block size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the block size)
		uint32_t SHIFT_NUMBER_FRAGMENT_SIZE;//log2 (fragment size) - 10. (In other words, the number to shift 1,024 to the left by to obtain the fragment size)
		uint32_t BLOCKS_PER_GROUP;  		//Number of blocks in each block group
		uint32_t FRAGMENTS_PER_BLOCK_GROUP;//Number of fragments in each block group
		uint32_t INODES_PER_GROUP; 		 //Number of inodes in each block group
		uint32_t LAST_MOUNT_TIME;         //Last mount time (in POSIX time)
		uint32_t LAST_WRITTEN_TIME;       //Last written time (in POSIX time)
		uint16_t TIMES_MOUNTED_SINCE_FSCK;//Number of times the volume has been mounted since its last consistency check (fsck)
		uint16_t MOUNTS_BEFORE_FSCK;      //Number of mounts allowed before a consistency check (fsck) must be done
		uint16_t EXT2_SIGNATURE;          //Ext2 signature (0xef53), used to help confirm the presence of Ext2 on a volume
		uint16_t FILE_SYSTEM_STATE;       //File system state
		uint16_t ERROR_FALLBACK;          //What to do when an error is detected
		uint16_t MINOR_VERSION;           //Minor portion of version (combine with Major portion below to construct full version field)
		uint32_t LAST_FSCK;               //POSIX time of last consistency check (fsck)
		uint32_t INTERVAL_BETWEEN_FSCK;   //Interval (in POSIX time) between forced consistency checks (fsck)
		uint32_t CREATING_OS_ID;          //Operating system ID from which the filesystem on this volume was created
		uint32_t MAJOR_VERSION;           //Major portion of version (combine with Minor portion above to construct full version field)
		uint16_t UID_RESERVED_BLOCKS;	  //User ID that can use reserved blocks
		uint16_t GID_RESERVED_BLOCKS;     //Group ID that can use reserved blocks
		uint32_t FIRST_FREE_INODE_FS;     //First non-reserved inode in file system. (In versions < 1.0, this is fixed as 11)
		uint16_t INODE_STRUCT_SIZE;		  //Size of each inode structure in bytes. (In versions < 1.0, this is fixed as 128)

	} SuperBlock_t;

	/*
	 * Block Group Descriptor
	Offset Size Description
		0 	4 	bg_block_bitmap
		4 	4 	bg_inode_bitmap
		8 	4 	bg_inode_table
		12 	2 	bg_free_blocks_count
		14 	2 	bg_free_inodes_count
		16 	2 	bg_used_dirs_count
		18 	2 	bg_pad
		20 	12 	bg_reserved
	*/


	typedef struct {
		uint32_t	BITMAP_BLOCK_ADDRESS;	//Direccion donde arranca el bitmap de bloque
		uint32_t	BITMAP_INODE_ADDRESS;	//Direccion donde arranca el bitmap de inodo
		uint32_t	START_INODE_TABLE;	//Donde empieza la tabla de inodos
		uint16_t	UNALLOCATED_BLOCKS;	//Cantidad de bloques libres del grupo
		uint16_t	UNALLOCATED_INODES;	//Cantidad de inodos libres del grupo
		uint16_t	NUMBER_DIRECTORIES_GROUP;	//Cantidad de inodos que se utilizan para directorios (carpetas) del grupo
		uint16_t 	PAD_16_32;	//Valor de 16bits usado para hacer padding en estructuras de 32 bits
		uint32_t    RESERVED1;	//No se usa
		uint32_t    RESERVED2;	//No se usa
		uint32_t    RESERVED3;	//no se usa

	} Group_Descriptor_t;


	//Estructura del tipo OSD2 usado en la estructura del INODO
	typedef struct {
		uint8_t frag;	//Numero de fragmento
		uint8_t fsize;	//tamaño del fragmento
		uint16_t mode_high;	//Los 2Bytes mas altos del mode
		uint16_t uid_high;	//2Bytes mas altos del userID
		uint16_t gid_high;	//2Bytes mas altos del GroupID
		uint32_t author;	//El userID del autor asignado al archivo. Si esta setiado en -1 entonces el userID del POSIX lo usará
	}osd2_t;


	//Estructura del INODO
	typedef struct {
		uint16_t mode;	//Indica el formato de los archivos descriptos y los derechos de acceso
		uint16_t uid;	//Id del usuario asociado al archivo
		int32_t size;	//En EXT2 Rev 0 es signado, e indica el tamaño del archivo en bytes
		uint32_t atime;	//Es un numero en SEGUNDOS desde 1970 hasta la ultima vez que el inodo fue accedido
		uint32_t ctime;	//Idem, pero de cuando fue creado
		uint32_t mtime;	//Idem, pero ultima vez que fue modificado
		uint32_t dtime;	//Idem, pero de cuando el inodo fue borrado
		uint16_t gid;	//El grupo POSIX que tiene acceso al archivo
		uint16_t links_count;	//Indica la cantidad de referencias que tiene el inodo (con Hard links. Con Symbolick link no suma al contador). Si tiene 0 significa que esta liberado
		uint32_t blocks;	//Valor que representa el numero total de bloques de 512Bytes reservados para contener los datos de ese inodo. Sin tener en cuenta la fragmentacion interna
		uint32_t flags;	//Indica cómo la implementacion de ext2 debe comportarse cuando se acceden a los datos para ese inodo
		uint32_t osd1;	//Valor que dependende del SO
		//i_block que tiene un peso de 15x4 bytes
		uint32_t pointer_data_0[12]; //Array de bloques de datos directos
		uint32_t pointer_data_1; //Bloque de datos indirectos nivel 1
		uint32_t pointer_data_2; //Bloque de datos indirectos nivel 2
		uint32_t pointer_data_3; //Bloque de datos indirectos nivel 3
		uint32_t generation;	//Se usa para indicar la version del archivo (usado por NFS)
		uint32_t file_acl;	//En la revision 0 este valor siempre es 0
		uint32_t dir_acl;	//En la revision 0 este valor siempre es 0
		uint32_t faddr;	//Valor que indica la locacion del fragmento del archivo
		osd2_t osd2; //Estructura de 12Bytes que depende del SO.
	}inode_t;


	struct Superblock_constants {
		  uint32_t TOTAL_NUMBER_INODES_FS;  //Total number of inodes in file system
		  uint32_t TOTAL_NUMBER_BLOCKS_FS;  //Total number of blocks in file system
		  uint32_t BLOCKS_PER_GROUP;  		//Number of blocks in each block group
		  uint32_t INODES_PER_GROUP; 		 //Number of inodes in each block group
		  uint32_t BLOCK_SIZE;
		  uint32_t TOTAL_GROUPS;
		  uint32_t GDT_START;
	} ;

	extern struct Superblock_constants Super_constants;

	/*****************************************************
					END   EXT2 STRUCTS
	******************************************************/



	/*****************************************************
					START EXT 2 FUNCTIONS
	******************************************************/

	SuperBlock_t* SuperBlock_read(void* fs);	//Lee el superbloque de un archivo
	void 	 SuperBlock_expose (SuperBlock_t *);	//Imprime estados del superbloque
	uint32_t SuperBlock_getBlockSize (SuperBlock_t *);	//Devuelve el tamaño del bloque
	uint32_t SuperBlock_getTotalGroups(SuperBlock_t *);	//Devuelve la cantidad de grupos
	uint32_t SuperBlock_getGDTSize();	//Devuelve el tamaño de la tabla de descriptores de grupos EN BLOQUES

	Group_Descriptor_t* Group_Descriptor_read(void* fs,uint32_t gd_position); //Lee un descriptor de grupo
	uint32_t Group_Number (Group_Descriptor_t*); //Devuelve el numero de un grupo del GDT

	inode_t* Inode_readTable(void* fs, uint32_t group_number); //Lee la tabla de inodo de un grupo determinado
	uint32_t Inode_locateGroup(uint32_t anInodeNumber); //Dado un inodo, devuelve el grupo al que pertenece
	uint32_t Inode_getLocalIndex(uint32_t anInodeNumber); //Devuelve el numero de inodo local al grupo
	inode_t* Inode_Local_Read(void* fs,uint32_t aGroupBlock,uint32_t anInodeIndex); //Lee un inodo segun un numero local al grupo de bloques que pertenece
	inode_t* Inode_Global_Read(void* fs,uint32_t anInodeNumber); //Lee un inodo en base a un numero global de todos los grupo. ese numero es irrepetible
	uint32_t Inode_Block_Read(inode_t* anInode, uint32_t aBlockNumber); //Lee una determinada cantidad de bloques de un determinado inodo
	uint32_t Inode_TableSize(void); //Devuelve el tamaño de la tabla de inodos

	void Inode_expose(inode_t* anInode); //Dado un inodo, imprime sus features

	uint32_t* Inode_Indirect_Read1 (void* fs, uint32_t indirect_block,uint32_t level, uint32_t blocks_to_read); //Lee indireccionamiento nivel 1. Es un caso base de una funcion que es recursiva
	uint32_t* Inode_Indirect_Read23 (void* fs, uint32_t indirect_block,uint32_t level, uint32_t blocks_to_read ); //Lee indireccionamiento nivel 2 y 3. Es la funcion recursiva de la anterior
	bool Indirect_Read(void* fs,uint32_t* address_to_copy, uint32_t indirect_address, uint32_t level, uint32_t cantidad); //NOSE QUE HACE!!!
	uint32_t* Inode_Get_Data_Adressess(void* fs,inode_t* anInode, uint32_t file_size); //Obtiene direcciones de datos de un inodo determinado


	uint32_t Data_Blocks_In_Group (Group_Descriptor_t* group_desc);//Devuelve la cantidad de bloques de datos en un grupo determinado??
	int64_t Size_In_Blocks(uint32_t bytes); //Transforma un tamaño determinado de bytes en cantidad de bloques. Genera Fragmentacion interna
	bool 	 is_Power(uint32_t number, uint64_t base); //Verifica que un numero sea potencia de otro


	uint32_t* Get_Free_Blocks_In_Group (void* fs, uint32_t how_many, Group_Descriptor_t* group_desc );
	uint32_t* Get_Free_Inodes_In_Group (void* fs, uint32_t how_many, Group_Descriptor_t* group_desc,uint32_t group_number );

	/*****************************************************
					END   EXT 2 FUNCTIONS
	******************************************************/




#endif /* EXT2_BLOCK_TYPES_H_ */
