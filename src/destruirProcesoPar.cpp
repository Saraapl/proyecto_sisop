#include "ProcesoPar.h"

static void setProcesoActivo(ProcesoPar_t* procesoPar, int valor) {
    EnterCriticalSection(&procesoPar->criticalSection);
    procesoPar->activo = valor;
    LeaveCriticalSection(&procesoPar->criticalSection);
}

Estado_t destruirProcesoPar(ProcesoPar_t* procesoPar) {

    if (!procesoPar) {
        return E_PROCESO_NULO;
    }

    if (procesoPar->hInputWrite && procesoPar->hInputWrite != INVALID_HANDLE_VALUE) {
        CloseHandle(procesoPar->hInputWrite);
        procesoPar->hInputWrite = nullptr;
    }

    if (procesoPar->processInfo.hProcess && 
        procesoPar->processInfo.hProcess != INVALID_HANDLE_VALUE) {
        DWORD waitResult = WaitForSingleObject(procesoPar->processInfo.hProcess, 1000);
        
        if (waitResult == WAIT_TIMEOUT) {
            DWORD exitCode;
            if (GetExitCodeProcess(procesoPar->processInfo.hProcess, &exitCode)) {
                if (exitCode == STILL_ACTIVE) {
                    TerminateProcess(procesoPar->processInfo.hProcess, 0);
                    WaitForSingleObject(procesoPar->processInfo.hProcess, 500);
                }
            }
        }
    }

    setProcesoActivo(procesoPar, FALSE);

    if (procesoPar->hOutputRead && procesoPar->hOutputRead != INVALID_HANDLE_VALUE) {
        CloseHandle(procesoPar->hOutputRead);
        procesoPar->hOutputRead = nullptr;
    }

    if (procesoPar->hListenerThread && procesoPar->hListenerThread != INVALID_HANDLE_VALUE) {
        DWORD waitResult = WaitForSingleObject(procesoPar->hListenerThread, 2000);
        if (waitResult == WAIT_TIMEOUT) {
            TerminateThread(procesoPar->hListenerThread, 1);
        }
        CloseHandle(procesoPar->hListenerThread);
        procesoPar->hListenerThread = nullptr;
    }

    if (procesoPar->processInfo.hProcess && 
        procesoPar->processInfo.hProcess != INVALID_HANDLE_VALUE) {
        CloseHandle(procesoPar->processInfo.hProcess);
        procesoPar->processInfo.hProcess = nullptr;
    }

    if (procesoPar->processInfo.hThread && 
        procesoPar->processInfo.hThread != INVALID_HANDLE_VALUE) {
        CloseHandle(procesoPar->processInfo.hThread);
        procesoPar->processInfo.hThread = nullptr;
    }

    DeleteCriticalSection(&procesoPar->criticalSection);

    free(procesoPar);
    
    return E_OK;
}