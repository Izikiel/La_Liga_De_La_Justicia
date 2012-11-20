/*
 * sockete.h
 *
 *  Created on: 03/05/2012
 *      Author: utnso
 */

#ifndef SOCKETE_H_
#define SOCKETE_H_

#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#define SOCKETEOK 0
#define SOCKETEERROR -1

typedef int32_t t_sockete;

typedef struct{
	char* ip;
	short puerto;
} t_informacion_sockete;

t_sockete sockete_conectar_cliente(char *ip, int32_t puerto);
int32_t sockete_enviar(t_sockete fd, void *mensaje, uint32_t longitud);
int32_t sockete_recibir(t_sockete fd, void *mensaje_recibido, uint32_t longitud_retorno);
int32_t sockete_cerrar(t_sockete fd);
t_sockete sockete_bindear_servidor(char *ip, int32_t puerto);
int32_t sockete_escuchar(t_sockete fd, int32_t backlog);
t_sockete sockete_aceptar(t_sockete fd);
int32_t sockete_multiplexar(int32_t maxima_cantidad, fd_set *read_fds);

#endif /* SOCKETE_H_ */
