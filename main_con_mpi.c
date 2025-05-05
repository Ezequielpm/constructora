#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h> // para tolower

// --- Includes de módulos ---

#include "base_de_datos.h"
#include "lectura.h"
#include "validacion.h"
#include "structs.h"
#include "empresa.h"     // <--- Necesitamos los prototipos ejecutar...DB
#include "supervisor.h"
#include "solicitud.h"
// #include "proyecto.h" // <-- Incluir cuando se implementen tareas
// #include "reportes.h" // <-- Incluir cuando se implementen tareas

// --- Códigos de Tarea para MPI ---
#define TAG_TAREA 1
#define TAG_RESULTADO 2
#define TAG_DATOS 3
#define TAG_DATOS_EXTRA1 4 // tags adicionales si enviamos múltiples partes
#define TAG_DATOS_EXTRA2 5

#define TAREA_TERMINAR 0
// tareas empresa
#define TAREA_REGISTRAR_EMPRESA 1
#define TAREA_MOSTRAR_EMPRESAS 2
#define TAREA_ACTUALIZAR_EMPRESA 3
#define TAREA_ELIMINAR_EMPRESA 4
// tareas supervisor
#define TAREA_REGISTRAR_SUPERVISOR 11
#define TAREA_MOSTRAR_SUPERVISORES 12
#define TAREA_ACTUALIZAR_SUPERVISOR 13
#define TAREA_ELIMINAR_SUPERVISOR 14
// tareas solicitud
#define TAREA_REGISTRAR_SOLICITUD 21
#define TAREA_MOSTRAR_SOLICITUDES 22
#define TAREA_CANCELAR_SOLICITUD 23
#define TAREA_ACEPTAR_SOLICITUD 24 // <-- añadir más adelante
// tareas proyecto aceptado
#define TAREA_MOSTRAR_PROYECTOS 31 // <-- añadir más adelante
#define TAREA_ACTUALIZAR_AVANCE 32 // <-- añadir más adelante
#define TAREA_ACTUALIZAR_PRIORIDAD 33 // <-- añadir más adelante
#define TAREA_TERMINAR_PROYECTO 34 // <-- añadir más adelante
#define TAREA_REASIGNAR_SUPERVISOR 35 // <-- añadir más adelante
// tareas reportes
// ... definir códigos para reportes ...

// --- Declaraciones de Funciones de Menú (Prototipos) ---
void mostrarMenuPrincipal();
void mostrarMenuEmpresas();
void mostrarMenuSupervisores();
void mostrarMenuSolicitudes();
// void mostrarMenuProyectosAceptados(); // <-- añadir más adelante
// void mostrarMenuReportes();          // <-- añadir más adelante

void procesarOpcionPrincipal(int opcion, PGconn *conn_rank0, int commSize, int rank0, int *contadorTareas); // pasar contador
// las siguientes ahora solo se usan para np=1
void procesarOpcionEmpresasLocal(int opcion, PGconn *conn);
void procesarOpcionSupervisoresLocal(int opcion, PGconn *conn);
void procesarOpcionSolicitudesLocal(int opcion, PGconn *conn);
// void procesarOpcionProyectosAceptadosLocal(int opcion, PGconn *conn); // <-- añadir más adelante
// void procesarOpcionReportesLocal(int opcion, PGconn *conn);          // <-- añadir más adelante


// --- Funciones de Menú (Mostrar - sin cambios) ---
// (Copiar aquí mostrarMenuPrincipal, mostrarMenuEmpresas, mostrarMenuSupervisores, mostrarMenuSolicitudes)
void mostrarMenuPrincipal() {
    printf("\n===== CONSTRUCTORA LA ROCA BONITA - MENÚ PRINCIPAL (MPI) =====\n");
    printf("[1] Gestionar Empresas\n");
    printf("[2] Gestionar Supervisores\n");
    printf("[3] Gestionar Solicitudes de Proyecto\n");
    printf("[4] Gestionar Proyectos Aceptados (Pendiente)\n");
    printf("[5] Generar Reportes (Pendiente)\n");
    printf("[6] Salir\n");
    printf("======================================================\n");
}

void mostrarMenuEmpresas() {
    printf("\n--- Menú Empresas ---\n");
    printf("[1] Registrar Nueva Empresa\n");
    printf("[2] Mostrar Todas las Empresas\n");
    printf("[3] Actualizar Datos de Empresa\n");
    printf("[4] Eliminar Empresa\n");
    printf("[5] Volver al Menú Principal\n");
    printf("---------------------\n");
}

