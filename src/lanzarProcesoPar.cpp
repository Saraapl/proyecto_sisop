#include "ProcesoPar.h"
#include <algorithm>

// Función auxiliar para verificar de forma segura si el proceso está activo
static int isProcesoActivo(ProcesoPar_t* procesoPar) {
    int resultado;
    EnterCriticalSection(&procesoPar->criticalSection);
    resultado = procesoPar->activo;
    LeaveCriticalSection(&procesoPar->criticalSection);
    return resultado;
}

// Función auxiliar para establecer de forma segura el estado activo del proceso
static void setProcesoActivo(ProcesoPar_t* procesoPar, int valor) {
    EnterCriticalSection(&procesoPar->criticalSection);
    procesoPar->activo = valor;
    LeaveCriticalSection(&procesoPar->criticalSection);
}

DWORD WINAPI threadEscucha(LPVOID lpParam) {
    ProcesoPar_t* procesoPar = static_cast<ProcesoPar_t*>(lpParam);
    char buffer[4096];
    DWORD bytesLeidos;

    while (isProcesoActivo(procesoPar)) {
        if (ReadFile(procesoPar->hOutputRead, buffer, sizeof(buffer) - 1,
                     &bytesLeidos, NULL) && bytesLeidos > 0) {
            buffer[bytesLeidos] = '\0';
            
            // Procesar el búfer línea por línea (dividir por \n)
            EnterCriticalSection(&procesoPar->criticalSection);
            if (procesoPar->funcionEscucha != nullptr) {
                char* start = buffer;
                char* end;
                
                // Encontrar cada línea que termina con \n
                while ((end = strchr(start, '\n')) != nullptr) {
                    // Calcular longitud incluyendo el \n
                    int lineLen = static_cast<int>(end - start + 1);
                    
                    // Llamar al oyente para esta línea
                    procesoPar->funcionEscucha(start, lineLen);
                    
                    // Mover a la siguiente línea
                    start = end + 1;
                }
                
                // Si hay datos restantes sin \n, llamar al oyente con ellos
                if (*start != '\0') {
                    int remaining = static_cast<int>(strlen(start));
                    procesoPar->funcionEscucha(start, remaining);
                }
            }
            LeaveCriticalSection(&procesoPar->criticalSection);
        } else {
            DWORD exitCode;
            if (GetExitCodeProcess(procesoPar->processInfo.hProcess, &exitCode)) {
                if (exitCode != STILL_ACTIVE) {
                    setProcesoActivo(procesoPar, FALSE);
                    break;
                }
            }
            Sleep(10);
        }
    }
    return 0;
}

// Función auxiliar para limpiar recursos - reemplaza el patrón goto error
static void limpiarRecursos(HANDLE hInputRead, HANDLE hInputWrite,
                           HANDLE hOutputRead, HANDLE hOutputWrite,
                           ProcesoPar_t* procesoPar, int limpiarProceso) {
    if (hInputRead && hInputRead != INVALID_HANDLE_VALUE) {
        CloseHandle(hInputRead);
    }
    if (hInputWrite && hInputWrite != INVALID_HANDLE_VALUE) {
        CloseHandle(hInputWrite);
    }
    if (hOutputRead && hOutputRead != INVALID_HANDLE_VALUE) {
        CloseHandle(hOutputRead);
    }
    if (hOutputWrite && hOutputWrite != INVALID_HANDLE_VALUE) {
        CloseHandle(hOutputWrite);
    }
    
    if (limpiarProceso && procesoPar) {
        if (procesoPar->processInfo.hProcess && 
            procesoPar->processInfo.hProcess != INVALID_HANDLE_VALUE) {
            TerminateProcess(procesoPar->processInfo.hProcess, 1);
            CloseHandle(procesoPar->processInfo.hProcess);
        }
        if (procesoPar->processInfo.hThread && 
            procesoPar->processInfo.hThread != INVALID_HANDLE_VALUE) {
            CloseHandle(procesoPar->processInfo.hThread);
        }
        DeleteCriticalSection(&procesoPar->criticalSection);
        free(procesoPar);
    }
}

