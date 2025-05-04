#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

/*
NOTA: Este archivo implementa el uso de mpi, sin embargo, aun hay algunos
errores que se deben corregir, por ahora se pueden encontrar algunos bugs si se ejecuta,
no obstante, la versión "original" del programa y como lucirá cuando se implemente
completamente el uso de mpi se encuentra en el archivo "main.c", gracias por su atencion.
*/


#include <ctype.h>

#include "base_de_datos.h" 
#include "lectura.h"       
#include "validacion.h"
#include "structs.h"       
#include "empresa.h"
#include "supervisor.h"
#include "solicitud.h"

//codigos de tarea para MPI
#define TAG_TAREA 1
#define TAG_RESULTADO 2
#define TAG_DATOS 3

#define TAREA_TERMINAR 0
//Tareas Empresa
#define TAREA_REGISTRAR_EMPRESA 1
#define TAREA_MOSTRAR_EMPRESAS 2
#define TAREA_ACTUALIZAR_EMPRESA 3 //rank 0 pide todo
#define TAREA_ELIMINAR_EMPRESA 4
//Tareas Supervisor
#define TAREA_REGISTRAR_SUPERVISOR 11
#define TAREA_MOSTRAR_SUPERVISORES 12
#define TAREA_ACTUALIZAR_SUPERVISOR 13
#define TAREA_ELIMINAR_SUPERVISOR 14
//Tareas Solicitud
#define TAREA_REGISTRAR_SOLICITUD 21
#define TAREA_MOSTRAR_SOLICITUDES 22
#define TAREA_CANCELAR_SOLICITUD 23

//falta por definirse más tareas

//declaraciones de Funciones de menu (prototipos)
//menus de mostrar
void mostrarMenuPrincipal();
void mostrarMenuEmpresas();
void mostrarMenuSupervisores();
void mostrarMenuSolicitudes();

//funciones de procesamiento de opciones
void procesarOpcionPrincipal(int opcion, PGconn *conn_rank0, int commSize, int rank0); 
void procesarOpcionEmpresas(int opcion, PGconn *conn);
void procesarOpcionSupervisores(int opcion, PGconn *conn);
void procesarOpcionSolicitudes(int opcion, PGconn *conn);



void mostrarMenuPrincipal() {
    printf("\n--- CONSTRUCTORA LA ROCA BONITA - MENÚ PRINCIPAL (MPI) ---\n");
    printf("[1] Gestionar Empresas\n");
    printf("[2] Gestionar Supervisores\n");
    printf("[3] Gestionar Solicitudes de Proyecto\n");
    printf("[4] Gestionar Proyectos Aceptados (Pendiente)\n");
    printf("[5] Generar Reportes (Pendiente)\n");
    printf("[6] Salir\n");
    printf("------------------------------------------------------\n");
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
    printf("[4] Volver al Menú Principal\n");
    //opciones pendientes:
    // printf("[5] Aceptar Solicitud\n");
    printf("------------------------------------\n");
}


