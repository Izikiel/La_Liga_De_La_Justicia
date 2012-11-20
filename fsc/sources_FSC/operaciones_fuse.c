/*
 * operaciones_fuse.h
 *
 *  Created on: 07/05/2012
 *      Author: utnso
 */


#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "../headers_FSC/operaciones_fuse.h"
#include "../headers_FSC/serializadores.h"
#include "../headers_FSC/manejador_de_socketes.h"
#include "../headers_FSC/manejador_memcached.h"
#include "../commons/sockete.h"
#include "../commons/string.h"
#include "../commons/config.h"
#include "../commons/log.h"
#include "libmemcached/memcached.h"

#define ERROR_HANDSHAKE_FAIL -1
#define MAX_KEY_SIZE 44
#define BACKLOG 11

//#define SERVER_IP "192.168.1.112"
//#define MEMCACHED_IP "127.0.0.1"
//#define MEMCACHED_PUERTO 11212
//#define SERVER_PUERTO 1234

extern t_sockete conexiones[BACKLOG];
extern t_log* logger;
//extern char* SERVER_IP;
extern char* MEMCACHED_IP;
extern uint16_t MEMCACHED_PUERTO;
extern pthread_mutex_t lock[BACKLOG];
//extern uint16_t SERVER_PUERTO;

void free_memory(t_stream** stream, t_header** header, char** payload){
	free((*stream)->data);
	free(*stream);
	free(*header);
	if(payload != NULL) free(*payload);
}


char* get_previus_path(const char* path){
	char* path_last = strrchr(path, '/');
	int size_splitted = strlen(path) - strlen(path_last);
	char* splitted_path;
	if(size_splitted != 0){
		splitted_path= calloc(size_splitted + 1, sizeof(char));
		memcpy(splitted_path, path, size_splitted);
	}
	else{
		splitted_path = calloc(2, sizeof(char));
		memcpy(splitted_path, "/", 1);
	}

	return splitted_path;
}

int32_t crear_archivo(const char *path, mode_t mode, struct fuse_file_info *fi) {
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Create File - Path: %s", path);
	log_debug(logger, mensaje);
	pthread_mutex_lock(&lock[CREAR_ARCHIVO]);
	t_sockete servidor = conexiones[CREAR_ARCHIVO];

	t_stream* stream = serializar_pedido_modo_nombre(CREAR_ARCHIVO, path, mode);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -EEXIST || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[CREAR_ARCHIVO]);
		return response;
	}

	char* previus_path = get_previus_path(path);
	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
	memcached_keyvalue_t* key = memcached_library_key(LEER_DIRECTORIO,previus_path);
	memcached_library_delete(memcached, key);

	memcached_library_free_keyvalue(&key);
	memcached_library_free_struct(&memcached);
	free_memory(&stream, &header_respuesta, NULL);
	pthread_mutex_unlock(&lock[CREAR_ARCHIVO]);
	return EXIT_SUCCESS;
}

int32_t abrir_archivo(const char *path, struct fuse_file_info *fi) {
	pthread_mutex_lock(&lock[ABRIR_ARCHIVO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Open File - Path: %s", path);
	log_debug(logger, mensaje);
	t_sockete servidor = conexiones[ABRIR_ARCHIVO];

	t_stream* stream = serializar_pedido_nombre(ABRIR_ARCHIVO, path);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[ABRIR_ARCHIVO]);
		return response;
	}

	free_memory(&stream, &header_respuesta, NULL);
	pthread_mutex_unlock(&lock[ABRIR_ARCHIVO]);
	return EXIT_SUCCESS;
}

int32_t leer_archivo(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi) {
	pthread_mutex_lock(&lock[LEER_ARCHIVO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Read File - Path: %s - Size: %u - Offset: %u", path, (uint32_t)size, (uint32_t)offset);
	log_debug(logger, mensaje);

	t_sockete servidor = conexiones[LEER_ARCHIVO];

	t_stream* stream = serializar_pedido_leer_archivo(path, size, offset);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[LEER_ARCHIVO]);
		return response;
	}
	char* payload = recibir_paquete(servidor, header_respuesta->longitud);

	t_respuesta_leer_archivo* respuesta = deserializar_respuesta_leer_archivo(payload, header_respuesta->longitud);
	size_t bytes;
	memcpy(&bytes, &respuesta->bytes_leidos, sizeof(size_t));
	memcpy(buffer, respuesta->buffer, bytes);

	free_memory(&stream, &header_respuesta, &payload);
	pthread_mutex_unlock(&lock[LEER_ARCHIVO]);
	free(respuesta);
	return bytes;
}


