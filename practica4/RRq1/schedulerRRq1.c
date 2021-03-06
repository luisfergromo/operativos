#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "virtual_processor.h"

extern struct PROCESO proceso[];
extern struct COLAPROC listos,bloqueados;
extern int tiempo;
extern int pars[];

#define TRUE 1
#define FALSE 0

// =============================================================================
// ESTE ES EL SCHEDULER
// ============================================================================

int scheduler(int evento)
{
    printf("    <Scheduler>\n");
    int cambia_proceso = FALSE; // bandera de cambio de proceso
    int prox_proceso_a_ejecutar = pars[1]; // pid del proximo proceso a ejecutar

    switch(evento)
    {
      case 0:
        printf("        EVENTO: TIMER, ejecucion actual: %d\n",pars[0]);

        if(cola_vacia(listos))
          printf("        - Cola vacia\n");
        else
        {
            printf("        - Cola no vacia\n");
            // lo que estaba en ejecucion se mete a listos
            proceso[pars[1]].estado = LISTO;
            mete_a_cola(&listos,pars[1]);
            // y se hace pop
            cambia_proceso = TRUE;
        }

        break;

      ///////////////////////////////////////////////
      case 1:
        printf("        EVENTO: SOLICITA_E_S, ejecucion actual: %d\n",pars[0]);

        // se mete a la cola de bloqueados y se hace pop
        proceso[pars[1]].estado = BLOQUEADO;
        mete_a_cola(&bloqueados,pars[1]);
        cambia_proceso = TRUE;

        break;

      ///////////////////////////////////////////////
      case 2:
        printf("        EVENTO: TERMINA_E_S, ejecucion actual: %d\n",pars[0]);

        // se desbloquea el proceso y se mete a la cola
        proceso[pars[0]].estado = LISTO;
        mete_a_cola(&listos,sacar_de_cola(&bloqueados));

        break;

      ///////////////////////////////////////////////
      case 3:
        printf("        EVENTO: PROCESO_NUEVO, ejecucion actual: %d\n",pars[0]);
        //if(!pars[1]== -1){
          proceso[pars[0]].estado = LISTO;// Agregar el nuevo proceso a la cola de listos
          mete_a_cola(&listos,pars[0]);
          if(tiempo==0)
            cambia_proceso = TRUE;
        //}

        break;

      ///////////////////////////////////////////////
      case 4:
        printf("        EVENTO: PROCESO_TERMINADO, ejecucion actual: %d\n",pars[0]);

        proceso[pars[0]].estado = TERMINADO;
        cambia_proceso = TRUE; // se pide cambio de proceso

        break;

    }


    if(cambia_proceso)
    {
        printf("        - Se solicita cambio de proceso\n");

        // Si la cola no esta vacia obtener de la cola el siguiente proceso listo
        if(!cola_vacia(listos))
        {
            prox_proceso_a_ejecutar=sacar_de_cola(&listos);
            proceso[prox_proceso_a_ejecutar].estado=EJECUCION;
            cambia_proceso=0;
        }
        else
        {
            printf("        - no hay listos\n");
            prox_proceso_a_ejecutar=NINGUNO;
        }
    }

    printf("        PROCESO siguiente: %d\n",prox_proceso_a_ejecutar);
    printf("    </Scheduler>\n");
    return(prox_proceso_a_ejecutar);
}

// =================================================================
