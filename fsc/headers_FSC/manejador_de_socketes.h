/*
 * manejador_de_socketes.h
 *
 *  Created on: 15/05/2012
 *      Author: utnso
 */
#include <stdint.h>
#include "../commons/sockete.h"
#include "../headers_FSC/serializadores.h"


#ifndef MANEJADOR_DE_SOCKETES_H_
#define MANEJADOR_DE_SOCKETES_H_

uint8_t enviar_paquete(t_sockete fd, t_stream *paquete);
t_header* recibir_header(t_sockete fd);
char* recibir_paquete(t_sockete fd, uint16_t longitud);
uint8_t cliente_iniciar_handshake(t_sockete fd);
uint8_t server_responder_handshake(t_sockete fd);
t_handshake cliente_recibir_handshake(t_sockete fd);
#endif /* MANEJADOR_DE_SOCKETES_H_ */

