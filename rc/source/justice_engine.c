/*
 * justice_engine.c
 *
 *  Created on: 16/05/2012
 *      Author: utnso
 */



#include <stdio.h>
#include <pthread.h>
#include "../headers/justice_engine.h"
#include <stdbool.h>
#include <memcached/config_parser.h>
#include "../commons/config.h"
#include "../commons/log.h"
#include <mcheck.h>

char* ESQUEMA;
int COMPACTION_FREQ;
int LRU;
int BESTFIT;

//#define TABLE_MAX_SIZE 32
//#define ESQUEMA "ParticionesDinamicas"
//#define SLAB_SIZE 4
//#define COMPACTION_FREQ 2
//#define LRU 1
//#define BESTFIT 0

static void justice_engine_destroy(ENGINE_HANDLE *handle, const bool force);
static ENGINE_ERROR_CODE justice_engine_allocate(ENGINE_HANDLE *handler, const void *cookie, item **item, const void* key, const size_t nkey, const size_t nbytes,const int flags, const rel_time_t exptime);
static bool justice_engine_get_item_info(ENGINE_HANDLE *handle,const void *cookie, const item* item, item_info *item_info);
static ENGINE_ERROR_CODE justice_engine_inicializar(ENGINE_HANDLE *handle, const char* config_str);
static ENGINE_ERROR_CODE justice_engine_inicializar(ENGINE_HANDLE *handle, const char* config_str);
static ENGINE_ERROR_CODE justice_engine_store(ENGINE_HANDLE *handler, const void *cookie, item* item, uint64_t *cas, ENGINE_STORE_OPERATION operation, uint16_t vbucket);
static void justice_engine_item_release(ENGINE_HANDLE *handle, const void *cookie, item* item);
static ENGINE_ERROR_CODE justice_engine_get(ENGINE_HANDLE *handler, const void* cookie, item** item,const void* key,const int nkey, uint16_t vbucket);
static const engine_info* justice_engine_get_info(ENGINE_HANDLE *handler);
static ENGINE_ERROR_CODE justice_engine_item_delete(ENGINE_HANDLE *handler, const void* cookie, const void* key, const size_t nkey,uint64_t cas,uint16_t vbucket);
static ENGINE_ERROR_CODE justice_engine_get_stats(ENGINE_HANDLE* handle, const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat);
static void justice_engine_reset_stats(ENGINE_HANDLE* handle, const void *cookie);
static ENGINE_ERROR_CODE justice_engine_unknown_command(ENGINE_HANDLE* handle, const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response);
static void justice_engine_item_set_cas(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t val);
static ENGINE_ERROR_CODE justice_engine_flush(ENGINE_HANDLE *handler,const void* cookie, time_t time);

t_diccionario *cache; //variable global donde se creara la cache
pthread_rwlock_t lock;
void justice_engine_dump();
void justice_engine_destroy_signal();
MEMCACHED_PUBLIC_API
ENGINE_ERROR_CODE create_instance(uint64_t interface, GET_SERVER_API get_server_api, ENGINE_HANDLE **handle){
	if(interface == 0){
		return ENGINE_ENOTSUP;
	}

	t_justice_engine *engine = calloc(1,sizeof(t_justice_engine));
	if(engine == NULL){
		return ENGINE_ENOMEM;
	}

	engine->engine.interface.interface = 1;

	engine->engine.initialize = justice_engine_inicializar;
	engine->engine.destroy = justice_engine_destroy;
	engine->engine.get_info = justice_engine_get_info;
	engine->engine.allocate = justice_engine_allocate;
	engine->engine.remove = justice_engine_item_delete;
	engine->engine.release = justice_engine_item_release;
	engine->engine.get = justice_engine_get;
	engine->engine.get_stats = justice_engine_get_stats;
	engine->engine.reset_stats = justice_engine_reset_stats;
	engine->engine.store = justice_engine_store;
	engine->engine.flush = justice_engine_flush;
	engine->engine.unknown_command = justice_engine_unknown_command;
	engine->engine.item_set_cas = justice_engine_item_set_cas;
	engine->engine.get_item_info = justice_engine_get_item_info;

	engine->get_server_api = get_server_api;

	*handle = (ENGINE_HANDLE*) engine;

	return ENGINE_SUCCESS;

}

