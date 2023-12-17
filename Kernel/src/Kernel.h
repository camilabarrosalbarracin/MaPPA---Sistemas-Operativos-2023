#ifndef KERNEL_H_
#define KERNEL_H_

#include <Shared/Utils.h>
#include <Shared/logYConfig.h>
#include <Shared/estructuras.h>
#include "pthread.h"
#include "Colas.h"
#include <commons/temporal.h>

t_log* logger_kernel;
t_config* config_kernel;
t_pcb* proceso_exec;
sem_t planificadores;

int socket_FS;
int socket_memoria;
int socket_dispatch;
int socket_interrupt;
int id_counter;
int grado_multiprogramacion;
bool ejecucion_pausada, volvio_pcb_cpu;
bool deadlock;
int tam_pag;

typedef struct Comando
{
    int cod_op;
    t_list* parametros; 
}t_comando;//hacer free de la lista antes de hacer free del registro


int conectarCpuDispatch();
int conectarCpuInterrupt();
int conectarMemoria();
int conectarFS();
void leer_consola();
void interpretar(t_comando* comando, char* leido);
t_pcb* generar_pcb(t_comando* comando);
void detectar_deadlock();

#endif /* KERNEL_H_ */