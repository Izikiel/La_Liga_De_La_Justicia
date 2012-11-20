
#ifndef DICCIONARIO_C_
#define DEFAULT_MAX_SIZE 20

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

//elementos de un diccionario
struct dict_element{
	struct dict_element *prev;
	char *key;
	int nkey;
	void *data;
	time_t hour;
	int chunk_size;
	size_t data_size;
	short int ocupado;
	int flags;
	struct dict_element *next;
};

typedef struct dict_element t_dict_element;


struct dictionary{
	struct dict_element *nodes;
	struct dict_element *elements;
	int table_max_size;
	int table_current_size;
	int slab_min_size;
	int lru;
	int bestfit;
	int compactarMemoria;
	int frecuenciaCompactacion;
};

typedef struct dictionary t_diccionario;


// PARTICIONES DINAMICAS
t_diccionario *diccionario_crear(int table_max_size, int slab_size);
t_dict_element *diccionario_obtenerDireccion(t_diccionario ** self, size_t data_size);
t_dict_element *diccionario_eliminar(t_diccionario ** self, t_dict_element* it);

//BUDDY
int power_two_low(int un_numero);
t_diccionario *diccionario_crear_buddy(int table_max_size, int slab_size);
t_dict_element* diccionario_obtenerDireccion_buddy(t_diccionario ** self, size_t data_size);
t_dict_element *diccionario_eliminar_buddy(t_diccionario ** self,t_dict_element* it);

//generales
int max(int a,int b);
short int estaReservado(t_dict_element **item);
t_dict_element *diccionario_obtener(t_diccionario **self, char*key,int nkey);
t_dict_element* diccionario_elegirVictima(t_diccionario* self);
void diccionario_limpiar(t_diccionario ** self,t_dict_element* (*f_eliminar)(t_diccionario**,t_dict_element*));
void inicializarElementos(t_diccionario ** self,int cant_max_el,void* mem);
void diccionario_dump(t_diccionario*,int lru);

#endif
