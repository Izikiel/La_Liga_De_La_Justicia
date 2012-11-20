/*
 * serializadores.h
 *
 *  Created on: 14/05/2012
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>


#ifndef SERIALIZADORES_H_
#define SERIALIZADORES_H_

typedef struct{
	char *nombre_de_archivo;
	mode_t modo;
} __attribute__ ((__packed__)) t_pedido_modo_nombre;

typedef char t_pedido_nombre;

typedef struct{
	char *nombre_de_archivo;
	size_t size;
	off_t offset;
} __attribute__ ((__packed__)) t_pedido_leer_archivo;

typedef struct{
	char *nombre_de_archivo;
	char *buffer;
	size_t size;
	off_t offset;
} __attribute__ ((__packed__)) t_pedido_escribir_archivo;

typedef struct{
	char *nombre_de_archivo;
	size_t size;
} __attribute__ ((__packed__)) t_pedido_truncar_archivo;

typedef struct{
	char *nombre_de_directorio;
	mode_t modo;
} __attribute__ ((__packed__)) t_pedido_crear_directorio;

typedef struct{
	uint32_t longitud;
	char *data;
} __attribute__ ((__packed__)) t_stream;

typedef int32_t t_respuesta_finalizacion;

typedef struct{
	char *buffer;
	size_t bytes_leidos;
} __attribute__ ((__packed__)) t_respuesta_leer_archivo;

typedef struct{
	ino_t numero_de_inodo;
	mode_t modo;
	nlink_t numero_de_hard_links;
	off_t size;
	blksize_t tamano_de_bloque_por_fs;
	blkcnt_t cantidad_de_bloques_de_512b;
} __attribute__ ((__packed__)) t_respuesta_obtener_atributos;

typedef struct {
	int8_t tipo;
	uint16_t longitud;
} __attribute__ ((__packed__)) t_header;

typedef uint8_t t_handshake;

typedef struct {
	size_t bytes_escritos;
} t_respuesta_escribir_archivo;

//ESTO DESPUES SE QUITA PORQUE FLASH YA LO DEFINIO!
typedef struct{
	uint32_t inode_number;
	uint16_t entry_len;
	uint8_t name_len;
	uint8_t file_type;
	char*  file_name;
} __attribute__ ((__packed__)) entry_t;



enum{CREAR_ARCHIVO, ABRIR_ARCHIVO, LEER_ARCHIVO, ESCRIBIR_ARCHIVO, ELIMINAR_ARCHIVO, TRUNCAR_ARCHIVO, CERRAR_ARCHIVO, CREAR_DIRECTORIO, LEER_DIRECTORIO, ELIMINAR_DIRECTORIO, OBTENER_ATRIBUTOS, HANDSHAKE_INIC, HANDSHAKE_OK, HANDSHAKE_FAIL, RESPUESTA_OK};

t_stream *serializar_handshake(int8_t tipo);
t_handshake* deserializar_handshake(char* stream);

t_stream* serializar_pedido_modo_nombre(int8_t tipo, const char *nombre_de_archivo, mode_t modo);
t_stream* serializar_pedido_nombre(int8_t tipo, const char *nombre_de_archivo);
t_stream* serializar_pedido_leer_archivo(const char *nombre_de_archivo, size_t size, off_t offset);
t_stream* serializar_pedido_escribir_archivo(const char *nombre_de_archivo, const char *buffer, size_t size, off_t desplazamiento);
t_stream* serializar_pedido_truncar_archivo(const char *nombre_de_archivo, size_t size);

t_header* deserializar_header(char* stream);
t_pedido_modo_nombre* deserializar_pedido_modo_nombre(char* stream, int longitud);
t_pedido_nombre* deserializar_pedido_nombre(char* stream, int longitud);
t_pedido_leer_archivo* deserializar_pedido_leer_archivo(char* stream, int longitud);
t_pedido_escribir_archivo* deserializar_pedido_escribir_archivo(char* stream, int longitud);
t_pedido_truncar_archivo* deserializar_pedido_truncar_archivo(char* stream, int longitud);

t_stream* serializar_respuesta_finalizacion(int8_t tipo);
t_stream* serializar_respuesta_escribir_archivo(int8_t tipo, size_t bytes_escritos);
t_stream* serializar_respuesta_leer_archivo(int8_t tipo, size_t bytes_leidos, void* buffer);
t_stream* serializar_respuesta_obtener_atributos(int8_t tipo, t_respuesta_obtener_atributos atributos);
t_stream* serializar_respuesta_leer_directorio(int8_t tipo, entry_t** entries);
t_stream* serializar_respuesta_escribir_archivo(int8_t tipo, size_t bytes_escritos);

t_respuesta_finalizacion* deserializar_respuesta_finalizacion(char* stream, int longitud);
t_respuesta_leer_archivo* deserializar_respuesta_leer_archivo(char* stream, int longiutd);
t_respuesta_obtener_atributos* deserializar_respuesta_obtener_atributos(char* stream, int longitud);
t_respuesta_escribir_archivo* deserializar_respuesta_escribir_archivo(char* stream, int longitud);



#endif /* SERIALIZADORES_H_ */
