/*
 * manejador_de_socketes.c
 *
 *  Created on: 30/05/2012
 *      Author: utnso
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "serializadores.h"
#include "manejador_de_socketes.h"
#include "sockete.h"



uint8_t enviar_paquete(t_sockete fd, t_stream *paquete){
	/*
	 * @DESC
	 * Esta funcion envía un paquete al socket que se le pasa por parametro.
	 *
	 *@PARAMETROS:
	 *		fd - Es el socket al que se le envía el paquete
	 *		paquete - Es un puntero a un t_stream que contiene el paquete y longitud a enviar.
	 *
	 *@RETURN
	 * 		SOCKETEOK - Si finalizo correctamente
	 * 		SOCKETEERROR - Si hubo eror al enviar
	 *
	 */
	if( sockete_enviar(fd, paquete->data, paquete->longitud)  == SOCKETEERROR) {
		perror("Error en sockete_enviar()");
		return SOCKETEERROR;
	}
	return SOCKETEOK;
}

t_header* recibir_header(t_sockete fd){
	/*
	 *@DESC
	 * Funcion que recibe el header de un paquete.
	 *
	 *@PARAMETROS:
	 *		fd - Es el socket del que recibe el paquete
	 *
	 *@RETURN
	 * 		header - Devuelve un puntero a t_header donde tiene el tipo y longitud del payload.
	 *
	 */

	char* data = calloc(3, sizeof(char));
	sockete_recibir(fd, data, 3);
	t_header* header = deserializar_header(data);
	free(data);
	return header;
}

char* recibir_paquete(t_sockete fd, uint16_t longitud){
	/*
	 *@DESC
	 * Función que recibe un paquete de un socket. Este debe ejecutarse luego de hacer un recibir_header.
	 *
	 *@PARAMETROS:
	 *		fd - Es el socket del que se recibe el paquete
	 *		longitud - Es la longitud del paquete a recibir. Esta se encuentra en el campo longitud del header recibido previamente.
	 *
	 *@RETURN
	 * 		stream - Devuelve un puntero a char que hay que deserializar como corresponda
	 *
	 */
	char* stream = calloc(longitud, sizeof(char));
	sockete_recibir(fd, stream, longitud);
	return stream;
}
uint8_t cliente_iniciar_handshake(t_sockete fd){
	/*
	 *@DESC
	 * Funcion que serializa el pedido de inicialización del handshake de parte del cliente
	 * y luego lo envía al socket pasado por parametro.
	 *
	 *@PARAMETROS:
	 *		fd - Es el socket al que se le envía el pedido de handshake
	 *
	 *@RETURN
	 * 		SOCKETEOK - Si finalizo correctamente
	 *
	 */
	t_stream* handshake = serializar_handshake(HANDSHAKE_INIC);
	sockete_enviar(fd, handshake->data, handshake->longitud);
	free(handshake->data);
	free(handshake);
	return SOCKETEOK;
}


uint8_t server_responder_handshake(t_sockete fd){
	/*
	 *@DESC
	 * Función que recibe el pedido de handshake de parte del server, lo evalua, y luego
	 * lo responde.
	 *
	 *@PARAMETROS:
	 *		fd - Es el socket al que se le envía la respuesta del handshake
	 *
	 *@RETURN
	 * 		es - Si finalizo correctamente es HANDSHAKE_OK, caso contrario HANDSHAKE_FAIL
	 *
	 */
	char* stream = calloc(3, sizeof(char));
	t_stream* respuesta;
	int res;
	sockete_recibir(fd, stream, 3);
	t_handshake* handshake = deserializar_handshake(stream);

	if( *handshake == HANDSHAKE_INIC){
		respuesta = serializar_handshake(HANDSHAKE_OK);
		res = HANDSHAKE_OK;
	}
	else{
		respuesta = serializar_handshake(HANDSHAKE_FAIL);
		res = HANDSHAKE_FAIL;
	}
	sockete_enviar(fd, respuesta->data, respuesta->longitud);

	free(handshake);
	free(respuesta->data);
	free(respuesta);
	free(stream);
	return res;
}



t_handshake cliente_recibir_handshake(t_sockete fd){
	/*
	 *@DESC
	 * Funcion que recibe la respuesta del handshake mandada por el servidor
	 *
	 *@PARAMETROS:
	 *		fd - Es el socket que le envia la respuesta
	 *
	 *@RETURN
	 * 		hand - Es simplemente un numero donde indica si es HANDSHAKE_OK o HANDSHAKE_FAIL
	 *
	 */

	char* stream = calloc(3, sizeof(char));
	sockete_recibir(fd, stream, 3);
	t_handshake* handshake = deserializar_handshake(stream);
	t_handshake hand = *handshake;
	free(handshake);
	return hand;
}