Estado_t lanzarProcesoPar(const char* nombreArchivoEjecutable,
                         const char** listaLineaComando,
                         ProcesoPar_t** procesoPar) {

    if (!nombreArchivoEjecutable || !procesoPar) {
        return E_PAR_INC;
    }

    *procesoPar = static_cast<ProcesoPar_t*>(malloc(sizeof(ProcesoPar_t)));
    if (!*procesoPar) {
        return E_MEMORIA;
    }

    memset(*procesoPar, 0, sizeof(ProcesoPar_t));
    InitializeCriticalSection(&(*procesoPar)->criticalSection);

    // --- Declaración de todas las variables al inicio ---
    SECURITY_ATTRIBUTES saAttr;
    HANDLE hInputRead = nullptr;
    HANDLE hInputWrite = nullptr;
    HANDLE hOutputRead = nullptr;
    HANDLE hOutputWrite = nullptr;
    STARTUPINFOA siStartInfo;
    char lineaComando[4096];
    Estado_t resultado = E_OK;

    // --- Inicialización ---
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.lpSecurityDescriptor = nullptr;
    saAttr.bInheritHandle = TRUE;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    ZeroMemory(lineaComando, sizeof(lineaComando));

    // --- Creación de pipes ---
    if (!CreatePipe(&hInputRead, &hInputWrite, &saAttr, 0)) {
        resultado = E_CREAR_PIPE;
        limpiarRecursos(nullptr, nullptr, nullptr, nullptr, *procesoPar, 0);
        DeleteCriticalSection(&(*procesoPar)->criticalSection);
        free(*procesoPar);
        *procesoPar = nullptr;
        return resultado;
    }
    
    if (!SetHandleInformation(hInputWrite, HANDLE_FLAG_INHERIT, 0)) {
        resultado = E_CREAR_PIPE;
        limpiarRecursos(hInputRead, hInputWrite, nullptr, nullptr, *procesoPar, 0);
        DeleteCriticalSection(&(*procesoPar)->criticalSection);
        free(*procesoPar);
        *procesoPar = nullptr;
        return resultado;
    }
    
    if (!CreatePipe(&hOutputRead, &hOutputWrite, &saAttr, 0)) {
        resultado = E_CREAR_PIPE;
        limpiarRecursos(hInputRead, hInputWrite, nullptr, nullptr, *procesoPar, 0);
        DeleteCriticalSection(&(*procesoPar)->criticalSection);
        free(*procesoPar);
        *procesoPar = nullptr;
        return resultado;
    }
    
    if (!SetHandleInformation(hOutputRead, HANDLE_FLAG_INHERIT, 0)) {
        resultado = E_CREAR_PIPE;
        limpiarRecursos(hInputRead, hInputWrite, hOutputRead, hOutputWrite, *procesoPar, 0);
        DeleteCriticalSection(&(*procesoPar)->criticalSection);
        free(*procesoPar);
        *procesoPar = nullptr;
        return resultado;
    }

    // --- Configuración del proceso hijo ---
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hOutputWrite;
    siStartInfo.hStdOutput = hOutputWrite;
    siStartInfo.hStdInput = hInputRead;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // --- Construcción segura de la línea de comando ---
    size_t remaining = sizeof(lineaComando) - 1;
    size_t len = strlen(nombreArchivoEjecutable);
    size_t toCopy = (len < remaining) ? len : remaining;
    memcpy(lineaComando, nombreArchivoEjecutable, toCopy);
    lineaComando[toCopy] = '\0';
    remaining -= toCopy;
    
    if (listaLineaComando) {
        for (int i = 0; listaLineaComando[i] && remaining > 1; i++) {
            size_t currentLen = strlen(lineaComando);
            size_t argLen = strlen(listaLineaComando[i]);
            
            if (remaining > argLen + 1) {
                lineaComando[currentLen] = ' ';
                lineaComando[currentLen + 1] = '\0';
                memcpy(lineaComando + currentLen + 1, listaLineaComando[i], 
                       std::min(argLen, remaining - 1));
                lineaComando[currentLen + 1 + std::min(argLen, remaining - 1)] = '\0';
                remaining -= (argLen + 1);
            }
        }
    }

    // --- Creación del proceso ---
    if (!CreateProcessA(nullptr, lineaComando, nullptr, nullptr, TRUE, 0,
                       nullptr, nullptr, &siStartInfo, &(*procesoPar)->processInfo)) {
        resultado = E_CREAR_PROCESO;
        limpiarRecursos(hInputRead, hInputWrite, hOutputRead, hOutputWrite, *procesoPar, 0);
        DeleteCriticalSection(&(*procesoPar)->criticalSection);
        free(*procesoPar);
        *procesoPar = nullptr;
        return resultado;
    }

    // --- Cierre de handles innecesarios ---
    CloseHandle(hInputRead);
    CloseHandle(hOutputWrite);
    hInputRead = nullptr;
    hOutputWrite = nullptr;

    (*procesoPar)->hInputWrite = hInputWrite;
    (*procesoPar)->hOutputRead = hOutputRead;
    (*procesoPar)->activo = TRUE;

    // --- Lanzar hilo de escucha ---
    (*procesoPar)->hListenerThread = CreateThread(
        nullptr, 0, threadEscucha, *procesoPar, 0, nullptr);
    if (!(*procesoPar)->hListenerThread) {
        resultado = E_THREAD_FALLIDO;
        // Limpiar el proceso que acabamos de crear
        limpiarRecursos(nullptr, hInputWrite, hOutputRead, nullptr, *procesoPar, 1);
        free(*procesoPar);
        *procesoPar = nullptr;
        return resultado;
    }

    return E_OK;
}
