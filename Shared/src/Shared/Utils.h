#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<commons/string.h>
#include<assert.h>
#include<signal.h>
#include<string.h>

#define PUERTO "4444"

typedef enum
{
	MENSAJE,
	PAQUETE,
	KERNEL_INTERRUPT,
	KERNEL_DISPATCH,
	KERNEL,
	FS,
	CPU,
	MEMORIA,
	CONTEXTO_EJECUCION,
	INICIAR_PLANIFICACION,
	DETENER_PLANIFICACION,
	MULTIPROGRAMACION,
	INICIAR_PROCESO,
	FINALIZAR_PROCESO,
	PROCESO_ESTADO,
	NEW,
	READY,
	EXEC,
	BLOCKED,
	EXIT,
	PCB,
	SET,
	SUM,
	SUB,
	MOV_IN,
	MOV_OUT,
	SLEEP,
	JNZ,
	WAIT,
	SIGNAL,
	F_CREATE,
	F_OPEN,
	F_TRUNCATE,
	F_SEEK,
	F_WRITE,
	F_READ,
	F_CLOSE,
	INVALID_RESOURCE,
	INVALID_WRITE,
	SUCCESS,
	PAUSAR_EJECUCION,
	INICIAR,
	PRIORIDADES,
	RR,
	INTERRUPCION,
	LIBERAR_ESPACIO_PROCESO,
	FETCH_INSTRUCCION,
	PEDIDO_MARCO,
	ENVIAR_MARCO,
	NO_RECONOCIDO,
	ESCRITURA_MEMORIA,
	LECTURA_MEMORIA,
	ASIGNAR_BLOQUE_SWAP,
	LIBERAR_BLOQUES,
	DESALOJAR_PROCESO,
	PAGE_FAULT,
	FILE_EXISTS,
	BLOQUES_ASIGNADOS,
	CARGAR_PAGINA,
	ESCRIBIR_PAGINA,
	FIFO,
	LEER_SWAP,
	ESCRIBIR_SWAP,
	DATO_REGISTRO,
	RECIBIR_POS_SWAP
}op_code;
//tener en cuenta la función get_motivo

extern t_log* logger;

void* recibir_buffer(int*, int);
int iniciar_servidor(char*);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int,t_log*);
int recibir_operacion(int);

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


t_buffer* crear_buffer();
int crear_conexion(char* ip, char* puerto);
void enviar_mensaje(char* mensaje, int socket_cliente);
t_paquete *crear_paquete(int codigo_operacion);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void agregar_a_paquete_solo(t_paquete *paquete, void *valor, int bytes);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
u_int8_t get_cant_parametros(u_int8_t identificador);
char *get_motivo(int motivo);

#endif /* UTILS_H_ */
