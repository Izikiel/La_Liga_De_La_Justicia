
#include <math.h>
#include <stdlib.h>

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include "../headers/diccionario.h"
#include "../commons/log.h"

//#define DUMP_DINAMICAS "dinamicas.dump"
#define MAX_LINE_OUTPUT_DUMP 128
#define KEY_SIZE_MAX 45
extern char* DUMP_SCHEME;
extern int LRU;
extern t_log* logger;

int max(int a,int b){
	if(a > b){
		return a;
	}else return b;
}

int power_two_low(int un_numero){
	double indice_real = log2(un_numero);
	return pow(2, (int)indice_real);
}

short int estaReservado(t_dict_element **item){
	/*
	 * @DESC:
	 * 	Esta funcion lo que hace es ver si el item que se le pasa cumple las siguientes condiciones:
	 * 	 1 - No es NULL.
	 * 	 2 - Tiene la variable ocupado en 0
	 * 	 3 - Tiene una Key Asignada.
	 * 	Esto indica que esta reservado el item pero no esta ocupado (puesto que se le asigno una key pero no llego a ocuparse)
	 * @PARAMETROS:
	 * 	item - Un item del diccionario
	 * @RETURN:
	 * 	1 - Si cumple las condiciones descriptas
	 * 	0 - En caso contrario
	 *
	 */
	if((*item) != NULL && (*item)->ocupado == 0 && (*item)->nkey != 0){
		return 1;
	}else return 0;
}

void inicializarElementos(t_diccionario ** self,int cant_max_el,void* mem){
	/*
	 * @DESC:
	 * 	Funcion que se encarga de inicializar los nodos de un diccionario
	 * 	asi como poner en cero el diccionario->elements[0] del diccionario
	 * @PARAMETROS:
	 * 	self			- Es el diccionario
	 * 	cant_max_el		- Es la cantidad maxima de nodos que va a tener
	 * 	mem				- Es la memoria propiamente dicha
	 *
	 */
	int i=0;
	t_dict_element *element = ((*self)->elements);
	t_dict_element *nodes = ((*self)->nodes);

	for(; i + 1 < cant_max_el; i++){

		if(i != 0){
			nodes[i].prev = (nodes + i -1);
		} else {
			nodes[i].prev = NULL;
		}
		nodes[i].data = NULL;
		nodes[i].nkey = 0;
		nodes[i].data_size = 0;
		nodes[i].hour = 0;
		nodes[i].key = malloc(KEY_SIZE_MAX);
		nodes[i].ocupado = 0;
		nodes[i].chunk_size = 0;
		nodes[i].flags = 0;
		nodes[i].next = (nodes + i + 1);

	}
	nodes[i].next = NULL;
	element[0].prev = NULL;
	element[0].chunk_size = (*self)->table_max_size;
	element[0].data = mem;
	element[0].nkey = 0;
	element[0].data_size = 0;
	element[0].flags = 0;
	element[0].hour = 0;
	element[0].key = malloc(KEY_SIZE_MAX);
	element[0].next = NULL;
	element[0].ocupado = 0;

}

t_dict_element *diccionario_obtener(t_diccionario **self, char* key,int nkey){
	/*
	 * @DESC:
	 * 	Funcion que recorre todos los elementos de un diccionario y verifica
	 * 	si la key que se pasa por parametro se encuentra en alguno de los
	 * 	elementos.
	 * 	Hay que asegurarse que el item este "storeado" porque devuelve una
	 * 	particion por la key por mas que en realidad este vacio y el campo
	 * 	data este apuntando a cualquier cosa.
	 * @PARAMETROS:
	 * 	self			- Es el diccionario
	 * 	key				- Es la key que se quiere buscar en los elementos del diccionario
	 * @RETURN:
	 * 	it				- En caso que se encuentre la key devuelve el item que la posee.
	 * 	NULL			- En caso que no se encuentre la key
	 *
	 */
	t_dict_element *it = (*self)->elements;

	while(it != NULL){
		if(it->nkey != 0){
			if(it->nkey == nkey && memcmp(it->key, key, nkey) == 0){
					return it;
			}
		}
		it = it->next;
	}
	return NULL; //FAILURE, no existe la key
}

