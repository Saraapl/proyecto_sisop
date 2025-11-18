// establecerFuncionDeEscucha.cpp
// Este archivo es igual en windows/src/ y linux/src/

#include "ProcesoPar.h"

Estado_t establecerFuncionDeEscucha(ProcesoPar_t* procesoPar,
                                   Estado_t (*f)(const char*, int)) {

    if (!procesoPar) {
        return E_PAR_INC;
    }

    EnterCriticalSection(&procesoPar->criticalSection);
    int activo = procesoPar->activo;
    if (activo) {
        procesoPar->funcionEscucha = f;
    }
    LeaveCriticalSection(&procesoPar->criticalSection);

    if (!activo) {
        return E_PROCESO_INACTIVO;
    }

    return E_OK;
}