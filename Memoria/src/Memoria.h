#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <Shared/Utils.h>
#include <Shared/logYConfig.h>
#include <commons/collections/list.h>
#include <commons/bitarray.h> 
#include "pthread.h"
#include "semaphore.h"
#include <math.h>
#include "MemoriaInstrucciones.h"
#include <commons/collections/queue.h>

t_log* logger_memoria;
t_config* config_memoria;
int socket_peticion_FS;
t_list* procesos_memoria;
void *memoria;
t_queue* cola_paginas;

int conectarFS();
int atenderFS(int* socket_fs);
int conectarCpu(int* socket_cpu);
int conectarKernel(int* socket_kernel);
void log_protegido_mem(char* mensaje);
int retardo_memoria();
int tamanio_memoria();
int frames_totales();
int tamanio_pagina();
char* algoritmo_reemplazo();
void crear_tabla_de_paginas(double size_proceso,t_proceso* proceso_nuevo);
void liberar_memoria_instrucciones(int pid_a_liberar);
void pedir_bloques_swap(t_proceso* proceso_nuevo);
bool recibir_pos_swap(t_proceso* proceso_nuevo);
int buscar_marco(int pid, int pagina_recibida);
void liberar_memoria_espacio_usuario(int pid);
int seleccionar_victima(char *algoritmo);
void cargar_pagina(int pid, int pagina, int marco);
void recibir_pagina_FS();
void pedir_pagina_FS(int pid, int pag, int marco);
void inicializar_bitmap_frames();
void sacar_pagina_memoria(int pid, int pag);
void leer_pagina(int pid, int pag);
int buscar_pidPag(t_list* lista, int pid, int pag);
void agregar_a_referenciadas(int pid, int pag);
void enviar_marco_asociado(int pid, int pagina, int *socket_cpu);
bool chequeo_mem_llena();
int buscar_frame_libre();
int resolver_page_fault(int pid, int pagina);
t_pidPag* buscar_proceso_asociado_a(int marco_in);

#endif /* MEMORIA_H_ */
