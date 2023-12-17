#include "Colas.h"

void inicializar_estructuras(){
	volvio_pcb_cpu=false;
	deadlock=false;
	ejecucion_pausada=true;
    //Semaforos Mutex
    pthread_mutex_init(&mlog, NULL);
    pthread_mutex_init(&mnew, NULL); 
	pthread_mutex_init(&mready, NULL); 
	pthread_mutex_init(&mexit, NULL);    
	pthread_mutex_init(&mexec, NULL);
	//pthread_mutex_init(&mbloqueado,NULL);
	pthread_mutex_init(&mbloqueados,NULL);
	pthread_mutex_init(&sem_fileSystem,NULL);
	pthread_mutex_init(&sem_memoria, NULL);
    log_protegido(string_from_format("Semaforos Mutex inicializados."));

	//Semaforos
    sem_init(&new_disponible, 0, 0);
    sem_init(&ready_disponible, 0, 0);
    sem_init(&procesos_en_ready, 0, 0);
	//sem_init(&bloqueado,0,0);
    sem_init(&nivel_multiprogramacion, 0, grado_multiprogramacion_ini());//CUANDO CAMBIA EL NIVEL DE MULTIPROGRAMACION HAY QUE CAMBIARLO
    sem_init(&procesos_en_exit,0,0);
    log_protegido(string_from_format("Semaforos inicializados"));
	proceso_exec=NULL;

	//Colas
    new_queue = queue_create(); 
    ready = list_create();      //Cada cola es una lista de pcbs
    exit_queue = queue_create();
    //block_queue = queue_create();
    log_protegido(string_from_format("Colas creadas."));

    //Contadores
    new_counter=0;
    log_protegido(string_from_format("Contadores inicializados."));

	//TABLA ARCHIVOS GLOBAL
	tabla_archivos_abiertos=list_create();

    //COLAS DE RECURSOS
	recursos=list_create();
	bloqueados=list_create();
    crear_colas_de_recursos();
	grado_multiprogramacion = grado_multiprogramacion_ini();
}

void free_estructuras(){
	//Colas
	queue_destroy(new_queue);
	queue_destroy(exit_queue);
	//queue_destroy(block_queue);
	list_destroy(ready);

	//Semaforos
	sem_destroy(&ready_disponible);
	sem_destroy(&new_disponible);
	sem_destroy(&procesos_en_ready);
	sem_destroy(&nivel_multiprogramacion);
	//sem_destroy(&bloqueado);

	//Semaforos Mutex
	pthread_mutex_destroy(&mlog);
	pthread_mutex_destroy(&mnew);
	pthread_mutex_destroy(&mready);
	pthread_mutex_destroy(&mexit);
	pthread_mutex_destroy(&mexec);
	//pthread_mutex_destroy(&mbloqueado);
	pthread_mutex_destroy(&mbloqueados);
	pthread_mutex_destroy(&sem_fileSystem);
	pthread_mutex_destroy(&sem_memoria);

}

void iniciar_hilos_estructuras()
{
	pthread_t t1, t2, t3;//, t4;
	pthread_create(&t1, NULL, (void *)planificador_a_largo_plazo, NULL);
	pthread_create(&t2, NULL, (void *)planificador_a_corto_plazo, NULL);
	pthread_create(&t3, NULL, (void *)hilo_exit, NULL);
	//pthread_create(&t4, NULL, (void *)hilo_bloqueados, NULL);
	//pthread_create(&t5,NULL, (void *)hilo_bloqueados_fs,NULL);

 	pthread_detach(t1);
	pthread_detach(t2);
	pthread_detach(t3); 
	//pthread_detach(t4);
}

void log_protegido(char *mensaje){
	pthread_mutex_lock(&mlog);
	log_info(logger_kernel, "%s", mensaje);
	pthread_mutex_unlock(&mlog);
	free(mensaje);
}

void agregar_a_new(t_pcb *pcb) {

	pthread_mutex_lock(&mnew);
	queue_push(new_queue, pcb);	 //agrega elemento al final de la cola
	pthread_mutex_unlock(&mnew);

	log_protegido(string_from_format("CONSOLA: Se crea el proceso de pid <%d> en NEW", pcb->id));

	sem_post(&new_disponible);
}

void planificador_a_largo_plazo(){
	while(1){
		while(!ejecucion_pausada){
			//sem_wait(&ejecutando_largo_plazo);
			sem_wait(&new_disponible);
			sem_wait(&nivel_multiprogramacion);
			grado_multiprogramacion--;

			pthread_mutex_lock(&mnew);
			t_pcb* pcb = queue_pop(new_queue);
			pthread_mutex_unlock(&mnew);
			
			while(ejecucion_pausada){ }
			
			if (!strcmp(algoritmo_planificacion(),"FIFO") || !strcmp(algoritmo_planificacion(),"RR"))
			{
				agregar_a_ready(pcb);
			}
			if (!strcmp(algoritmo_planificacion(),"PRIORIDADES")) //to do
			{
				agregar_a_ready_prioridades(pcb);
			}
			log_protegido(string_from_format("PID: %d - Estado Anterior: NEW - Estado Actual: READY\n", pcb->id));
			//sem_post(&ejecutando_largo_plazo);
		}
	}
}

void planificador_a_corto_plazo(){
    
	while(1) {
		while(!ejecucion_pausada){
		//sem_wait(&ejecutando_corto_plazo);

		while (ejecucion_pausada){}

		sem_wait(&procesos_en_ready);
		t_pcb* pcb = quitar_de_ready();
		
		//El proceso ejecutando pasa a ser pcb

		pthread_mutex_lock(&mexec);
    	proceso_exec = pcb;
    	pthread_mutex_unlock(&mexec);

		//Ejecutar proceso
		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EJECUTANDO>", pcb->id));

		enviar_pcb(pcb, socket_dispatch);
		log_protegido(string_from_format("EXECUTE:Proceso enviado a CPU para su ejecucion."));
		volvio_pcb_cpu=false;
 		if (!strcmp(algoritmo_planificacion(),"RR"))
		{
			pthread_t rr;
			pthread_create(&rr, NULL, (void*) contador_round_robin, (void *)pcb->id);
			recibir_pcb_CPU();
			pthread_detach(rr);
		} 
		if (!strcmp(algoritmo_planificacion(),"FIFO") || !strcmp(algoritmo_planificacion(),"PRIORIDADES"))
		{
			recibir_pcb_CPU();
		}
		
		//sem_post(&ejecutando_corto_plazo);
    }
	}
}


void contador_round_robin(int pcb_id){
	usleep(quantum()*1000);
	if(!volvio_pcb_cpu && proceso_exec->id==pcb_id){
	desalojar_proceso(RR);
	log_protegido(string_from_format("PID: <%d> - Desalojado por fin de Quantum",pcb_id));
	} 
} 


