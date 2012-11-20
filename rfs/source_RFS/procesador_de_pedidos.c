/*
 * procesador_de_pedidos.c
 *
 *  Created on: 06/06/2012
 *      Author: Marcelo Javier López Luksenberg
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "../headers_RFS/file_operations.h"
#include "../headers_RFS/ext2_block_types.h"
#include "../headers_RFS/server_threading.h"
#include "../headers_RFS/procesador_de_pedidos.h"
#include "../commons/serializadores.h"
#include "../commons/sockete.h"


task_t* generar_task_listar_directorio(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_listar_directorio = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_nombre* pedido = deserializar_pedido_nombre(payload, longitud);

	parametros_pedido->path = strdup(pedido);
	parametros_pedido->socket = cliente;

	tarea_listar_directorio->params = parametros_pedido;
	tarea_listar_directorio->run = list_directory;
	tarea_listar_directorio->deliver_response = listar_directorio_response;

	free(pedido);

	return tarea_listar_directorio;
}

task_t* generar_task_leer_archivo(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_leer_archivo= calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_leer_archivo* pedido = deserializar_pedido_leer_archivo(payload, longitud);

	parametros_pedido->path = pedido->nombre_de_archivo;
	parametros_pedido->bytes_to_read = pedido->size;
	parametros_pedido->offset = pedido->offset;
	parametros_pedido->socket = cliente;

	tarea_leer_archivo->params = parametros_pedido;
	tarea_leer_archivo->run = read_file;
	tarea_leer_archivo->deliver_response = leer_archivo_response;

	free(pedido);

	return tarea_leer_archivo;
}

task_t* generar_task_obtener_atributos(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_obtener_atributos = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_nombre* pedido = deserializar_pedido_nombre(payload, longitud);

	parametros_pedido->path = strdup(pedido);
	parametros_pedido->socket = cliente;

	tarea_obtener_atributos->params = parametros_pedido;
	tarea_obtener_atributos->run = getattr;
	tarea_obtener_atributos->deliver_response = obtener_atributos_response;

	free(pedido);

	return tarea_obtener_atributos;
}

task_t* generar_task_crear_archivo(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_crear_archivo = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_modo_nombre* pedido = deserializar_pedido_modo_nombre(payload, longitud);

	parametros_pedido->path = pedido->nombre_de_archivo;
	parametros_pedido->mode = pedido->modo;
	parametros_pedido->file = true;
	parametros_pedido->socket = cliente;

	tarea_crear_archivo->params = parametros_pedido;
	tarea_crear_archivo->run = create_file_dir;
	tarea_crear_archivo->deliver_response = crear_archivo_response;

	free(pedido);

	return tarea_crear_archivo;
}

task_t* generar_task_escribir_archivo(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_escribir_archivo = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_escribir_archivo* pedido = deserializar_pedido_escribir_archivo(payload, longitud);

	parametros_pedido->path = pedido->nombre_de_archivo;
	parametros_pedido->offset = pedido->offset;
	parametros_pedido->bytes_to_write= pedido->size;
	parametros_pedido->write_buffer = pedido->buffer;
	parametros_pedido->socket = cliente;

	tarea_escribir_archivo->params = parametros_pedido;
	tarea_escribir_archivo->run = write_file;
	tarea_escribir_archivo->deliver_response = escribir_archivo_response;

	free(pedido);

	return tarea_escribir_archivo;
}

task_t* generar_task_crear_directorio(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_crear_directorio = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_modo_nombre* pedido = deserializar_pedido_modo_nombre(payload, longitud);

	parametros_pedido->path = pedido->nombre_de_archivo;
	parametros_pedido->mode = pedido->modo;
	parametros_pedido->file = false;
	parametros_pedido->socket = cliente;

	tarea_crear_directorio->params = parametros_pedido;
	tarea_crear_directorio->run = create_file_dir;
	tarea_crear_directorio->deliver_response = crear_directorio_response;

	free(pedido);

	return tarea_crear_directorio;
}
task_t* generar_task_eliminar_archivo(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_eliminar_archivo = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_nombre* pedido = deserializar_pedido_nombre(payload, longitud);

	parametros_pedido->path = strdup(pedido);
	parametros_pedido->socket = cliente;

	tarea_eliminar_archivo->params = parametros_pedido;
	tarea_eliminar_archivo->run = delete_file;
	tarea_eliminar_archivo->deliver_response = eliminar_archivo_response;

	free(pedido);

	return tarea_eliminar_archivo;
}

task_t* generar_task_eliminar_directorio(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_eliminar_directorio = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_nombre* pedido = deserializar_pedido_nombre(payload, longitud);

	parametros_pedido->path = strdup(pedido);
	parametros_pedido->socket = cliente;

	tarea_eliminar_directorio->params = parametros_pedido;
	tarea_eliminar_directorio->run = delete_dir;
	tarea_eliminar_directorio->deliver_response = eliminar_directorio_response;

	free(pedido);

	return tarea_eliminar_directorio;
}

task_t* generar_task_truncar_archivo(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_truncar_archivo = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_truncar_archivo* pedido = deserializar_pedido_truncar_archivo(payload, longitud);

	parametros_pedido->path = pedido->nombre_de_archivo;
	parametros_pedido->size = pedido->size;
	parametros_pedido->socket = cliente;

	tarea_truncar_archivo->params = parametros_pedido;
	tarea_truncar_archivo->run = truncate_file;
	tarea_truncar_archivo->deliver_response = truncar_archivo_response;

	free(pedido);

	return tarea_truncar_archivo;
}

task_t* generar_task_abrir_archivo(t_sockete cliente, char* payload, uint16_t longitud){
	task_t* tarea_abrir_archivo = calloc(1, sizeof(task_t));
	params_t* parametros_pedido = calloc(1, sizeof(params_t));

	t_pedido_nombre* pedido = deserializar_pedido_nombre(payload, longitud);

	parametros_pedido->path = strdup(pedido);
	parametros_pedido->socket = cliente;

	tarea_abrir_archivo->params = parametros_pedido;
	tarea_abrir_archivo->run = open_file;
	tarea_abrir_archivo->deliver_response = abrir_archivo_response;

	free(pedido);

	return tarea_abrir_archivo;
}

//task_t* generar_task_cerrar_archivo(t_sockete cliente, char* payload, uint16_t longitud){
//	task_t* tarea_cerrar_archivo = calloc(1, sizeof(task_t));
//	params_t* parametros_pedido = calloc(1, sizeof(params_t));
//
//	t_pedido_nombre* pedido = deserializar_pedido_nombre(payload, longitud);
//
//	parametros_pedido->path = pedido;
//	parametros_pedido->socket = cliente;
//
//	tarea_cerrar_archivo->params = parametros_pedido;
//	tarea_cerrar_archivo->run = close_file;
//	tarea_cerrar_archivo->deliver_response = crear_archivo_response;
//	return tarea_cerrar_archivo;
//}

void listar_directorio_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){

	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if(result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-ENOENT);
		}

		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		entry_t** entries = (entry_t**) result->data;
		respuesta = serializar_respuesta_leer_directorio(RESPUESTA_OK, entries);
	}
	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);

}

void leer_archivo_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if(result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_leer_archivo(-ENOENT,0, NULL);
		}

		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_leer_archivo(-ENAMETOOLONG, 0, NULL);
		}
	}
	else{
			respuesta = serializar_respuesta_leer_archivo(RESPUESTA_OK, result->read_bytes, result->data);
			sockete_enviar(socket, respuesta->data, respuesta->longitud);
		}

	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void obtener_atributos_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){

	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	t_respuesta_obtener_atributos atributos_a_enviar;

	if( result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-ENOENT);
		}
		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		reduced_stat_t* atributos = (reduced_stat_t*) result->data;
		atributos_a_enviar.numero_de_inodo = atributos->inode_number;
		atributos_a_enviar.modo = atributos->mode;
		atributos_a_enviar.numero_de_hard_links = atributos->links_count;
		atributos_a_enviar.size = atributos->size;
		atributos_a_enviar.cantidad_de_bloques_de_512b = Size_In_Blocks(atributos->size);
		atributos_a_enviar.tamano_de_bloque_por_fs = Super_constants.BLOCK_SIZE;
		respuesta = serializar_respuesta_obtener_atributos(RESPUESTA_OK, atributos_a_enviar);
	}
	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void crear_archivo_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){

	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if( result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-ENOENT);
		}

		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void escribir_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if( result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-ENOENT);
		}
		else if (result->error == ENOSPC){
					respuesta = serializar_respuesta_finalizacion(-ENOSPC);
				}
		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		size_t* bytes_escritos = (size_t *) result->data;
		respuesta = serializar_respuesta_escribir_archivo(RESPUESTA_OK,  *bytes_escritos);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void crear_directorio_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if( result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-EEXIST);
		}

		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void eliminar_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if( result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-EEXIST);
		}

		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void eliminar_directorio_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if( result->data == NULL){
		if (result->error == ENOENT){
			respuesta = serializar_respuesta_finalizacion(-EEXIST);
		}

		else if (result->error == ENAMETOOLONG){
			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
		}
	}
	else{
		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

void truncar_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if (result->error == ENOENT){
		respuesta = serializar_respuesta_finalizacion(-ENOENT);
	}

	else if (result->error == ENAMETOOLONG){
		respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
	}

	else{
		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}
void abrir_archivo_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
	RDLOCK(data_file->rwlock);

	t_stream* respuesta;
	if (result->error == ENOENT){
		respuesta = serializar_respuesta_finalizacion(-ENOENT);
	}

	else if (result->error == ENAMETOOLONG){
		respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
	}

	else{
		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
	}

	sockete_enviar(socket, respuesta->data, respuesta->longitud);
	free(respuesta->data);
	free(respuesta);

	UNLOCK(data_file->rwlock);
	active_file_finished_using(s_file->active_file_list, data_file);
}

//void cerrar_archivo_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file){
//
//	active_file_t* data_file = active_file_list_find(s_file->active_file_list, result->path);
//	RDLOCK(data_file->rwlock);
//
//	t_stream* respuesta;
//	if( result->data == NULL){
//		if (result->error == ENOENT){
//			respuesta = serializar_respuesta_finalizacion(-ENOENT);
//		}
//
//		else if (result->error == ENAMETOOLONG){
//			respuesta = serializar_respuesta_finalizacion(-ENAMETOOLONG);
//		}
//	}
//	else{
//		respuesta = serializar_respuesta_finalizacion(RESPUESTA_OK);
//	}
//
//	sockete_enviar(socket, respuesta->data, respuesta->longitud);
//	free(respuesta);
//
//	UNLOCK(data_file->rwlock);
//	active_file_finished_using(s_file->active_file_list, data_file);
//}
