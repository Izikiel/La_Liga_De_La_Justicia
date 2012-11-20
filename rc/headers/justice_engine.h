/*
 * justice_engine.h
 *
 *  Created on: 16/05/2012
 *      Author: utnso
 */

//#include<memcached/config_parser.h>
#include "memcached/types.h"
#include "memcached/engine_common.h"
#include "memcached/server_api.h"
//#include<memcached/visibility.h>
#include "memcached/engine.h"
#include <signal.h>



#include "diccionario.h"

typedef struct{
	size_t cache_max_size;
	size_t block_max_size;
	size_t chunk_size;
}t_justice_engine_config;


//estructura que representa a la engine, contiene el engine handler
typedef struct{
	ENGINE_HANDLE_V1 engine;
	GET_SERVER_API get_server_api;
	t_justice_engine_config config;
	t_diccionario* (*f_crearDiccionario)(int, int);
	t_dict_element* (*f_obtenerDireccion)(t_diccionario**,size_t);
	t_dict_element* (*f_eliminar)(t_diccionario**,t_dict_element*);
}t_justice_engine;