static void justice_engine_destroy(ENGINE_HANDLE *handle, const bool force){
	/*
	 * @DESC:
	 * 	Funcion que destrue el engine
	 */
	free(handle);
}

/*
 * se llama una sola vez y luego de create_instance
 */

void configfile_init(char* unPath, char** scheme, int* compaction_freq, int* lru, int* bestfit, char** dump_scheme, char** log_path){
	t_config* config = config_create(unPath);
	*compaction_freq = config_get_int_value(config, "compaction_freq");
	*lru= config_get_int_value(config, "lru");
	*bestfit = config_get_int_value(config, "bestfit");
	*scheme = strdup(config_get_string_value(config, "scheme"));
	*dump_scheme = strdup(config_get_string_value(config, "dump_scheme"));
	*log_path= strdup(config_get_string_value(config, "log_path"));
	config_destroy(config);
}

char* LOG_PATH;
char* DUMP_SCHEME;
t_log* logger;
static ENGINE_ERROR_CODE justice_engine_inicializar(ENGINE_HANDLE *handle, const char* config_str){
	/*
	 * @DESC:
	 * 	Funcion que se llama una sola vez y esto es justo luego que
	 * 	memcached haga el create_instance
	 */

	mtrace();
	char* PATH = "engine_config.ini";
	configfile_init(PATH, &ESQUEMA, &COMPACTION_FREQ, &LRU, &BESTFIT, &DUMP_SCHEME, &LOG_PATH);

	logger = log_create(LOG_PATH, "Justice_Engine", false, LOG_LEVEL_DEBUG);

	pthread_rwlock_init(&lock, NULL);
	t_justice_engine *engine = (t_justice_engine*) handle;

	if(strcmp(ESQUEMA, "ParticionesDinamicas") == 0){
		engine->f_crearDiccionario = diccionario_crear;
		engine->f_eliminar = diccionario_eliminar;
		engine->f_obtenerDireccion = diccionario_obtenerDireccion;
	}
	else if (strcmp(ESQUEMA, "BuddySystem") == 0){
		engine->f_crearDiccionario = diccionario_crear_buddy;
		engine->f_eliminar = diccionario_eliminar_buddy;
		engine->f_obtenerDireccion = diccionario_obtenerDireccion_buddy;
	}
	size_t* table_max_size = malloc(sizeof(size_t));
	size_t* chunk_min_size = malloc(sizeof(size_t));


	if(config_str != NULL){

		struct config_item items[] = {
				{.key = "chunk_size",
				.datatype = DT_SIZE,
				.value.dt_size = chunk_min_size},
				{.key = "item_size_max",
				.datatype = DT_SIZE,
				.value.dt_size = table_max_size},
				{.key = NULL}
		};

		parse_config(config_str, items, NULL);
	}
	cache = engine->f_crearDiccionario(*table_max_size, *chunk_min_size);


	cache->frecuenciaCompactacion = COMPACTION_FREQ;
	cache->lru = LRU; //aca le digo si usa el algoritmo lru (sino usa el fifo)
	cache->bestfit = BESTFIT; //aca le digo si usa el algoritmo besfit (sino usa el firstfit)

	signal(SIGUSR1,justice_engine_dump);
	signal(SIGKILL,justice_engine_destroy_signal);
	/*
	 * Para probar la signal en una consola hay que hacer lo siguiente:
	 * 1 - hacer "ps -axu" y ver el pid del proceso de memcached
	 * 2 - hacer un "kill -SIGUSR1 pid". Por ejemplo.. si el pid es 3920 entonces debo hacer
	 * kill -SIGUSR1 3920.
	 */

	return ENGINE_SUCCESS;
}

