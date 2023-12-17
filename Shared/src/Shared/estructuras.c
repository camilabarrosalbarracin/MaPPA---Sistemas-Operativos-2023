#include "estructuras.h"


void inicializar_registros(t_registros_cpu* registros)
{
        registros->AX=0;
	    registros->BX=0;
	    registros->CX=0;
	    registros->DX=0;
}

void enviar_pcb(t_pcb* pcb, int socket){
	t_paquete* paquete_pcb = empaquetar_pcb(pcb);

	enviar_paquete(paquete_pcb, socket);
	eliminar_paquete(paquete_pcb);
}

t_paquete* empaquetar_pcb(t_pcb* pcb){
	t_paquete *paquete = crear_paquete(PCB);
	agregar_a_paquete_solo(paquete,&(pcb->id),sizeof(int));
	agregar_a_paquete_solo(paquete,&(pcb->program_counter),sizeof(int));

	empaquetar_registros_cpu(paquete,pcb->registros_cpu);
	
	/*
	FALTA DECIDIR EL TIPO ARCHIVO Y EMPAQUETAR LA LISTA DE ARCHIVOS ABIERTOS

	agregar_a_paquete_solo(paquete,&(list_size(pcb->archivos_abiertos)),sizeof(int));
	agregar_a_paquete_solo(paquete,&(pcb->archivos_abiertos),sizeof(t_list*));
	*/
	return paquete;
}

void empaquetar_registros_cpu(t_paquete *paquete, t_registros_cpu registros_cpu)
{
 	agregar_a_paquete_solo(paquete, &(registros_cpu.AX), sizeof(uint32_t));
	agregar_a_paquete_solo(paquete, &(registros_cpu.BX), sizeof(uint32_t));
	agregar_a_paquete_solo(paquete, &(registros_cpu.CX), sizeof(uint32_t));
	agregar_a_paquete_solo(paquete, &(registros_cpu.DX), sizeof(uint32_t));
}

