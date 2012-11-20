/*
 * manejador_memcached.h
 *
 *  Created on: 15/06/2012
 *      Author: utnso
 */

#include <libmemcached/memcached.h>
#define MAX_KEY_SIZE 44

#ifndef MANEJADOR_MEMCACHED_H_
#define MANEJADOR_MEMCACHED_H_

typedef struct{
	memcached_st* memcached;
	memcached_server_st* servers;
	memcached_return response;
} memcached_struct_t ;

typedef struct{
	void* keyvalue;
	size_t size;
} memcached_keyvalue_t;


memcached_struct_t* memcached_library_connect(char* memcached_ip, uint16_t memcached_puerto);
memcached_return memcached_library_set(memcached_struct_t* memcached, memcached_keyvalue_t* key, char* memcached_valu, uint16_t size);
memcached_return memcached_library_delete(memcached_struct_t* memcached, memcached_keyvalue_t* key);
memcached_keyvalue_t* memcached_library_get(memcached_struct_t* memcached, memcached_keyvalue_t* key);
memcached_keyvalue_t* memcached_library_key(int8_t type, const char* key);
memcached_keyvalue_t* memcached_library_value(char* value);
void memcached_library_free_struct(memcached_struct_t** memcached);
void memcached_library_free_keyvalue(memcached_keyvalue_t** keyvalue);

memcached_st* conectar_con_memcached(memcached_return *cache_remota, memcached_server_st** servers, char* memcached_ip, uint16_t memcached_puerto);
memcached_return insertar_en_memcached(memcached_st* memcached, const char* clave, char* valor, uint32_t size);
memcached_return obtener_valor_de_memcached(memcached_st* memcached, const char* clave, char** valor, uint32_t* size);
void eliminar_memcached(memcached_st* memcached, memcached_server_st** servers);

#endif /* MANEJADOR_MEMCACHED_H_ */
