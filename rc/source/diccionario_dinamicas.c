


#include "../headers/diccionario.h"
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../commons/log.h"

extern t_log* logger;

void *mem;

static void diccionario_compactarMemoria(t_diccionario **self);

t_diccionario *diccionario_crear(int table_max_size, int slab_size){
	/*
	 * @DESC:
	 * 	Funcion crea el diccionario para particiones dinamicas e inicializa la memoria.
	 * @PARAMETROS:
	 * 	table_max_size		- Es el tamaño maximo de la memoria
	 * 	slab_size			- Es el tamaño de un slab.
	 * @RETURN:
	 * 	self				- Es el diccionario inicializado
	 */

	t_diccionario *self = calloc(1, sizeof(t_diccionario)); //estaba con malloc... lo puse con calloc

	self->table_max_size = table_max_size;
	self->slab_min_size = slab_size;

	int cant_max_el = (self->table_max_size/self->slab_min_size);

	self->elements = calloc(1, sizeof(t_dict_element));
	self->nodes = calloc(cant_max_el - 1, sizeof(t_dict_element));
	self->table_current_size = 0;
	mem = malloc(self->table_max_size); //habria que ver si se puede hacer con calloc :P para que empieze en 0
	inicializarElementos(&self, cant_max_el, mem); //en vez de cant_max_el habia self->table_max_size/self->slab_min_size :-o

	return self;
}




/*
 *first fit - best fit
 * retorna el elemento por exito, NULL por failure
 * * si compactarMemoria es X > 0, llama a compactarMemoria luego de de X intentos fallidos de alocar el dato
 * si compactarMemoria es -1, nunca llama a compactarMemoria
 */
