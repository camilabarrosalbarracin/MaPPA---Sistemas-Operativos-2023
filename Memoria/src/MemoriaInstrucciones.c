#include "MemoriaInstrucciones.h"

t_proceso *agregar_proceso_instrucciones(FILE *f, int pid)
{
	char *instruccion = NULL;
	size_t longitud = 0;
	int cant_instrucciones = 0;

	t_proceso *proceso = malloc(sizeof(t_proceso));
	proceso->id = pid;
	proceso->instrucciones = list_create();

	while (getline(&instruccion, &longitud, f) != -1)
	{
		if (strcmp(instruccion, "\n"))
		{
			int longitud = strlen(instruccion);
			if (instruccion[longitud - 1] == '\n')
				instruccion[longitud - 1] = '\0';
			char* aux = malloc(strlen(instruccion)+1);
			strcpy(aux,instruccion);
			list_add(proceso->instrucciones, aux);
			cant_instrucciones++;
		} 
	}
	free(instruccion);
	fclose(f);
	for(int i=0; i<list_size(proceso->instrucciones); i++){
		char* aux= list_get(proceso->instrucciones, i);
		printf("%s\n",aux);
	}
	list_add(procesos_memoria, proceso);
	return proceso;
}

char *buscar_instruccion_proceso(int pid, int program_counter)
{
	t_proceso *proceso = buscar_proceso(procesos_memoria, pid);
	char *instruccion = list_get(proceso->instrucciones, program_counter);
	printf("%s\n", instruccion);
	return instruccion;
}

t_proceso *buscar_proceso(t_list *lista, int pid_buscado)
{
	int elementos = list_size(lista);
	for (int i = 0; i < elementos; i++)
	{
		t_proceso *proceso = list_get(lista, i);
		if (pid_buscado == proceso->id)
		{
			return proceso;
		}
	}
	printf("proceso no encontrado\n");
	return NULL;
}

void eliminar_proceso(int pid)
{
	t_proceso *proceso_a_eliminar = buscar_proceso(procesos_memoria, pid);
	list_remove_element(procesos_memoria, proceso_a_eliminar);
	free(proceso_a_eliminar);
}