void mostrarMenuSupervisores() {
    printf("\n--- Menú Supervisores ---\n");
    printf("[1] Registrar Nuevo Supervisor\n");
    printf("[2] Mostrar Todos los Supervisores\n");
    printf("[3] Actualizar Datos de Supervisor\n");
    printf("[4] Eliminar Supervisor\n");
    printf("[5] Volver al Menú Principal\n");
    printf("-------------------------\n");
}

void mostrarMenuSolicitudes() {
    printf("\n--- Menú Solicitudes de Proyecto ---\n");
    printf("[1] Registrar Nueva Solicitud\n");
    printf("[2] Mostrar Solicitudes por Estado\n");
    printf("[3] Cancelar Solicitud (Estado APERTURADO)\n");
    // printf("[4] Aceptar Solicitud (Crear Proyecto)\n"); // <-- añadir más adelante
    printf("[4] Volver al Menú Principal\n"); // <-- temporalmente volver es 4
    printf("------------------------------------\n");
}

// --- Función Principal ---
int main(int argc, char *argv[]) {
    int rank, size;
    PGconn *conexion = NULL;
    bool salir = false;
    MPI_Status status; // para recibir
    int contadorTareas = 0; // para round-robin simple

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conexion = conectarDB();

    if (conexion == NULL) {
        fprintf(stderr, "Rank %d: Fallo al conectar a la BD. Abortando.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // --- Lógica Principal Dividida por Rank ---
    if (rank == 0) {
        // --- Rank 0: Coordinador / UI ---
        printf("Coordinador (Rank 0) iniciado. Trabajadores activos: %d\n", (size > 1 ? size - 1 : 0) );
        char opcionStr[5];
        int opcionSeleccionada;

        while (!salir) {
            mostrarMenuPrincipal();
            leerEntrada("Seleccione una opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
            opcionSeleccionada = atoi(opcionStr);

            if (opcionSeleccionada == 6) { // Opción Salir
                salir = true;
                if (size > 1) {
                    // Enviar señal de terminar a TODOS los trabajadores (1 a size-1)
                    int tareaTerminar = TAREA_TERMINAR;
                    printf("Rank 0: Enviando TAREA_TERMINAR a todos los trabajadores...\n");
                    for(int i = 1; i < size; i++) {
                        MPI_Send(&tareaTerminar, 1, MPI_INT, i, TAG_TAREA, MPI_COMM_WORLD);
                    }
                }
            } else if (opcionSeleccionada > 0 && opcionSeleccionada < 6) {
                procesarOpcionPrincipal(opcionSeleccionada, conexion, size, rank, &contadorTareas); // <--- Pasar contador
            } else {
                 printf("Opción no válida. Por favor, intente de nuevo.\n");
            }

            // Pausa simple (solo en Rank 0)
            if (!salir && opcionSeleccionada != 6) {
                 printf("\n(Rank 0) Presione Enter para continuar...");
                 int c; while ((c = getchar()) != '\n' && c != EOF);
                 getchar();
            }
        } // Fin while (!salir) Rank 0

    } else { // --- Ranks > 0: Trabajadores ---
        printf("Trabajador (Rank %d) iniciado y esperando tareas...\n", rank);
        int tareaRecibida;
        int resultadoOperacion = 0; // 0 = Éxito, -1 = Error DB, -2 = No encontrado/0 filas

        while (true) {
            // Esperar tarea de Rank 0
            MPI_Recv(&tareaRecibida, 1, MPI_INT, 0, TAG_TAREA, MPI_COMM_WORLD, &status);
            // printf("Rank %d: Tarea recibida = %d\n", rank, tareaRecibida); // Descomentar para depurar

            resultadoOperacion = 0; // Resetear

            if (tareaRecibida == TAREA_TERMINAR) {
                printf("Rank %d: Recibida señal de terminar. Saliendo...\n", rank);
                break; // Salir del bucle while(true)
            }

            // Ejecutar la tarea solicitada
            switch (tareaRecibida) {
                // --- EMPRESAS ---
                case TAREA_REGISTRAR_EMPRESA: {
                    Empresa empRecibida;
                    MPI_Recv(&empRecibida, sizeof(Empresa), MPI_BYTE, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    // printf("Rank %d: Registrando empresa '%s'...\n", rank, empRecibida.nombre); // depuración
                    resultadoOperacion = ejecutarRegistrarEmpresaDB(conexion, &empRecibida); // <--- LLAMADA A FUNCIÓN DB-ONLY
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                case TAREA_MOSTRAR_EMPRESAS: {
                    // printf("Rank %d: Ejecutando mostrarEmpresas...\n", rank); // depuración
                    mostrarEmpresas(conexion); // El trabajador imprime directamente (salida puede mezclarse si np > 2)
                    resultadoOperacion = 0; // asumimos que mostrar no falla críticamente
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                case TAREA_ACTUALIZAR_EMPRESA: {
                    char idRecibido[10];
                    char campoRecibido[30]; // tamaño razonable para nombre de campo
                    char valorRecibido[151]; // tamaño del campo más grande (dirección)
                    MPI_Recv(idRecibido, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(campoRecibido, 30, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    MPI_Recv(valorRecibido, 151, MPI_CHAR, 0, TAG_DATOS_EXTRA2, MPI_COMM_WORLD, &status);
                    // printf("Rank %d: Actualizando campo '%s' de empresa ID %s...\n", rank, campoRecibido, idRecibido); // depuración
                    resultadoOperacion = ejecutarActualizarCampoEmpresaDB(conexion, idRecibido, campoRecibido, valorRecibido); // <--- LLAMADA A FUNCIÓN DB-ONLY
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                 case TAREA_ELIMINAR_EMPRESA: {
                    char idParaEliminar[10];
                    MPI_Recv(idParaEliminar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    // printf("Rank %d: Eliminando empresa ID %s...\n", rank, idParaEliminar); // depuración
                    resultadoOperacion = ejecutarEliminarEmpresaDB(conexion, idParaEliminar); // <--- LLAMADA A FUNCIÓN DB-ONLY
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }

                // --- SUPERVISORES ---
                 case TAREA_MOSTRAR_SUPERVISORES: {
                    // printf("Rank %d: Ejecutando mostrarSupervisores...\n", rank); // depuración
                    mostrarSupervisores(conexion); // el trabajador imprime
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 // ... Implementar casos para TAREA_REGISTRAR_SUPERVISOR, TAREA_ACTUALIZAR_SUPERVISOR, TAREA_ELIMINAR_SUPERVISOR
                 //     llamando a las correspondientes funciones ejecutar...DB que habría que crear en supervisor.c/.h

                // --- SOLICITUDES ---
                 case TAREA_MOSTRAR_SOLICITUDES: {
                    char estadoSolicitado[20];
                    MPI_Recv(estadoSolicitado, 20, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    // printf("Rank %d: Mostrando solicitudes con estado '%s'...\n", rank, estadoSolicitado); // depuración
                    // mostrarSolicitudesPorEstado(conexion); // ¡Necesitaría refactorización para tomar estado!
                    // Crear una función ejecutarMostrarSolicitudesDB(conn, estado) o refactorizar la original
                    printf("Rank %d: (Simulado) Mostrando solicitudes '%s'.\n", rank, estadoSolicitado);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 // ... Implementar casos para TAREA_REGISTRAR_SOLICITUD, TAREA_CANCELAR_SOLICITUD, TAREA_ACEPTAR_SOLICITUD
                 //     llamando a las correspondientes funciones ejecutar...DB que habría que crear en solicitud.c/.h

                 // ... Implementar casos para PROYECTOS y REPORTES ...


                default:
                    fprintf(stderr, "Rank %d: Tarea desconocida recibida: %d\n", rank, tareaRecibida);
                    resultadoOperacion = -1; // Indicar error
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
            } // Fin switch(tareaRecibida)
        } // Fin while(true) trabajador

    } // Fin else (trabajadores)

    // --- Sincronización y Finalización ---
    MPI_Barrier(MPI_COMM_WORLD);

    if (conexion != NULL) {
        // printf("Rank %d: Desconectando de la BD...\n", rank); // Puede ser mucho output
        desconectarDB(conexion);
    }

    // printf("Rank %d: Finalizando MPI...\n", rank); // Puede ser mucho output
    MPI_Finalize();

    return 0;
}


// --- Implementación de Funciones de Procesamiento de Opciones (MODIFICADAS para MPI) ---

// procesa opciones del menú principal (EJECUTADO SOLO EN RANK 0)
void procesarOpcionPrincipal(int opcion, PGconn* conn_rank0, int commSize, int rank0_siempre_cero, int *contadorTareas) {
    // nota: conn_rank0 solo se usa si commSize == 1
    char opcionSubMenuStr[5];
    int opcionSubMenu;
    int tareaAEnviar;
    int resultadoRecibido;
    double tInicio=0.0, tFin=0.0; // para timing

    // determinar el rank del trabajador (round-robin simple si hay trabajadores)
    int rankTrabajador = 0; // por defecto, local si size es 1
    if (commSize > 1) {
        rankTrabajador = (*contadorTareas % (commSize - 1)) + 1;
        (*contadorTareas)++; // incrementar para la próxima tarea
    }

    // printf("Rank 0: Delegando tarea a Rank %d (commSize=%d)\n", rankTrabajador, commSize); // depuración

    switch (opcion) {
        case 1: // Empresas
            do {
                mostrarMenuEmpresas(); // Rank 0 siempre muestra menú
                leerEntrada("Seleccione una opción del menú Empresas:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 5) break; // Volver

                 if (rankTrabajador == 0) { // --- EJECUCIÓN LOCAL (NP=1) ---
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionEmpresasLocal(opcionSubMenu, conn_rank0); // Llamada directa a la version local
                     tFin = MPI_Wtime();
                     printf("Tiempo de ejecución local: %.4f segundos\n", tFin - tInicio);
                 } else { // --- DELEGAR A RANK TRABAJADOR (NP > 1) ---
                     bool tareaValidaParaEnviar = true;
                     // --- lógica para preparar y enviar tarea de empresa ---
                     switch(opcionSubMenu) {
                         case 1: { // registrar empresa
                                tareaAEnviar = TAREA_REGISTRAR_EMPRESA;
                                Empresa emp; // estructura temporal para recolectar datos
                                printf("--- Registro de Nueva Empresa (Datos) ---\n");
                                leerEntrada("Nombre:", emp.nombre, sizeof(emp.nombre), noEsVacio);
                                leerEntrada("Dirección:", emp.direccion, sizeof(emp.direccion), noEsVacio);
                                leerEntrada("Teléfono:", emp.telefono, sizeof(emp.telefono), sonSoloNumeros);
                                leerEntrada("Correo:", emp.correo, sizeof(emp.correo), esCorreoValido);
                                leerEntrada("RFC:", emp.rfc, sizeof(emp.rfc), esRFCValido);
                                leerEntrada("Contacto:", emp.contacto_encargado, sizeof(emp.contacto_encargado), sonSoloLetras);

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(&emp, sizeof(Empresa), MPI_BYTE, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                break;
                            }
                         case 2: { // mostrar empresas
                                tareaAEnviar = TAREA_MOSTRAR_EMPRESAS;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                // no se envían datos adicionales
                                break;
                            }
                         case 3: { // actualizar empresa
                                tareaAEnviar = TAREA_ACTUALIZAR_EMPRESA;
                                char idActualizar[10];
                                char campoActualizar[30]; // ej: "nombre", "telefono"
                                char valorActualizar[151]; // tamaño del más grande (dirección)
                                char opCampoStr[5];
                                int opCampo;

                                printf("--- Actualizar Empresa (Datos) ---\n");
                                // mostrarEmpresas(); // rank 0 no puede llamar a esto si np > 1
                                printf("(Se recomienda ejecutar 'Mostrar Empresas' primero para ver IDs)\n");
                                leerEntrada("ID de empresa a actualizar:", idActualizar, sizeof(idActualizar), esNumeroEnteroPositivoValido);
                                printf("Campo a actualizar: [1]Nombre [2]Dirección [3]Teléfono [4]Correo [5]RFC [6]Contacto\n");
                                leerEntrada("Opción Campo:", opCampoStr, sizeof(opCampoStr), esNumeroEnteroPositivoValido);
                                opCampo = atoi(opCampoStr);

                                bool (*validadorCampo)(const char*) = NULL;
                                switch(opCampo) {
                                    case 1: strcpy(campoActualizar, "nombre"); validadorCampo = noEsVacio; break;
                                    case 2: strcpy(campoActualizar, "direccion"); validadorCampo = noEsVacio; break;
                                    case 3: strcpy(campoActualizar, "telefono"); validadorCampo = sonSoloNumeros; break;
                                    case 4: strcpy(campoActualizar, "correo"); validadorCampo = esCorreoValido; break;
                                    case 5: strcpy(campoActualizar, "rfc"); validadorCampo = esRFCValido; break;
                                    case 6: strcpy(campoActualizar, "contacto_encargado"); validadorCampo = sonSoloLetras; break;
                                    default: printf("Opción de campo inválida.\n"); tareaValidaParaEnviar = false; break;
                                }

                                if(tareaValidaParaEnviar) {
                                    leerEntrada("Nuevo Valor:", valorActualizar, sizeof(valorActualizar), validadorCampo);
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(idActualizar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                    MPI_Send(campoActualizar, 30, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                    MPI_Send(valorActualizar, 151, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA2, MPI_COMM_WORLD);
                                }
                                break;
                            }
                         case 4: { // eliminar empresa
                                tareaAEnviar = TAREA_ELIMINAR_EMPRESA;
                                char idEliminar[10];
                                char confirmacion[5];
                                printf("--- Eliminar Empresa ---\n");
                                // mostrarEmpresas(); // rank 0 no puede
                                printf("(Se recomienda ejecutar 'Mostrar Empresas' primero para ver IDs)\n");
                                leerEntrada("ID a eliminar:", idEliminar, sizeof(idEliminar), esNumeroEnteroPositivoValido);
                                printf("Confirmar eliminación (s/n):");
                                fgets(confirmacion, sizeof(confirmacion), stdin);
                                confirmacion[strcspn(confirmacion, "\n")] = '\0';
                                int c; while ((c = getchar()) != '\n' && c != EOF);

                                if (tolower(confirmacion[0]) == 's') {
                                        tInicio = MPI_Wtime();
                                        MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                        MPI_Send(idEliminar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                } else {
                                    printf("Cancelado.\n");
                                    tareaValidaParaEnviar = false;
                                }
                                break;
                            }
                         default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                     } // fin switch(opcionSubMenu)

                     // --- recibir resultado del trabajador ---
                     if (tareaValidaParaEnviar) {
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        // interpretar resultado
                        switch(resultadoRecibido) {
                            case 0: printf("Rank 0: Operación completada exitosamente por Rank %d.\n", rankTrabajador); break;
                            case -1: printf("Rank 0: Rank %d reportó un ERROR de base de datos.\n", rankTrabajador); break;
                            case -2: printf("Rank 0: Operación ejecutada por Rank %d, pero no afectó filas (ID no encontrado?).\n", rankTrabajador); break;
                            default: printf("Rank 0: Rank %d reportó un resultado desconocido (%d).\n", rankTrabajador, resultadoRecibido); break;
                        }
                        printf("Tiempo de operación (MPI Send/Recv + Ejecución Rank %d): %.4f segundos\n", rankTrabajador, tFin - tInicio);
                     }
                 } // fin else (delegar)

                 // pausa dentro del submenú (solo rank 0)
                 if (opcionSubMenu != 5) { // 5 es volver
                     printf("\n(Rank 0) Presione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                 }

            } while (opcionSubMenu != 5);
            break; // Fin case 1 (Empresas)

        case 2: // Supervisores
            // ... (IMPLEMENTAR LÓGICA MPI SIMILAR A EMPRESAS PARA SUPERVISORES) ...
             printf("Gestión de Supervisores vía MPI pendiente.\n");
            break;

        case 3: // Solicitudes
            // ... (IMPLEMENTAR LÓGICA MPI SIMILAR A EMPRESAS PARA SOLICITUDES) ...
            printf("Gestión de Solicitudes vía MPI pendiente.\n");
            break;

        case 4: // Proyectos Aceptados
        case 5: // Reportes
            printf("Funcionalidad aún no implementada.\n");
            break;
        // Caso 6 (Salir) se maneja en el bucle principal de Rank 0
        default:
            // ya se maneja en el bucle principal
            break;
    }
}


// --- Funciones de procesamiento de submenús (AHORA SOLO PARA EJECUCIÓN LOCAL SI NP=1) ---
// Renombradas para claridad
void procesarOpcionEmpresasLocal(int opcion, PGconn *conn) {
    switch (opcion) {
        case 1: registrarEmpresa(conn); break; // Llama a la original con UI + DB
        case 2: mostrarEmpresas(conn); break;
        case 3: actualizarEmpresa(conn); break;
        case 4: eliminarEmpresa(conn); break;
        default: printf("Opción local inválida.\n"); break;
    }
}

void procesarOpcionSupervisoresLocal(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: registrarSupervisor(conn); break; // Llama a la original con UI + DB
        case 2: mostrarSupervisores(conn); break;
        case 3: actualizarSupervisor(conn); break;
        case 4: eliminarSupervisor(conn); break;
        default: printf("Opción local inválida.\n"); break;
    }
}

void procesarOpcionSolicitudesLocal(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: registrarSolicitud(conn); break; // Llama a la original con UI + DB
        case 2: mostrarSolicitudesPorEstado(conn); break;
        case 3: cancelarSolicitud(conn); break;
        // case 4: aceptarSolicitud(conn); break; // <-- añadir cuando se implemente
        default: printf("Opción local inválida o pendiente.\n"); break;
    }
}