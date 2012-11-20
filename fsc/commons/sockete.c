/*
 * sockete.c
 *
 *  Created on: 03/05/2012
 *      Author: utnso
 *
 *      HAY QUE TRABAJAR EL TEMA DE LOS LOGS!!!!!!!
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include "sockete.h"

t_sockete sockete_conectar_cliente(char *ip, int32_t puerto){
	/*
	 * Esta funcion se conecta con un server a traves de una ip y un puerto que se indican en los parametros
	 * @return
	 * 		fd: en caso que haya salido bien
	 * 		ERRORCREAR: si hubo error al crear el socket
	 * 		ERRORCONECTAR: si hubo error al conectar con el servidor
	 */
	t_sockete fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1){
		perror("Error en socket()!");
		return SOCKETEERROR;
	}

	//Esto es la estructura de socket asociada a la fuente//
//	int yes = 1;
//	struct sockaddr_in fuente;
//	bzero(&fuente, sizeof(struct sockaddr_in));
//	fuente.sin_family = AF_INET;
//	fuente.sin_port = htons(puerto);
//	fuente.sin_addr.s_addr = htonl(INADDR_ANY);

	//hago el puerto reusable
//	if (setsockopt(fd ,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
//		perror("Error en setsockopt() del cliente");
//	    return ERROR;
//	}

	//Bindeo el cliente
//	if( bind(fd, (struct sockaddr *)&fuente, sizeof(struct sockaddr)) ==-1 ){
//		perror("Error en el bind() del cliente");
//		return ERROR;
//	}

	//Esto es la estructura de socket asociada al servidor//
	struct sockaddr_in destino;
	bzero(&destino, sizeof(struct sockaddr_in));
	destino.sin_family = AF_INET;

	destino.sin_port = htons(puerto);
	inet_aton(ip, &destino.sin_addr);

	//Realizo la conexion
	if ( connect(fd, (struct sockaddr *)&destino, sizeof(struct sockaddr)) == -1){
		perror("Error en el connect()");
		return SOCKETEERROR;
	}
	return fd;
}

int32_t sockete_enviar(t_sockete fd, void *mensaje, uint32_t longitud){
	/*
	 * Este es un wrapper de la funcion send
	 * @return
	 * 		OK: en caso que haya salido todo bien
	 * 		ERRORENVIAR: si hubo error al enviar
	 */

	//Si ponemos la flag MSG_DONTWAIT se transforma en NO BLOQUEANTE. Hay que preguntar si conviene que se bloquee por el tema de si hay muchos clientes y los threads
	int32_t bytes_enviados = send(fd, mensaje, longitud, 0);

	if ( bytes_enviados == -1){
		perror("Error en el send()");
		return SOCKETEERROR;
	}
	int32_t aux = bytes_enviados;
	while (aux < longitud){
		bytes_enviados = send(fd, mensaje+aux, longitud, 0);
		aux += bytes_enviados;
	}
	return aux;
}

int32_t sockete_recibir(t_sockete fd, void *mensaje_recibido, unsigned int longitud_retorno){
	/*
	 * Funcion que se encarga de recibir todos los datos
	 * @return
	 *		bytes_recibidos: son la cantidad de bytes que se leyeron
	 *		ERRORRECIBIR: si hubo error al recibir
	 */
	int32_t bytes_recibidos = recv(fd, mensaje_recibido, longitud_retorno, 0);
	if (bytes_recibidos < 0){
		perror("Error en recv()");
		return SOCKETEERROR;
	}
	if (bytes_recibidos == 0){
		perror("Cliente cerro la conexion");
		return SOCKETEERROR;
	}
	int32_t aux = bytes_recibidos;
	while (aux < longitud_retorno){
		bytes_recibidos = recv(fd, mensaje_recibido+aux, longitud_retorno, 0);
		aux += bytes_recibidos;
	}
	return aux;
}

int32_t sockete_cerrar(t_sockete fd){
	/*
	 * Funcion que cierra un socket
	 * @return
	 * 		OK: En caso de estar todo ok
	 * 		ERRORCERRAR: si hubo error al cerrar
	 */

	if(shutdown(fd,2) == -1){
		perror("Error en el shutdown()");
		return SOCKETEERROR;
	}
	return SOCKETEOK;
}

t_sockete sockete_bindear_servidor(char *ip, int32_t puerto){
	/*
	 * Esta funcion sirve para realizar una conexion desde el lado del servidor
	 * @return
	 * 		fd: si todo salio bien.
	 * 		ERRORCREAR: si hubo error al crear el socket
	 * 		ERRORREUSABLE: si hubo error al hacer el puerto reusable
	 * 		ERRORBINDEAR: si hubo error al bindear el puerto y la ip
	 *
	 */
	t_sockete fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1){
		perror("Error en el socket()");
		return SOCKETEERROR;
	}
	//Creo la estructura sockaddr_in para hacer el bind
	uint32_t yes = 1;
	struct sockaddr_in direccion_local;
	bzero(&direccion_local, sizeof(struct sockaddr_in));
	direccion_local.sin_family = AF_INET;
	direccion_local.sin_port = htons(puerto);
	inet_aton(ip, &direccion_local.sin_addr);

	//Por si el socket esta bloqueado hago esto. La variable yes no se porque la usa.
	if ( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int32_t)) == -1 ){
		perror("Error en setsockopt() del servidor");
		return SOCKETEERROR;
	}

	if( bind(fd, (struct sockaddr *)&direccion_local, sizeof(struct sockaddr)) == -1 ){
		perror("Error en el bind() del servidor");
		return SOCKETEERROR;
	}

	return fd;
}

int32_t sockete_escuchar(t_sockete fd, int32_t backlog){
	/*
	 * Funcion que se encarga de ponerse en escucha de nuevas conexiones
	 * @return
	 * 		OK: en caso que todo haya salido bien
	 * 		ERRORESCUCHAR: si hubo error al hacer un listen()
	 */
	if ( listen(fd, backlog) == -1){
		perror("Error en listen()");
		return SOCKETEERROR;
	}
	return SOCKETEOK;
}

t_sockete sockete_aceptar(t_sockete fd){
	/*
	 * Funcion que se encarga de aceptar las conexiones
	 * @return
	 * 		fd_remoto: descriptor del socket remoto
	 * 		ERRORACEPTAR: si hubo error al hacer un accept
	 */

	struct sockaddr_in direccion_remota;
	uint32_t size_sockaddr = sizeof(struct sockaddr_in);
	t_sockete fd_remoto = accept(fd, (struct sockaddr *)&direccion_remota, &size_sockaddr);

	if (fd == -1){
		perror("Error en accept()");
		return SOCKETEERROR;
	}

//	informacion_direccion_remota->ip = inet_ntoa(direccion_remota.sin_addr);
//	informacion_direccion_remota->puerto = ntohs(direccion_remota.sin_port);
	return fd_remoto;
}

int32_t sockete_multiplexar(int32_t fd_mayor, fd_set *read_fds){
	/*
	 * Funcion que hace el Select. fd_mayor es el socket descriptor mas grande que hay
	 */
	if( select(fd_mayor+1, read_fds, NULL, NULL, NULL) == -1 ){
		perror("Error en select");
		return SOCKETEERROR;
	}
	return SOCKETEOK;
}