t_dict_element *diccionario_obtenerDireccion(t_diccionario ** self, size_t data_size){
	/*
	 * @DESC:
	 * 	Funcion que obtiene la direccion donde se almacenara un item eliminando otro
	 * 	si es necesario y compactando la memoria cuando corresponda. Basicamente
	 * 	es el core del set
	 * 	Nota: Solo llama a compactar memoria si luego de hacerlo va a haber lugar suficiente para el dato.
	 * @PARAMETROS:
	 * 	self				- Es el diccionario
	 * 	data_size			- Es el tamaño del dato a almacenar
	 * @RETURN:
	 * 	item				- Es el item donde se almacenara el dato.
	 * 	NULL				- En caso que no sea posible almacenar el dato.
	 */

int aux_data_size = max(data_size,(*self)->slab_min_size);
while(aux_data_size <= (*self)->table_max_size){

		t_dict_element* item = (*self)->elements;

		while(item != NULL && (item->ocupado || estaReservado(&item) || item->chunk_size < aux_data_size)){
			item = item->next;
		}

		if(item == NULL){
			if((*self)->compactarMemoria > 0){(*self)->compactarMemoria--;}

			if((*	self)->compactarMemoria == 0 && (*self)->table_max_size -(*self)->table_current_size >= aux_data_size){
				diccionario_compactarMemoria(self);
				(*self)->compactarMemoria = (*self)->frecuenciaCompactacion;
				continue;
			}

			t_dict_element* victima = diccionario_elegirVictima(*self);
			item = diccionario_eliminar(self,victima);
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

			if((*self)->nodes != NULL && (item->chunk_size - aux_data_size) >= (*self)->slab_min_size){
				t_dict_element* nuevo_nodo = (*self)->nodes;
				(*self)->nodes = nuevo_nodo->next;
				if((*self)->nodes != NULL){(*self)->nodes->prev = NULL;}
				nuevo_nodo->next = item->next;
				nuevo_nodo->prev = item;
				if(item->next != NULL){item->next->prev = nuevo_nodo;}
				item->next = nuevo_nodo;
				item->next->chunk_size += item->chunk_size - aux_data_size;
				item->chunk_size = aux_data_size;
				item->next->data = item->data + item->chunk_size;
			}



			item->data_size = data_size;
			(*self)->table_current_size += data_size;
			return item;

}
	return NULL; //elemento mas grande que la memoria
}

/*
 * Marca como eliminados al elemento a eliminar y junta las particiones contiguas vacias
 * (tanto para adelante como para atras)
 */

t_dict_element *diccionario_eliminar(t_diccionario ** self, t_dict_element* it){
	/*
	 * @DESC:
	 * 	Funcion que marca como eliminados los elementos a eliminar y luego
	 * 	junta todas las particiones (tanto para adelante como para atras)
	 * 	contiguas que estan vacias
	 * @PARAMETROS:
	 * 	self				- Es el diccionario
	 * 	it					. Es el item a eliminar
	 * @RETURN:
	 * 	it					- Es el item nuevo que ahora tiene mas espacio
	 * 	NULL				- En caso que no exista la key.
	 */
	if(it != NULL){
		(*self)->table_current_size -= it->data_size;
		memcpy(it->key,"",1);
		it->hour = 0;
		it->nkey = 0;
		it->flags = 0;
		it->ocupado = 0;
		it->data_size = 0;

		t_dict_element *anterior = it->prev;      //se juntan los pedazos de memoria libre contiguos
		while(anterior != NULL && (!anterior->ocupado)){
			anterior->chunk_size += it->chunk_size;
			it->chunk_size = 0;
			anterior->next= it->next;
			if(it->next != NULL){
				it->next->prev = anterior;
			}
			it->prev = NULL;
			it->next = (*self)->nodes;
			if((*self)->nodes != NULL){(*self)->nodes->prev = it;}
			(*self)->nodes = it;
			it = anterior;
			anterior = it->prev;
		}
		t_dict_element *posterior = it->next;
		while(posterior != NULL && (!posterior->ocupado)){
			it->chunk_size += posterior->chunk_size;
			posterior->chunk_size = 0;
			it->next = posterior->next;
			posterior->prev = NULL;
			posterior->next = (*self)->nodes;
			if((*self)->nodes != NULL){(*self)->nodes->prev = posterior;}
			(*self)->nodes = posterior;
			posterior = it->next;
		}

		return it;
	}else return NULL;//FAILURE, no existe la key

}




static void diccionario_compactarMemoria(t_diccionario **self){
	/*
	 * @DESC:
	 * 	Funcion que junta todos los espacios de memoria ocupados de forma contigua en el comienzo
	 * 	de la memoria.
	 * 	Esto lo hace de la siguiente forma:
	 * 	1 - Busca items que esten libres y se fija si el que le sigue tambien
	 * 	esta libre, que en ese caso los une, repitiendo el proceso nuevamente
	 * 	para el siguiente elemento
	 * 	2 - Si encontro un item que estaba ocupado o reservado y el anterior
	 * 	estaba libre entonces intercambia el item. Luego repite el paso 1.
	 * @PARAMETROS:
	 * 	self				- Es el diccionario
	 */

	char mensaje[255];
	sprintf(mensaje, "Operation: Compacting Memory");
	log_info(logger, mensaje);

	t_dict_element *item = (*self)->elements;
	while(item != NULL){
		while(item != NULL && (item->ocupado || estaReservado(&item))){
			item = item->next;
		}
		if(item == NULL) break;
		t_dict_element *item_libre = item;
		item = item->next;
	while(item != NULL){
			while(item != NULL && !item->ocupado && !estaReservado(&item)){ //este ciclo no tiene sentido si se juntan los pedazos contiguos de memoria libre al eliminarlos
				item_libre->chunk_size += item->chunk_size;

				item_libre->next = item->next;
				if(item->next != NULL){item->next->prev = item_libre;}
				item->prev = NULL;
				item->next = (*self)->nodes;
				if((*self)->nodes != NULL){(*self)->nodes->prev = item;}
				(*self)->nodes = item;
				(*self)->nodes->chunk_size = 0;
				item = item_libre->next;
			}
			if(item != NULL){
				if((item->ocupado || estaReservado(&item))){
					int espacioLibre = item_libre->chunk_size;
					item_libre->chunk_size = item->chunk_size;
					memcpy(item_libre->data,item->data,item->chunk_size);
					item->data = item_libre->data + item_libre->chunk_size;
					item_libre->data_size = item->data_size;
					item_libre->hour = item->hour;
					memcpy(item_libre->key,item->key + '\0',item->nkey + 1);
					item_libre->ocupado = 1;

					item->prev = item_libre;
					item->chunk_size = espacioLibre;
					item->ocupado = 0;
					memcpy(item->key,"",1);
					item->hour = 0;
					item->data_size = 0;
				}
			}else break;

		item_libre = item_libre->next;
		item = item->next;

		}
	}
}




