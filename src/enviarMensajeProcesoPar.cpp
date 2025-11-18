#include "ProcesoPar.h"

Estado_t enviarMensajeProcesoPar(ProcesoPar_t* procesoPar,
                                const char* mensaje,
                                int longitud) {

    if (!procesoPar || !mensaje || longitud <= 0) {
        return E_PAR_INC;
    }
    
    EnterCriticalSection(&procesoPar->criticalSection);
    int activo = procesoPar->activo;
    LeaveCriticalSection(&procesoPar->criticalSection);
    
    if (!activo) {
        return E_PROCESO_INACTIVO;
    }

    if (!procesoPar->hInputWrite || procesoPar->hInputWrite == INVALID_HANDLE_VALUE) {
        return E_ESCRITURA;
    }

    DWORD bytesEscritos;
    if (!WriteFile(procesoPar->hInputWrite, mensaje, static_cast<DWORD>(longitud), 
                   &bytesEscritos, nullptr)) {
        return E_ESCRITURA;
    }
    
    if (bytesEscritos != static_cast<DWORD>(longitud)) {
        return E_ENVIO_FALLIDO;
    }
    
    FlushFileBuffers(procesoPar->hInputWrite);
    return E_OK;
}