//función principal
int main(int argc, char *argv[]) {
    int rank, size;
    PGconn *conexion = NULL; //cada rank tendrá su conexión
    bool salir = false;
    MPI_Status status;
    double tiempoInicio, tiempoFin;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    conexion = conectarDB(); 

    if (conexion == NULL) {
        fprintf(stderr, "Rank %d: Fallo al conectar a la BD. Abortando.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1); //se terminan todos los procesos MPI
    }

    //lógica principal dividida por rank (aun falta pulir)
    if (rank == 0) {
        //el rank 0 es el coordinador
        printf("Coordinador (Rank 0) iniciado. Trabajadores esperados: %d\n", size - 1);
        char opcionStr[5];
        int opcionSeleccionada;

        while (!salir) {
            mostrarMenuPrincipal();
            leerEntrada("Seleccione una opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
            opcionSeleccionada = atoi(opcionStr);

            //procesar opción DELEGANDO si es necesario
             if (opcionSeleccionada == 6) { //opción Salir
                salir = true;
                if (size > 1) {
                    //enviar señal de terminar al trabajador (Rank 1)
                    int tareaTerminar = TAREA_TERMINAR;
                    printf("Rank 0: Enviando TAREA_TERMINAR a Rank 1...\n");
                    MPI_Send(&tareaTerminar, 1, MPI_INT, 1, TAG_TAREA, MPI_COMM_WORLD);
                }
            } else if (opcionSeleccionada > 0 && opcionSeleccionada < 6) {
                //procesar otras opciones llamando a funciones que DELEGAN
                procesarOpcionPrincipal(opcionSeleccionada, conexion, size, rank); //se pasa size y rank para delegar
            }
            else {
                 printf("Opción no válida. Por favor, intente de nuevo.\n");
            }


            //pausa simple (solo en rank 0)
            if (!salir && opcionSeleccionada != 6) { //no pausar si ya vamos a salir
                 printf("\n(Rank 0) Presione Enter para continuar...");
                 int c; while ((c = getchar()) != '\n' && c != EOF);
                 getchar();
            }
        }

    } else if (rank == 1) {
        // el rank 1 es el trabajador en la base de datos
        printf("Trabajador (Rank 1) iniciado y esperando tareas...\n");
        int tareaRecibida;
        int resultadoOperacion = 0; // 0 = exito, -1 = error

        while (true) {
            //esperar tarea de Rank 0
            MPI_Recv(&tareaRecibida, 1, MPI_INT, 0, TAG_TAREA, MPI_COMM_WORLD, &status);
            printf("Rank 1: Tarea recibida = %d\n", tareaRecibida);

            resultadoOperacion = 0; //resetear resultado

            if (tareaRecibida == TAREA_TERMINAR) {
                printf("Rank 1: Recibida señal de terminar. Saliendo...\n");
                break; //salir del bucle while(true)
            }

            //ejecutar la tarea solicitada
            switch (tareaRecibida) {
                // --- EMPRESAS ---
                case TAREA_REGISTRAR_EMPRESA:
                    //el rank 1 llama a la función original, pero necesita recibir los datos
                    {
                        Empresa emp; //recibir datos en una struct temporal
                        MPI_Recv(&emp, sizeof(Empresa), MPI_BYTE, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                        printf("Rank 1: Registrando empresa '%s'...\n", emp.nombre);
                        // registrarEmpresa(conexion);
                         printf("Rank 1: (Simulado) Empresa registrada.\n");
                         resultadoOperacion = 0; //se asume exito por ahora
                    }
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                case TAREA_MOSTRAR_EMPRESAS:
                    printf("Rank 1: Ejecutando mostrarEmpresas...\n");
                    mostrarEmpresas(conexion); //el trabajador imprime directamente
                    //enviar solo confirmación de ejecución
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 case TAREA_ELIMINAR_EMPRESA:
                     {
                         char idParaEliminar[10];
                         MPI_Recv(idParaEliminar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                         printf("Rank 1: Eliminando empresa ID %s...\n", idParaEliminar);
                         // eliminarEmpresa(conexion); 
                         printf("Rank 1: (Simulado) Empresa eliminada o no encontrada.\n");
                         resultadoOperacion = 0; //se asume exito/completado
                     }
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;

                //SUPERVISORES
                 case TAREA_MOSTRAR_SUPERVISORES:
                    printf("Rank 1: Ejecutando mostrarSupervisores...\n");
                    mostrarSupervisores(conexion); //el trabajador imprime
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;

                //SOLICITUDES
                 case TAREA_MOSTRAR_SOLICITUDES:
                     {
                         char estadoSolicitado[20];
                         MPI_Recv(estadoSolicitado, 20, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                         printf("Rank 1: Mostrando solicitudes con estado '%s'...\n", estadoSolicitado);
                         // mostrarSolicitudesPorEstado(conexion); 
                         printf("Rank 1: (Simulado) Mostrando solicitudes.\n");
                          resultadoOperacion = 0; //se asume exito
                     }
                     MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                default:
                    fprintf(stderr, "Rank 1: Tarea desconocida recibida: %d\n", tareaRecibida);
                    resultadoOperacion = -1; //indicar error
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
            }
        } //fin while(true) trabajador

    } else {
        //ranks > 1 inactivos por ahora
        printf("Rank %d: Inactivo.\n", rank);
    }

    //sincronización y finalización
    MPI_Barrier(MPI_COMM_WORLD); //esperar a que todos lleguen aquí

    //desconexión de la Base de Datos (cada rank)
    if (conexion != NULL) {
        printf("Rank %d: Desconectando de la BD...\n", rank);
        desconectarDB(conexion);
    }

    //finalización de MPI
    printf("Rank %d: Finalizando MPI...\n", rank);
    MPI_Finalize();

    return 0;
}


//procesa opciones del menú principal (EJECUTADO SOLO EN RANK 0)
void procesarOpcionPrincipal(int opcion, PGconn* conn_rank0, int commSize, int rank0) {
    //nota: conn_rank0 no se usa para operaciones delegadas
    char opcionSubMenuStr[5];
    int opcionSubMenu;
    int tareaAEnviar;
    int resultadoRecibido;
    double tInicio, tFin; //para medir el tiempo

    //dterminar el rank del trabajador (simple: rank 1 si existe)
    int rankTrabajador = (commSize > 1) ? 1 : 0; //si solo hay 1 proceso,entonces rannk 0 hace todo


    switch (opcion) {
        case 1: //empresas
            do {
                mostrarMenuEmpresas(); //rank 0 muestra menú
                leerEntrada("Seleccione una opción del menú Empresas:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 5) break; //volver

                 if (rankTrabajador == 0) { //EJECUCIÓN LOCAL (NP=1) 
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionEmpresas(opcionSubMenu, conn_rank0); //llamada directa
                     tFin = MPI_Wtime();
                     printf("Tiempo de ejecución local: %.4f segundos\n", tFin - tInicio);
                 } else { //DELEGAR A RANK 1 (NP > 1)
                     bool tareaValida = true;
                     switch(opcionSubMenu) {
                         case 1: tareaAEnviar = TAREA_REGISTRAR_EMPRESA;
                                //recolectar datos en Rank 0
                                {
                                    Empresa emp;
                                    printf("--- Registro de Nueva Empresa (Datos) ---\n");
                                    leerEntrada("Nombre:", emp.nombre, sizeof(emp.nombre), noEsVacio);
                                    leerEntrada("Dirección:", emp.direccion, sizeof(emp.direccion), noEsVacio);
                                    leerEntrada("Teléfono:", emp.telefono, sizeof(emp.telefono), sonSoloNumeros);
                                    leerEntrada("Correo:", emp.correo, sizeof(emp.correo), esCorreoValido);
                                    leerEntrada("RFC:", emp.rfc, sizeof(emp.rfc), esRFCValido);
                                    leerEntrada("Contacto:", emp.contacto_encargado, sizeof(emp.contacto_encargado), sonSoloLetras);
                                    //enviar tarea y luego datos
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(&emp, sizeof(Empresa), MPI_BYTE, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                }
                                break;
                         case 2: tareaAEnviar = TAREA_MOSTRAR_EMPRESAS;
                                 tInicio = MPI_Wtime();
                                 MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                 break;
                         case 3: tareaAEnviar = TAREA_ACTUALIZAR_EMPRESA;
                                 //rank 0 pide ID, campo y valor, envía todo
                                 printf("Funcionalidad ACTUALIZAR EMPRESA vía MPI pendiente de implementación detallada (envío de datos).\n");
                                 tareaValida = false; //no enviar tarea incompleta
                                 break;
                         case 4: tareaAEnviar = TAREA_ELIMINAR_EMPRESA;
                                 {
                                     char idEliminar[10];
                                     char confirmacion[5];
                                     printf("--- Eliminar Empresa ---\n");
                                     // mostrarEmpresas(); // como mostrar desde aquí si rank 1 lo hace? Complicado, omitir por ahora
                                     leerEntrada("ID a eliminar:", idEliminar, sizeof(idEliminar), esNumeroEnteroPositivoValido);
                                     printf("Confirmar eliminación (s/n):");
                                     fgets(confirmacion, sizeof(confirmacion), stdin); //lectura simple
                                     confirmacion[strcspn(confirmacion, "\n")] = '\0';
                                     int c; while ((c = getchar()) != '\n' && c != EOF); //limpiar buffer

                                     if (tolower(confirmacion[0]) == 's') {
                                          tInicio = MPI_Wtime();
                                          MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                          MPI_Send(idEliminar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                     } else {
                                         printf("Cancelado.\n");
                                         tareaValida = false;
                                     }
                                 }
                                 break;
                         default: printf("Opción inválida.\n"); tareaValida = false; break;
                     }

                     if (tareaValida) {
                        //esperar resultado del trabajador
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        if (resultadoRecibido == 0) {
                            printf("Rank 0: Operación completada por Rank %d.\n", rankTrabajador);
                        } else {
                            printf("Rank 0: Rank %d reportó un error en la operación.\n", rankTrabajador);
                        }
                        printf("Tiempo de operación (MPI Send/Recv + Ejecución Rank %d): %.4f segundos\n", rankTrabajador, tFin - tInicio);
                     }
                 } // fin else (delegar)

                 //pausa dentro del submenú (solo rank 0)
                 if (opcionSubMenu != 5) {
                     printf("\n(Rank 0) Presione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                 }

            } while (opcionSubMenu != 5);
            break; //fin case 1(Empresas)

        case 2: // Supervisores
            do {
                mostrarMenuSupervisores();
                leerEntrada("Seleccione una opción del menú Supervisores:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                 if (opcionSubMenu == 5) break; //volver

                 if (rankTrabajador == 0) { //EJECUCIÓN LOCAL (NP=1)
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionSupervisores(opcionSubMenu, conn_rank0); //llamada directa
                     tFin = MPI_Wtime();
                     printf("Tiempo de ejecución local: %.4f segundos\n", tFin - tInicio);
                 } else { //DELEGAR A RANK 1 (NP > 1)
                      bool tareaValida = true;
                     switch(opcionSubMenu) {
                         // case 1: TAREA_REGISTRAR_SUPERVISOR 
                         //pendiente por ahora
                         case 2: tareaAEnviar = TAREA_MOSTRAR_SUPERVISORES;
                                 tInicio = MPI_Wtime();
                                 MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                 break;
                         // case 3: TAREA_ACTUALIZAR_SUPERVISOR 
                                 //pendiente por ahora
                         // case 4: TAREA_ELIMINAR_SUPERVISOR
                                 //pendiente por ahora
                         default: printf("Opción inválida o pendiente MPI.\n"); tareaValida = false; break;
                     }
                      if (tareaValida) {
                         //esperar resultado del trabajador
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        if (resultadoRecibido == 0) printf("Rank 0: Operación completada por Rank %d.\n", rankTrabajador);
                        else printf("Rank 0: Rank %d reportó un error.\n", rankTrabajador);
                        printf("Tiempo de operación (MPI Send/Recv + Ejecución Rank %d): %.4f segundos\n", rankTrabajador, tFin - tInicio);
                      }
                 }
                  if (opcionSubMenu != 5) {
                     printf("\n(Rank 0) Presione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                 }
            } while (opcionSubMenu != 5);
            break; //fin case 2 (Supervisores)

        case 3: // Solicitudes
            do {
                mostrarMenuSolicitudes();
                 leerEntrada("Seleccione una opción del menú Solicitudes:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 4) break; //volver

                if (rankTrabajador == 0) { //EJECUCIÓN LOCAL (NP=1)
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionSolicitudes(opcionSubMenu, conn_rank0); //llamada directa
                     tFin = MPI_Wtime();
                     printf("Tiempo de ejecución local: %.4f segundos\n", tFin - tInicio);
                } else { //DELEGAR A RANK 1 (NP > 1)
                    bool tareaValida = true;
                     switch(opcionSubMenu) {
                         // case 1: TAREA_REGISTRAR_SOLICITUD 
                         //pendiente por ahora
                         case 2: tareaAEnviar = TAREA_MOSTRAR_SOLICITUDES;
                                 {
                                     char estado[20];
                                     printf("--- Mostrar Solicitudes por Estado ---\n");
                                     printf("[1] APERTURADO [2] ACEPTADO [3] CANCELADO\n");
                                     char opEstadoStr[5];
                                     int opEstado;
                                     leerEntrada("Estado a mostrar:", opEstadoStr, sizeof(opEstadoStr), esNumeroEnteroPositivoValido);
                                     opEstado = atoi(opEstadoStr);
                                     if (opEstado == 1) strcpy(estado, "APERTURADO");
                                     else if (opEstado == 2) strcpy(estado, "ACEPTADO");
                                     else if (opEstado == 3) strcpy(estado, "CANCELADO");
                                     else { printf("Estado inválido.\n"); tareaValida = false; break;}

                                     tInicio = MPI_Wtime();
                                     MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                     MPI_Send(estado, 20, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                 }
                                 break;
                         // case 3: TAREA_CANCELAR_SOLICITUD, pendiente
                         default: printf("Opción inválida o pendiente MPI.\n"); tareaValida = false; break;
                     }
                     if (tareaValida) {
                          //se espera el resultado del trabajador
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                         if (resultadoRecibido == 0) printf("Rank 0: Operación completada por Rank %d.\n", rankTrabajador);
                        else printf("Rank 0: Rank %d reportó un error.\n", rankTrabajador);
                        printf("Tiempo de operación (MPI Send/Recv + Ejecución Rank %d): %.4f segundos\n", rankTrabajador, tFin - tInicio);
                     }
                }
                 if (opcionSubMenu != 4) {
                     printf("\n(Rank 0) Presione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                 }
            } while (opcionSubMenu != 4);
            break; //finn case 3 (Solicitudes)

        case 4: //proyectos aceptados (Pendiente)
        case 5: //reportes (Pendiente)
            printf("Funcionalidad aún no implementada.\n");
            break;
        //caso 6 (salir), se maneja en el bucle principal de Rank 0
        default:
            // ya se maneja en el bucle principal si la opcion inicial no es valida
            break;
    }
}


//funciones de procesamiento de submenus (AHORA SOLO PARA EJECUCIÓN LOCAL SI NP=1) ---
// (estas funciones ya NO son llamadas por Rank 0 si NP > 1)

void procesarOpcionEmpresas(int opcion, PGconn *conn) {
     //esta funcion ahora solo se usa si rank == 0 y size == 1
    switch (opcion) {
        case 1: registrarEmpresa(conn); break;
        case 2: mostrarEmpresas(conn); break;
        case 3: actualizarEmpresa(conn); break;
        case 4: eliminarEmpresa(conn); break;
        default: printf("Opción local inválida.\n"); break;
    }
}

void procesarOpcionSupervisores(int opcion, PGconn *conn) {
     //esta funcion ahora solo se usa si rank == 0 y size == 1
     switch (opcion) {
        case 1: registrarSupervisor(conn); break;
        case 2: mostrarSupervisores(conn); break;
        case 3: actualizarSupervisor(conn); break;
        case 4: eliminarSupervisor(conn); break;
        default: printf("Opción local inválida.\n"); break;
    }
}

void procesarOpcionSolicitudes(int opcion, PGconn *conn) {
     //esta funcion ahora solo se usa si rank == 0 y size == 1
     switch (opcion) {
        case 1: registrarSolicitud(conn); break;
        case 2: mostrarSolicitudesPorEstado(conn); break;
        case 3: cancelarSolicitud(conn); break;
        default: printf("Opción local inválida o pendiente.\n"); break;
    }
}