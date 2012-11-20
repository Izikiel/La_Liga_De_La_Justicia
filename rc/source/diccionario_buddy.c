/*
 * diccionario_buddy.c
 *
 *  Created on: 21/06/2012
 *      Author: utnso
 */


#include "../headers/diccionario.h"
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void* mem;


static void agregarHijosDePadre(t_diccionario** self, t_dict_element** padre);

t_diccionario *diccionario_crear_buddy(int table_max_size, int slab_size){
	/*
	 * @DESC:
	 * 	Funcion crea el diccionario para BuddySystem e inicializa la memoria.
	 * 	La memoria se ajusta a la potencia de dos menor mas cercana
	 * @PARAMETROS:
	 * 	table_max_size		- Es el tamaño maximo de la memoria
	 * 	slab_size			- Es el tamaño de un slab.
	 * @RETURN:
	 * 	self				- Es el diccionario inicializado
	 */

	t_diccionario *self = malloc(sizeof(t_diccionario));
	self->table_max_size = table_max_size;
	self->table_current_size = 0;
	self->slab_min_size = slab_size;
	int cant_max_hojas = (self->table_max_size / self->slab_min_size);
	self->elements = calloc(1, sizeof(t_dict_element));
	self->nodes = calloc(cant_max_hojas - 1, sizeof(t_dict_element));
	mem = malloc(power_two_low(self->table_max_size));
	inicializarElementos(&self, cant_max_hojas, mem);
	return self;
}

t_dict_element* diccionario_obtenerDireccion_buddy(t_diccionario ** self, size_t data_size){
	/*
	 * @DESC:
	 * 	Funcion crea el diccionario para BuddySystem e inicializa la memoria.
	 * 	La memoria debe ser potencia de 2
	 * @PARAMETROS:
	 * 	table_max_size		- Es el tamaño maximo de la memoria
	 * 	slab_size			- Es el tamaño de un slab.
	 * @RETURN:
	 * 	self				- Es el diccionario inicializado
	 */
	int aux_data_size = max(data_size,(*self)->slab_min_size);
	while(aux_data_size <= (*self)->table_max_size){
	t_dict_element* item = (*self)->elements;

		while(item != NULL && (item->ocupado || estaReservado(&item) || item->chunk_size < aux_data_size)){
			item = item->next;
		}

		if(item == NULL){
			t_dict_element* victima = diccionario_elegirVictima(*self);
			diccionario_eliminar_buddy(self, victima);
			continue;
		}

		if((*self)->bestfit){
			t_dict_element* aux_item = item;
			while(aux_item != NULL){
				if(!aux_item->ocupado && !estaReservado(&aux_item) &&
						aux_item->chunk_size < item->chunk_size && aux_item->chunk_size >= aux_data_size){
					item = aux_item;
				}
				aux_item = aux_item->next;
			}
		}

		//determinar si tengo q hacerlo padre o no
		while(item->chunk_size / 2 >= aux_data_size && (*self)->nodes != NULL){
				agregarHijosDePadre(self,&item);
		}
		//reservar
		item->data_size = data_size;
		(*self)->table_current_size += data_size;
		return item;

	}

	return NULL; //failure, element too big
}



static void agregarHijosDePadre(t_diccionario** self, t_dict_element** padre){
	/*
	 * @DESC:
	 * 	Funcion agrega un nuevo hijo al padre dividiendo el
	 * 	chunksize del nodo en 2
	 * @PARAMETROS:
	 * 	self 				- El diccionario
	 * 	padre				- Es el item padre
	 */
	t_dict_element* hno = (*self)->nodes;
	if(hno->next != NULL){hno->next->prev = NULL;}
	(*self)->nodes = hno->next;
	if((*padre)->next != NULL){(*padre)->next->prev = hno;}
	hno->next = (*padre)->next;
	hno->prev = *padre;
	(*padre)->next = hno;
	hno->chunk_size = (*padre)->chunk_size/2;
	(*padre)->chunk_size = (*padre)->chunk_size/2;
	hno->data = (*padre)->data + (*padre)->chunk_size;
}

int esElPrimerHermano(t_diccionario** self, t_dict_element** item){
	t_dict_element* aux_item = (*self)->elements;
	int cantidad = 0;
	int subtamanio = 0;
	while(aux_item != NULL && aux_item != *item){
		if(aux_item->chunk_size < (*item)->chunk_size){
		subtamanio += (aux_item->chunk_size % (*item)->chunk_size);
			if(subtamanio == (*item)->chunk_size){
				cantidad ++;
				subtamanio = 0;
			}
		}else cantidad += (aux_item->chunk_size / (*item)->chunk_size);
		aux_item = aux_item->next;
		}
	if(cantidad % 2 == 0){
		return 1;
	}
	return 0;
}

t_dict_element *diccionario_eliminar_buddy(t_diccionario ** self,t_dict_element* it){
	/*
	 * @DESC:
	 * 	Funcion
	 * @PARAMETROS:
	 * 	self				- Es el diccionario
	 * 	it					- Es el item a borrar
	 * @RETURN:
	 */

	t_dict_element * hno = it->next;
	void* data = it->data;
	if(!esElPrimerHermano(self, &it)){
		hno = it->prev;
		data = hno->data;
	}

	//Limpio el item
	(*self)->table_current_size -= it->data_size;
	memcpy(it->key,"",1);
	it->nkey = 0;
	it->hour = 0;
	it->flags = 0;
	it->ocupado = 0;
	it->data_size = 0;

	//Aca lo que hago es unir 2 hermanos
	while(hno != NULL && !hno->ocupado && !estaReservado(&hno) && hno->chunk_size == it->chunk_size){
		hno->chunk_size = 0;
		hno->data = NULL;
		if(hno->prev != NULL){
			hno->prev->next = hno->next;
		}else (*self)->elements = hno->next;
		if(hno->next != NULL){hno->next->prev = hno->prev;}
		hno->prev = NULL;
		hno->next = (*self)->nodes;
		if((*self)->nodes != NULL){(*self)->nodes->prev = hno;}
		(*self)->nodes = hno;
		it->data = data;
		it->chunk_size = it->chunk_size*2;
		//determino el nuevo hermano
		hno = it->next;
		if(!esElPrimerHermano(self, &it)){
			hno = it->prev;
			data = hno->data;
		}
	}
	return it;
}


