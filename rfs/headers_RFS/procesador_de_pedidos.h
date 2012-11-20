/*
 * procesador_de_pedidos.h
 *
 *  Created on: 06/06/2012
 *      Author: Marcelo Javier López Luksenberg
 */

#include "../headers_RFS/server_threading.h"
#include "../commons/sockete.h"
#ifndef PROCESADOR_DE_PEDIDOS_H_
#define PROCESADOR_DE_PEDIDOS_H_

void listar_directorio_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void leer_archivo_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void obtener_atributos_response (const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void crear_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void escribir_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void crear_directorio_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void eliminar_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void eliminar_directorio_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void truncar_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
void abrir_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);
//void cerrar_archivo_response(const operation_result_t* const result, int32_t socket, s_Mapped_File_t* s_file);

task_t* generar_task_listar_directorio(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_leer_archivo(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_obtener_atributos(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_crear_archivo(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_escribir_archivo(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_crear_directorio(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_eliminar_archivo(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_eliminar_directorio(t_sockete cliente, char* payload, uint16_t longitud);
task_t* generar_task_truncar_archivo(t_sockete cliente, char* payload, uint16_t longitud);

task_t* generar_task_abrir_archivo(t_sockete cliente, char* payload, uint16_t longitud);
//task_t* generar_task_cerrar_archivo(t_sockete cliente, char* payload, uint16_t longitud);
#endif /* PROCESADOR_DE_PEDIDOS_H_ */