t_dict_element* diccionario_elegirVictima(t_diccionario* self){
	/*
	 * @DESC:
	 * 	Funcion que busca el item que sera la victima y lo hace en
	 * 	base al tiempo que lleva en memoria.
	 *
	 * @PARAMETROS:
	 * 	self			- Es el diccionario
	 *
	 * @RETURN:
	 * 	victima			- Es el item que debe eliminarse
	 */

	t_dict_element* victima = self->elements;

	while(victima != NULL && victima->hour == 0){
					victima = victima->next;
				}
	t_dict_element* item = victima;
	while(item != NULL){
			if((item->hour != 0) && (item->hour < victima->hour)){ //La victima es el mas viejo.
				victima = item;
			}
			item = item->next;
		}

		char mensaje[255];
		char* alg;
		if(LRU){
			alg = "LRU";
		}
		else{
			alg = "FIFO";
		}
		sprintf(mensaje, "Operation: Replace item - Algorithm: %s - Key: %s", alg, victima->key);
		log_debug(logger, mensaje);

		return victima;
}

void diccionario_limpiar(t_diccionario ** self, t_dict_element* (*f_eliminar)(t_diccionario** ,t_dict_element*)){
	/*
	 * @DESC:
	 * 	Funcion que recorre todos los items de un diccionario y los elimina
	 * 	usando el puntero a funcin f_eliminar.
	 * @PARAMETROS:
	 * 	self			- Es el diccionario
	 * 	f_eliminar		- Es un puntero a funcion que se encarga de hacer la eliminacion.
	 *
	 */
	t_dict_element *item = (*self)->elements;
	while(item != NULL){
		if(item->nkey != 0){
			item = f_eliminar(self,item);
		}
		item = item->next;
	}
}

void diccionario_dump(t_diccionario* self,int lru){
	/*
	 * @DESC:
	 * 	Funcion que que hace el dump de la cache del esquema de parciciones
	 * 	dinamicas.
	 * @PARAMETROS:
	 * 	self				- Es el diccionario
	 * 	lru					- Indica si la eliminacion de la victima se hace por LRU
	 */
	FILE* archivo_dump = fopen(DUMP_SCHEME, "w");
	t_dict_element *item = self->elements;
	int  i = 1;
	time_t tiempo = time(NULL);
	struct tm *tlocal = localtime(&tiempo);
	char output[MAX_LINE_OUTPUT_DUMP];
	strftime(output,MAX_LINE_OUTPUT_DUMP,"%d/%m/%y %H:%M:%S",tlocal);
	fprintf(archivo_dump, "Dump: %s\n",output);
	while(item != NULL){
		fprintf(archivo_dump,"Item %d: ",i);
		fprintf(archivo_dump,"%X - ",(unsigned int)item->data); //Direccion de Inicio
		fprintf(archivo_dump,"%X ",(unsigned int)item->data + item->chunk_size); //Direccion de Finalizacion
		if(item->ocupado){
			fprintf(archivo_dump,"[X] ");
		}else fprintf(archivo_dump,"[L] ");
		fprintf(archivo_dump,"chunk size: %d b ",item->chunk_size);
		if(lru){
			fprintf(archivo_dump,"LRU:<%d> ",(int)item->hour);
		}else fprintf(archivo_dump,"FIFO:<%d> ",(int)item->hour);
	int j = 0;
	for(;j < item->nkey; j++) putc(item->key[j],archivo_dump);
	item = item->next;
	i++;
	fprintf(archivo_dump, "\n");
	}
	fclose(archivo_dump);
}
