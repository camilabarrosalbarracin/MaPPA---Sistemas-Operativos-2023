#ifndef ESTRUCTURAS_H_
#define ESTRUCTURAS_H_

#include <stdint.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include <stdlib.h>
#include <commons/log.h>
#include <assert.h>
#include "Utils.h"
#include <string.h>

/* typedef enum {
	CREAR_NUEVA_TABLA=30,
	CREAR_SEGMENTO,
	ELIMINAR_SEGMENTO,
	LIBERAR_ESPACIO_PROCESO,
	MOV_IN_MEM,
	MOV_OUT_MEM,
	BASE_SEGMENTO,
	REGISTRO,
	KERNEL,
	CPU,
	FS,
	LECTURA_MEMORIA,
	ESCRITURA_MEMORIA,
	COMPACTAR,
	TABLAS_SEGMENTOS,
}instrucciones_memoria; */

typedef struct{
	uint32_t AX,BX,CX,DX;
}t_registros_cpu;

typedef struct t_pcb 
{
int id;
int prioridad;
int program_counter;
t_temporal* tiempo_llegada;
t_list* recursos_asignados;
t_list* archivos_abiertos;
t_registros_cpu registros_cpu;
}t_pcb;

typedef struct archivos_global
{
	char* nombre;
	int aperturas;
	bool escritura;
	t_list* bloqueados;
	int tamanio;
}t_archivo_global;

typedef struct t_archivo
{
	int pid;
	char* nombre;
	int puntero;
	t_archivo_global* entrada_tdp_global;
	char modo_apertura;
}t_archivo;

typedef struct tabla_paginas
{
    int marco;			
    bool presencia;
    bool modificado;
    int posicion_swap;
}tabla_paginas;

typedef	struct t_pidPag
{
	int pid;
	int pagina;
}t_pidPag;

typedef struct t_proceso // checkear
{
    int id;
    t_list* instrucciones;
    t_list* tdp;
    int cant_paginas;
}t_proceso;

typedef struct{
    int identificador; // dice que instruccion es
	int cant_parametros;
	t_list* parametros; // lista de parametros (separados entre si), sin contar el identificador
}instruccion;

typedef struct{
	t_pcb* pcb;
	int motivo;
	instruccion* info_instruccion;
}t_info_bloqueado;


void inicializar_registros(t_registros_cpu* registros);
void empaquetar_registros_cpu(t_paquete *paquete, t_registros_cpu registros_cpu);
void enviar_pcb(t_pcb* pcb, int socket);
t_paquete* empaquetar_pcb(t_pcb* pcb);
void empaquetar_registro(t_paquete *paquete, uint32_t registro_cpu[]);



#endif /* ESTRUCTURAS_H_ */
