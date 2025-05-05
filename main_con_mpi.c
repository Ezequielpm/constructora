#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h> // para tolower
#include <unistd.h>

// Includes de módulos 
#include "base_de_datos.h"
#include "lectura.h"
#include "validacion.h"
#include "structs.h"
#include "empresa.h"     
#include "supervisor.h"
#include "solicitud.h"
// #include "proyecto.h" //incluir cuando se implementen tareas
// #include "reportes.h" //incluir cuando se implementen tareas

// Códigos de Tarea para MPI
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
// tareas supervisor (pendientes de implementar ejecutar...DB y lógica mpi)
#define TAREA_REGISTRAR_SUPERVISOR 11
#define TAREA_MOSTRAR_SUPERVISORES 12
#define TAREA_ACTUALIZAR_SUPERVISOR 13
#define TAREA_ELIMINAR_SUPERVISOR 14
// tareas solicitud (pendientes de implementar ejecutar...DB y lógica mpi)
#define TAREA_REGISTRAR_SOLICITUD 21
#define TAREA_MOSTRAR_SOLICITUDES 22
#define TAREA_CANCELAR_SOLICITUD 23
#define TAREA_ACEPTAR_SOLICITUD 24
// ......


//Declaraciones de Funciones de Menú (Prototipos)
void mostrarMenuPrincipal();
void mostrarMenuEmpresas();
void mostrarMenuSupervisores();
void mostrarMenuSolicitudes();
// ... prototipos para proyecto y reportes ...

void procesarOpcionPrincipal(int opcion, PGconn *conn_rank0, int commSize, int rank0, int *contadorTareas);
// versiones locales para np=1
void procesarOpcionEmpresasLocal(int opcion, PGconn *conn);
void procesarOpcionSupervisoresLocal(int opcion, PGconn *conn);
void procesarOpcionSolicitudesLocal(int opcion, PGconn *conn);
// ... (prototipos para proyecto y reportes locales) ...


//Funciones de Menú
//Copiar/Mantener mostrarMenuPrincipal, mostrarMenuEmpresas, etc. aquí
void mostrarMenuPrincipal() {
    printf("\n===== CONSTRUCTORA LA ROCA BONITA - MENÚ PRINCIPAL (MPI) =====\n");
    printf("[1] Gestionar Empresas\n");
    printf("[2] Gestionar Supervisores\n");
    printf("[3] Gestionar Solicitudes de Proyecto\n");
    printf("[4] Gestionar Proyectos Aceptados (Pendiente)\n");
    printf("[5] Generar Reportes (Pendiente)\n");
    printf("[6] Salir\n");
    //printf("Seleccione una opción:\n");
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
    // printf("[4] Aceptar Solicitud (Crear Proyecto)\n"); //se añadirá cuando se implemente
    printf("[4] Volver al Menú Principal\n"); //temporalmente volver es 4
    printf("------------------------------------\n");
}


