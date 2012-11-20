/*
 * main.c
 *
 *  Created on: 05/05/2012
 *      Author: utnso
 */
#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS 64
#define BACKLOG 11

#include <fuse.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <stddef.h>
#include <pthread.h>
#include "commons/config.h"
#include "commons/string.h"
#include "commons/log.h"
#include "headers_FSC/operaciones_fuse.h"
#include "headers_FSC/manejador_de_socketes.h"
#include "headers_FSC/serializadores.h"
#include "commons/sockete.h"


struct t_runtime_options {
	char* welcome_msg;
} runtime_options;

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }


static struct fuse_operations operaciones_de_fuse = {
		.create = crear_archivo,
		.open = abrir_archivo,
		.read = leer_archivo,
		.write = escribir_archivo,
		.release = cerrar_archivo,
		.truncate = truncar_archivo,
		.unlink = eliminar_archivo,
		.mkdir = crear_directorio,
		.readdir = leer_directorio,
		.rmdir = eliminar_directorio,
		.getattr = obtener_atributos, };

enum {
	KEY_VERSION,
	KEY_HELP,
};

static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),

		// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};


char* SERVER_IP;
char* MEMCACHED_IP;
uint16_t MEMCACHED_PUERTO;
uint16_t SERVER_PUERTO;
char* LOG_PATH;

void load_config(char* path, char** server_ip, char** memcached_ip, uint16_t* server_port, uint16_t* memcached_port, char** logger_path){
	t_config* config = config_create(path);
	*server_ip = strdup(config_get_string_value(config, "server_ip"));
	*memcached_ip = strdup(config_get_string_value(config, "memcached_ip"));
	*server_port = config_get_int_value(config, "server_port");
	*memcached_port = config_get_int_value(config, "memcached_port");
	*logger_path = strdup(config_get_string_value(config, "logger_path"));
	config_destroy(config);
}

//#define SERVER_IP "192.168.1.112"
//#define MEMCACHED_IP "127.0.0.1"
//#define MEMCACHED_PUERTO 11212
//#define SERVER_PUERTO 1234

t_sockete conexiones[BACKLOG];
t_log* logger;

pthread_mutex_t lock[BACKLOG];

int main(int argc, char *argv[]) {
	int i;
	for(i = 0; i < BACKLOG; i++){
		pthread_mutex_init(&lock[i], NULL);
	}
	/*Inicio las conexiones!*/
//	char* PATH = "/home/utnso/Desarrollo/Workspace/cliente/Debug/cliente_config.ini";
	char* PATH = "cliente_config.ini";
	load_config(PATH, &SERVER_IP, &MEMCACHED_IP, &SERVER_PUERTO, &MEMCACHED_PUERTO, &LOG_PATH);

	logger = log_create(LOG_PATH, "Yanero_Client",false, LOG_LEVEL_DEBUG);
	for(i=0; i<BACKLOG; i++){
		conexiones[i] = sockete_conectar_cliente(SERVER_IP, SERVER_PUERTO);
		cliente_iniciar_handshake(conexiones[i]);
		if(cliente_recibir_handshake(conexiones[i]) == HANDSHAKE_FAIL){
			sockete_cerrar(conexiones[i]);
		}
	}

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	memset(&runtime_options, 0, sizeof(struct t_runtime_options));
	if (fuse_opt_parse(&args, &runtime_options, fuse_options, NULL) == -1){
		/** error parsing options */
		perror("Invalid arguments!");
		return EXIT_FAILURE;
	}
	if( runtime_options.welcome_msg != NULL ){
		printf("%s\n", runtime_options.welcome_msg);
	}
	fuse_main(args.argc, args.argv, &operaciones_de_fuse, NULL);
	log_destroy(logger);
//	config_destroy(config);
	for(i=0; i<BACKLOG; i++){
		sockete_cerrar(conexiones[i]);
	}

	return EXIT_SUCCESS;
}
