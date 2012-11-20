/*
 * manejador_memcached.c
 *
 *  Created on: 15/06/2012
 *      Author: utnso
 */

#include <string.h>
#include "../headers_FSC/serializadores.h"
#include "../headers_FSC/manejador_memcached.h"

memcached_struct_t* memcached_library_connect(char* memcached_ip, uint16_t memcached_puerto){
	memcached_struct_t* memcached = calloc(1, sizeof(memcached_struct_t));
	memcached->memcached = memcached_create(NULL);
	memcached_behavior_set(memcached->memcached, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
	memcached->servers = memcached_server_list_append(NULL, memcached_ip, memcached_puerto, &memcached->response);
	memcached->response = memcached_server_push(memcached->memcached, memcached->servers);
	return memcached;
}

memcached_return memcached_library_set(memcached_struct_t* memcached, memcached_keyvalue_t* key, char* value, uint16_t size){
	memcached->response = memcached_set(memcached->memcached, (const char*)key->keyvalue, key->size, value, size, (time_t)0, (int)0);
	return memcached->response;
}

memcached_return memcached_library_delete(memcached_struct_t* memcached, memcached_keyvalue_t* key){
	memcached->response = memcached_delete(memcached->memcached, key->keyvalue, key->size, 0);
	return memcached->response;
}

memcached_keyvalue_t* memcached_library_get(memcached_struct_t* memcached, memcached_keyvalue_t* key){
	memcached_keyvalue_t* response = calloc(1, sizeof(memcached_keyvalue_t));
	unsigned int flags;
	response->keyvalue= memcached_get(memcached->memcached, key->keyvalue, key->size, &response->size, &flags, &memcached->response);
	response->size = sizeof(response->keyvalue);
	return response;
}

memcached_keyvalue_t* memcached_library_key(int8_t type, const char* path){
	memcached_keyvalue_t* memcached_key = calloc(1, sizeof(memcached_keyvalue_t));
	int offset_key = strlen(path);
	int key_size;
	switch (type) {
		case LEER_DIRECTORIO:
			key_size = offset_key + strlen("dir-");
			memcached_key->keyvalue = calloc(strlen("dir-")+offset_key, sizeof(char));
			memcpy(memcached_key->keyvalue, "dir-", strlen("dir-"));
			memcpy(memcached_key->keyvalue + strlen("dir-"), path, offset_key);
			break;
		default:
			key_size = offset_key + strlen("attr-");
			memcached_key->keyvalue = calloc(strlen("attr-")+offset_key, sizeof(char));
			memcpy(memcached_key->keyvalue, "attr-", strlen("attr-"));
			memcpy(memcached_key->keyvalue+strlen("attr-"), path, offset_key);
			break;
	}


	memcached_key->size = key_size;
	return memcached_key;
}

memcached_keyvalue_t* memcached_library_value(char* value){
	memcached_keyvalue_t* keyvalue = calloc(1, sizeof(memcached_keyvalue_t));
	keyvalue->size = strlen(value);
	return keyvalue;
}

void memcached_library_free_struct(memcached_struct_t** memcached){
	free((*memcached)->servers);
	memcached_free((*memcached)->memcached);
	free(*memcached);
}
void memcached_library_free_keyvalue(memcached_keyvalue_t** keyvalue){
	free(*keyvalue);
}
void eliminar_memcached(memcached_st* memcached, memcached_server_st** servers){
	/*
	 *@DESC
	 * Funcion que libera todos los punteros que crea la memcached
	 *
	 *@PARAMETROS:
	 *		memcached
	 *		servers
	 */
	free(*servers);
	memcached_free(memcached);
}


memcached_return obtener_valor_de_memcached(memcached_st* memcached, const char* clave, char** valor, uint32_t* size) {
	/*
	 *@DESC
	 * Funcion que se usa para obtener un valor de la memcached dandole una key.
	 *
	 *@PARAMETROS:
	 *		memcached - Es la memcached misma
	 *		clave - Es la clave que se va a buscar en la memcached.
	 *		size - Es un puntero a uint32_t. Aqui se almacenará la longitud del valor obtenido
	 *
	 *@RETURN
	 * 		valor - Devuelve el valor obtenido
	 */

	uint32_t memcached_flags;
	memcached_return memcached_error;
	*valor = memcached_get(memcached, clave, strlen(clave), size, &memcached_flags,&memcached_error);
	return memcached_error;
}

memcached_return insertar_en_memcached(memcached_st* memcached, const char* clave, char* valor, uint32_t size) {
	/*
	 *@DESC
	 * Funcion que agrega una key y value a memcached.
	 *
	 *@PARAMETROS:
	 *		memcached - Es la memcached en si misma
	 *		clave - Es la clave que uno quiere guardar.
	 *		valor - Es el valor que va a corresponder a la clave
	 *		size - Es la longitud del valor a agregar en la memcached
	 *
	 *@RETURN
	 * 		cache_remota - Indica como termino la operacion. Puede ser MEMCACHED_SUCCESS o MEMCACHED_FAILED
	 *
	 */

	memcached_return cache_remota = memcached_set(memcached, clave, strlen(clave), valor, size, (time_t) 0, (uint32_t) 0);
	return cache_remota;

}

memcached_st* conectar_con_memcached(memcached_return* cache_remota, memcached_server_st** servers, char* memcached_ip, uint16_t memcached_puerto) {
	/*
	 *@DESC
	 * Funcion que se conecta con memcached
	 *
	 *@PARAMETROS:
	 *		cache_remota - Debe ser del tipo memcached_return*
	 *		servers - Debe ser del tipo memcached_servers_st*
	 *		memcached_ip - Es del tipo char* y corresponde a la IP del memcached.
	 *		memcached_puerto - Es de tipo uint16_t y corresponde al PUERTO donde escucha la memcached.
	 *
	 *@RETURN
	 * 		memcached - Es del tipo memcached_st*. A traves de esta variable se comunica con memcached
	 *
	 */
	*servers = NULL;
	memcached_st *memcached;
	memcached = memcached_create(NULL);
	*servers = memcached_server_list_append(*servers, memcached_ip, memcached_puerto, cache_remota);
	*cache_remota = memcached_server_push(memcached, *servers);
	return memcached;

}
