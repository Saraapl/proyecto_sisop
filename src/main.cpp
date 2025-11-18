#include <stdio.h>
#include <string.h>
#include "ProcesoPar.h"

#include <windows.h>

// Constantes de tiempo para esperas
static const DWORD TIEMPO_ESPERA_RESPUESTA = 500;
static const DWORD TIEMPO_ESPERA_PROCESAMIENTO = 1000;

// Variables globales para validación
static const char* mensajeEsperado = NULL;
static int longitudEsperada = 0;
static int mensajesRecibidos = 0;
static int mensajesCorrectos = 0;

Estado_t funcionEscucha(const char* mensaje, int longitud) {
    Estado_t estado = E_OK;
    
    mensajesRecibidos++;
    printf("[HIJO] (%d bytes): %.*s", longitud, longitud, mensaje);
    if (longitud > 0 && mensaje[longitud-1] != '\n') printf("\n");
    
    // Validar que el mensaje recibido coincide con el esperado
    if (mensajeEsperado != NULL) {
        // Eliminar salto de linea y carriage return del mensaje recibido para comparar
        int longitudRecibida = longitud;
        while (longitudRecibida > 0 && 
               (mensaje[longitudRecibida-1] == '\n' || mensaje[longitudRecibida-1] == '\r')) {
            longitudRecibida--;
        }
        
        if (longitudRecibida == longitudEsperada && 
            memcmp(mensaje, mensajeEsperado, longitudRecibida) == 0) {
            printf("         [OK] Respuesta correcta\n");
            mensajesCorrectos++;
        } else {
            printf("         [ERROR] Respuesta no coincide\n");
            printf("         Esperado: %.*s\n", longitudEsperada, mensajeEsperado);
            printf("         Recibido: %.*s\n", longitudRecibida, mensaje);
            
            estado = E_LECTURA;
        }
    }
    
    return estado;
}

int main() {
    printf("=== PRUEBA PROCESOS PARES ===\n\n");

    ProcesoPar_t* procesoPar = NULL;
    Estado_t estado = E_OK;

    const char* ejecutable = "cat.exe";
    const char* args[] = {NULL};

    // PASO 1: Lanzar proceso
    printf("\n[PASO 1] Lanzando proceso: %s\n", ejecutable);
    estado = lanzarProcesoPar(ejecutable, args, &procesoPar);
    imprimirEstado("lanzarProcesoPar", estado);
    if (estado != E_OK) {
        printf("ERROR: No se pudo lanzar el proceso. Abortando.\n");
        return 1;
    }

    // PASO 2: Establecer funcion de escucha
    printf("\n[PASO 2] Estableciendo funcion de escucha\n");
    estado = establecerFuncionDeEscucha(procesoPar, funcionEscucha);
    imprimirEstado("establecerFuncionDeEscucha", estado);
    if (estado != E_OK) {
        printf("ERROR: No se pudo establecer escucha.\n");
        destruirProcesoPar(procesoPar);
        return 1;
    }

    // PASO 3: Enviar mensajes (pares de numeros para sumar)
    printf("\n[PASO 3] Enviando pares de numeros y verificando sumas\n");
    
    // Estructura: {input, expected_output}
    struct TestCase {
        const char* input;      // "a b\n" para enviar
        const char* expected;   // "suma" para recibir (sin \n)
    };
    
    TestCase testCases[] = {
        {"3 5\n", "8"},
        {"10 20\n", "30"},
        {"100 200\n", "300"},
        {nullptr, nullptr}
    };

    int totalMensajes = 0;
    for(int i = 0; testCases[i].input != nullptr; i++) {
        totalMensajes++;
        printf("\n[Test %d] Enviando: %s", i+1, testCases[i].input);
        
        // Configurar mensaje esperado para validacion
        mensajeEsperado = testCases[i].expected;
        longitudEsperada = static_cast<int>(strlen(testCases[i].expected));
        
        estado = enviarMensajeProcesoPar(procesoPar, testCases[i].input, 
                                        static_cast<int>(strlen(testCases[i].input)));
        imprimirEstado("enviarMensajeProcesoPar", estado);
        
        if (estado != E_OK) {
            printf("ADVERTENCIA: Error al enviar test %d\n", i+1);
        }
        
        // Esperar respuesta del hijo
        Sleep(TIEMPO_ESPERA_RESPUESTA);
    }
    
    // Enviar senal de terminacion al hijo (0 0)
    printf("\n[Enviando senal de terminacion: 0 0]\n");
    const char* endSignal = "0 0\n";
    estado = enviarMensajeProcesoPar(procesoPar, endSignal, static_cast<int>(strlen(endSignal)));
    imprimirEstado("enviarMensajeProcesoPar (senal fin)", estado);

    // PASO 4: Esperar procesamiento final y validar
    printf("\n[PASO 4] Esperando procesamiento final\n");
    Sleep(TIEMPO_ESPERA_PROCESAMIENTO);
    
    // Resumen de validacion
    printf("\n[VALIDACION]\n");
    printf("  Mensajes enviados:  %d\n", totalMensajes);
    printf("  Mensajes recibidos: %d\n", mensajesRecibidos);
    printf("  Mensajes correctos: %d\n", mensajesCorrectos);
    
    if (mensajesRecibidos != totalMensajes) {
        printf("  [ERROR] No se recibieron todos los mensajes\n");
        estado |= E_LECTURA;
    } else if (mensajesCorrectos != totalMensajes) {
        printf("  [ERROR] Algunos mensajes no coinciden\n");
        estado |= E_LECTURA;
    } else {
        printf("  [OK] Todas las respuestas son correctas\n");
    }

    // PASO 5: Destruir proceso
    printf("\n[PASO 5] Destruyendo proceso\n");
    estado = destruirProcesoPar(procesoPar);
    imprimirEstado("destruirProcesoPar", estado);

    printf("\n=== FIN PRUEBA ===\n");
    return (estado == E_OK) ? 0 : 1;
}