int32_t escribir_archivo (const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi){
	pthread_mutex_lock(&lock[ESCRIBIR_ARCHIVO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Write File - Path: %s - Size: %u - Offset: %u", path, (uint32_t)size, (uint32_t)offset);
	log_debug(logger, mensaje);
	t_sockete servidor = conexiones[ESCRIBIR_ARCHIVO];

	t_stream* stream = serializar_pedido_escribir_archivo(path, buffer, size, offset);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[ESCRIBIR_ARCHIVO]);
		return response;
	}

	char* payload = recibir_paquete(servidor, header_respuesta->longitud);

	t_respuesta_escribir_archivo* respuesta = deserializar_respuesta_escribir_archivo(payload, header_respuesta->longitud);

	size_t bytes;
	memcpy(&bytes, &respuesta->bytes_escritos, sizeof(size_t));

	free_memory(&stream, &header_respuesta, &payload);
	pthread_mutex_unlock(&lock[ESCRIBIR_ARCHIVO]);
	return bytes;
}

int32_t cerrar_archivo(const char *path, struct fuse_file_info *fi){
	pthread_mutex_lock(&lock[CERRAR_ARCHIVO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Close File - Path: %s", path);
	log_debug(logger, mensaje);
//	t_sockete servidor = conexiones[
//	t_stream* stream = serializar_pedido_nombre(CERRAR_ARCHIVO, nombre_de_archivo);
//	enviar_paquete(servidor, stream);
//
//	t_header* header_respuesta = recibir_header(servidor);
//	if(header_respuesta->tipo == -ENOENT){
//		free(stream->data);
//		free(stream);
//		free(header_respuesta);
//		return -ENOENT;
//	}
//	if(header_respuesta->tipo == -ENAMETOOLONG){
//		free(stream->data);
//		free(stream);
//		free(header_respuesta);
//		return -ENAMETOOLONG;
//	}
//
//	free(stream->data);
//	free(stream);
//	free(header_respuesta);

	pthread_mutex_unlock(&lock[CERRAR_ARCHIVO]);
	return EXIT_SUCCESS;

}

int32_t truncar_archivo(const char *path, off_t size){
	pthread_mutex_lock(&lock[TRUNCAR_ARCHIVO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Truncate File - Path: %s - Size: %u", path, (uint32_t)size);
	log_debug(logger, mensaje);
	t_sockete servidor = conexiones[TRUNCAR_ARCHIVO];

	t_stream* stream = serializar_pedido_truncar_archivo(path, size);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[TRUNCAR_ARCHIVO]);
		return response;
	}

	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
	memcached_keyvalue_t* key = memcached_library_key(OBTENER_ATRIBUTOS,path);
	memcached_library_delete(memcached, key);

	memcached_library_free_keyvalue(&key);
	memcached_library_free_struct(&memcached);

	free_memory(&stream, &header_respuesta, NULL);
	pthread_mutex_unlock(&lock[TRUNCAR_ARCHIVO]);
	return EXIT_SUCCESS;
}

int32_t eliminar_archivo(const char *path){
	pthread_mutex_lock(&lock[ELIMINAR_ARCHIVO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Unlink File - Path: %s", path);
	log_debug(logger, mensaje);
	t_sockete servidor = conexiones[ELIMINAR_ARCHIVO];

	t_stream* stream = serializar_pedido_nombre(ELIMINAR_ARCHIVO, path);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[ELIMINAR_ARCHIVO]);
		return response;
	}

	char* previus_path = get_previus_path(path);
	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
	memcached_keyvalue_t* key = memcached_library_key(LEER_DIRECTORIO,previus_path);
	memcached_library_delete(memcached, key);

	memcached_library_free_keyvalue(&key);
	memcached_library_free_struct(&memcached);

	free_memory(&stream, &header_respuesta, NULL);
	pthread_mutex_unlock(&lock[ELIMINAR_ARCHIVO]);
	return EXIT_SUCCESS;
}

int32_t crear_directorio(const char *path, mode_t mode){
	pthread_mutex_lock(&lock[CREAR_DIRECTORIO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Create Directory - Path: %s", path);
	log_debug(logger, mensaje);
	t_sockete servidor = conexiones[CREAR_DIRECTORIO];

	t_stream* stream = serializar_pedido_modo_nombre(CREAR_DIRECTORIO, path, mode);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -EEXIST || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[CREAR_DIRECTORIO]);
		return response;
	}

	char* previus_path = get_previus_path(path);
	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
	memcached_keyvalue_t* key = memcached_library_key(LEER_DIRECTORIO,previus_path);
	memcached_library_delete(memcached, key);

	memcached_library_free_keyvalue(&key);
	memcached_library_free_struct(&memcached);

	free_memory(&stream, &header_respuesta, NULL);
	pthread_mutex_unlock(&lock[CREAR_DIRECTORIO]);
	return EXIT_SUCCESS;
}

int32_t leer_directorio(const char *path, void *buffer,fuse_fill_dir_t filler, off_t offset,struct fuse_file_info *fi){
	pthread_mutex_lock(&lock[LEER_DIRECTORIO]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Read Directory - Path: %s", path);
	log_debug(logger, mensaje);

	(void) offset;
	(void) fi;
	int i = 0;
	char clave[MAX_KEY_SIZE] = "dir-";
	strcat(clave, path);


	/*  Me conecto con la Memcached*/
	memcached_return respuesta_conexion_cache;
	memcached_server_st* servers;
	memcached_st* memcached = conectar_con_memcached(&respuesta_conexion_cache, &servers, MEMCACHED_IP, MEMCACHED_PUERTO);

	uint32_t size_valor_obtenido;
	char* valor_obtenido;
	respuesta_conexion_cache = obtener_valor_de_memcached(memcached, clave, &valor_obtenido, &size_valor_obtenido);
	if(respuesta_conexion_cache == MEMCACHED_SUCCESS && valor_obtenido!= NULL){
		char** vector_memcached;
		vector_memcached = string_split(valor_obtenido, "#");

		for(i=0; vector_memcached[i] != NULL; i++){
			filler(buffer, vector_memcached[i], NULL, 0);
			free(vector_memcached[i]);
		}
		free(vector_memcached);
		eliminar_memcached(memcached, &servers);
		free(valor_obtenido);
		pthread_mutex_unlock(&lock[LEER_DIRECTORIO]);
		return EXIT_SUCCESS;
	}


//	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
//	memcached_keyvalue_t* key = memcached_library_key(LEER_DIRECTORIO, path);
//	memcached_keyvalue_t* value = memcached_library_get(memcached, key);

//	if(memcached->response != MEMCACHED_NOTFOUND && memcached->response != MEMCACHED_ERRNO){
//		char** vector_memcached = string_split(value->keyvalue, "#");
//
//		for(i=0; vector_memcached[i] != NULL; i++){
//			filler(buffer, vector_memcached[i], NULL, 0);
//			free(vector_memcached[i]);
//		}
//		free(vector_memcached);
//
//		memcached_library_free_struct(&memcached);
//		memcached_library_free_keyvalue(&value);
//		memcached_library_free_keyvalue(&key);
//		return EXIT_SUCCESS;
//	}

	t_sockete servidor = conexiones[LEER_DIRECTORIO];

	t_stream* stream = serializar_pedido_nombre(LEER_DIRECTORIO, path);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		eliminar_memcached(memcached, &servers);
//		memcached_library_free_struct(&memcached);
//		memcached_library_free_keyvalue(&value);
//		memcached_library_free_keyvalue(&key);
		pthread_mutex_unlock(&lock[LEER_DIRECTORIO]);
		return response;
	}

	char* payload = recibir_paquete(servidor, header_respuesta->longitud);

	/* Accedo a la cache y le envio el payload	*/
	insertar_en_memcached(memcached, clave, payload, header_respuesta->longitud);
	char** vector = string_split(payload, "#");
	for(i=0; vector[i] != NULL; i++){
		filler(buffer, vector[i], NULL, 0);
		free(vector[i]);
	}
	free(vector);

	free_memory(&stream, &header_respuesta, &payload);
	eliminar_memcached(memcached, &servers);
//	memcached_library_free_struct(&memcached);
//	memcached_library_free_keyvalue(&value);
//	memcached_library_free_keyvalue(&key);
	pthread_mutex_unlock(&lock[LEER_DIRECTORIO]);
	return EXIT_SUCCESS;
}

int32_t eliminar_directorio(const char *path){
	pthread_mutex_lock(&lock[ELIMINAR_DIRECTORIO]);
	char mensaje[255];
	sprintf(mensaje, "Operation: Delete Directory - Path: %s", path);
	log_debug(logger, mensaje);

	t_sockete servidor = conexiones[ELIMINAR_DIRECTORIO];

	t_stream* stream = serializar_pedido_nombre(ELIMINAR_DIRECTORIO, path);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);

	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		pthread_mutex_unlock(&lock[ELIMINAR_DIRECTORIO]);
		return response;
	}

	char* previus_path = get_previus_path(path);

	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
	memcached_keyvalue_t* key = memcached_library_key(LEER_DIRECTORIO,previus_path);
	memcached_library_delete(memcached, key);
	char cache_delete[255];
	sprintf(cache_delete, "Operation: Memcached Delete- Key: %s", (char*)key->keyvalue);
	log_debug(logger, cache_delete);

	memcached_library_free_keyvalue(&key);
	memcached_library_free_struct(&memcached);
	free_memory(&stream, &header_respuesta, NULL);
	pthread_mutex_unlock(&lock[ELIMINAR_DIRECTORIO]);
	return EXIT_SUCCESS;
}