void recibir_pcb_CPU(){
		int motivo;
		instruccion* info_instruccion=malloc(sizeof(instruccion));
		info_instruccion->parametros =list_create();
		// char* mensaje_truncate;
		recibir_contexto_ejecucion_de_cpu(proceso_exec, &motivo, info_instruccion);//, info_instruccion);
		// Actualizar pcb se hace dentro de recibir contexto
		volvio_pcb_cpu=true;
		log_protegido(string_from_format("EXECUTE:Proceso recibido de CPU."));
		// Acciones por si el proceso finalizo, va a bloqueado o vuelve a ready

		while(ejecucion_pausada){ }

		if (proceso_exec!=NULL)	{
		switch (motivo)
		{
		case RR:
		case PRIORIDADES:
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
			vuelve_a_ready(proceso_exec);
			proceso_exec=NULL;
			liberar_info_instruccion(info_instruccion);
		break;
		case FINALIZAR_PROCESO:
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <EXIT>", proceso_exec->id));
			finalizar_proceso(proceso_exec,motivo);
			liberar_info_instruccion(info_instruccion);
			proceso_exec=NULL;
		break;
		case WAIT:
			char *nombre_recurso_wait = list_get(info_instruccion->parametros, 0); // list_get(inst_wait->parametros,0);
			t_recurso *recurso_wait;
			t_recurso *recurso_del_proceso_;
			if ((recurso_wait = existe_el_recurso(nombre_recurso_wait, recursos)))
			{
				if (recurso_wait->instancias <= 0){
					queue_push(recurso_wait->bloqueados, proceso_exec);
					pthread_mutex_lock(&mbloqueados);
					list_add(bloqueados,proceso_exec);
					pthread_mutex_unlock(&mbloqueados);
					log_protegido(string_from_format("PID: <%d> - BLOQUEADO POR: <%s>", proceso_exec->id, recurso_wait->nombre));
					log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <BLOQUEADO>", proceso_exec->id));
					detectar_deadlock();
				}
				else{
					agregar_recurso_pcb(proceso_exec, recurso_wait);
					recurso_del_proceso_ = existe_el_recurso(nombre_recurso_wait, proceso_exec->recursos_asignados);
					log_protegido(string_from_format("PID: <%d> - Wait: <%s> - Instancias: <%d>", proceso_exec->id, recurso_del_proceso_->nombre, recurso_del_proceso_->instancias));
					log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
					// agregar_a_ready(proceso_exec);
					vuelve_a_ready(proceso_exec);
				}
				recurso_wait->instancias--;
			}
			else{
				log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <EXIT>", proceso_exec->id));
				finalizar_proceso(proceso_exec, INVALID_RESOURCE);
			}
			liberar_info_instruccion(info_instruccion);
			proceso_exec = NULL;
		break;
		case SIGNAL:
			char *nombre_recurso_signal = list_get(info_instruccion->parametros, 0);
			t_recurso *recurso_signal;
			t_recurso *recurso_del_proceso;
			if ((recurso_signal = existe_el_recurso(nombre_recurso_signal, recursos)))
			{
				if ((recurso_del_proceso = existe_el_recurso(nombre_recurso_signal, proceso_exec->recursos_asignados))){
					quitar_recurso_pcb(proceso_exec, recurso_del_proceso);
				}
				else{
					log_protegido(string_from_format("El proceso PID: <%d> agrego una instancia del recurso <%s>", proceso_exec->id, recurso_signal->nombre));
					log_protegido(string_from_format("PID: <%d> - Signal: <%s> - Instancias: <0>", proceso_exec->id, recurso_signal->nombre));
				}
				if (recurso_signal->instancias < 0){
					t_pcb *pcb_bloqueada = queue_pop(recurso_signal->bloqueados);
					pthread_mutex_lock(&mbloqueados);
					list_remove_element(bloqueados, pcb_bloqueada);
					pthread_mutex_unlock(&mbloqueados);
					agregar_recurso_pcb(pcb_bloqueada, recurso_signal);
					// agregar_a_ready(pcb_bloqueada);
					vuelve_a_ready(pcb_bloqueada);
					log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_bloqueada->id));
				}
				log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
				// agregar_a_ready(proceso_exec);
				vuelve_a_ready(proceso_exec);
				recurso_signal->instancias++;
			}
			else{
				finalizar_proceso(proceso_exec, INVALID_RESOURCE);
			}
			liberar_info_instruccion(info_instruccion);
			proceso_exec=NULL;
		break;
		case SLEEP:
			t_info_bloqueado *info_sleep = malloc(sizeof(t_info_bloqueado)); // o malloc(sizeof(t_info_io*))?
			info_sleep->pcb = proceso_exec;
			info_sleep->info_instruccion = info_instruccion;
			info_sleep->motivo = SLEEP;
			log_protegido(string_from_format("PID: <%d> - Bloqueado por: <SLEEP>", info_sleep->pcb->id));
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <BLOQUEADO>", info_sleep->pcb->id));
			pthread_mutex_lock(&mbloqueados);
			list_add(bloqueados,proceso_exec);
			pthread_mutex_unlock(&mbloqueados);
			/*pthread_mutex_lock(&mbloqueado);
			queue_push(block_queue, info_sleep);
			pthread_mutex_unlock(&mbloqueado);
			sem_post(&bloqueado); */
			pthread_t hilo_de_sleep;
			pthread_create(&hilo_de_sleep, NULL, (void *)hilo_sleep, (void *)info_sleep);
			pthread_detach(hilo_de_sleep);

			proceso_exec=NULL;
		break;
		case EXIT:
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <EXIT>", proceso_exec->id));
			finalizar_proceso(proceso_exec,SUCCESS);
			liberar_info_instruccion(info_instruccion);
			proceso_exec=NULL;
		break;
		case F_OPEN:
			char *nombre_archivo = list_get(info_instruccion->parametros, 0);
			t_archivo_global *archivo_abrir = buscar_archivo_abierto(list_get(info_instruccion->parametros, 0));
			t_archivo* archivo = malloc(sizeof(t_archivo));
			archivo->pid=proceso_exec->id;
			archivo->nombre= malloc(strlen(nombre_archivo)+1);
			strcpy(archivo->nombre,nombre_archivo); //tiene que hacerse malloc??
			archivo->puntero=0;

			if (archivo_abrir != NULL) //el archivo ya esta abierto por otro proceso
			{
				archivo->entrada_tdp_global=archivo_abrir;
				if(!strcmp(list_get(info_instruccion->parametros, 1),"W")){
					archivo->modo_apertura='W';
					list_add(archivo_abrir->bloqueados, archivo);
					pthread_mutex_lock(&mbloqueados);
					list_add(bloqueados,proceso_exec);
					pthread_mutex_unlock(&mbloqueados);
					log_protegido(string_from_format("PID: %d - Bloqueado por: %s", proceso_exec->id, archivo_abrir->nombre));
					log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <BLOQUEADO>", proceso_exec->id));
					detectar_deadlock();
					proceso_exec = NULL;
				}
				else{
					if(archivo_abrir->escritura==true){
						archivo->modo_apertura='R';
						list_add(archivo_abrir->bloqueados, archivo);

						pthread_mutex_lock(&mbloqueados);
						list_add(bloqueados,proceso_exec);
						pthread_mutex_unlock(&mbloqueados);
						
						log_protegido(string_from_format("PID: %d - Bloqueado por: %s", proceso_exec->id, archivo_abrir->nombre));
						detectar_deadlock();
						log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <BLOQUEADO>", proceso_exec->id));
						proceso_exec = NULL;
					}
					else{
						abrir_archivo(archivo);
						agregar_archivo_al_proceso(archivo, proceso_exec);
						archivo_abrir->aperturas++;
						// agregar_a_ready(proceso_exec);
						vuelve_a_ready(proceso_exec);
						log_protegido(string_from_format("PID: <%d> - Abrir Archivo: <%s>", proceso_exec->id, nombre_archivo));
						log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
					}
				}
			}
			else// el archivo no estaba abierto por otro proceso
			{	
				int tamanio_archivo=abrir_archivo(archivo);
				int tamanio_nombre_open=strlen(nombre_archivo);
				
				t_archivo_global* archivo_global = (t_archivo_global*)malloc(sizeof(t_archivo_global));
				archivo_global->nombre = malloc(tamanio_nombre_open+1);
				strcpy(archivo_global->nombre,nombre_archivo);
				archivo_global->aperturas = 1;
				archivo_global->bloqueados=list_create();
				archivo_global->tamanio = tamanio_archivo;
				list_add(tabla_archivos_abiertos, archivo_global);
				char* modo_de_apertura= list_get(info_instruccion->parametros, 1);
				if(!strcmp(modo_de_apertura,"W")){
					archivo->modo_apertura = 'W';
					archivo_global->escritura=true;
				}
				else{
					archivo->modo_apertura = 'R';
					archivo_global->escritura=false;
				}
				archivo->entrada_tdp_global=archivo_global;
				agregar_archivo_al_proceso(archivo, proceso_exec);
				// agregar_a_ready(proceso_exec);
				vuelve_a_ready(proceso_exec);
				log_protegido(string_from_format("PID: <%d> - Abrir Archivo: <%s>", proceso_exec->id, nombre_archivo));
				log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
			}

			liberar_info_instruccion(info_instruccion);
			proceso_exec=NULL;
		break;
		case F_CLOSE:
			char *archivo_close = list_get(info_instruccion->parametros, 0);
			cerrar_archivo(archivo_close, proceso_exec);

			log_protegido(string_from_format("PID: <%d> - Cerrar Archivo: <%s>", proceso_exec->id, archivo_close));
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
			// agregar_a_ready(proceso_exec);
			vuelve_a_ready(proceso_exec);
			liberar_info_instruccion(info_instruccion);
			proceso_exec=NULL;
		break;
		case F_WRITE:
		char* archivo_write = list_get(info_instruccion->parametros, 0);
		t_archivo* archivo_del_proceso = buscar_archivo_en_el_proceso(archivo_write, proceso_exec);

		if(archivo_del_proceso->modo_apertura == 'W'){

			t_info_bloqueado *info_fs_w =  malloc(sizeof(t_info_bloqueado)); 
			info_fs_w->pcb = proceso_exec;
			info_fs_w->info_instruccion = info_instruccion;
			info_fs_w->motivo = F_WRITE;

			pthread_mutex_lock(&mbloqueados);
			list_add(bloqueados,proceso_exec);
			pthread_mutex_unlock(&mbloqueados);

			pthread_t hilo_de_espera_fs;
			pthread_create(&hilo_de_espera_fs, NULL, (void *)hilo_espera_fs, (void *)info_fs_w);
			pthread_detach(hilo_de_espera_fs);
		}
		else{
			finalizar_proceso(proceso_exec, INVALID_WRITE);
			liberar_info_instruccion(info_instruccion);
		}
		proceso_exec=NULL;
		
		break;
		case F_READ:
			t_info_bloqueado* info_fs_r = malloc(sizeof(t_info_bloqueado)); 
			info_fs_r->pcb = proceso_exec;
			info_fs_r->info_instruccion = info_instruccion;
			info_fs_r->motivo = F_READ;

			pthread_mutex_lock(&mbloqueados);
			list_add(bloqueados,proceso_exec);
			pthread_mutex_unlock(&mbloqueados);

			pthread_t hilo_de_espera_fs;
			pthread_create(&hilo_de_espera_fs, NULL, (void *)hilo_espera_fs, (void *)info_fs_r);
			pthread_detach(hilo_de_espera_fs);
			proceso_exec=NULL;
		
		break;
		case F_SEEK:
			char *archivo_seek = list_get(info_instruccion->parametros, 0);
			char *posicion_seek = list_get(info_instruccion->parametros, 1);
			uint32_t puntero_seek = (uint32_t)atoi(posicion_seek);

			t_archivo *archivo_de_tabla = get_archivo_proceso(archivo_seek, proceso_exec);
			archivo_de_tabla->puntero = puntero_seek;

			log_protegido(string_from_format("PID: <%d> - Actualizar puntero Archivo: <%s> - Puntero <%d>", proceso_exec->id, archivo_seek, puntero_seek));
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <READY>", proceso_exec->id));
			// agregar_a_ready(proceso_exec);
			pthread_mutex_lock(&mready);
			list_add_in_index(ready,0,proceso_exec);
			pthread_mutex_unlock(&mready);
			sem_post(&procesos_en_ready);

		break;
		case F_TRUNCATE:
			t_info_bloqueado* info_fs = malloc(sizeof(t_info_bloqueado));
			info_fs->pcb = proceso_exec;
			info_fs->info_instruccion = info_instruccion;
			info_fs->motivo = F_TRUNCATE;

			// Agregar un hilo que espera a que termine y sigue procesando
			// SE VA A BLOQUEAR EL PROCESO

			pthread_mutex_lock(&mbloqueados);
			list_add(bloqueados,proceso_exec);
			pthread_mutex_unlock(&mbloqueados);
			
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <BLOQUEADO>", proceso_exec->id));

			pthread_t hilo_de_espera_truncate;
			pthread_create(&hilo_de_espera_truncate, NULL, (void *)hilo_espera_fs, (void *)info_fs);
			pthread_detach(hilo_de_espera_truncate);

			proceso_exec=NULL;

		break;
		case PAGE_FAULT:
			t_info_bloqueado* info_pf = malloc(sizeof(t_info_bloqueado)); // o malloc(sizeof(t_info_io*))?
			info_pf->pcb = proceso_exec;
			info_pf->info_instruccion = info_instruccion;
			info_pf->motivo = PAGE_FAULT;

			pthread_mutex_lock(&mbloqueados);
			list_add(bloqueados,proceso_exec);
			pthread_mutex_unlock(&mbloqueados);
			
			int nro_pag = atoi(list_get(info_instruccion->parametros,2));
			log_protegido(string_from_format("Page Fault PID: <%d> - Pagina: <%d>", proceso_exec->id, nro_pag));
			log_protegido(string_from_format("PID: <%d> - Estado Anterior: <EJECUTANDO> - Estado Actual: <BLOQUEADO>", proceso_exec->id));
			
			pthread_t hilo_de_pf;
			pthread_create(&hilo_de_pf, NULL, (void *)hilo_pf, (void *)info_pf);
			pthread_detach(hilo_de_pf);
			proceso_exec=NULL;
		
		break;
		default:
		log_protegido(string_from_format("OPERACION DESCONOCIDA"));
		liberar_info_instruccion(info_instruccion);
		break;
		}
		}
		else liberar_info_instruccion(info_instruccion);
}

