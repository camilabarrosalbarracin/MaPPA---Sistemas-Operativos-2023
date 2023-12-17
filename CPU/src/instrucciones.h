#ifndef INSTRUCCIONES_H
#define INSTRUCCIONES_H

#include <Shared/Utils.h>
#include <Shared/logYConfig.h>
#include "semaphore.h"
#include <Shared/estructuras.h>
#include <math.h>

extern int pid;
extern int tam_pag;
extern int program_counter;
extern int socket_memoria;
extern int socket_kernel_dispatch;
extern t_registros_cpu registros_cpu;
extern bool ejecutando_un_proceso,interrupcion;
extern t_log* logger_cpu;


/* typedef struct{
    int identificador; // dice que instruccion es
	int cant_parametros;
	t_list* parametros; // lista de parametros (separados entre si), sin contar el identificador
}instruccion; */

void set(instruccion *inst);
void exit_(instruccion *inst);
void mostrar_parametros(instruccion *inst, int);
void devolver_contexto_de_ejecucion(int, instruccion *info);
void empaquetar_contexto(t_paquete *paquete_contexto, int motivo, instruccion *info);
char* recibir_instruccion(int socket_cliente);
u_int8_t get_identificador(char* termino);
char* get_termino(char* instruccion, int index);
uint32_t* get_direccion_registro(char* string_registro);
t_list *get_parametros(char *inst, u_int8_t cant_parametros);
void sum(instruccion* inst);
void sub(instruccion* inst);
void exit_(instruccion* inst);
void sleep_(instruccion* inst);
void wait(instruccion* inst);
void signal_(instruccion* inst);
void jnz(instruccion* inst);
void f_open(instruccion* inst);
void f_close(instruccion* inst);
void f_truncate(instruccion* inst);
void f_seek(instruccion* inst);
void f_write(instruccion* inst);
void f_read(instruccion* inst);
void mov_in(instruccion* inst);
void mov_out(instruccion* inst);
int traducir_direcciones(instruccion* inst,int dir_log);

#endif /* INSTRUCCIONES_H */