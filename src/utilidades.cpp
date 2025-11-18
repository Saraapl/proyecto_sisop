#include "ProcesoPar.h"

// Estructura para mapear bits de error a sus descripciones
typedef struct {
    Estado_t flag;
    const char* descripcion;
} ErrorInfo_t;

static const ErrorInfo_t errores[] = {
    {E_PAR_INC, "Parametro incorrecto"},
    {E_CREAR_PIPE, "No se pudo crear tuberia"},
    {E_CREAR_PROCESO, "No se pudo crear proceso"},
    {E_PROCESO_NULO, "Proceso nulo"},
    {E_ENVIO_FALLIDO, "Fallo al enviar mensaje"},
    {E_THREAD_FALLIDO, "No se pudo crear thread"},
    {E_PROCESO_INACTIVO, "Proceso inactivo"},
    {E_ESCRITURA, "Error de escritura"},
    {E_LECTURA, "Error de lectura"},
    {E_MEMORIA, "Error de asignacion de memoria"},
    {0, nullptr} // Centinela
};

void imprimirEstado(const char* operacion, Estado_t estado) {
    if (!operacion) {
        return;
    }
    
    printf("[%s] ", operacion);
    
    if (estado == E_OK) {
        printf("OK\n");
        return;
    }
    
    int primera = 1;
    for (size_t i = 0; errores[i].flag != 0; i++) {
        if (TIENE_ERROR(estado, errores[i].flag)) {
            if (!primera) {
                printf(" | ");
            }
            printf("ERROR: %s", errores[i].descripcion);
            primera = 0;
        }
    }
    printf("\n");
}

const char* obtenerDescripcionError(Estado_t estado) {
    // Si no hay errores, retornar mensaje de exito
    if (estado == E_OK) {
        return "Operacion exitosa";
    }
    
    // Buscar el primer error en el estado
    for (size_t i = 0; errores[i].flag != 0; i++) {
        if (TIENE_ERROR(estado, errores[i].flag)) {
            return errores[i].descripcion;
        }
    }
    
    return "Error desconocido";
}