int abrir_archivo(t_archivo* archivo){

	t_paquete* abrir = crear_paquete(F_OPEN);
	int cantcar = strlen(archivo->nombre);
	agregar_a_paquete(abrir, archivo->nombre, cantcar+1);

	pthread_mutex_lock(&sem_fileSystem);
	enviar_paquete(abrir, socket_FS);
	eliminar_paquete(abrir);
	recibir_operacion(socket_FS);
	int size = 0;
	void* buffer_abrir = recibir_buffer(&size,socket_FS);
	pthread_mutex_unlock(&sem_fileSystem);
	int tamanio_archivo;
	memcpy(&tamanio_archivo,buffer_abrir,sizeof(int));
	free(buffer_abrir);

	return tamanio_archivo;
}


//Saca el primer pcb de la cola de ready
t_pcb* quitar_de_ready(){
    pthread_mutex_lock(&mready);
	t_pcb* pcb = list_remove(ready, 0);
    pthread_mutex_unlock(&mready);
  return pcb;
}

void vuelve_a_ready(t_pcb* pcb){
	if (!strcmp(algoritmo_planificacion(),"FIFO") || !strcmp(algoritmo_planificacion(),"RR"))
	{
		agregar_a_ready(pcb);
	}
	if (!strcmp(algoritmo_planificacion(),"PRIORIDADES")) //to do
	{
		agregar_a_ready_prioridades(pcb);
	}
}