static ENGINE_ERROR_CODE justice_engine_allocate(ENGINE_HANDLE *handler, const void *cookie, item **item, const void* key, const size_t nkey, const size_t nbytes,const int flags, const rel_time_t exptime){
	/*
	 * @DESC:
	 * 	Funcion que se llama siempre antes de hacer, por ejemplo, un store
	 * 	(y luego store llama a release). Es posible que se llame 2 veces
	 * 	para la mimsa operacion puesto que memcached tiene una forma de
	 * 	trabajar con el protocolo ASCII en donde en una parte hace un envio
	 * 	de la metadata y en otra parte envia la data misma.
	 */
	char mensaje[255];
	sprintf(mensaje, "Operation: Allocate - Key: %s", (char*)key);
	log_debug(logger, mensaje);
	pthread_rwlock_wrlock(&lock);

	t_justice_engine *engine = (t_justice_engine*) handler;

	t_dict_element *element = diccionario_obtener(&cache, (char*)key,nkey);

	if(element != NULL && element->ocupado){
		engine->f_eliminar(&cache, element); //<--negrada(?) - Fede dice:JAJAJA
		//return ENGINE_KEY_EEXISTS;
	}

	element = engine->f_obtenerDireccion(&cache,nbytes);


	if(element == NULL){
		pthread_rwlock_unlock(&lock);
		return ENGINE_E2BIG;
	}

	element->nkey = nkey;
	element->flags = flags;
	memcpy(element->key,key+'\0',nkey + 1);
	*item = element;
	pthread_rwlock_unlock(&lock);
	return ENGINE_SUCCESS;
}

static ENGINE_ERROR_CODE justice_engine_store(ENGINE_HANDLE *handler, const void *cookie, item* item, uint64_t *cas, ENGINE_STORE_OPERATION operation, uint16_t vbucket){
	/*
	 * @DESC:
	 * 	Funcion que se llama cuando a memcached le envian
	 * 	un set.
	 */
	t_dict_element* it = (t_dict_element*) item; //casteo el item de memcached al item nuestro
	char mensaje[255];
	sprintf(mensaje, "Operation: Store - Key: %s", it->key);
	log_debug(logger, mensaje);

	if(it == NULL){
		return ENGINE_NOT_STORED;
	}

	it = diccionario_obtener(&cache, it->key,it->nkey);
	if(it != NULL){
		it->ocupado = 1;
		it->hour = time(NULL);
		*cas = 0;
		return ENGINE_SUCCESS;
	}else{
		return ENGINE_NOT_STORED;
	}
}

static bool justice_engine_get_item_info(ENGINE_HANDLE *handle,const void *cookie, const item* item, item_info *item_info){
	/*
	 * @DESC:
	 * 	Funcion que mapea el item_info que memcached sabe manejar con
	 * 	el item_info que nosotros manejamos
	 */
	t_dict_element* it = (t_dict_element*) item; //casteo el item de memcached al tiem nuestro

	char mensaje[255];
	sprintf(mensaje, "Operation: Get Item Info - Key: %s", it->key);
	log_debug(logger, mensaje);

	if(it == NULL){
		return false;
	}

	//si no es null procedo a hacer las equivalencias...
	item_info->cas = 0;
	item_info->clsid = 0;
	item_info->exptime = 0;
	item_info->flags = it->flags;
	item_info->nbytes = it->data_size;
	item_info->key = it->key;
	item_info->nkey = it->nkey;
	item_info->nvalue = 1;
	item_info->value[0].iov_base = it->data;		/*aca le digo donde lo va a guardar*/
	item_info->value[0].iov_len = it->data_size;    /*longitud del dato (no del chunk size, el chunk size lo conozco yo en los elementos del diccionario)*/

	return true;
}

static void justice_engine_item_release(ENGINE_HANDLE *handle, const void *cookie, item* item){
//si se llama al release de un item no storeado, se tiene que borrar
	t_dict_element* it = (t_dict_element*) item;
	char mensaje[255];
	sprintf(mensaje, "Operation: Item Release - Key: %s", it->key);
	log_debug(logger, mensaje);

	if(!it->ocupado){
		pthread_rwlock_wrlock(&lock);
		diccionario_eliminar(&cache,it);
		pthread_rwlock_unlock(&lock);
	}
}

