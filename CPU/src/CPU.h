#ifndef CPU_H_
#define CPU_H_

#include <Shared/Utils.h>
#include <Shared/logYConfig.h>
#include "pthread.h"
#include "semaphore.h"
#include "instrucciones.h"

t_log* logger_cpu;
t_config* config_cpu;

int pid;
int program_counter;
int motivo_interrupt,pid_interrupt;
int tam_pag;
t_registros_cpu registros_cpu;
int socket_memoria;
int socket_servidor_dispatch,socket_kernel_dispatch;
bool ejecutando_un_proceso,interrupcion;

sem_t mlog;


int conectarKernel();
void conectarMemoria();
void log_protegido_cpu(char* mensaje);
int conectarKernelDispatch(/*int *socket_kernel_dispatch*/);
int conectarKernelInterrupt(/*int *socket_kernel_interrupt*/);
void ejecutar_proceso();
char* fetch();
instruccion* decode(char*);
instruccion* execute(instruccion* inst);
void check_interrupt(instruccion* inst);
void recibir_contexto_ejecucion(int socket);
void desempaquetar_contexto_ejecucion(void *buffer);



#endif /* CPU_H_ */