void agregar_a_ready(t_pcb* pcb){
    temporal_gettime(pcb->tiempo_llegada);
	pthread_mutex_lock(&mready);
    list_add(ready, pcb);
    pthread_mutex_unlock(&mready);

	loggear_pids_ready();
    sem_post(&procesos_en_ready);
}

void agregar_a_ready_prioridades(t_pcb* pcb){
	if (proceso_exec)
	{
		desalojar_proceso(PRIORIDADES);
	}
	pthread_mutex_lock(&mready);
    list_add(ready, pcb);
    pthread_mutex_unlock(&mready);

	reordenar_ready();//reordenar de acuerdo a las prioridades y el tiempo de llegada

	loggear_pids_ready();
    sem_post(&procesos_en_ready);
}

void finalizar_proceso(t_pcb *pcb, int motivo)
{
	int pid_proceso = pcb->id;
	pthread_mutex_lock(&mexit);
	queue_push(exit_queue, pcb);
	pthread_mutex_unlock(&mexit);
	sem_post(&procesos_en_exit);
	log_protegido(string_from_format("Finaliza el proceso <%d>- Motivo: <%s>", pid_proceso, get_motivo(motivo)));
	sem_post(&nivel_multiprogramacion);
	grado_multiprogramacion++;
}

void hilo_exit()
{
	while(1)
	{
		sem_wait(&procesos_en_exit);
		/*liberar todos los recursos que tenga asignados y dar aviso al modulo Memoria para que este libere sus estructuras.*/
		pthread_mutex_lock(&mexit);
		t_pcb *pcb = queue_pop(exit_queue);
		pthread_mutex_unlock(&mexit);
		liberar_recursos(pcb);
		liberar_archivos_abiertos(pcb);
		liberar_memoria(pcb);
		liberar_pcb(pcb);
	}
}

void liberar_pcb(t_pcb* pcb){
	temporal_destroy(pcb->tiempo_llegada);
	//free(&(pcb->registros_cpu));

	int size_recursos=list_size(pcb->recursos_asignados);
	int size_archivos=list_size(pcb->archivos_abiertos);

	for (int i = 0; i < size_recursos; i++)
	{	
		t_recurso* recurso=list_get(pcb->recursos_asignados,i);
		free(recurso->nombre);
		free(recurso);
	}
	for (int i = 0; i < size_archivos; i++)
	{
		t_archivo* archivo=list_get(pcb->archivos_abiertos,i);
		archivo->entrada_tdp_global=NULL;
		free(archivo->nombre);
		free(archivo);
	}
	list_destroy(pcb->recursos_asignados);
	list_destroy(pcb->archivos_abiertos);
	free(pcb);
}


void liberar_archivos_abiertos(t_pcb* pcb){
	t_archivo* archivo_pcb;
	int size_list = list_size(pcb->archivos_abiertos);
	for (int i = size_list-1 ; i >=0 ; i--)
	{
		archivo_pcb = list_get(pcb->archivos_abiertos, i);
		cerrar_archivo(archivo_pcb->nombre, pcb);
	}
}

void liberar_recursos(t_pcb *pcb)
{	
	bool ejecucion_antes=ejecucion_pausada;
	t_recurso *recurso;
	t_recurso *recurso_pcb;
	int size_list = list_size(recursos);
	t_pcb* pcb_bloqueada; //= malloc(sizeof(t_pcb));
	for (int i = 0; i < size_list; i++)
	{
		recurso = list_get(recursos, i);
		// char* nombre = malloc(strlen(recurso->nombre)+1);
		// strcpy(nombre,recurso->nombre);
		if ((recurso_pcb = existe_el_recurso(recurso->nombre, pcb->recursos_asignados)))
		{
			if (recurso->instancias < 0)
			{
				ejecucion_pausada=true;
				for (int j = 0; 0 < recurso_pcb->instancias && recurso->instancias < 0; j++)
				{
					pcb_bloqueada = queue_pop(recurso->bloqueados);
					pthread_mutex_lock(&mbloqueados);
					list_remove_element(bloqueados, pcb_bloqueada);
					pthread_mutex_unlock(&mbloqueados);
					agregar_recurso_pcb(pcb_bloqueada, recurso_pcb);
					// agregar_a_ready(pcb_bloqueada);
					vuelve_a_ready(pcb_bloqueada);

					log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_bloqueada->id));
					recurso->instancias++;
					recurso_pcb->instancias--;
				}
			}
			recurso->instancias += recurso_pcb->instancias;
		}
	}
	ejecucion_pausada=ejecucion_antes;
}

/*------------------------ CONEXIONES ------------------------*/
void recibir_contexto_ejecucion_de_cpu(t_pcb *pcb, int *motivo, instruccion* info_instruccion)
{
	int cod_op=recibir_operacion(socket_dispatch);
	// recv(socket_dispatch, &cod_op, sizeof(int), MSG_WAITALL);
	if (cod_op == CONTEXTO_EJECUCION)
	{
		desempaquetar_contexto_ejecucion_de_cpu(pcb, motivo,info_instruccion);//, info_instruccion);
		//free(buffer);
	}
	else
	printf("CÓDIGO DE OPERACION CPU incorrecto\n");
}

