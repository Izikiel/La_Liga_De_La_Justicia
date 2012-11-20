/*
 * serializadores.c
 *
 *  Created on: 14/05/2012
 *      Author: utnso
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "serializadores.h"

t_stream *serializar_handshake(int8_t tipo){
	//TIPO|PAYLOADLENGHT=0
	t_stream* stream = calloc(1, sizeof(t_stream));
	uint8_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint16_t payload_lenght = 0;
	stream->data = calloc(header_lenght, sizeof(char));
	memcpy(stream->data, &tipo, sizeof(int8_t));
	memcpy(stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));

	stream->longitud = header_lenght;
	return stream;
}

t_handshake* deserializar_handshake(char* stream){
	//Devuelve HANDSHAKE_INIC, HANDSHAKE_OK o HANDSHAKE_FAIL
	t_handshake* handshake = calloc(1, sizeof(t_handshake));
	memcpy(handshake, stream, sizeof(uint8_t));
	return handshake;
}

t_stream* serializar_pedido_modo_nombre(int8_t tipo, const char *nombre_de_archivo, mode_t modo){
	//TIPO|PAYLOADLENGHT|MODO|NOMBRE

	uint32_t payload_lenght = sizeof(mode_t) + strlen(nombre_de_archivo) + 1;
	uint32_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);

	t_stream* stream = calloc(1, sizeof(t_stream));
	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));

	memcpy(stream->data, &tipo, sizeof(int8_t));
	memcpy(stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));
	memcpy(stream->data + header_lenght, &modo, sizeof(mode_t));
	memcpy(stream->data + header_lenght + sizeof(mode_t), nombre_de_archivo, strlen(nombre_de_archivo) + 1);

	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_stream* serializar_pedido_nombre(int8_t tipo, const char *nombre_de_archivo){
	//TIPO|PAYLOADLENGHT|NOMBRE
	uint32_t payload_lenght = strlen(nombre_de_archivo) + 1;
	uint32_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);

	t_stream* stream = calloc(1, sizeof(t_stream));
	stream->data = calloc( header_lenght + payload_lenght, sizeof(char) );

	memcpy( stream->data, &tipo, sizeof(int8_t) );
	memcpy( stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));
	memcpy( stream->data + header_lenght, nombre_de_archivo, strlen(nombre_de_archivo) + 1);

	stream->longitud = header_lenght + payload_lenght;
	return stream;
}

t_stream* serializar_pedido_leer_archivo(const char *nombre_de_archivo, size_t size, off_t offset){
	//TIPO|PAYLOADLENGHT|SIZE|OFFSET|NOMBRE
	uint32_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint32_t payload_lenght = sizeof(size_t) + sizeof(off_t) + strlen(nombre_de_archivo) + 1;

	t_stream* stream = calloc(1, sizeof(t_stream));
	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));

	uint32_t tipo = LEER_ARCHIVO;

	memcpy( stream->data, &tipo, sizeof(int8_t) );
	memcpy( stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t) );
	memcpy( stream->data + header_lenght, &size, sizeof(size_t) );
	memcpy( stream->data + header_lenght + sizeof(size_t), &offset, sizeof(off_t) );
	memcpy( stream->data + header_lenght + sizeof(size_t) + sizeof(off_t), nombre_de_archivo, strlen(nombre_de_archivo) + 1);

	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_stream* serializar_pedido_escribir_archivo(const char *nombre_de_archivo, const char *buffer, size_t size, off_t desplazamiento){
	//TIPO|PAYLOADLENGHT|SIZE|DESPLAZAMIENTO|BUFFER|NOMBRE

	uint32_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint32_t payload_lenght = sizeof(size_t) + sizeof(off_t) + size + strlen(nombre_de_archivo) + 1;

	t_stream* stream = calloc(1, sizeof(t_stream));
	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));

	uint32_t tipo = ESCRIBIR_ARCHIVO;
	uint32_t tmp = 0;

	memcpy( stream->data, &tipo, tmp = sizeof(int8_t) );
	uint32_t offset = tmp;
	memcpy( stream->data + offset, &payload_lenght, tmp = sizeof(uint16_t) );
	offset += tmp;
	memcpy( stream->data + offset, &size, tmp = sizeof(size_t) );
	offset += tmp;
	memcpy( stream->data + offset, &desplazamiento, tmp = sizeof(off_t) );
	offset += tmp;
	memcpy( stream->data + offset , buffer, tmp = size);
	offset += tmp;
	memcpy( stream->data + offset, nombre_de_archivo, strlen(nombre_de_archivo) + 1);
	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_stream* serializar_pedido_truncar_archivo(const char *nombre_de_archivo, size_t size){
	//TIPO|PAYLOADLENGHT|SIZE|NOMBRE
	uint32_t payload_lenght = sizeof(size_t) + strlen(nombre_de_archivo) + 1;
	uint32_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);

	t_stream* stream = calloc(1, sizeof(t_stream));
	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));

	uint8_t tipo = TRUNCAR_ARCHIVO;

	memcpy( stream->data, &tipo, sizeof(uint8_t) );
	memcpy( stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t) );
	memcpy( stream->data + header_lenght, &size, sizeof(size_t) );
	memcpy( stream->data + header_lenght + sizeof(size_t), nombre_de_archivo, strlen(nombre_de_archivo) + 1);

	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_header* deserializar_header(char* stream){
	//Devuelve un puntero a un t_header donde contiene el tipo y longitud del paquete
	t_header* header = calloc(1, sizeof(t_header));
	memcpy(&header->tipo, stream, 1);
	memcpy(&header->longitud, stream+1, 2);
	return header;
}

t_pedido_modo_nombre* deserializar_pedido_modo_nombre(char *stream, int longitud){
	//Devuelve un puntero t_pedido_modo_nombre que posee los campos modo y nombre_de_archivo
	t_pedido_modo_nombre* pedido = calloc(1, sizeof(t_pedido_modo_nombre));
	pedido->nombre_de_archivo = calloc(longitud-sizeof(mode_t), sizeof(char));
	memcpy(&pedido->modo, stream, sizeof(mode_t));
	memcpy(pedido->nombre_de_archivo, stream+sizeof(mode_t), longitud-sizeof(mode_t));
	return pedido;
}



t_pedido_nombre* deserializar_pedido_nombre(char* stream, int longitud){
	//Devuelve un puntero a t_pedido_nombre que es simplemente un char*
	t_pedido_nombre* pedido = calloc(longitud, sizeof(char));
	memcpy(pedido, stream, longitud);
	return pedido;
}

t_pedido_leer_archivo* deserializar_pedido_leer_archivo(char* stream, int longitud){
	//Devuelve un puntero a t_pedido_leer_archivo que contiene los campos size, offset, y nombre_de_archivo
	t_pedido_leer_archivo* pedido = calloc(1, sizeof(t_pedido_leer_archivo));
	pedido->nombre_de_archivo = calloc(longitud - sizeof(size_t) - sizeof(off_t) , sizeof(char));
	memcpy(&pedido->size, stream, sizeof(size_t));
	memcpy(&pedido->offset, stream + sizeof(size_t), sizeof(off_t));
	memcpy(pedido->nombre_de_archivo, stream + sizeof(size_t) + sizeof(off_t), longitud - sizeof(size_t) - sizeof(off_t));
	return pedido;
}

t_pedido_escribir_archivo* deserializar_pedido_escribir_archivo(char* stream, int longitud){
	//Devuelve un puntero a t_pedido_escibir_archivo que contiene los campos size, offset, buffer, y nombre_de_archivo
	t_pedido_escribir_archivo* pedido = calloc(1, sizeof(t_pedido_escribir_archivo));
	int32_t tmp = 0;
	int32_t offset = 0;
	memcpy(&pedido->size, stream, tmp = sizeof(size_t));
	offset += tmp;
	memcpy(&pedido->offset, stream + offset, tmp = sizeof(off_t));
	offset += tmp;
	pedido->buffer = calloc(pedido->size, sizeof(char));
	memcpy(pedido->buffer, stream + offset, tmp = pedido->size);
	offset += tmp;
	pedido->nombre_de_archivo = calloc(longitud - offset, sizeof(char));
	memcpy(pedido->nombre_de_archivo, stream + offset, longitud - offset);

	return pedido;
}

t_pedido_truncar_archivo* deserializar_pedido_truncar_archivo(char* stream, int longitud){
	//Devuelve un puntero a t_pedido_truncar_archivo que contiene los campos size, y nombre_de_archivo
	t_pedido_truncar_archivo* pedido = calloc(1, sizeof(t_pedido_truncar_archivo));
	pedido->nombre_de_archivo = calloc(longitud - sizeof(size_t), sizeof(char));
	memcpy(&pedido->size, stream, sizeof(size_t));
	memcpy(pedido->nombre_de_archivo, stream + sizeof(size_t), longitud - sizeof(size_t));
	return pedido;
}

t_stream* serializar_respuesta_finalizacion(int8_t tipo){
	//TIPO|PAYLOADLENGHT=0
	t_stream* stream = calloc(1, sizeof(t_stream));
	stream->data = calloc(3, sizeof(char));
	uint8_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	int16_t payload_lenght = 0;
	memcpy(stream->data, &tipo, sizeof(int8_t));
	memcpy(stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));

	stream->longitud = header_lenght;

	return stream;

}

t_stream* serializar_respuesta_escribir_archivo(int8_t tipo, size_t bytes_escritos){
	//TIPO|PAYLOADLENGHT|BYTESESCRITOS
	t_stream* stream = calloc(1, sizeof(t_stream));
	uint8_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint16_t payload_lenght = sizeof(size_t);

	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));
	memcpy(stream->data, &tipo, sizeof(int8_t));
	memcpy(stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));
	memcpy(stream->data + header_lenght, &bytes_escritos, sizeof(size_t));

	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_stream* serializar_respuesta_leer_archivo(int8_t tipo, size_t bytes_leidos, void* buffer){
	//TIPO|PAYLOADLENGHT|BYTESLEIDOS|BUFFER
	t_stream* stream = calloc(1, sizeof(t_stream));
	uint8_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint16_t payload_lenght = sizeof(size_t) + bytes_leidos; //-> Aca esta! bytes_leidos = 65536 = 2**16

	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));
	memcpy(stream->data, &tipo, sizeof(int8_t));
	memcpy(stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));
	memcpy(stream->data + header_lenght, &bytes_leidos, sizeof(size_t));
	memcpy(stream->data + header_lenght + sizeof(size_t), buffer, bytes_leidos);

	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_stream* serializar_respuesta_obtener_atributos(int8_t tipo, t_respuesta_obtener_atributos atributos){
	//TIPO|PAYLOADLENGHT|NUMINODO|MODO|NUMHARDLINKS|SIZE|TAMBLOQFS|CANTBLOQ512

	t_stream* stream = calloc(1, sizeof(t_stream));

	uint8_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint16_t payload_lenght = sizeof(ino_t) + sizeof(mode_t) + sizeof(nlink_t) + sizeof(off_t) + sizeof(blksize_t) + sizeof(blkcnt_t);

	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));

	int tmp;
	int offset = 0;
	memcpy(stream->data, &tipo, tmp = sizeof(int8_t));
	offset += tmp;
	memcpy(stream->data + offset, &payload_lenght, tmp = sizeof(uint16_t));
	offset += tmp;
	memcpy(stream->data + offset, &atributos.numero_de_inodo, tmp = sizeof(ino_t));
	offset += tmp;
	memcpy(stream->data + offset, &atributos.modo, tmp = sizeof(mode_t));
	offset += tmp;
	memcpy(stream->data + offset, &atributos.numero_de_hard_links, tmp = sizeof(nlink_t));
	offset += tmp;
	memcpy(stream->data + offset, &atributos.size, tmp = sizeof(size_t));
	offset += tmp;
	memcpy(stream->data + offset, &atributos.tamano_de_bloque_por_fs, tmp = sizeof(blksize_t));
	offset += tmp;
	memcpy(stream->data + offset, &atributos.cantidad_de_bloques_de_512b, tmp = sizeof(blkcnt_t));

	stream->longitud = header_lenght + payload_lenght;

	return stream;
}

t_stream* serializar_respuesta_leer_directorio(int8_t tipo, entry_t** entries){
	//TIPO|PAYLOADLENGHT|ENTRIES
	t_stream* stream = calloc(1, sizeof(t_stream));
	uint16_t payload_lenght = 0;
	uint8_t header_lenght = sizeof(int8_t) + sizeof(uint16_t);
	uint32_t i, cantidad = 0;

	for (i = 0; entries[i] != NULL; i++){
		payload_lenght += entries[i]->name_len + 1;
		cantidad++;
	}
	cantidad--;
	payload_lenght = payload_lenght + cantidad;

	stream->data = calloc(header_lenght + payload_lenght, sizeof(char));

	memcpy(stream->data, &tipo, sizeof(int8_t));
	memcpy(stream->data + sizeof(int8_t), &payload_lenght, sizeof(uint16_t));

	uint32_t offset = 0;
	uint32_t tmp = 0;
	i = 0;
	do{
	    memcpy( stream->data + header_lenght + offset, entries[i]->file_name, tmp = entries[i]->name_len);
	    offset +=tmp;
	    i++;
	    if(entries[i] != NULL){
	    	memcpy( stream->data + header_lenght + offset, "#", 1);
	    	offset += 1;
	    }
	} while(entries[i] != NULL);

	stream->longitud = header_lenght + payload_lenght;
	return stream;
}

t_respuesta_finalizacion* deserializar_respuesta_finalizacion(char* stream, int longitud){
	//Devuelve un puntero a t_respuesta_finalizacion que es simplemente un numero donde indica como finalizo la operacion
	t_respuesta_finalizacion* respuesta = calloc(1, sizeof(t_respuesta_finalizacion));
	memcpy(respuesta, stream, longitud);
	return respuesta;
}

t_respuesta_leer_archivo* deserializar_respuesta_leer_archivo(char* stream, int longiutd){
	//Devuelve un puntero a t_respuesta_leer_archivo que contiene los campos bytes_leidos y buffer.
	t_respuesta_leer_archivo* respuesta = calloc(1, sizeof(t_respuesta_leer_archivo));
	memcpy(&respuesta->bytes_leidos, stream, sizeof(size_t));
	respuesta->buffer = calloc((uint32_t)(respuesta->bytes_leidos), sizeof(char));
	memcpy(respuesta->buffer, stream + sizeof(size_t), (uint32_t)(respuesta->bytes_leidos));
	return respuesta;
}
t_respuesta_obtener_atributos* deserializar_respuesta_obtener_atributos(char* stream, int longitud){
	//Devuelve un puntero a t_respuesta_obtener_atributos
	t_respuesta_obtener_atributos* respuesta = calloc(1, sizeof(t_respuesta_obtener_atributos));

	int tmp;
	int offset = 0;
	memcpy(&respuesta->numero_de_inodo, stream + offset, tmp = sizeof(ino_t));
	offset += tmp;
	memcpy(&respuesta->modo, stream + offset, tmp = sizeof(mode_t) );
	offset += tmp;
	memcpy(&respuesta->numero_de_hard_links, stream + offset, tmp = sizeof(nlink_t) );
	offset += tmp;
	memcpy(&respuesta->size, stream + offset, tmp = sizeof(off_t) );
	offset += tmp;
	memcpy(&respuesta->tamano_de_bloque_por_fs, stream + offset, tmp = sizeof(blksize_t) );
	offset += tmp;
	memcpy(&respuesta->cantidad_de_bloques_de_512b, stream + offset, tmp = sizeof(blkcnt_t) );

	return respuesta;
}

t_respuesta_escribir_archivo* deserializar_respuesta_escribir_archivo(char* stream, int longitud){
	//Devuelve un puntero a t_respuesta_escribir_archivo
	t_respuesta_escribir_archivo* respuesta = calloc(1, sizeof(t_respuesta_escribir_archivo));

	memcpy(&respuesta->bytes_escritos, stream, longitud);

	return respuesta;
}
