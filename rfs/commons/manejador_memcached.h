/*
 * manejador_memcached.h
 *
 *  Created on: 15/06/2012
 *      Author: utnso
 */

#include <libmemcached/memcached.h>

#ifndef MANEJADOR_MEMCACHED_H_
#define MANEJADOR_MEMCACHED_H_

memcached_st* conectar_con_memcached(memcached_return *cache_remota, memcached_server_st** servers, char* memcached_ip, uint16_t memcached_puerto);
memcached_return insertar_en_memcached(memcached_st* memcached, const char* clave, char* valor, uint32_t size);
memcached_return obtener_valor_de_memcached(memcached_st* memcached, const char* clave, char** valor, uint32_t* size);
void eliminar_memcached(memcached_st* memcached, memcached_server_st** servers);

#endif /* MANEJADOR_MEMCACHED_H_ */
