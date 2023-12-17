#ifndef MEMORIA_INSTRUCCIONES_H_
#define MEMORIA_INSTRUCCIONES_H_

#include <Shared/Utils.h>
#include <Shared/logYConfig.h>
#include <Shared/estructuras.h>
#include "pthread.h"
#include "semaphore.h"

extern t_log* logger_memoria;
extern t_config* config_memoria;
extern t_list* procesos_memoria;

t_proceso* agregar_proceso_instrucciones(FILE* f, int pid);
char* buscar_instruccion_proceso(int pid, int program_counter);
t_proceso *buscar_proceso(t_list *lista, int pid_buscado);
void eliminar_proceso(int pid);

#endif /* MEMORIA_INSTRUCCIONES_H_ */