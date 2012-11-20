/*
 * main.c
 *
 *  Created on: 21/05/2012
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include "commons/sockete.h"
#include "commons/manejador_de_socketes.h"
#include "commons/serializadores.h"
#include "headers_RFS/procesador_de_pedidos.h"
#include  "headers_RFS/server_threading.h"


#define BACKLOG 100


void inicializar_vector(struct pollfd* clientes, uint32_t cantidad_clientes) {
	int32_t i;
	for (i = 0; i < cantidad_clientes; i++) {
		clientes[i].fd = 0;
		clientes[i].events = POLLIN;
		clientes[i].revents = 0;
	}
}

uint32_t aceptar_cliente(struct pollfd* clientes, t_sockete server, uint32_t cantidad_clientes) {
	uint32_t i = 0, indice = cantidad_clientes;
	while (i <= indice) {
		if (clientes[i].fd == 0) {
			indice = i;
			clientes[indice].fd = sockete_aceptar(server);
		}
		i++;
	}
	return indice;
}


int main(int argc, char **argv) {


	char* config_path = "./config.ini";
	char* log_path = "./server_log.txt";  // Para RELEASE!

//	char* config_path = "/home/utnso/Desarrollo/Workspace/Yanero_Server/Debug/config.ini";
//	char* log_path = "/home/utnso/Desarrollo/Workspace/Yanero_Server/Debug/server_log.txt";

	t_config* config = config_create(config_path);

	char* fs_path = strdup(config_get_string_value(config, "FS_PATH"));

	char* server_ip = strdup(config_get_string_value(config,"SERVER_IP"));
	int   server_port = config_get_int_value(config,"SERVER_PORT");

	char* cache_ip = strdup(config_get_string_value(config,"CACHE_IP"));
	int   cache_port =  config_get_int_value(config,"CACHE_PORT");

	int   Nthreads = config_get_int_value(config,"WORKERS");
	int   sleep_value = config_get_int_value(config,"SLEEP_VALUE");

	config_destroy(config);

	Worker_params_t* params = calloc(1, sizeof(Worker_params_t));

	s_Mapped_File_t* s_file = s_Mapped_File_init(fs_path, log_path);

	free(fs_path);

	set_counter(s_file->fs_sleep, sleep_value);

	s_queue_t* task_queue = s_queue_create();

	params->filesystem = s_file;
	params->task_queue = task_queue;
	params->cache_server = cache_ip;
	params->cache_port = cache_port;


	pthread_t workers[Nthreads];

	for (int var = 0; var < Nthreads; ++var){
		if(pthread_create(&workers[var],NULL,(void*)worker,params))
			log_debug(s_file->server_log,"ERROR WHILE CREATING THREAD");
		log_debug(s_file->server_log,"WORKER CREATED");
	}


//	inotify_params* iparams = calloc(1,sizeof(inotify_params));
//
//	iparams->path = strdup(config_path);
//	iparams->sleep_counter = s_file->fs_sleep;
//	iparams->logger = s_file->server_log;
//
//	pthread_t inotify_thread;
//
//	if(pthread_create(&inotify_thread,NULL,(void*)th_inotify,(void*) iparams))
//		log_debug(s_file->server_log,"ERROR WHILE CREATING INOTIFY THREAD");
//	log_debug(s_file->server_log,"INOTIFY THREAD CREATED");



	pthread_t sync_thread;

	if(pthread_create(&sync_thread,NULL,(void*)syncer,(void*) s_file))
		log_debug(s_file->server_log,"ERROR WHILE CREATING SYNC THREAD");
	log_debug(s_file->server_log,"SYNC THREAD CREATED");


	struct pollfd read_fds[BACKLOG+1];

	t_sockete server = sockete_bindear_servidor(server_ip, server_port);
	free(server_ip);

	log_debug(s_file->server_log,"Inicializo socket server");
	sockete_escuchar(server, BACKLOG);
	log_info(s_file->server_log,"Server escuchando");

	/* Inicializo los clientes */
	inicializar_vector(read_fds, BACKLOG);
	/* Agrego socket server para atenderlo */
	read_fds[0].fd = server;

	int32_t return_poll = 0;
	int32_t indice = 0;

	while(1){
		return_poll = poll(read_fds,BACKLOG, -1); //Espera infinito

		if(return_poll == -1)
			log_debug(s_file->server_log,"Problema con poll");
		else{
			if(read_fds[0].revents == POLLIN){//Server!!!
				indice = aceptar_cliente(&read_fds[1],read_fds[0].fd,BACKLOG);
				indice++;
				if(server_responder_handshake(read_fds[indice].fd) == HANDSHAKE_OK){
					log_info(s_file->server_log,"Cliente Conectado");
				}
			}
			else{
				for(int32_t i = 1; i<(BACKLOG+1); i++){
					if(read_fds[i].revents == POLLIN){
						task_t* tarea;
						t_header* header = recibir_header(read_fds[i].fd);

						if(errno == EBADFD){
							read_fds[i].fd = 0;
							errno = 0;
							log_info(s_file->server_log, "Cliente Desconectado");
						}
						else {
							char* payload = recibir_paquete(read_fds[i].fd,  header->longitud);
							switch (header->tipo) {
								case CREAR_ARCHIVO:{
									tarea = generar_task_crear_archivo(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s", "CREATE FILE", tarea->params->path);
									break;
								}
								case CREAR_DIRECTORIO:{
									tarea = generar_task_crear_directorio(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s","CREATE DIRECTORY",tarea->params->path);
									break;
								}
								case ABRIR_ARCHIVO:
									tarea = generar_task_abrir_archivo(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s", "OPEN FILE",tarea->params->path);
									break;
								case LEER_ARCHIVO:{
									tarea = generar_task_leer_archivo(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s %s %d %s %d","READ FILE",tarea->params->path,"SIZE",tarea->params->bytes_to_read,
											 "OFFSET",tarea->params->offset);
									break;
								}
								case ESCRIBIR_ARCHIVO:{
									tarea = generar_task_escribir_archivo(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s %s %d %s %d ","WRITE FILE",tarea->params->path,"SIZE:",tarea->params->bytes_to_read,
											 "OFFSET:", tarea->params->offset);
									break;
								}
								case ELIMINAR_ARCHIVO:{
									tarea = generar_task_eliminar_archivo(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s", "DELETE FILE",tarea->params->path);
									break;
								}
								case ELIMINAR_DIRECTORIO:{
									tarea = generar_task_eliminar_directorio(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s", "DELETE DIRECTORY",tarea->params->path);
									break;
								}
								case TRUNCAR_ARCHIVO:{
									tarea = generar_task_truncar_archivo(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s %s %d","TRUNCATE FILE",tarea->params->path,"SIZE",tarea->params->size);
									break;
								}
								case OBTENER_ATRIBUTOS:{
									tarea = generar_task_obtener_atributos(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s","GET ATTRIBUTES",tarea->params->path);
									break;
								}
								case LEER_DIRECTORIO:{
									tarea = generar_task_listar_directorio(read_fds[i].fd, payload, header->longitud);
									log_debug(s_file->server_log,"%s %s", "LIST DIRECTORY",tarea->params->path);
									break;
								}
								default:
									break;
							}

							free(payload);
							free(header);
							s_queue_push(task_queue, tarea);

							read_fds[i].revents = 0;
						}
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}