// Función Principal
int main(int argc, char *argv[]) {
    int rank, size;
    PGconn *conexion = NULL;
    bool salir = false;
    MPI_Status status;
    int contadorTareas = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Conexión BD (Cada Rank)
    // (modificar conectarDB para que no imprima en éxito, o ignorar)
    conexion = conectarDB();

    if (conexion == NULL) {
        fprintf(stderr, "Rank %d: Fallo al conectar a la BD. Abortando.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
     // solo rank 0 informa del éxito general de la conexión
     if (rank == 0) {
         // printf("Rank 0: Conexión a BD establecida.\n");
     }



    // Lógica Principal Dividida por Rank 
    if (rank == 0) {
        // Rank 0: Coordinador / UI 
        if(size > 1) {
            printf("Coordinador (Rank 0) iniciado. Trabajadores activos: %d\n", size - 1);
        } else {
            printf("Ejecutando en modo local (1 solo proceso).\n");
        }
        char opcionStr[5];
        int opcionSeleccionada;

        while (!salir) {
            //usleep(10000);
            mostrarMenuPrincipal();
            //usleep(10000);

            fflush(stdout);
            leerEntrada("Seleccione una opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
            opcionSeleccionada = atoi(opcionStr);

            if (opcionSeleccionada == 6) { //salir
                salir = true;
                if (size > 1) {
                    int tareaTerminar = TAREA_TERMINAR;
                    // printf("Rank 0: Enviando TAREA_TERMINAR a todos los trabajadores...\n"); 
                    for(int i = 1; i < size; i++) {
                        MPI_Send(&tareaTerminar, 1, MPI_INT, i, TAG_TAREA, MPI_COMM_WORLD);
                    }
                }
            } else if (opcionSeleccionada > 0 && opcionSeleccionada < 6) {
                procesarOpcionPrincipal(opcionSeleccionada, conexion, size, rank, &contadorTareas);
            } else {
                 printf("Opción no válida. Por favor, intente de nuevo.\n");
            }

            // pausa simple (solo en rank 0)
            if (!salir && opcionSeleccionada != 6) {
                 printf("\n(Rank 0) Presione Enter para continuar...");
                 int c; while ((c = getchar()) != '\n' && c != EOF);
                 getchar();
            }
        } // fin while (!salir) rank 0

    } else { // Ranks > 0: Trabajadores
        // printf("Trabajador (Rank %d) iniciado y esperando tareas...\n", rank);
        int tareaRecibida;
        int resultadoOperacion = 0;

        while (true) {
            MPI_Recv(&tareaRecibida, 1, MPI_INT, 0, TAG_TAREA, MPI_COMM_WORLD, &status);
            // printf("Rank %d: Tarea recibida = %d\n", rank, tareaRecibida); 

            resultadoOperacion = 0; //resetear

            if (tareaRecibida == TAREA_TERMINAR) {
                // printf("Rank %d: Recibida señal de terminar. Saliendo...\n", rank);
                break;
            }

            // ejecutar la tarea solicitada
            switch (tareaRecibida) {
                //EMPRESAS
                case TAREA_REGISTRAR_EMPRESA: {
                    Empresa empRecibida;
                    MPI_Recv(&empRecibida, sizeof(Empresa), MPI_BYTE, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarRegistrarEmpresaDB(conexion, &empRecibida);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                case TAREA_MOSTRAR_EMPRESAS: {
                    // no hay datos que recibir
                    mostrarEmpresas(conexion); // <<<--- trabajador llama a la función original (imprime)
                    resultadoOperacion = 0; //asumimos éxito si no hay crash
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD); //envía ack
                    break;
                }
                case TAREA_ACTUALIZAR_EMPRESA: {
                    char idRecibido[10];
                    char campoRecibido[30];
                    char valorRecibido[151]; //tamaño del más grande (dirección)
                    MPI_Recv(idRecibido, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(campoRecibido, 30, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    MPI_Recv(valorRecibido, 151, MPI_CHAR, 0, TAG_DATOS_EXTRA2, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarActualizarCampoEmpresaDB(conexion, idRecibido, campoRecibido, valorRecibido);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                 case TAREA_ELIMINAR_EMPRESA: {
                    char idParaEliminar[10];
                    MPI_Recv(idParaEliminar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarEliminarEmpresaDB(conexion, idParaEliminar);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }

                //SUPERVISORES
                 case TAREA_MOSTRAR_SUPERVISORES: {
                    // no hay datos que recibir
                    mostrarSupervisores(conexion); // <<<--- trabajador llama a la función original (imprime)
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD); // envía ack
                    break;
                 }
                 // ... (Casos para otras tareas de supervisor pendientes de ejecutar...DB y lógica MPI) ...

                //SOLICITUDES
                 case TAREA_MOSTRAR_SOLICITUDES: {
                    char estadoSolicitado[20];
                    MPI_Recv(estadoSolicitado, 20, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    //necesitaríamos refactorizar o crear ejecutarMostrarSolicitudesDB(conn, estado)
                    //por ahora, simular o llamar a la original si se adapta:
                    printf("Rank %d: (Simulado) Mostrando solicitudes '%s'...\n", rank, estadoSolicitado);
                    // mostrarSolicitudesPorEstado(conexion); // <-- Esta pide input, No usar.
                                                            //se necesita una función nueva o refactorizada.
                    resultadoOperacion = -1; // indica que esta parte falta
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 // ... (Casos para otras tareas de solicitud pendientes de ejecutar...DB y lógica MPI) ...

                 // ... (Casos para PROYECTOS y REPORTES pendientes) ...
                default:
                    fprintf(stderr, "Rank %d: Tarea desconocida recibida: %d\n", rank, tareaRecibida);
                    resultadoOperacion = -1; // indicar error
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
            } // fin switch(tareaRecibida)
        } // fin while(true) trabajador

    } // fin else (trabajadores)

    //Sincronización y Finalización
    MPI_Barrier(MPI_COMM_WORLD); // esperar a todos

    if (conexion != NULL) {
        desconectarDB(conexion);
    }

    MPI_Finalize();

    return 0;
}


// Implementación de funciones de procesamiento de opciones

// procesa opciones del menú principal (EJECUTADO SOLO EN RANK 0)
void procesarOpcionPrincipal(int opcion, PGconn* conn_rank0, int commSize, int rank0_siempre_cero, int *contadorTareas) {
    // conn_rank0 solo se usa si commSize == 1
    char opcionSubMenuStr[5];
    int opcionSubMenu;
    int tareaAEnviar;
    int resultadoRecibido;
    double tInicio=0.0, tFin=0.0;

    // determinar trabajador (round-robin simple si hay trabajadores)
    int rankTrabajador = 0; //ejecución local por defecto
    if (commSize > 1) {
        rankTrabajador = (*contadorTareas % (commSize - 1)) + 1;
        (*contadorTareas)++;
    }

    switch (opcion) {
        case 1: // Empresas
            do {
                mostrarMenuEmpresas(); // rank 0 muestra menú
                leerEntrada("Seleccione opción Empresas:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 5) break; // volver

                 if (rankTrabajador == 0) { // EJECUCIÓN LOCAL (NP=1) 
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionEmpresasLocal(opcionSubMenu, conn_rank0);
                     tFin = MPI_Wtime();
                     printf("Tiempo ejecución local: %.4f seg\n", tFin - tInicio);
                 } else { // SE DELEGA A RANK TRABAJADOR (NP > 1)
                     bool tareaValidaParaEnviar = true;
                     // logica para preparar y enviar tarea de empresa
                     switch(opcionSubMenu) {
                         case 1: { // registrar empresa
                                tareaAEnviar = TAREA_REGISTRAR_EMPRESA;
                                Empresa emp;
                                printf("--- Registro Empresa (Datos) ---\n");
                                leerEntrada("Nombre:", emp.nombre, sizeof(emp.nombre), noEsVacio);
                                leerEntrada("Dirección:", emp.direccion, sizeof(emp.direccion), noEsVacio);
                                leerEntrada("Teléfono:", emp.telefono, sizeof(emp.telefono), sonSoloNumeros);
                                leerEntrada("Correo:", emp.correo, sizeof(emp.correo), esCorreoValido);
                                leerEntrada("RFC:", emp.rfc, sizeof(emp.rfc), esRFCValido);
                                leerEntrada("Contacto:", emp.contacto_encargado, sizeof(emp.contacto_encargado), sonSoloLetras);

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(&emp, sizeof(Empresa), MPI_BYTE, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                break; // fin case 1 (registrar)
                            }
                         case 2: { // mostrar empresas
                                tareaAEnviar = TAREA_MOSTRAR_EMPRESAS;
                                tInicio = MPI_Wtime();
                                // printf("Rank 0: Enviando TAREA_MOSTRAR_EMPRESAS a Rank %d\n", rankTrabajador);// debug
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                // no se envían datos adicionales
                                break; // fin case 2 (mostrar)
                            }
                         case 3: { // actualizar empresa
                                tareaAEnviar = TAREA_ACTUALIZAR_EMPRESA;
                                char idActualizar[10];
                                char campoActualizar[30];
                                char valorActualizar[151];
                                char opCampoStr[5];
                                int opCampo;

                                printf("--- Actualizar Empresa (Datos) ---\n");
                                printf("(Se recomienda ejecutar 'Mostrar Empresas' primero para ver IDs)\n");
                                leerEntrada("ID empresa a actualizar:", idActualizar, sizeof(idActualizar), esNumeroEnteroPositivoValido);
                                printf("Campo: [1]Nombre [2]Dirección [3]Teléfono [4]Correo [5]RFC [6]Contacto\n");
                                leerEntrada("Opción Campo:", opCampoStr, sizeof(opCampoStr), esNumeroEnteroPositivoValido);
                                opCampo = atoi(opCampoStr);

                                bool (*validadorCampo)(const char*) = NULL;
                                switch(opCampo) { // determinar nombre de campo y validador
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
                                break; // fin case 3 (actualizar)
                            }
                         case 4: { // eliminar empresa
                                tareaAEnviar = TAREA_ELIMINAR_EMPRESA;
                                char idEliminar[10];
                                char confirmacion[5];
                                printf("--- Eliminar Empresa ---\n");
                                printf("(Se recomienda ejecutar 'Mostrar Empresas' primero para ver IDs)\n");
                                leerEntrada("ID a eliminar:", idEliminar, sizeof(idEliminar), esNumeroEnteroPositivoValido);
                                printf("Confirmar eliminación (s/n):");
                                //se usa fgets para lectura simple de s/n
                                fgets(confirmacion, sizeof(confirmacion), stdin);
                                confirmacion[strcspn(confirmacion, "\n")] = '\0';
                                int c; while ((c = getchar()) != '\n' && c != EOF); //limpiar buffer

                                if (tolower(confirmacion[0]) == 's') {
                                        tInicio = MPI_Wtime();
                                        MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                        MPI_Send(idEliminar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                } else {
                                    printf("Cancelado.\n");
                                    tareaValidaParaEnviar = false;
                                }
                                break; // fin case 4 (eliminar)
                            }
                         default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                     } // fin switch(opcionSubMenu) Empresa

                     //  recibir resultado del trabajador (si se envió tarea) 
                     if (tareaValidaParaEnviar) {
                        // esperar respuesta específica del trabajador asignado
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        // interpretar resultado
                        switch(resultadoRecibido) {
                            case 0: printf("Rank 0: Operación completada exitosamente por Rank %d.\n", rankTrabajador); break;
                            case -1: printf("Rank 0: Rank %d reportó un ERROR de base de datos.\n", rankTrabajador); break;
                            case -2: printf("Rank 0: Operación ejecutada por Rank %d, pero no afectó filas (ID no encontrado?).\n", rankTrabajador); break;
                            default: printf("Rank 0: Rank %d reportó un resultado desconocido (%d).\n", rankTrabajador, resultadoRecibido); break;
                        }
                        // mostrar tiempo solo si la tarea no fue 'mostrar' (cuya duración no es tan relevante aquí)
                        if(opcionSubMenu != 2) {
                            printf("Tiempo de operación (MPI Send/Recv + Ejecución Rank %d): %.4f segundos\n", rankTrabajador, tFin - tInicio);
                        } else {
                            printf("Tiempo de coordinación (MPI Send/Recv): %.4f segundos\n", tFin - tInicio);
                        }
                     }
                 } // fin else (delegar)

                 // pausa dentro del submenú (solo rank 0)
                 if (opcionSubMenu != 5) { // 5 es volver
                     printf("\n(Rank 0) Presione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                 }

            } while (opcionSubMenu != 5);
            break; // fin case 1 (Empresas)

        case 2: // Supervisores
             // --- IMPLEMENTAR LÓGICA MPI PARA SUPERVISORES ---
             //     (similar a Empresas, usando TAREA_..._SUPERVISOR y
             //      se necesitará crear ejecutar...DB en supervisor.c)
             printf("Gestión de Supervisores vía MPI pendiente de implementar.\n");
            break;

        case 3: // Solicitudes
             // --- IMPLEMENTAR LÓGICA MPI PARA SOLICITUDES ---
             //     (similar a Empresas, usando TAREA_..._SOLICITUD y
             //      se necesitará crear ejecutar...DB en solicitud.c, y refactorizar/crear
             //      una función para mostrar solicitudes por estado sin input)
             printf("Gestión de Solicitudes vía MPI pendiente de implementar.\n");
            break;

        case 4: // Proyectos Aceptados
             // --- IMPLEMENTAR LÓGICA MPI PARA PROYECTOS ---
             printf("Gestión de Proyectos Aceptados vía MPI pendiente.\n");
            break;
        case 5: // Reportes
             // --- IMPLEMENTAR LÓGICA MPI PARA REPORTES ---
             //     (Rank 0 pide parámetros si es necesario, envía tarea/parámetros,
             //      trabajador llama a función de reporte original (imprime), envía ack)
             printf("Generación de Reportes vía MPI pendiente.\n");
            break;
        // Caso 6 (Salir) se maneja en el bucle principal de Rank 0
        default:
            // ya se maneja en el bucle principal
            break;
    }
}


// --- Funciones de procesamiento de submenús (SOLO PARA EJECUCIÓN LOCAL SI NP=1) ---
// Renombradas para claridad
void procesarOpcionEmpresasLocal(int opcion, PGconn *conn) {
    switch (opcion) {
        case 1: registrarEmpresa(conn); break;
        case 2: mostrarEmpresas(conn); break;
        case 3: actualizarEmpresa(conn); break;
        case 4: eliminarEmpresa(conn); break;
        default: printf("Opción local inválida.\n"); break;
    }
}

void procesarOpcionSupervisoresLocal(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: registrarSupervisor(conn); break;
        case 2: mostrarSupervisores(conn); break;
        case 3: actualizarSupervisor(conn); break;
        case 4: eliminarSupervisor(conn); break;
        default: printf("Opción local inválida.\n"); break;
    }
}

void procesarOpcionSolicitudesLocal(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: registrarSolicitud(conn); break;
        case 2: mostrarSolicitudesPorEstado(conn); break; // <-- Pide input, ok si np=1
        case 3: cancelarSolicitud(conn); break;
        // case 4: aceptarSolicitud(conn); break; // <-- añadir cuando se implemente
        default: printf("Opción local inválida o pendiente.\n"); break;
    }
}