int32_t obtener_atributos(const char *path, struct stat *attrib){
	pthread_mutex_lock(&lock[OBTENER_ATRIBUTOS]);
	char	 mensaje[255];
	sprintf(mensaje, "Operation: Get Attributes- Path: %s", path);
	log_debug(logger, mensaje);

	memset(attrib, 0, sizeof(struct stat));

	memcached_struct_t* memcached = memcached_library_connect(MEMCACHED_IP, MEMCACHED_PUERTO);
	memcached_keyvalue_t* key = memcached_library_key(OBTENER_ATRIBUTOS, path);
	char cache_get[255];
	sprintf(cache_get, "Operation: Memcached Get - Key: %s", (char*)key->keyvalue);
	log_debug(logger, cache_get);

	memcached_keyvalue_t* value = memcached_library_get(memcached, key);
	/* Deserializo los directorios. Recordar que se separan con # */
	if(memcached->response == MEMCACHED_SUCCESS && value->keyvalue != NULL){
		t_respuesta_obtener_atributos* attrib_memcached = deserializar_respuesta_obtener_atributos(value->keyvalue, value->size);
		attrib->st_ino = attrib_memcached->numero_de_inodo;
		attrib->st_mode = (attrib_memcached->modo & 0x4000? (S_IFDIR | 0755): (S_IFREG | 0444));
		attrib->st_nlink = (attrib_memcached->modo & 0x4000)? 2: 1;
		attrib->st_size = attrib_memcached->size;
//		attrib->st_blksize = attrib_memcached->tamano_de_bloque_por_fs;
		attrib->st_blocks = attrib_memcached->cantidad_de_bloques_de_512b;
		free(attrib_memcached);
		memcached_library_free_struct(&memcached);
		memcached_library_free_keyvalue(&value);
		memcached_library_free_keyvalue(&key);
		pthread_mutex_unlock(&lock[OBTENER_ATRIBUTOS]);
		return EXIT_SUCCESS;
	}

	/* Me conecto con el RFS */

	t_sockete servidor = conexiones[OBTENER_ATRIBUTOS];

	t_stream* stream = serializar_pedido_nombre(OBTENER_ATRIBUTOS, path);
	enviar_paquete(servidor, stream);

	t_header* header_respuesta = recibir_header(servidor);
	if(header_respuesta->tipo == -ENOENT || header_respuesta->tipo == -ENAMETOOLONG){
		int response = header_respuesta->tipo;
		free_memory(&stream, &header_respuesta, NULL);
		memcached_library_free_struct(&memcached);
		memcached_library_free_keyvalue(&value);
		memcached_library_free_keyvalue(&key);
		pthread_mutex_unlock(&lock[OBTENER_ATRIBUTOS]);
		return response;
	}

	char* payload = recibir_paquete(servidor, header_respuesta->longitud);
	memcached_library_set(memcached, key, payload, header_respuesta->longitud);
	char cache_set[255];
	sprintf(cache_set, "Operation: Memcached Set- Key: %s", (char*)key->keyvalue);
	log_debug(logger, cache_set);
	t_respuesta_obtener_atributos* attrib_payload = deserializar_respuesta_obtener_atributos(payload, header_respuesta->longitud);

	attrib->st_mode = (attrib_payload->modo & 0x4000? (S_IFDIR | 0755): (S_IFREG | 0444));
	attrib->st_nlink = (attrib_payload->modo & 0x4000)? 2: 1;
	attrib->st_size = attrib_payload->size;
//	attrib->st_blksize = attrib_payload->tamano_de_bloque_por_fs;
	attrib->st_blocks = attrib_payload->cantidad_de_bloques_de_512b;
	attrib->st_ino = attrib_payload->numero_de_inodo;


	/*libero la memoria correspondiente*/
	free(attrib_payload);
	free_memory(&stream, &header_respuesta, &payload);
	memcached_library_free_keyvalue(&value);
	memcached_library_free_struct(&memcached);
	pthread_mutex_unlock(&lock[OBTENER_ATRIBUTOS]);
	return EXIT_SUCCESS;
}