void desempaquetar_contexto_ejecucion_de_cpu(t_pcb *pcb, int* motivo,instruccion* info_instruccion){
    int size;
    void* buffer = recibir_buffer(&size, socket_dispatch);
    int desplazamiento = 0;

	memcpy(&(pcb->program_counter), buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	memcpy(&(pcb->registros_cpu.AX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(pcb->registros_cpu.BX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(pcb->registros_cpu.CX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(pcb->registros_cpu.DX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);

	memcpy(motivo, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	
	memcpy(&(info_instruccion->cant_parametros), buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	for(int i=0; i<info_instruccion->cant_parametros;i++){
		char* parametro;
		int size_parametro;
		memcpy(&(size_parametro), buffer + desplazamiento, sizeof(int));
		desplazamiento += sizeof(int);

		parametro=malloc(size_parametro);

		memcpy(parametro, buffer + desplazamiento, size_parametro);
		desplazamiento += size_parametro;

		list_add(info_instruccion->parametros, parametro);
	}
	//free(parametro);?
	free(buffer);
}	
/*
	FALTA DECIDIR EL TIPO ARCHIVO Y EMPAQUETAR LA LISTA DE ARCHIVOS ABIERTOS
	memcpy(tamania_de_lista_de_archivos_abiertos, buffer + desplazamiento, sizeof(int));
	memcpy(archivos_abiertos, buffer + desplazamiento, sizeof(uint32_t));
*/

/*------------------------ FUNCIONES AUXILIARES ------------------------*/

t_pcb *buscar_pcb(t_list *lista, int pid_buscado)
{
	int elementos = list_size(lista);
	for (int i = 0; i < elementos; i++)
	{
		t_pcb *pcb = list_get(lista, i);
		if (pid_buscado == pcb->id)
		{
			return pcb;
		}
	}
	return NULL;
}

void buscar_y_sacar_pcb_cola(t_recurso* recurso_que_bloquea,int pid_buscado){
	int size_queue = queue_size(recurso_que_bloquea->bloqueados);
	t_pcb* pcb;
	for(int i = 0; i<size_queue;i++){
		pcb = queue_pop(recurso_que_bloquea->bloqueados);
		if(pcb->id != pid_buscado){
			queue_push(recurso_que_bloquea->bloqueados,pcb);
		}
		else{
			recurso_que_bloquea->instancias++;
		}
	}
}

void buscar_y_finalizar_proceso(int pid_buscado,int motivo){
	t_pcb* pcb_fin;
	t_recurso* recurso;
	t_archivo_global* archivo;

	for(int i = 0; i< list_size(recursos);i++){
		recurso = list_get(recursos,i);
		buscar_y_sacar_pcb_cola(recurso,pid_buscado);
	}

	for(int i = 0; i< list_size(tabla_archivos_abiertos);i++){
		archivo = list_get(tabla_archivos_abiertos,i);
		if((pcb_fin = buscar_pcb(archivo->bloqueados,pid_buscado))!= NULL){
			list_remove_element(archivo->bloqueados, pcb_fin);
		}
	}

	if(buscar_pcb(ready,pid_buscado) != NULL){
		pcb_fin = buscar_pcb(ready,pid_buscado);
		pthread_mutex_lock(&mready);
		list_remove_element(ready,pcb_fin);	
		pthread_mutex_unlock(&mready);
		sem_wait(&procesos_en_ready);
		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <READY> - Estado Actual: <EXIT>", pid_buscado));
	}

	if(buscar_pcb(new_queue->elements,pid_buscado)!= NULL){
		pcb_fin = buscar_pcb(new_queue->elements,pid_buscado);
		list_remove_element(new_queue->elements, pcb_fin);
		// sem_wait(&new_disponible); -------------------------------------------------------------------------------------------------------------------(?
		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <NEW> - Estado Actual: <EXIT>", pid_buscado));
	}

 	if(buscar_pcb(bloqueados,pid_buscado)!= NULL){
		pcb_fin = buscar_pcb(bloqueados,pid_buscado);
		pthread_mutex_lock(&mbloqueados);
		list_remove_element(bloqueados, pcb_fin);
		pthread_mutex_unlock(&mbloqueados);
		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <EXIT>", pid_buscado));
	}

	finalizar_proceso(pcb_fin, motivo);
}

/*------------------------ LOGGEAR KERNEL ------------------------*/

/* void loggear_pids_ready(){
	char *pids = NULL;
	if (list_size(ready) >= 1)
	{
		t_pcb *pcb_ready = list_get(ready, 0);
		//pids = (char *)malloc(strlen(string_itoa(pcb_ready->id))+1); //saco el 
		//strcpy(pids, string_itoa(pcb_ready->id));
		pids = strdup(string_itoa(pcb_ready->id));
		for (int i = 1; i < list_size(ready); i++)
		{
			pcb_ready = list_get(ready, i);
			char *pid_i = string_itoa(pcb_ready->id);
			pids = (char *)realloc(pids, strlen(pids) + strlen(pid_i) + 3 ); // +2 de la ", " y +1 de /0
			pids = strcat(pids, ", ");
			strcat(pids, pid_i);
			free(pid_i);
		}
		log_protegido(string_from_format("Cola Ready %s: [%s]", algoritmo_planificacion(), pids));
		free(pids);
	}
} */

void loggear_pids_ready(){
	char* pids = string_new();
	string_append(&pids, "[");    

	for(int i=0; i<list_size(ready);i++){
		t_pcb* pcb = list_get(ready,i);
		char* pid_string = string_itoa(pcb->id);
        string_append(&pids, pid_string);
		if(i!=list_size(ready)-1)string_append(&pids, ",");
		free(pid_string);
	}

	string_append(&pids, "]");
	log_protegido(string_from_format("Cola Ready %s: %s\n", algoritmo_planificacion(),pids));
	free(pids);
}

void loggear_pids_cola(char* nombre_cola){
	char *pids = string_new();
	t_queue* cola=NULL;
	if (!strcmp(nombre_cola,"NEW"))cola=new_queue;
	if (!strcmp(nombre_cola,"EXIT"))cola=exit_queue;
	if(queue_size(cola)>=1){
	for(int i=0; i<queue_size(cola);i++){
		t_pcb* pcb_i = list_get(cola->elements,i);
		char* pid = string_itoa(pcb_i->id);
        string_append(&pids, pid);
		if(i!=queue_size(cola)-1) string_append(&pids, ",");
		//free(pid);
	}
		log_protegido(string_from_format("Estado: %s - Procesos: %s", nombre_cola, pids));
		
	}
	else{
		log_protegido(string_from_format("Estado %s: - Procesos: no hay procesos", nombre_cola));
	}
	free(pids);
}


void loggear_pids_lista(char* nombre_lista){
	t_list* lista=NULL;
	if (!strcmp(nombre_lista,"READY"))lista=ready;
	if (!strcmp(nombre_lista,"BLOCKED"))lista=bloqueados;
	char *pids = string_new();
	if (list_size(lista) >= 1)
	{
		t_pcb *pcb_lista = list_get(lista, 0);
		pids = (char *)malloc(strlen(string_itoa(pcb_lista->id)) + 1);
		strcpy(pids, string_itoa(pcb_lista->id));
		for (int i = 1; i < list_size(lista); i++)
		{
			pcb_lista = list_get(lista, i);
			char *pid_i = string_itoa(pcb_lista->id);
			pids = (char *)realloc(pids, strlen(pids) + strlen(pid_i) + 3 ); // +2 de la ", " y +1 de /0
			pids = strcat(pids, ", ");
			strcat(pids, pid_i);
		}
		log_protegido(string_from_format("Estado %s: - Procesos: %s", nombre_lista, pids));
	}
	else{
		log_protegido(string_from_format("Estado %s: - Procesos: no hay procesos", nombre_lista));
	}		
	free(pids);
}

void listar_procesos_por_estado(){
	loggear_pids_cola("NEW");
	loggear_pids_lista("READY");
	loggear_pids_lista("BLOCKED");
	loggear_pids_cola("EXIT");

}

/*
Listar procesos por estado: 
"Estado: <NOMBRE_ESTADO> - Procesos: <PID_1>, <PID_2>, <PID_N>" (por cada estado)
*/

void liberar_memoria(t_pcb *pcb)
{
	t_paquete *pcb_a_liberar = crear_paquete(LIBERAR_ESPACIO_PROCESO);
	agregar_a_paquete_solo(pcb_a_liberar, &pcb->id, sizeof(int));
	pthread_mutex_lock(&sem_memoria); 
	enviar_paquete(pcb_a_liberar, socket_memoria);
	pthread_mutex_unlock(&sem_memoria); 
	eliminar_paquete(pcb_a_liberar);
	//free(pcb->tabla_paginas);
}

/*------------------------ CONFIGURACION KERNEL ------------------------*/

char* algoritmo_planificacion(){
    return config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION");
}
int grado_multiprogramacion_ini(){
    return atoi(config_get_string_value(config_kernel,"GRADO_MULTIPROGRAMACION_INI"));
}
double quantum(){
    return atoi(config_get_string_value(config_kernel,"QUANTUM"));
}

void crear_colas_de_recursos(){
    char* recursos_char = eliminar_primer_ultimo_caracter(config_get_string_value(config_kernel,"RECURSOS"));
    char* instancias_recursos = eliminar_primer_ultimo_caracter(config_get_string_value(config_kernel,"INSTANCIAS_RECURSOS"));
	//Obtengo los recursos y sus instancias del config
	//INSTANCIAS_RECURSOS=[1, 2] 1,2
    
    char** recursos_array=string_split(recursos_char,","); //Los separo cuando encuentra un ","
    char** instancias_recursos_array=string_split(instancias_recursos,",");
   
    for(int i=0; i< string_array_size(recursos_array);i++){ //recorro el array segun la cant de recursos tenga
        t_recurso* recurso= (t_recurso* )malloc(sizeof(t_recurso));
        //recurso->nombre=recursos_array[i];                  
		recurso->nombre=strdup(recursos_array[i]);			//asigno al recurso el nombre obtenido del config
        char* instancia = instancias_recursos_array[i];     
        recurso->instancias=atoi(instancia);                //asigno al recurso las instancias obtenidas del config
        recurso->bloqueados = queue_create();        
        log_protegido(string_from_format("CREANDO RECURSO: %s de %d instancias\n",recurso->nombre,recurso->instancias));

        list_add(recursos,recurso);   //agrego el recurso a la lista de recursos
    }
    string_array_destroy(recursos_array);
    string_array_destroy(instancias_recursos_array);
}

// int cantidad_recursos(){
// 	char* recursos_char = eliminar_primer_ultimo_caracter(config_get_string_value(config_kernel,"RECURSOS"));
//     char** recursos_array=string_split(recursos_char,",");
//     int cantidad= string_array_size(recursos_array);
// 	free(recursos_array);
// 	return cantidad;
// }

char* eliminar_primer_ultimo_caracter(char *cadena) {
    if (strlen(cadena) <= 2) {
        return NULL;
    }
    memmove(cadena, cadena + 1, strlen(cadena));
    cadena[strlen(cadena) - 1] = '\0';
    return cadena;
}

void desalojar_proceso(int motivo){
	 //(“PID: <PID> - Desalojado por fin de Quantum”)
	t_paquete* paquete_desalojar= crear_paquete(DESALOJAR_PROCESO);
	agregar_a_paquete_solo(paquete_desalojar,&motivo, sizeof(int));
	agregar_a_paquete_solo(paquete_desalojar,&(proceso_exec->id),sizeof(int));
	enviar_paquete(paquete_desalojar,socket_interrupt);
	eliminar_paquete(paquete_desalojar);
}

void pausar_ejecucion(){
/* 	sem_wait(&ejecutando_corto_plazo);
	sem_wait(&ejecutando_largo_plazo); */
	//sem_wait(&ejecutando_exit);
	ejecucion_pausada=true;
	log_protegido(string_from_format(("PAUSA DE PLANIFICACIÓN")));
}

void reanudar_ejecucion(){
	// sem_post(&ejecutando_corto_plazo);
	// sem_post(&ejecutando_largo_plazo);
	//sem_wait(&ejecutando_exit);
	ejecucion_pausada=false;
	log_protegido(string_from_format(("INICIO DE PLANIFICACIÓN")));
}

void reordenar_ready(){
	t_list* aux = list_create();
	int j;
	int prioridad=100;
	int indice=0;
	while(!list_is_empty(ready)){
		j= list_size(ready);
		for(int i=0; i<j; i++){
			t_pcb* aux_pcb= list_get(ready,i);
			if(aux_pcb->prioridad < prioridad){
				indice=i;
				prioridad=aux_pcb->prioridad;
			}
			else{
				if(aux_pcb->prioridad == prioridad){
					t_pcb* aux2=list_get(ready, indice);
					if(temporal_diff((aux_pcb->tiempo_llegada),(aux2->tiempo_llegada))<0){ 
						indice=i;
						prioridad=aux_pcb->prioridad;
					}
				}
			}

		}
		prioridad = 100;
		pthread_mutex_lock(&mready);
		t_pcb* mayor_prioridad= list_remove(ready,indice);
		pthread_mutex_unlock(&mready);
		list_add(aux, mayor_prioridad);
		indice=0;
	}
	for (int i = list_size(aux)-1; i>=0; i--)
	{
		t_pcb* a = list_remove(aux,i);
		list_add_in_index(ready,0,a);
	}
	// ready = aux;
	list_destroy(aux);
}
//tiempo_llegada

void hilo_pf(t_info_bloqueado *info_pf){
	
	int nro_pag = atoi(list_get(info_pf->info_instruccion->parametros,2));
	t_paquete* aviso_pf = crear_paquete(PAGE_FAULT);
	agregar_a_paquete_solo(aviso_pf,&(info_pf->pcb->id), sizeof(int));
	agregar_a_paquete_solo(aviso_pf,&nro_pag, sizeof(int));

	pthread_mutex_lock(&sem_memoria); 

	enviar_paquete(aviso_pf, socket_memoria);
	eliminar_paquete(aviso_pf);

	recibir_operacion(socket_memoria);
	int size;
	char* buffer = recibir_buffer(&size, socket_memoria);

	pthread_mutex_unlock(&sem_memoria); 

	printf("%s\n",buffer);
	if (strcmp(buffer,"OK")==0)
	{	
		pthread_mutex_lock(&mbloqueados);
		list_remove_element(bloqueados, info_pf->pcb);
		pthread_mutex_unlock(&mbloqueados);
		vuelve_a_ready(info_pf->pcb);
	}
	free(buffer);
	liberar_info_instruccion(info_pf->info_instruccion);
	free(info_pf);
}

void hilo_espera_fs(t_info_bloqueado *info_fs){
	int size;
	int dirFisica;
	char* buffer;
	instruccion* inst_fs= info_fs->info_instruccion;
	t_pcb* pcb_fs=info_fs->pcb;
	char* archivo = list_get(inst_fs->parametros, 0);
	int tamanio_nombre_archivo = strlen(archivo);

	t_archivo *archivo_de_tabla = get_archivo_proceso(archivo, pcb_fs);

	switch (info_fs->motivo)
	{
	case F_TRUNCATE:

		t_paquete *f_truncate = crear_paquete(F_TRUNCATE);
		char *tam_truncate = list_get(inst_fs->parametros, 1);
		int cantbytes_truncate = atoi(tam_truncate);
		t_archivo_global* archivo_actualizar_tam = buscar_archivo_abierto(archivo);
		printf("NOMBRE DEL ARCHIVO %s\n", archivo);

		agregar_a_paquete(f_truncate, archivo, tamanio_nombre_archivo+1);
		agregar_a_paquete_solo(f_truncate, &cantbytes_truncate, sizeof(int));

		pthread_mutex_lock(&sem_fileSystem);

		enviar_paquete(f_truncate, socket_FS);
		recibir_operacion(socket_FS);
		buffer = recibir_buffer(&size, socket_FS);

		pthread_mutex_unlock(&sem_fileSystem);
		eliminar_paquete(f_truncate);
		
		archivo_actualizar_tam->tamanio = cantbytes_truncate;

		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_fs->id));
		log_protegido(string_from_format("PID: <%d> - Archivo: <%s> - Tamanio <%d>",pcb_fs->id, archivo, cantbytes_truncate));
		break;
	case F_WRITE: 
		int puntero_w=archivo_de_tabla->puntero;
		int tam_w = atoi(list_get(inst_fs->parametros,3));
		int marco_w = atoi(list_get(inst_fs->parametros, 2));

		t_paquete *f_write = crear_paquete(F_WRITE);
		agregar_a_paquete(f_write, archivo,tamanio_nombre_archivo+1);
		agregar_a_paquete_solo(f_write, &puntero_w,sizeof(int));
		agregar_a_paquete_solo(f_write, &tam_w,sizeof(int));
		agregar_a_paquete_solo(f_write, &marco_w,sizeof(int));
		agregar_a_paquete_solo(f_write, &pcb_fs->id,sizeof(int));

		pthread_mutex_lock(&sem_fileSystem); 

		enviar_paquete(f_write, socket_FS);
		recibir_operacion(socket_FS);
		buffer = recibir_buffer(&size, socket_FS);

		pthread_mutex_unlock(&sem_fileSystem); 

		eliminar_paquete(f_write);
		archivo_de_tabla->puntero += tam_w;

		dirFisica=marco_w*tam_w;
		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_fs->id));
		log_protegido(string_from_format("PID: <%d> - Escribir Archivo: <%s> - Puntero <%d> - Dirección Memoria <%d> - Tamaño <%d>",pcb_fs->id, archivo, puntero_w,dirFisica,tam_w));

		break;

	case F_READ:
		int marco_r = atoi(list_get(inst_fs->parametros, 2));
		int bytesALeer = atoi(list_get(inst_fs->parametros, 3));
		int puntero_r=archivo_de_tabla->puntero;
		
		t_paquete *f_read = crear_paquete(F_READ);
		agregar_a_paquete(f_read, archivo,tamanio_nombre_archivo+1);
		agregar_a_paquete_solo(f_read, &puntero_r, sizeof(int));
		agregar_a_paquete_solo(f_read, &bytesALeer, sizeof(int));
		agregar_a_paquete_solo(f_read, &marco_r, sizeof(int));
		agregar_a_paquete_solo(f_read, &pcb_fs->id, sizeof(int));
		
		pthread_mutex_lock(&sem_fileSystem); 

		enviar_paquete(f_read, socket_FS);
		recibir_operacion(socket_FS);
		buffer = recibir_buffer(&size, socket_FS);

		pthread_mutex_unlock(&sem_fileSystem); 

		eliminar_paquete(f_read);
		//archivo_de_tabla->puntero += bytesALeer;
		dirFisica=marco_r*bytesALeer;
		log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_fs->id));
		log_protegido(string_from_format("PID: <%d> - Leer Archivo: <%s> - Puntero <%d> - Dirección Memoria <%d> - Tamaño <%d>",pcb_fs->id, archivo, puntero_r,dirFisica,bytesALeer));
		break;
	default:
		break;
	}
	pthread_mutex_lock(&mbloqueados);
	list_remove_element(bloqueados,info_fs->pcb);
	pthread_mutex_unlock(&mbloqueados);
	
	vuelve_a_ready(pcb_fs);	

	liberar_info_instruccion(inst_fs);
	free(buffer);
	free(info_fs);
}

void hilo_sleep(t_info_bloqueado *info_sleep)
{
	char *tiempo_char = list_get(info_sleep->info_instruccion->parametros, 0);
	int tiempo = atoi(tiempo_char);
	sleep(tiempo);

	if(buscar_pcb(bloqueados,info_sleep->pcb->id)){ //ver si hay q matar el hilo	
	// agregar_a_ready(info_sleep->pcb);
	vuelve_a_ready(info_sleep->pcb);
	log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", info_sleep->pcb->id));
	pthread_mutex_lock(&mbloqueados);
	list_remove_element(bloqueados,info_sleep->pcb);
	pthread_mutex_unlock(&mbloqueados);
	}
	liberar_info_instruccion(info_sleep->info_instruccion);
	free(info_sleep);
}

t_recurso *existe_el_recurso(char* recurso, t_list *lista_recursos){
	t_recurso *recurso_buscado;
	char* nombre;
	for (int i = 0; i < list_size(lista_recursos); i++)
	{
		recurso_buscado = list_get(lista_recursos, i);
		nombre= recurso_buscado->nombre;
		if (strcmp(recurso,nombre) == 0)
		{
			return recurso_buscado;
		}
	}
	return NULL;
}

void agregar_recurso_pcb(t_pcb *pcb, t_recurso *recurso){
	t_recurso *recurso_a_asignar;
	if ((recurso_a_asignar = existe_el_recurso(recurso->nombre, pcb->recursos_asignados)))
	{
		recurso_a_asignar->instancias++;
	}
	else
	{
		recurso_a_asignar = (t_recurso *)malloc(sizeof(t_recurso));
		recurso_a_asignar->nombre = strdup(recurso->nombre);
		recurso_a_asignar->instancias = 1;
		recurso_a_asignar->bloqueados = NULL;
		list_add(pcb->recursos_asignados, recurso_a_asignar);
	}
}

int buscar_indice(t_list *recursos_, char *nombre){
	t_recurso *recurso_ind;
	int a = -1;
	for (int i = 0; i < list_size(recursos_); i++)
	{
		recurso_ind = list_get(recursos_, i);
		if (recurso_ind->nombre == nombre)
		{
			a = i;
		}
	}
	return a;
}

void quitar_recurso_pcb(t_pcb *pcb, t_recurso *recurso_del_proceso){
	// t_recurso *recurso_asignado = existe_el_recurso(recurso_del_proceso->nombre, pcb->recursos_asignados);
	int indice = buscar_indice(pcb->recursos_asignados, recurso_del_proceso->nombre);
	if (recurso_del_proceso->instancias == 1)
	{
		log_protegido(string_from_format("PID: <%d> - Signal: <%s> - Instancias: <0>", pcb->id, recurso_del_proceso->nombre));
		free(recurso_del_proceso->nombre);
		free(recurso_del_proceso);
		list_remove(pcb->recursos_asignados, indice);
	}
	else
	{
		// recurso_del_proceso = list_get(pcb->recursos_asignados, indice);
		recurso_del_proceso->instancias--;
		log_protegido(string_from_format("PID: <%d> - Signal: <%s> - Instancias: <%d>", pcb->id, recurso_del_proceso->nombre, recurso_del_proceso->instancias));
	}
}


void detectar_deadlock(){
	log_protegido(string_from_format("ANÁLISIS DE DETECCIÓN DE DEADLOCK"));

	hay_deadlock_recursos();
	hay_deadlock_archivos();
}

void hay_deadlock_recursos(){
	t_recurso* recurso; //= malloc(sizeof(t_recurso));
	t_pcb* pcb; //= malloc(sizeof(t_pcb));
	int cant_recursos= list_size(recursos);
	if (list_size(ready) == 0 && list_size(bloqueados) > 1){
		for (int i = 0; i < cant_recursos; i++){
				recurso = list_get(recursos, i);
				int size = queue_size(recurso->bloqueados);
				if (size > 0){
					for (int j = 0; j < size; j++){
						pcb = queue_pop(recurso->bloqueados);
						loggear_deadlock_recursos(pcb, recurso->nombre);
						queue_push(recurso->bloqueados, pcb);
					}
				}
			}
	}
	//free(recurso);
	//free(pcb);
}

void loggear_deadlock_recursos(t_pcb* pcb, char* recurso_necesario){
	int proceso_deadlock=0;
	if(list_size(pcb->recursos_asignados)>0){
		t_recurso* recurso = list_get(pcb->recursos_asignados,0);
		char* nombre = (char *)malloc(strlen(recurso->nombre) + 1);
		strcpy(nombre, recurso->nombre);
		for (int i = 1; i < list_size(pcb->recursos_asignados); i++)
		{
			recurso = list_get(pcb->recursos_asignados, i);
			char* nombrei = recurso->nombre;
			nombre = (char *)realloc(nombre, strlen(nombre) + strlen(nombrei) + 3 ); // +2 de la ", " y +1 de /0
			nombre = strcat(nombre, ", ");
			strcat(nombre, nombrei);
		}
		log_protegido(string_from_format("Deadlock detectado: <%d> - Recursos en posesión <%s> - Recurso requerido: <%s> ",pcb->id, nombre, recurso_necesario));
		free(nombre);
		proceso_deadlock++;
	}else{
		log_protegido(string_from_format("Inanicion detectada: <%d> - Recursos en posesión <> - Recurso requerido: <%s> ",pcb->id,recurso_necesario));
	}
	/* if(proceso_deadlock==0){
		deadlock=false;
		reanudar_ejecucion();
	}
	else{
		if(!ejecucion_pausada)pausar_ejecucion();
		deadlock=true;
	} */

}

void hay_deadlock_archivos(){
	t_archivo_global* archivo; //= malloc(sizeof(t_recurso));
	t_pcb* pcb; // = malloc(sizeof(t_pcb));//VER ESTO, QUE NO SE HAGA FREE DE LA PCB ORIGINAL
	int cant_archivos_abiertos= list_size(tabla_archivos_abiertos);
	if(list_size(ready)==0 && list_size(bloqueados)>1){
		for (int i = 0; i < cant_archivos_abiertos; i++){
			archivo = list_get(tabla_archivos_abiertos,i);
			if(list_size(archivo->bloqueados)>0){
				for(int j = 0; j<list_size(archivo->bloqueados); j++){
					pcb = list_get(archivo->bloqueados,j);
					loggear_deadlock_archivos(pcb, archivo->nombre);
					// printf("El proceso pid <%d> se encuentra en deadlock por un archivo\n", pcb->id);
				}
			}
		}
	}
}

void loggear_deadlock_archivos(t_pcb* pcb, char* archivo_necesario){
	int proceso_deadlock=0;
	if(list_size(pcb->archivos_abiertos)>0){
		t_archivo* archivo = list_get(pcb->recursos_asignados,0);
		char* nombre = (char *)malloc(strlen(archivo->nombre) + 1);
		strcpy(nombre, archivo->nombre);
		for (int i = 1; i < list_size(pcb->archivos_abiertos); i++)
		{
			archivo = list_get(pcb->archivos_abiertos, i);
			char* nombrei = archivo->nombre;
			nombre = (char *)realloc(nombre, strlen(nombre) + strlen(nombrei) + 3 ); // +2 de la ", " y +1 de /0
			nombre = strcat(nombre, ", ");
			strcat(nombre, nombrei);
		}
		log_protegido(string_from_format("Deadlock detectado: <%d> - Archivos abiertos <%s> - Archivo requerido: <%s> ",pcb->id, nombre, archivo_necesario));
		free(nombre);
		proceso_deadlock++;
	}else{
		log_protegido(string_from_format("Inanicion detectada: <%d> - Archivos abiertos <> - Archivo requerido: <%s> ",pcb->id,archivo_necesario));
	}
/* 	if(proceso_deadlock==0){
		deadlock=false;
		reanudar_ejecucion();
	}
	else{
		if(!ejecucion_pausada)pausar_ejecucion();
		deadlock=true;
	} */

}

t_archivo* buscar_archivo_en_el_proceso(char* nombre, t_pcb* pcb){
	for(int i=0; i< list_size(pcb->archivos_abiertos); i++){
		t_archivo* archivo_i = list_get(pcb->archivos_abiertos, i);
		if(strcmp(archivo_i->nombre, nombre) == 0){
			return archivo_i;
		}
	}
	return NULL;
}

t_archivo_global* buscar_archivo_abierto(char* nombre){
	for(int i=0; i< list_size(tabla_archivos_abiertos); i++){
		t_archivo_global* archivo_i = list_get(tabla_archivos_abiertos, i);
		if(strcmp(archivo_i->nombre, nombre) == 0){
			return archivo_i;
		}
	}
	return NULL;
}

void cerrar_archivo(char* nombre_archivo, t_pcb* pcb){
	t_archivo_global* archivo_global;//malloc para que no se borre al hacer free del archivo abierto del proceso
	t_archivo* archivo;

	for(int i=list_size(pcb->archivos_abiertos)-1; i>=0 ;i--){
		archivo = list_get(pcb->archivos_abiertos, i);
		char* nombre_i = archivo->nombre;
		if(strcmp(nombre_archivo,nombre_i)==0){
 			archivo_global=archivo->entrada_tdp_global;
			archivo->entrada_tdp_global=NULL;
			free(list_remove(pcb->archivos_abiertos, i)); //free?
		}
	}
	if(list_size(archivo_global->bloqueados)>0){
		if(archivo_global->escritura==true){
			archivo_global->aperturas=0;
			t_archivo* primer_archivo = list_get(archivo_global->bloqueados, 0);
			if(primer_archivo->modo_apertura == 'W'){
				t_pcb* pcb_desbloquear = buscar_pcb(bloqueados,primer_archivo->pid);
				agregar_archivo_al_proceso(primer_archivo, pcb_desbloquear);
				vuelve_a_ready(pcb_desbloquear);
				log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_desbloquear->id));
				pthread_mutex_lock(&mbloqueados);
				list_remove_element(bloqueados, pcb_desbloquear);
				pthread_mutex_unlock(&mbloqueados);
				list_remove(archivo_global->bloqueados, 0);
			}
			else{
				archivo_global->escritura=false;
				for(int i=list_size(archivo_global->bloqueados)-1; i>=0; i--){
					archivo = list_get(archivo_global->bloqueados, i);
					printf("modo de apertura %d\n", archivo->modo_apertura);
					if(archivo->modo_apertura == 'R'){
						t_pcb* pcb_desbloquear = buscar_pcb(bloqueados,archivo->pid);
						agregar_archivo_al_proceso(archivo, pcb_desbloquear);
						vuelve_a_ready(pcb_desbloquear);
						log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_desbloquear->id));
						list_remove(archivo_global->bloqueados, i);
						pthread_mutex_lock(&mbloqueados);
						list_remove_element(bloqueados, pcb_desbloquear);
						pthread_mutex_unlock(&mbloqueados);
						archivo_global->aperturas++;
						
					}
				}
			}
		}
		else{
			if(archivo_global->aperturas>1){
				archivo_global->aperturas--;
			}
			else{
				t_archivo* primer_archivo = list_get(archivo_global->bloqueados, 0);
				t_pcb* pcb_desbloquear = buscar_pcb(bloqueados,primer_archivo->pid);
				agregar_archivo_al_proceso(primer_archivo, pcb_desbloquear);
				// agregar_a_ready(pcb_desbloquear);
				vuelve_a_ready(pcb_desbloquear);
				log_protegido(string_from_format("PID: <%d> - Estado Anterior: <BLOQUEADO> - Estado Actual: <READY>", pcb_desbloquear->id));
				pthread_mutex_lock(&mbloqueados);
				list_remove_element(bloqueados, pcb_desbloquear);
				pthread_mutex_unlock(&mbloqueados);
				list_remove(archivo_global->bloqueados, 0);
				archivo_global->aperturas=1;
				archivo_global->escritura=true;
			}
		}
	}
	else{
		list_destroy(archivo_global->bloqueados);
		list_remove_element(tabla_archivos_abiertos, archivo_global);
		free(archivo_global);
	}
}

