#include <stdio.h>
#include "mmu.h"

#define RESIDENTSETSIZE 24

extern char *base;
extern int framesbegin;
extern int idproc;
extern int nframes;
extern int pagesperproc;

extern struct FRAMETABLE *frametable;
extern struct PAGETABLE pagetable[];

int pagefault(char *vaddress);
int getfreeframe();
int getLRU();
void execSwap(int newPageIndex);
int getFreeSwapAddress();
int hasFisicalAddress(int pageIndex);
void write(struct FRAMETABLE *frame, int address);

/**
* @method pagefault Rutina de fallos de página
* @return failure/success
*/
int pagefault(char *vaddress)
{
    int pageIndex     = (int) vaddress >> 12; // page index in pagetable

    // mientras haya menos de 2 paginas por proceso en la tabla de marcos, asignale nueva
    if(countframesassigned()< FRAMESPERPROC)
        pagetable[pageIndex].framenumber = getfreeframe();

    // si la página que se pide no tiene una posición en la parte alta de la tabla de marcos,
    // ejecuta el intercambio de ésta con alguna vieja
    if(!hasFisicalAddress(pageIndex))
        execSwap(pageIndex);

    // marca la nueva como presente
    pagetable[pageIndex].presente    = TRUE;

    return SUCCESS;
}

/**
* @method getfreeframe
* @return index of new assigned frame
*/
int getfreeframe()
{
    int frame;
    for( frame=framesbegin ;frame<framesbegin+nframes ; frame++)
      if(!frametable[frame].assigned)
      {
        frametable[frame].assigned = TRUE;
        return frame;
      }

    return NINGUNO;
}

/**
* @method execSwap
* @return index of new assigned frame
*/
void execSwap(int newPageIndex)
{
    FILE   *swapFile = fopen("swap","rb");

    // 1 nos aseguramos de tener los indices basicos:
    /*
    * newPageIndex --indice en pagetable de la pagina nueva
    * oldPageIndex --indice en pagetable de la pagina a desplazar
    * addressUp    --direccion fisica cuyo contendio cambiara de pagina vieja <-a nueva
    * addressDown  --direccion fisica cuyo contendio cambiara de pagina nueva <-a vieja
    */
    int oldPageIndex   = getLRU();//indice de la pagina removida

    // si la página es nueva, busca un índice en swap libre, debajo del márgen de la memoria física
    if (pagetable[newPageIndex].framenumber == NINGUNO)
        pagetable[newPageIndex].framenumber = getFreeSwapAddress();

    // almacenas las direcciones relativas de la tabla de marcos fisicos
    int addressUp= pagetable[oldPageIndex].framenumber;
    int addressDown= pagetable[newPageIndex].framenumber;

    // 2 leemos contenidos de swap
    struct FRAMETABLE swapContentUp; // viejo
    struct FRAMETABLE swapContentDown; // nuevo

    fseek(swapFile, addressUp-framesbegin,SEEK_SET);
    fread(&swapContentUp, sizeof(struct FRAMETABLE), 1 ,swapFile);
    fseek(swapFile, addressDown-framesbegin,SEEK_SET);
    fread(&swapContentDown, sizeof(struct FRAMETABLE), 1 ,swapFile);

    // 3 guardamos contenido bajo
    // se escribe el marco viejo en la direccion baja de swap;
    // si se modificó viene de la talba de marcos, 
    // de lo contrario se obtiene del mismo swap
    if(pagetable[oldPageIndex].modificado)
        write(&frametable[addressUp], addressDown);
    else
        write(&swapContentUp, addressDown);

    // actualiza las propiedades de la página vieja 
    // en la tabla de páginas del proceso
    pagetable[oldPageIndex].presente    = FALSE;
    pagetable[oldPageIndex].modificado  = FALSE;

    // 4 guardamos contenido alto
    // ahora se escribe el marco nuevo en la parte alta
    fflush(stdout);
    //if(swapContentDown.shmidframe < 0)
    //    {
    //      frametable[addressUp]= swapContentDown;
    //      printf("Prueba de memoria compratida");
    //    }
    //else
    write(&swapContentDown, addressUp);

    // 5 actualizamos indices
    // se actualizan los apuntadores de las páginas del proceso
    pagetable[newPageIndex].framenumber = addressUp;
    pagetable[oldPageIndex].framenumber = addressDown;

    fclose(swapFile);
}

void write(struct FRAMETABLE *frame, int address)
{
    FILE   *swapFile = fopen("swap","wb");
    fseek(swapFile, address-framesbegin,SEEK_SET);
    fwrite(frame, sizeof(struct FRAMETABLE), 1, swapFile);
    fclose(swapFile);
}

/**
* @method getFreeSwapAddress
* @return address of free space
*/
int getFreeSwapAddress()
{
    FILE   *swapFile = fopen("swap","r+b");
    struct  FRAMETABLE swapBuffer;
    int     freeAddress;

    // se coloca en la parte baja de la memoria (frame 9-16)
    fseek(swapFile, MEMSIZE,SEEK_SET);

    // itera desde la posicion del proceso en el swap: cada proceso tiene asignados
    // tantos frames como páginas sobren de la tabla de marcos físicos (en este caso, 2 páginas de sobra)
    for(freeAddress = MEMSIZE + (FRAMESIZE * (PAGESPERPROC - FRAMESPERPROC) * idproc); 
        freeAddress < SWAPSIZE; 
        freeAddress += FRAMESIZE)
    {
        fread(&swapBuffer, sizeof(struct FRAMETABLE), 1 ,swapFile);
        //printf("\nP%d iteracion:%d leemos: %d\n------------------\n------------------\n",idproc,freeAddress,swapBuffer.assigned);
        if(swapBuffer.assigned==0)
        {
                swapBuffer.assigned=1;
                fseek(swapFile, -sizeof(struct FRAMETABLE),SEEK_CUR);//se regresa y prende bit de assigned
                fwrite(&swapBuffer, sizeof(struct FRAMETABLE), 1, swapFile);
                break;
        }
    }
    fclose(swapFile);
    //printf("\nP%d otorgarmos el libre: %d\n------------------\n------------------\n",idproc,freeAddress);

    if(freeAddress > SWAPSIZE)
        return ERROR;
    else
        // sumamos el offset de la direccion de la tabla de marcos fisica.
        // este offset debe ser restado al manejar direcciones de I/O del swap
        return freeAddress+framesbegin; 
}

/**
* @method getLRU
* @return Last Recently Used index from page table
*/
int getLRU()
{
    int i=0;
    int index=0;

    for(i = 0; i < PAGESPERPROC; i++)
      if(pagetable[i].tlastaccess < pagetable[index].tlastaccess &&
          hasFisicalAddress(i))
            index=i;

    return index;
}

int hasFisicalAddress(int pageIndex)
{
  return pagetable[pageIndex].framenumber >= framesbegin
      && pagetable[pageIndex].framenumber < MEMSIZE+framesbegin;
}
