#ifndef FS_H_
#define FS_H_


#include <Shared/Utils.h>
#include <Shared/logYConfig.h>
#include "pthread.h"
#include "semaphore.h"
#include <fcntl.h>            // funciones de archivos
#include <sys/mman.h>         // map archivos a memoria - sync actualizar
#include <sys/stat.h>
#include <stdio.h>
#include <commons/bitarray.h> 
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/memory.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <commons/collections/dictionary.h> // diccionarios
#include <math.h>
#include <stdint.h> // unit32_max


t_log* logger_fs;
t_config* config_fs;

// Enteros
int *bufferFAT;
void *bufferBloques;
t_bitarray *bitmap_swap;
// Conexiones
int conectarKernel();
int conectarMemoria();
int atenderMemoria();
t_log* logger_FS();
void log_protegido_fs(char *mensaje);
//inits
void inicializar_FAT();
void inicializar_bloques();
void inicializar_bitmap_swap();
//funciones
void abrir_archivo(char *);
void crear_archivo(char *archivo);
void truncar_archivo(char *archivo, int tamanioTruncar);
void sacarBloques(int bloquesASacar, int bloque_inicial, char *nombre_archivo,int tamanio_Archivo);
void agregarBloques(int bloquesAAgregar, int *puntero, char *nombre_archivo);
void leer_archivo(char *archivo, int puntero, int bytesALeer, int direccionFisica, int pid);
void escribir_archivo(char *archivo, int puntero, int bytesAEscribir, int direccionFisica, int pid);

void iniciar_proceso(int ,int);
void finalizar_proceso(void*, int);
//valores  cfg
int retardo_acceso_FAT();
int retardo_acceso_bloques();
int tamanio_bloque();
int cant_bloques_SWAP();
int cant_bloques_total();
char* path_FCB();
char* path_bloques();
char* path_FAT();
// Alto getter
void get_array_punteros(void* punteros_archivo, int bloque_inicial,int tamanio);


#endif /* FS_H_ */