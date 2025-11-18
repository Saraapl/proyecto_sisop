#ifndef PROCESOPAR_H
#define PROCESOPAR_H

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tipo de estado
typedef unsigned int Estado_t;

// Valores de retorno - Bit encoding
#define E_OK                (0x00)           // 0000 0000
#define E_PAR_INC           (1 << 0)         // 0000 0001
#define E_CREAR_PIPE        (1 << 1)         // 0000 0010
#define E_CREAR_PROCESO     (1 << 2)         // 0000 0100
#define E_PROCESO_NULO      (1 << 3)         // 0000 1000
#define E_ENVIO_FALLIDO     (1 << 4)         // 0001 0000
#define E_THREAD_FALLIDO    (1 << 5)         // 0010 0000
#define E_PROCESO_INACTIVO  (1 << 6)         // 0100 0000
#define E_ESCRITURA         (1 << 7)         // 1000 0000
#define E_LECTURA           (1 << 8)         // 1 0000 0000
#define E_MEMORIA           (1 << 9)         // 10 0000 0000

// Macro para verificar si un flag de error está establecido
#define TIENE_ERROR(estado, error) ((estado) & (error))

// Estructura del proceso par
typedef struct ProcesoPar {
    PROCESS_INFORMATION processInfo;
    HANDLE hInputWrite;
    HANDLE hOutputRead;
    HANDLE hListenerThread;
    CRITICAL_SECTION criticalSection;
    int activo;
    Estado_t (*funcionEscucha)(const char*, int);
} ProcesoPar_t;

// Prototipos de funciones
Estado_t lanzarProcesoPar(const char* nombreArchivoEjecutable,
                         const char** listaLineaComando,
                         ProcesoPar_t** procesoPar);

Estado_t destruirProcesoPar(ProcesoPar_t* procesoPar);

Estado_t enviarMensajeProcesoPar(ProcesoPar_t* procesoPar,
                                const char* mensaje,
                                int longitud);

Estado_t establecerFuncionDeEscucha(ProcesoPar_t* procesoPar,
                                   Estado_t (*f)(const char*, int));

void imprimirEstado(const char* operacion, Estado_t estado);
const char* obtenerDescripcionError(Estado_t estado);

#endif