void agregar_archivo_al_proceso(t_archivo* archivo, t_pcb* pcb){
	list_add(pcb->archivos_abiertos,archivo);
}

t_archivo* get_archivo_proceso(char* nombre_archivo, t_pcb* pcb){
	for(int i=0; i< list_size(pcb->archivos_abiertos);i++){
		t_archivo* archivo = list_get(pcb->archivos_abiertos, i);
		char* nombre_i = archivo->nombre;
		if(strcmp(nombre_archivo,nombre_i)==0){
			return archivo;
		}
	}
	return NULL;
}

void liberar_info_instruccion(instruccion* info_instruccion){
	
	for (int i = 0; i < list_size(info_instruccion->parametros); i++)
	{
		free(list_get(info_instruccion->parametros,i));
	}
	list_destroy(info_instruccion->parametros);
	//info_instruccion->pcb=NULL;(?
	free(info_instruccion);
}

void cambiar_multiprogramacion(int nuevo_gm){
	if(nuevo_gm < grado_multiprogramacion){
		for(int i = nuevo_gm; i < grado_multiprogramacion; i++){
			sem_wait(&nivel_multiprogramacion);
		}
	}
	else{
		for(int i = nuevo_gm; i > grado_multiprogramacion; i--){
			sem_post(&nivel_multiprogramacion);
		}
	}
	grado_multiprogramacion=nuevo_gm;
}
