/*
 * operaciones_fuse.h
 *
 *  Created on: 30/04/2012
 *      Author: utnso
 */

#ifndef OPERACIONES_FUSE_H_
#define OPERACIONES_FUSE_H_

#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS 64

#include <fuse.h>
#include <stdint.h>



int32_t crear_archivo (const char *nombre_de_archivo, mode_t modo, struct fuse_file_info *informacion_de_archivo);
int32_t abrir_archivo (const char *nombre_de_archivo, struct fuse_file_info *informacion_de_archivo);
int32_t leer_archivo (const char *nombre_de_archivo, char *buffer, size_t size, off_t desplazamiento, struct fuse_file_info *informacion_de_archivo);
int32_t escribir_archivo (const char *nombre_de_archivo, const char *buffer, size_t size, off_t desplazamiento, struct fuse_file_info *informacion_de_archivo);
int32_t cerrar_archivo (const char *nombre_de_archivo, struct fuse_file_info *informacion_de_archivo);
int32_t truncar_archivo (const char *nombre_de_archivo, off_t size);
int32_t eliminar_archivo (const char *nombre_de_archivo);
int32_t crear_directorio (const char *nombre_de_directorio, mode_t modo);
int32_t leer_directorio (const char *nombre_de_directorio, void *buffer, fuse_fill_dir_t agregar_al_buffer, off_t offset, struct fuse_file_info *informacion_de_directorio);
int32_t eliminar_directorio (const char *nombre_de_directorio);
int32_t obtener_atributos (const char *nombre_de_archivo, struct stat *atributos_de_archivo);
#endif /* OPERACIONES_FUSE_H_ */
