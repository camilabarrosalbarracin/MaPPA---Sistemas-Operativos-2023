#ifndef COLAS_H_
#define COLAS_H_
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <Shared/estructuras.h>
#include <semaphore.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <sys/time.h>
#include "pthread.h"

extern int tam_pag;
extern bool deadlock;
extern t_pcb *proceso_exec;
extern int socket_dispatch, socket_interrupt,socket_memoria, socket_FS;
extern t_log *logger_kernel;
extern t_config* config_kernel;
sem_t new_disponible,ready_disponible,procesos_en_ready,
            nivel_multiprogramacion,procesos_en_exit,bloqueado;

pthread_mutex_t mlog, mnew, mready, mexit, mexec, mbloqueado,mbloqueados, sem_fileSystem, sem_memoria;
t_queue *exit_queue, *block_queue, *new_queue, *exec_queue;
t_list *ready, *recursos, *bloqueados;
int new_counter;
extern int grado_multiprogramacion;
extern bool ejecucion_pausada;
t_list *tabla_archivos_abiertos;
extern bool volvio_pcb_cpu;

typedef struct {

    char* nombre;
    int instancias;
    t_queue* bloqueados;
}t_recurso;

void log_protegido(char *mensaje);
void inicializar_estructuras(); 
void iniciar_hilos_estructuras();
void free_estructuras();
void agregar_a_new(t_pcb *pcb);
void crear_colas_de_recursos();
char* algoritmo_planificacion();
int grado_multiprogramacion_ini();
double quantum();
void planificador_a_largo_plazo();
void planificador_a_corto_plazo();
void crear_colas_de_recursos();
char* eliminar_primer_ultimo_caracter(char *cadena);
void agregar_a_ready(t_pcb* pcb);
void loggear_pids_ready();
void loggear_pids_cola(char* nombre_cola);
void loggear_pids_lista(char* nombre_lista);
t_pcb* quitar_de_ready();
void agregar_a_ready_prioridades(t_pcb* pcb);
t_pcb* buscar_pcb(t_list *lista, int pid_buscado);
void buscar_y_finalizar_proceso(int pid_buscado,int motivo);
void hilo_exit();
void recibir_pcb_CPU();
void liberar_memoria(t_pcb *pcb);
void finalizar_proceso(t_pcb* pcb, int motivo);
void pausar_ejecucion();
void reanudar_ejecucion();
void recibir_contexto_ejecucion_de_cpu(t_pcb *pcb, int *motivo, instruccion* info_instruccion);
void desempaquetar_contexto_ejecucion_de_cpu(t_pcb *pcb, int* motivo, instruccion* info_instruccion);
void desalojar_proceso(int motivo);
void reordenar_ready();
void hilo_bloqueados();
void hilo_sleep(t_info_bloqueado *info_sleep);
t_recurso* existe_el_recurso(char *recurso, t_list *lista_recursos);
void agregar_recurso_pcb(t_pcb *pcb, t_recurso *recurso);
int buscar_indice(t_list *recursos_, char *nombre);
void quitar_recurso_pcb(t_pcb *pcb, t_recurso *recurso_del_proceso);
void contador_round_robin(int pcb_id);
void hay_deadlock_recursos();
void hay_deadlock_archivos();
void liberar_recursos(t_pcb* pcb);
void listar_procesos_por_estado();
t_archivo_global* buscar_archivo_abierto(char* nombre);
void cerrar_archivo(char* nombre_archivo, t_pcb* pcb);
void agregar_archivo_al_proceso(t_archivo* archivo, t_pcb* pcb);
t_archivo* get_archivo_proceso(char* nombre_archivo, t_pcb* pcb);
void loggear_deadlock_recursos(t_pcb* pcb, char* recurso_necesario);
void detectar_deadlock();
t_archivo* buscar_archivo_en_el_proceso(char* nombre, t_pcb* pcb);
void vuelve_a_ready(t_pcb* pcb);
void liberar_archivos_abiertos(t_pcb* pcb);
void buscar_y_sacar_pcb_cola(t_recurso* recurso_que_bloquea,int pid_buscado);
void hilo_espera_fs(t_info_bloqueado *info_sleep);
void hilo_pf(t_info_bloqueado *info_fs);
void loggear_deadlock_archivos(t_pcb* pcb, char* archivo);
void liberar_info_instruccion(instruccion* info_instruccion);
void liberar_pcb(t_pcb* pcb);
void cambiar_multiprogramacion(int grado_multiprogramacion);
int abrir_archivo(t_archivo* archivo);

#endif