static ENGINE_ERROR_CODE justice_engine_get(ENGINE_HANDLE *handler, const void* cookie, item** item,const void* key,const int nkey, uint16_t vbucket){
	/*
	 * @DESC:
	 * 	Funcion que se llama cuando memcached recibe un get
	 */
	char mensaje[255];
	sprintf(mensaje, "Operation: Get - Key: %s", (char*)key);
	log_debug(logger, mensaje);
	pthread_rwlock_rdlock(&lock);
	t_dict_element *it = diccionario_obtener(&cache, (char*)key,nkey);
	if(it == NULL || (it != NULL && !it->ocupado)){
		pthread_rwlock_unlock(&lock);
		return ENGINE_KEY_ENOENT;
	}

	if(cache->lru){ //Cambia en algo que el fifo tenga el time? Porque si no es asi podemos sacar este if
		it->hour = time(NULL);
	}
	*item = it;

	pthread_rwlock_unlock(&lock);
	return ENGINE_SUCCESS;
}

static const engine_info* justice_engine_get_info(ENGINE_HANDLE *handler){
	/*
	 * @DESC:
	 * 	Funcion que posee informacion varia sobre el engine
	 */
	char mensaje[255];
	sprintf(mensaje, "Operation: Get Info");
	log_debug(logger, mensaje);
	static engine_info info = {
			.description = "Justice Engine v1.0	",
			.num_features = 0,
	};
	return &info;
}

static ENGINE_ERROR_CODE justice_engine_item_delete(ENGINE_HANDLE *handler, const void* cookie, const void* key, const size_t nkey,uint64_t cas,uint16_t vbucket){
	/*
	 * @DESC:
	 * 	Funcion que se llama cuando memcached recibe un delete
	 */

	char mensaje[255];
	sprintf(mensaje, "Operation: Item Delete - Key: %s", (char*)key);
	log_debug(logger, mensaje);

	pthread_rwlock_wrlock(&lock);
	t_justice_engine *engine = (t_justice_engine*) handler;

	t_dict_element* dict_element = diccionario_obtener(&cache,(char*)key,nkey);
	if(dict_element == NULL){
		pthread_rwlock_unlock(&lock);
		return ENGINE_KEY_ENOENT;
	}


	engine->f_eliminar(&cache,dict_element);
	pthread_rwlock_unlock(&lock);
	return ENGINE_SUCCESS;
}


static ENGINE_ERROR_CODE justice_engine_flush(ENGINE_HANDLE *handler,const void* cookie, time_t time){
	/*
	 * @DESC:
	 * 	Funcion que se llama cuando memcached recibe un flush_all.
	 * 	Limpia toda la cache
	 */
	char mensaje[255];
	sprintf(mensaje, "Operation: Flush");
	log_debug(logger, mensaje);

	pthread_rwlock_wrlock(&lock);
	t_justice_engine *engine = (t_justice_engine*) handler;
	diccionario_limpiar(&cache, engine->f_eliminar);
	pthread_rwlock_unlock(&lock);
	return ENGINE_SUCCESS;
}

static ENGINE_ERROR_CODE justice_engine_get_stats(ENGINE_HANDLE* handle, const void* cookie, const char* stat_key, int nkey, ADD_STAT add_stat) {
	return ENGINE_SUCCESS;
}

static void justice_engine_reset_stats(ENGINE_HANDLE* handle, const void *cookie) {

}

static ENGINE_ERROR_CODE justice_engine_unknown_command(ENGINE_HANDLE* handle, const void* cookie, protocol_binary_request_header *request, ADD_RESPONSE response) {
	return ENGINE_ENOTSUP;
}

static void justice_engine_item_set_cas(ENGINE_HANDLE *handle, const void *cookie, item* item, uint64_t val) {

}

void justice_engine_dump(){
	/*
	 * @DESC:
	 * 	Funcion que cuando se recibe la SIGURS1
	 */
	char mensaje[255];
	sprintf(mensaje, "Operation: Dump");
	log_debug(logger, mensaje);

	pthread_rwlock_rdlock(&lock);
	diccionario_dump(cache, cache->lru);
	pthread_rwlock_unlock(&lock);
}

void justice_engine_destroy_signal(){
	log_destroy(logger);
	pthread_rwlock_destroy(&lock);
}
