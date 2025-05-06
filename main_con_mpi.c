#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h> // para tolower
#include <unistd.h> 

#include "base_de_datos.h"
#include "lectura.h"
#include "validacion.h"
#include "structs.h"
#include "empresa.h"
#include "supervisor.h"
#include "solicitud.h" 
#include "proyecto.h"
#include "reportes.h" 

//codigos de tareas para MPI
#define TAG_TAREA 1
#define TAG_RESULTADO 2
#define TAG_DATOS 3
#define TAG_DATOS_EXTRA1 4 
#define TAG_DATOS_EXTRA2 5
#define TAG_DATOS_EXTRA3 6
#define TAG_DATOS_EXTRA4 7 
#define TAG_DATOS_EXTRA5 8
#define TAG_DATOS_EXTRA6 9

#define TAREA_TERMINAR 0

#define TAREA_REGISTRAR_EMPRESA 1
#define TAREA_MOSTRAR_EMPRESAS 2
#define TAREA_ACTUALIZAR_EMPRESA 3
#define TAREA_ELIMINAR_EMPRESA 4

#define TAREA_REGISTRAR_SUPERVISOR 11
#define TAREA_MOSTRAR_SUPERVISORES 12
#define TAREA_ACTUALIZAR_SUPERVISOR 13
#define TAREA_ELIMINAR_SUPERVISOR 14

#define TAREA_REGISTRAR_SOLICITUD 21
#define TAREA_MOSTRAR_SOLICITUDES 22 
#define TAREA_CANCELAR_SOLICITUD 23
#define TAREA_ACEPTAR_SOLICITUD 24

#define TAREA_MOSTRAR_PROYECTOS 31   
#define TAREA_ACTUALIZAR_AVANCE 32
#define TAREA_ACTUALIZAR_PRIORIDAD 33
#define TAREA_TERMINAR_PROYECTO 34
#define TAREA_REASIGNAR_SUPERVISOR 35

#define TAREA_REPORTE_PROYECTOS_PERIODO 41
#define TAREA_REPORTE_PROYECTOS_AVANCE_50 42
#define TAREA_REPORTE_EMPRESAS_CANCELADAS 43
#define TAREA_REPORTE_SUPERVISOR_AVANCE_20 44
#define TAREA_REPORTE_PROYECTOS_GENERAL 45


//prototipos de funciones de menu
void mostrarMenuPrincipal();
void mostrarMenuEmpresas();
void mostrarMenuSupervisores();
void mostrarMenuSolicitudes(); 
void mostrarMenuProyectosAceptados(); 
void mostrarMenuReportes();

//prototipos de procesamiento de opciones
void procesarOpcionPrincipal(int opcion, PGconn *conn_rank0, int commSize, int rank0, int *contadorTareas);
//versiones locales para np=1
void procesarOpcionEmpresasLocal(int opcion, PGconn *conn);
void procesarOpcionSupervisoresLocal(int opcion, PGconn *conn);
void procesarOpcionSolicitudesLocal(int opcion, PGconn *conn); 
void procesarOpcionProyectosAceptadosLocal(int opcion, PGconn *conn);
void procesarOpcionReportesLocal(int opcion, PGconn *conn);


//funciones de menu
void mostrarMenuPrincipal() {
    printf("\n===== CONSTRUCTORA LA ROCA BONITA - MENÚ PRINCIPAL (MPI) =====\n");
    printf("[1] Gestionar Empresas\n");
    printf("[2] Gestionar Supervisores\n");
    printf("[3] Gestionar Solicitudes de Proyecto\n");
    printf("[4] Gestionar Proyectos Aceptados\n");
    printf("[5] Generar Reportes (Pendiente)\n");
    printf("[6] Salir\n");
    printf("======================================================\n");
    fflush(stdout);
}

void mostrarMenuEmpresas() {
    printf("\n--- Menú Empresas ---\n");
    printf("[1] Registrar Nueva Empresa\n");
    printf("[2] Mostrar Todas las Empresas\n");
    printf("[3] Actualizar Datos de Empresa\n");
    printf("[4] Eliminar Empresa\n");
    printf("[5] Volver al Menú Principal\n");
    printf("---------------------\n");
    fflush(stdout);
}

void mostrarMenuSupervisores() {
    printf("\n--- Menú Supervisores ---\n");
    printf("[1] Registrar Nuevo Supervisor\n");
    printf("[2] Mostrar Todos los Supervisores\n");
    printf("[3] Actualizar Datos de Supervisor\n");
    printf("[4] Eliminar Supervisor\n");
    printf("[5] Volver al Menú Principal\n");
    printf("-------------------------\n");
    fflush(stdout);
}

void mostrarMenuSolicitudes() {
    printf("\n--- Menú Solicitudes de Proyecto ---\n");
    printf("[1] Registrar Nueva Solicitud\n");
    printf("[2] Mostrar Solicitudes por Estado\n");
    printf("[3] Cancelar Solicitud (Estado APERTURADO)\n");
    printf("[4] Aceptar Solicitud (Crear Proyecto)\n");
    printf("[5] Volver al Menú Principal\n"); 
    printf("------------------------------------\n");
    fflush(stdout);
}

void mostrarMenuProyectosAceptados() {
    printf("\n--- Menú Proyectos Aceptados ---\n");
    printf("[1] Mostrar Proyectos (con filtros)\n");
    printf("[2] Actualizar Avance de Proyecto\n");
    printf("[3] Actualizar Prioridad de Proyecto\n");
    printf("[4] Reasignar Supervisor a Proyecto\n");
    printf("[5] Marcar Proyecto como Terminado\n");
    printf("[6] Volver al Menú Principal\n"); 
    printf("--------------------------------\n");
    fflush(stdout);
}

void mostrarMenuReportes() {
    printf("\n--- Menú Reportes ---\n");
    printf("[1] Proyectos Aceptados por Periodo\n");
    printf("[2] Proyectos con Avance > 50%%\n");
    printf("[3] Empresas con Solicitudes Canceladas\n");
    printf("[4] Supervisores (Proyectos Activos Avance < 20%%)\n");
    printf("[5] Listado General de Proyectos Aceptados\n");
    printf("[6] Volver al Menú Principal\n"); 
    printf("-----------------------\n");
    fflush(stdout);
}

//función principal
int main(int argc, char *argv[]) {
    int rank, size;
    PGconn *conexion = NULL;
    bool salir = false;
    MPI_Status status;
    int contadorTareas = 0; //para distribuir tareas usando el algoritmo round robin

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //cada rank establece su propia conexion
    conexion = conectarDB(); 
    if (conexion == NULL) {
        fprintf(stderr, "Rank %d: Fallo al conectar a la BD. Abortando.\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);//si hay un error en alguna conexion se aborta el programa
    }
    // if (rank == 0) { printf("Rank 0: Conexión BD OK.\n"); }


    //logica principal dividida por ranks
    if (rank == 0) {
        //rank 0: coordinador / UI
        if(size > 1) {
            printf("Coordinador (Rank 0) iniciado. Trabajadores activos: %d\n", size - 1);
        } else {
            printf("Ejecutando en modo local (1 solo proceso).\n");
        }
        char opcionStr[5];
        int opcionSeleccionada;

        while (!salir) {
            mostrarMenuPrincipal();
            fflush(stdout);
            leerEntrada("Seleccione una opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
            opcionSeleccionada = atoi(opcionStr);

            if (opcionSeleccionada == 6) { //salir
                salir = true;
                if (size > 1) {
                    int tareaTerminar = TAREA_TERMINAR;
                    // printf("Rank 0: Enviando TAREA_TERMINAR a todos los trabajadores...\n"); //debug
                    for(int i = 1; i < size; i++) {
                        MPI_Send(&tareaTerminar, 1, MPI_INT, i, TAG_TAREA, MPI_COMM_WORLD);
                    }
                }
            } else if (opcionSeleccionada > 0 && opcionSeleccionada < 6) {
                //llamar a la función que procesa la opción (esta manejará si es local o MPI)
                procesarOpcionPrincipal(opcionSeleccionada, conexion, size, rank, &contadorTareas);
            } else {
                 printf("Opción no válida. Por favor, intente de nuevo.\n");
            }

            //pausa simple para ver resultados (solo en rank 0)
            if (!salir && opcionSeleccionada != 6) {
                 printf("\n(Rank 0) Presione Enter para continuar...");
                 fflush(stdout);
                 //se impia el buffer antes de esperar el enter
                 int c; while ((c = getchar()) != '\n' && c != EOF);
                 // getchar(); 
            }
        } // fin while (!salir) rank 0

    } else { //ranks > 0: Trabajadores
        // printf("Trabajador (Rank %d) iniciado y esperando tareas...\n", rank); //debug
        int tareaRecibida;
        int resultadoOperacion = 0; // Código de resultado a devolver

        while (true) {
            //se espera una tarea del Rank 0
            MPI_Recv(&tareaRecibida, 1, MPI_INT, 0, TAG_TAREA, MPI_COMM_WORLD, &status);
            // printf("Rank %d: Tarea recibida = %d\n", rank, tareaRecibida); //debug

            resultadoOperacion = 0; //se resetea el resultado

            if (tareaRecibida == TAREA_TERMINAR) {
                // printf("Rank %d: Recibida señal de terminar. Saliendo...\n", rank); 
                break; //salir del bucle while(true) del trabajador
            }

            //se ejecuta la tarea solicitada
            switch (tareaRecibida) {
                case TAREA_REGISTRAR_EMPRESA: {
                    Empresa empRecibida;
                    MPI_Recv(&empRecibida, sizeof(Empresa), MPI_BYTE, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarRegistrarEmpresaDB(conexion, &empRecibida);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                case TAREA_MOSTRAR_EMPRESAS: {
                    mostrarEmpresas(conexion);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                }
                case TAREA_ACTUALIZAR_EMPRESA: {
                    char idRecibido[10];
                    char campoRecibido[30];
                    char valorRecibido[151];
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

                 case TAREA_REGISTRAR_SUPERVISOR: {
                    Supervisor supRecibido;
                    MPI_Recv(&supRecibido, sizeof(Supervisor), MPI_BYTE, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarRegistrarSupervisorDB(conexion, &supRecibido);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_MOSTRAR_SUPERVISORES: {
                    mostrarSupervisores(conexion);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_ACTUALIZAR_SUPERVISOR: {
                    char idSupRecibido[10];
                    char campoSupRecibido[30];
                    char valorSupRecibido[101];
                    MPI_Recv(idSupRecibido, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(campoSupRecibido, 30, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    MPI_Recv(valorSupRecibido, 101, MPI_CHAR, 0, TAG_DATOS_EXTRA2, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarActualizarCampoSupervisorDB(conexion, idSupRecibido, campoSupRecibido, valorSupRecibido);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_ELIMINAR_SUPERVISOR: {
                    char idSupParaEliminar[10];
                    MPI_Recv(idSupParaEliminar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarEliminarSupervisorDB(conexion, idSupParaEliminar);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }

                 case TAREA_REGISTRAR_SOLICITUD: {
                    char idEmpresaRecibido[10];
                    char fechaRecibida[12];
                    char presupuestoRecibido[20];
                    char anticipoRecibido[20];
                    char folioRecibido[51];
                    MPI_Recv(idEmpresaRecibido, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(fechaRecibida, 12, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    MPI_Recv(presupuestoRecibido, 20, MPI_CHAR, 0, TAG_DATOS_EXTRA2, MPI_COMM_WORLD, &status);
                    MPI_Recv(anticipoRecibido, 20, MPI_CHAR, 0, TAG_DATOS_EXTRA3, MPI_COMM_WORLD, &status);
                    MPI_Recv(folioRecibido, 51, MPI_CHAR, 0, TAG_DATOS_EXTRA4, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarRegistrarSolicitudDB(conexion, idEmpresaRecibido, fechaRecibida, presupuestoRecibido, anticipoRecibido, folioRecibido);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_MOSTRAR_SOLICITUDES: {
                    char estadoRecibido[15]; 
                    MPI_Recv(estadoRecibido, 15, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarMostrarSolicitudesPorEstadoDB(conexion, estadoRecibido); //el trabajador imprime
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD); //envía ack
                    break;
                 }
                 case TAREA_CANCELAR_SOLICITUD: {
                    char idSolCancelar[10];
                    char razonCancelar[256]; 
                    MPI_Recv(idSolCancelar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(razonCancelar, 256, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarCancelarSolicitudDB(conexion, idSolCancelar, razonCancelar);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_ACEPTAR_SOLICITUD: {
                    char idSolAceptar[10];
                    ProyectoAceptado proyRecibido; //para recibir struct completo
                    char montoProyRecibido[20];
                    char idSupRecibido[10];
                    char prioridadRecibida[10];

                    MPI_Recv(idSolAceptar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(&proyRecibido, sizeof(ProyectoAceptado), MPI_BYTE, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    MPI_Recv(montoProyRecibido, 20, MPI_CHAR, 0, TAG_DATOS_EXTRA2, MPI_COMM_WORLD, &status);
                    MPI_Recv(idSupRecibido, 10, MPI_CHAR, 0, TAG_DATOS_EXTRA3, MPI_COMM_WORLD, &status);
                    MPI_Recv(prioridadRecibida, 10, MPI_CHAR, 0, TAG_DATOS_EXTRA4, MPI_COMM_WORLD, &status);

                    resultadoOperacion = ejecutarAceptarSolicitudDB(conexion, idSolAceptar, &proyRecibido, montoProyRecibido, idSupRecibido, prioridadRecibida);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }

                 case TAREA_MOSTRAR_PROYECTOS: {
                    char filtroRecibido[15]; 
                    MPI_Recv(filtroRecibido, 15, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    //si la cadena recibida está vacía, se pasa NULL a la función ejecutora
                    const char* filtroReal = (strlen(filtroRecibido) > 0) ? filtroRecibido : NULL;
                    resultadoOperacion = ejecutarMostrarProyectosAceptadosDB(conexion, filtroReal);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_ACTUALIZAR_AVANCE: {
                    char idProyRecibido[10];
                    char porcentajeRecibido[10];
                    MPI_Recv(idProyRecibido, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(porcentajeRecibido, 10, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarActualizarAvanceDB(conexion, idProyRecibido, porcentajeRecibido);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_ACTUALIZAR_PRIORIDAD: {
                    char idProyRecibido[10];
                    char prioridadRecibida[10]; // "ROJO", etc.
                    MPI_Recv(idProyRecibido, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(prioridadRecibida, 10, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarActualizarPrioridadDB(conexion, idProyRecibido, prioridadRecibida);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_TERMINAR_PROYECTO: {
                    char idProyTerminar[10];
                    MPI_Recv(idProyTerminar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarTerminarProyectoDB(conexion, idProyTerminar);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_REASIGNAR_SUPERVISOR: {
                    char idProyReasignar[10];
                    char idNuevoSup[10];
                    MPI_Recv(idProyReasignar, 10, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(idNuevoSup, 10, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    resultadoOperacion = ejecutarReasignarSupervisorDB(conexion, idProyReasignar, idNuevoSup);
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }

                 case TAREA_REPORTE_PROYECTOS_PERIODO: {
                    char fechaInicioRecibida[12];
                    char fechaFinRecibida[12];
                    MPI_Recv(fechaInicioRecibida, 12, MPI_CHAR, 0, TAG_DATOS, MPI_COMM_WORLD, &status);
                    MPI_Recv(fechaFinRecibida, 12, MPI_CHAR, 0, TAG_DATOS_EXTRA1, MPI_COMM_WORLD, &status);
                    reporteProyectosPorPeriodo(conexion, fechaInicioRecibida, fechaFinRecibida);
                    resultadoOperacion = 0; //se asume exito si la función no crashea
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_REPORTE_PROYECTOS_AVANCE_50: {
                    // No necesita parámetros
                    reporteProyectosAvanceMayor50(conexion);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_REPORTE_EMPRESAS_CANCELADAS: {
                     //no necesita parámetros
                    reporteEmpresasCanceladas(conexion);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_REPORTE_SUPERVISOR_AVANCE_20: {
                     //no necesita parámetros
                    reporteSupervisorAvanceMenor20(conexion);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                 case TAREA_REPORTE_PROYECTOS_GENERAL: {
                     //no necesita parámetros
                    reporteProyectosAceptadosGeneral(conexion);
                    resultadoOperacion = 0;
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
                 }
                default:
                    fprintf(stderr, "Rank %d: Tarea desconocida recibida: %d\n", rank, tareaRecibida);
                    resultadoOperacion = -1; //indica error
                    MPI_Send(&resultadoOperacion, 1, MPI_INT, 0, TAG_RESULTADO, MPI_COMM_WORLD);
                    break;
            } // fin switch(tareaRecibida)
        } //fin while(true) trabajador

    } //fin else (trabajadores)

    //se sincroniza y finaliza
    MPI_Barrier(MPI_COMM_WORLD);

    if (conexion != NULL) {
        desconectarDB(conexion);
    }

    MPI_Finalize();

    return 0;
}


//implementación de funciones de procesamiento de opciones
//aqui se procesan las opciones del menu principal (EJECUTADO SOLO EN RANK 0)
void procesarOpcionPrincipal(int opcion, PGconn* conn_rank0, int commSize, int rank0_siempre_cero, int *contadorTareas) {
    char opcionSubMenuStr[5];
    int opcionSubMenu;
    int tareaAEnviar;
    int resultadoRecibido;
    double tInicio=0.0, tFin=0.0; //para medir tiempo

    int rankTrabajador = 0;
    if (commSize > 1) {
        rankTrabajador = (*contadorTareas % (commSize - 1)) + 1;
        (*contadorTareas)++;
    }

    switch (opcion) {
        case 1: //empresas (ya implementado)
            do {
                mostrarMenuEmpresas();
                leerEntrada("Seleccione opción Empresas:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 5) break;

                 if (rankTrabajador == 0) { // NP=1
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionEmpresasLocal(opcionSubMenu, conn_rank0);
                     tFin = MPI_Wtime();
                     printf("Tiempo ejecución local: %.4f seg\n", tFin - tInicio);
                 } else { // NP > 1
                     bool tareaValidaParaEnviar = true;
                     switch(opcionSubMenu) {
                         case 1: {
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
                                break;
                            }
                         case 2: {
                                tareaAEnviar = TAREA_MOSTRAR_EMPRESAS;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                break;
                            }
                         case 3: {
                                tareaAEnviar = TAREA_ACTUALIZAR_EMPRESA;
                                char idActualizar[10], campoActualizar[30], valorActualizar[151], opCampoStr[5];
                                int opCampo;
                                printf("--- Actualizar Empresa (Datos) ---\n");
                                leerEntrada("ID empresa a actualizar:", idActualizar, sizeof(idActualizar), esNumeroEnteroPositivoValido);
                                printf("Campo: [1]Nombre [2]Dirección [3]Teléfono [4]Correo [5]RFC [6]Contacto\n");
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
                         case 4: {
                                tareaAEnviar = TAREA_ELIMINAR_EMPRESA;
                                char idEliminar[10], confirmacion[5];
                                printf("--- Eliminar Empresa ---\n");
                                leerEntrada("ID a eliminar:", idEliminar, sizeof(idEliminar), esNumeroEnteroPositivoValido);
                                int c; while ((c = getchar()) != '\n' && c != EOF); //limpiar el buffer
                                printf("Confirmar eliminación (s/n):\n"); fflush(stdout);
                                if (fgets(confirmacion, sizeof(confirmacion), stdin) != NULL) {
                                    confirmacion[strcspn(confirmacion, "\n")] = '\0';
                                    int len = strlen(confirmacion);
                                    if (len == sizeof(confirmacion) - 1 && confirmacion[len-1] != '\0') { while ((c = getchar()) != '\n' && c != EOF); }
                                    else if (len == 0) { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }
                                } else { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }
                                if (tolower(confirmacion[0]) == 's') {
                                        tInicio = MPI_Wtime();
                                        MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                        MPI_Send(idEliminar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                } else { printf("Cancelado.\n"); tareaValidaParaEnviar = false; }
                                break;
                            }
                         default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                     }
                     if (tareaValidaParaEnviar) {
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        switch(resultadoRecibido) {
                            case 0: printf("Rank 0: Operación completada exitosamente por Rank %d.\n", rankTrabajador); break;
                            case -1: printf("Rank 0: Rank %d reportó un ERROR de base de datos.\n", rankTrabajador); break;
                            case -2: printf("Rank 0: Operación ejecutada por Rank %d, pero no afectó filas (¿ID no encontrado?).\n", rankTrabajador); break;
                            default: printf("Rank 0: Rank %d reportó un resultado desconocido (%d).\n", rankTrabajador, resultadoRecibido); break;
                        }
                        if(opcionSubMenu != 2) printf("Tiempo de operación Empresa: %.4f seg\n", tFin - tInicio);
                        else printf("Tiempo de coordinación 'Mostrar': %.4f seg\n", tFin - tInicio);
                     }
                 }
                 if (opcionSubMenu != 5) { printf("\n(Rank 0) Presione Enter..."); fflush(stdout); int c; while ((c = getchar()) != '\n' && c != EOF); }
            } while (opcionSubMenu != 5);
            break;

        case 2: //supervisores
             do {
                mostrarMenuSupervisores();
                leerEntrada("Seleccione opción Supervisores:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 5) break;

                 if (rankTrabajador == 0) { // NP=1
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionSupervisoresLocal(opcionSubMenu, conn_rank0);
                     tFin = MPI_Wtime();
                     printf("Tiempo ejecución local: %.4f seg\n", tFin - tInicio);
                 } else { // NP > 1
                     bool tareaValidaParaEnviar = true;
                     switch(opcionSubMenu) {
                         case 1: {
                                tareaAEnviar = TAREA_REGISTRAR_SUPERVISOR;
                                Supervisor sup;
                                printf("--- Registro Supervisor (Datos) ---\n");
                                leerEntrada("Nombre(s):", sup.nombre, sizeof(sup.nombre), sonSoloLetras);
                                leerEntrada("Apellidos:", sup.apellidos, sizeof(sup.apellidos), sonSoloLetras);
                                leerEntrada("Teléfono:", sup.telefono, sizeof(sup.telefono), sonSoloNumeros);
                                leerEntrada("Correo:", sup.correo, sizeof(sup.correo), esCorreoValido);
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(&sup, sizeof(Supervisor), MPI_BYTE, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                break;
                            }
                         case 2: {
                                tareaAEnviar = TAREA_MOSTRAR_SUPERVISORES;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                break;
                            }
                         case 3: {
                                tareaAEnviar = TAREA_ACTUALIZAR_SUPERVISOR;
                                char idSupActualizar[10], campoSupActualizar[30], valorSupActualizar[101], opCampoStr[5];
                                int opCampo;
                                printf("--- Actualizar Supervisor (Datos) ---\n");
                                leerEntrada("ID supervisor a actualizar:", idSupActualizar, sizeof(idSupActualizar), esNumeroEnteroPositivoValido);
                                printf("Campo: [1]Nombre(s) [2]Apellidos [3]Teléfono [4]Correo\n");
                                leerEntrada("Opción Campo:", opCampoStr, sizeof(opCampoStr), esNumeroEnteroPositivoValido);
                                opCampo = atoi(opCampoStr);
                                bool (*validadorCampo)(const char*) = NULL;
                                switch(opCampo) {
                                    case 1: strcpy(campoSupActualizar, "nombre"); validadorCampo = sonSoloLetras; break;
                                    case 2: strcpy(campoSupActualizar, "apellidos"); validadorCampo = sonSoloLetras; break;
                                    case 3: strcpy(campoSupActualizar, "telefono"); validadorCampo = sonSoloNumeros; break;
                                    case 4: strcpy(campoSupActualizar, "correo"); validadorCampo = esCorreoValido; break;
                                    default: printf("Opción de campo inválida.\n"); tareaValidaParaEnviar = false; break;
                                }
                                if(tareaValidaParaEnviar) {
                                    leerEntrada("Nuevo Valor:", valorSupActualizar, sizeof(valorSupActualizar), validadorCampo);
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(idSupActualizar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                    MPI_Send(campoSupActualizar, 30, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                    MPI_Send(valorSupActualizar, 101, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA2, MPI_COMM_WORLD);
                                }
                                break;
                            }
                         case 4: {
                                tareaAEnviar = TAREA_ELIMINAR_SUPERVISOR;
                                char idSupEliminar[10], confirmacion[5];
                                printf("--- Eliminar Supervisor ---\n");
                                mostrarSupervisores(conn_rank0);
                                leerEntrada("ID a eliminar:", idSupEliminar, sizeof(idSupEliminar), esNumeroEnteroPositivoValido);
                                int c; while ((c = getchar()) != '\n' && c != EOF); // Limpiar buffer
                                printf("Confirmar eliminación (s/n):\n"); fflush(stdout);
                                if (fgets(confirmacion, sizeof(confirmacion), stdin) != NULL) {
                                    confirmacion[strcspn(confirmacion, "\n")] = '\0';
                                    int len = strlen(confirmacion);
                                    if (len == sizeof(confirmacion) - 1 && confirmacion[len-1] != '\0') { while ((c = getchar()) != '\n' && c != EOF); }
                                    else if (len == 0) { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }
                                } else { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }
                                if (tolower(confirmacion[0]) == 's') {
                                        tInicio = MPI_Wtime();
                                        MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                        MPI_Send(idSupEliminar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                } else { printf("Cancelado.\n"); tareaValidaParaEnviar = false; }
                                break;
                            }
                         default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                     }
                     if (tareaValidaParaEnviar) {
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        switch(resultadoRecibido) {
                            case 0: printf("Rank 0: Operación de supervisor completada exitosamente por Rank %d.\n", rankTrabajador); break;
                            case -1: printf("Rank 0: Rank %d reportó un ERROR de base de datos para supervisor.\n", rankTrabajador); break;
                            case -2: printf("Rank 0: Operación de supervisor ejecutada por Rank %d, pero no afectó filas (¿ID no encontrado?).\n", rankTrabajador); break;
                            default: printf("Rank 0: Rank %d reportó un resultado desconocido para supervisor (%d).\n", rankTrabajador, resultadoRecibido); break;
                        }
                        if(opcionSubMenu != 2) printf("Tiempo de operación Supervisor: %.4f seg\n", tFin - tInicio);
                        else printf("Tiempo de coordinación 'Mostrar Supervisores': %.4f seg\n", tFin - tInicio);
                     }
                 }
                 if (opcionSubMenu != 5) { printf("\n(Rank 0) Presione Enter..."); fflush(stdout); int c; while ((c = getchar()) != '\n' && c != EOF); }
            } while (opcionSubMenu != 5);
            break;

        case 3: //solicitudes 
             do {
                mostrarMenuSolicitudes(); //rank 0 muestra menú
                leerEntrada("Seleccione opción Solicitudes:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 5) break; //volver (ahora es 5)

                 if (rankTrabajador == 0) { // NP=1
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionSolicitudesLocal(opcionSubMenu, conn_rank0); //llama a la función local
                     tFin = MPI_Wtime();
                     printf("Tiempo ejecución local: %.4f seg\n", tFin - tInicio);
                 } else { // NP > 1
                     bool tareaValidaParaEnviar = true;
                     switch(opcionSubMenu) {
                         case 1: { //registrar solicitud
                                tareaAEnviar = TAREA_REGISTRAR_SOLICITUD;
                                char idEmpresaStr[10], fechaSolStr[12], presupuestoStr[20], anticipoStr[20], folioStr[51];
                                printf("--- Registro Solicitud (Datos) ---\n");
                                printf("Seleccione la empresa:\n");
                                mostrarEmpresas(conn_rank0); //el rank 0 necesita mostrar las empresas para elegir ID
                                leerEntrada("ID Empresa:", idEmpresaStr, sizeof(idEmpresaStr), esNumeroEnteroPositivoValido);
                                leerEntrada("Fecha (YYYY-MM-DD):", fechaSolStr, sizeof(fechaSolStr), esFechaValida);
                                leerEntrada("Folio:", folioStr, sizeof(folioStr), noEsVacio);
                                leerEntrada("Presupuesto:", presupuestoStr, sizeof(presupuestoStr), esDecimalValido);
                                leerEntrada("Anticipo:", anticipoStr, sizeof(anticipoStr), esDecimalValido);

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(idEmpresaStr, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                MPI_Send(fechaSolStr, 12, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                MPI_Send(presupuestoStr, 20, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA2, MPI_COMM_WORLD);
                                MPI_Send(anticipoStr, 20, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA3, MPI_COMM_WORLD);
                                MPI_Send(folioStr, 51, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA4, MPI_COMM_WORLD);
                                break;
                            }
                         case 2: { //mostrar solicitudes por estado
                                tareaAEnviar = TAREA_MOSTRAR_SOLICITUDES;
                                char opEstadoStr[5];
                                char estadoFiltro[15]; // "APERTURADO", "ACEPTADO", "CANCELADO"
                                printf("--- Mostrar Solicitudes por Estado ---\n");
                                printf("[1] APERTURADO\n[2] ACEPTADO\n[3] CANCELADO\n");
                                leerEntrada("Seleccione estado:", opEstadoStr, sizeof(opEstadoStr), esNumeroEnteroPositivoValido);
                                int opEstado = atoi(opEstadoStr);
                                switch(opEstado) {
                                    case 1: strcpy(estadoFiltro, "APERTURADO"); break;
                                    case 2: strcpy(estadoFiltro, "ACEPTADO"); break;
                                    case 3: strcpy(estadoFiltro, "CANCELADO"); break;
                                    default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                                }
                                if (tareaValidaParaEnviar) {
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(estadoFiltro, 15, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                }
                                break;
                            }
                         case 3: { //cancelar solicitud
                                tareaAEnviar = TAREA_CANCELAR_SOLICITUD;
                                char idSolCancelar[10];
                                char razonCancelar[256];
                                printf("--- Cancelar Solicitud ---\n");
                                leerEntrada("ID Solicitud a cancelar:", idSolCancelar, sizeof(idSolCancelar), esNumeroEnteroPositivoValido);
                                leerEntrada("Razón de cancelación:", razonCancelar, sizeof(razonCancelar), noEsVacio);

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(idSolCancelar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                MPI_Send(razonCancelar, 256, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                break;
                            }
                          case 4: { //aceptar solicitud
                                tareaAEnviar = TAREA_ACEPTAR_SOLICITUD;
                                char idSolAceptar[10];
                                ProyectoAceptado datosProy; //struct para enviar
                                char montoProyStr[20];
                                char idSupStr[10];
                                char prioridadProyStr[10];
                                char opPrioridadStr[5];
                                int opPrioridad;

                                printf("--- Aceptar Solicitud (Crear Proyecto) ---\n");
                                leerEntrada("ID Solicitud a aceptar:", idSolAceptar, sizeof(idSolAceptar), esNumeroEnteroPositivoValido);
                                printf("--- Datos del Nuevo Proyecto ---\n");
                                leerEntrada("Nombre Proyecto:", datosProy.nombre_proyecto, sizeof(datosProy.nombre_proyecto), noEsVacio);
                                leerEntrada("Fecha Inicio (YYYY-MM-DD):", datosProy.fecha_inicio, sizeof(datosProy.fecha_inicio), esFechaValida);
                                leerEntrada("Fecha Fin (YYYY-MM-DD):", datosProy.fecha_fin, sizeof(datosProy.fecha_fin), esFechaValida);
                                leerEntrada("Monto Proyecto:", montoProyStr, sizeof(montoProyStr), esDecimalValido);
                                leerEntrada("Ubicación:", datosProy.ubicacion, sizeof(datosProy.ubicacion), noEsVacio);
                                printf("Descripción (opcional): "); fflush(stdout);
                                fgets(datosProy.descripcion, sizeof(datosProy.descripcion), stdin);
                                datosProy.descripcion[strcspn(datosProy.descripcion, "\n")] = '\0';
                                if (strlen(datosProy.descripcion) == sizeof(datosProy.descripcion) - 1 && datosProy.descripcion[sizeof(datosProy.descripcion)-2] != '\0') {
                                    int c; while ((c = getchar()) != '\n' && c != EOF);
                                }

                                printf("Seleccione Supervisor:\n");
                                mostrarSupervisores(conn_rank0); //rank 0 muestra supervisores
                                leerEntrada("ID Supervisor:", idSupStr, sizeof(idSupStr), esNumeroEnteroPositivoValido);

                                printf("Prioridad: [1]ROJO [2]NARANJA [3]AMARILLO\n");
                                leerEntrada("Opción Prioridad:", opPrioridadStr, sizeof(opPrioridadStr), esNumeroEnteroPositivoValido);
                                opPrioridad = atoi(opPrioridadStr);
                                switch(opPrioridad) {
                                    case 1: strcpy(prioridadProyStr, "ROJO"); break;
                                    case 2: strcpy(prioridadProyStr, "NARANJA"); break;
                                    case 3: strcpy(prioridadProyStr, "AMARILLO"); break;
                                    default: printf("Inválido, usando AMARILLO.\n"); strcpy(prioridadProyStr, "AMARILLO"); break;
                                }

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(idSolAceptar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                MPI_Send(&datosProy, sizeof(ProyectoAceptado), MPI_BYTE, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                MPI_Send(montoProyStr, 20, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA2, MPI_COMM_WORLD);
                                MPI_Send(idSupStr, 10, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA3, MPI_COMM_WORLD);
                                MPI_Send(prioridadProyStr, 10, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA4, MPI_COMM_WORLD);
                                break;
                            }
                         default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                     } //fin switch(opcionSubMenu) Solicitud

                     //se recibe el resultado del trabajador (si se envió tarea)
                     if (tareaValidaParaEnviar) {
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        //interpretar el resultado
                        switch(resultadoRecibido) {
                            case 0: printf("Rank 0: Operación de solicitud completada exitosamente por Rank %d.\n", rankTrabajador); break;
                            case -1: printf("Rank 0: Rank %d reportó un ERROR de base de datos para solicitud.\n", rankTrabajador); break;
                            case -2: printf("Rank 0: Operación de solicitud ejecutada por Rank %d, pero no afectó filas/condición no cumplida.\n", rankTrabajador); break;
                            default: printf("Rank 0: Rank %d reportó un resultado desconocido para solicitud (%d).\n", rankTrabajador, resultadoRecibido); break;
                        }
                        //mostrar tiempo
                        if(opcionSubMenu != 2) { //no se muestra tiempo detallado para 'Mostrar'
                            printf("Tiempo de operación Solicitud: %.4f seg\n", tFin - tInicio);
                        } else {
                            printf("Tiempo de coordinación 'Mostrar Solicitudes': %.4f seg\n", tFin - tInicio);
                        }
                     }
                 } //fin else (delegar solicitud)

                 //pausa dentro del submenú (solo rank 0)
                 if (opcionSubMenu != 5) { // 5 es volver
                     printf("\n(Rank 0) Presione Enter para continuar en Solicitudes...");
                     fflush(stdout);
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                 }

            } while (opcionSubMenu != 5);
            break; //fin case 3 (solicitudes)


        case 4: //proyectos sceptados
             do {
                mostrarMenuProyectosAceptados(); //rank 0 muestra el menu
                leerEntrada("Seleccione opción Proyectos:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 6) break; //volver 

                 if (rankTrabajador == 0) { // NP=1
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionProyectosAceptadosLocal(opcionSubMenu, conn_rank0); //llama a la función local
                     tFin = MPI_Wtime();
                     printf("Tiempo ejecución local: %.4f seg\n", tFin - tInicio);
                 } else { // NP > 1
                     bool tareaValidaParaEnviar = true;
                     switch(opcionSubMenu) {
                         case 1: { //mostrar proyectos (con filtro)
                                tareaAEnviar = TAREA_MOSTRAR_PROYECTOS;
                                char opFiltroStr[5];
                                char filtroEnviar[15] = ""; //cadena vacía por defecto (todos)
                                printf("--- Mostrar Proyectos por Estado ---\n");
                                printf("[1] EN PROCESO\n[2] TERMINADO\n[3] TODOS\n");
                                leerEntrada("Seleccione filtro:", opFiltroStr, sizeof(opFiltroStr), esNumeroEnteroPositivoValido);
                                int opFiltro = atoi(opFiltroStr);
                                switch(opFiltro) {
                                    case 1: strcpy(filtroEnviar, "EN_PROCESO"); break;
                                    case 2: strcpy(filtroEnviar, "TERMINADO"); break;
                                    case 3: /* filtroEnviar ya es "" */ break;
                                    default: printf("Opción inválida, mostrando todos.\n"); break;
                                }
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(filtroEnviar, 15, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                break;
                            }
                         case 2: { //actualizar avance
                                tareaAEnviar = TAREA_ACTUALIZAR_AVANCE;
                                char idProyActualizar[10];
                                char porcentajeNuevo[10];
                                printf("--- Actualizar Avance Proyecto ---\n");
                                leerEntrada("ID Proyecto:", idProyActualizar, sizeof(idProyActualizar), esNumeroEnteroPositivoValido);
                                leerEntrada("Nuevo Porcentaje (0-100):", porcentajeNuevo, sizeof(porcentajeNuevo), esDecimalValido);
                                double p = atof(porcentajeNuevo);
                                if (p < 0.0 || p > 100.0) {
                                    printf("Error: Porcentaje fuera de rango.\n");
                                    tareaValidaParaEnviar = false;
                                } else {
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(idProyActualizar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                    MPI_Send(porcentajeNuevo, 10, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                }
                                break;
                            }
                         case 3: { //actualizar prioridad
                                tareaAEnviar = TAREA_ACTUALIZAR_PRIORIDAD;
                                char idProyPrioridad[10];
                                char opPrioridadStr[5];
                                char prioridadNueva[10];
                                printf("--- Actualizar Prioridad Proyecto ---\n");
                                leerEntrada("ID Proyecto:", idProyPrioridad, sizeof(idProyPrioridad), esNumeroEnteroPositivoValido);
                                printf("Nueva Prioridad: [1]ROJO [2]NARANJA [3]AMARILLO\n");
                                leerEntrada("Opción:", opPrioridadStr, sizeof(opPrioridadStr), esNumeroEnteroPositivoValido);
                                int opP = atoi(opPrioridadStr);
                                switch(opP) {
                                    case 1: strcpy(prioridadNueva, "ROJO"); break;
                                    case 2: strcpy(prioridadNueva, "NARANJA"); break;
                                    case 3: strcpy(prioridadNueva, "AMARILLO"); break;
                                    default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                                }
                                if (tareaValidaParaEnviar) {
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(idProyPrioridad, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                    MPI_Send(prioridadNueva, 10, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                }
                                break;
                            }
                         case 4: { //reasignar Supervisor
                                tareaAEnviar = TAREA_REASIGNAR_SUPERVISOR;
                                char idProyReasignar[10];
                                char idNuevoSup[10];
                                printf("--- Reasignar Supervisor ---\n");
                                leerEntrada("ID Proyecto:", idProyReasignar, sizeof(idProyReasignar), esNumeroEnteroPositivoValido);
                                printf("Seleccione nuevo supervisor:\n");
                                mostrarSupervisores(conn_rank0); //rank 0 muestra lista
                                leerEntrada("ID Nuevo Supervisor:", idNuevoSup, sizeof(idNuevoSup), esNumeroEnteroPositivoValido);

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(idProyReasignar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                MPI_Send(idNuevoSup, 10, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                break;
                            }
                         case 5: { //terminar proyecto
                                tareaAEnviar = TAREA_TERMINAR_PROYECTO;
                                char idProyTerminar[10];
                                char confirmacion[5];
                                printf("--- Terminar Proyecto ---\n");
                                leerEntrada("ID Proyecto a terminar:", idProyTerminar, sizeof(idProyTerminar), esNumeroEnteroPositivoValido);
                                int c; while ((c = getchar()) != '\n' && c != EOF); //limpiar buffer
                                printf("Confirmar terminación (s/n):\n"); fflush(stdout);
                                if (fgets(confirmacion, sizeof(confirmacion), stdin) != NULL) {
                                    confirmacion[strcspn(confirmacion, "\n")] = '\0';
                                    int len = strlen(confirmacion);
                                    if (len == sizeof(confirmacion) - 1 && confirmacion[len-1] != '\0') { while ((c = getchar()) != '\n' && c != EOF); }
                                    else if (len == 0) { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }
                                } else { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }

                                if (tolower(confirmacion[0]) == 's') {
                                    tInicio = MPI_Wtime();
                                    MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                    MPI_Send(idProyTerminar, 10, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                } else {
                                    printf("Cancelado.\n");
                                    tareaValidaParaEnviar = false;
                                }
                                break;
                            }
                         default: printf("Opción inválida.\n"); tareaValidaParaEnviar = false; break;
                     } //fin switch(opcionSubMenu) Proyecto

                     //recibir resultado del trabajador (si se envió tarea)
                     if (tareaValidaParaEnviar) {
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        //interpretar resultado
                        switch(resultadoRecibido) {
                            case 0: printf("Rank 0: Operación de proyecto completada exitosamente por Rank %d.\n", rankTrabajador); break;
                            case -1: printf("Rank 0: Rank %d reportó un ERROR de base de datos para proyecto.\n", rankTrabajador); break;
                            case -2: printf("Rank 0: Operación de proyecto ejecutada por Rank %d, pero no afectó filas/condición no cumplida.\n", rankTrabajador); break;
                            default: printf("Rank 0: Rank %d reportó un resultado desconocido para proyecto (%d).\n", rankTrabajador, resultadoRecibido); break;
                        }
                        //mostrar tiempo
                        if(opcionSubMenu != 1) { //no mostrar tiempo detallado para 'Mostrar'
                            printf("Tiempo de operación Proyecto: %.4f seg\n", tFin - tInicio);
                        } else {
                            printf("Tiempo de coordinación 'Mostrar Proyectos': %.4f seg\n", tFin - tInicio);
                        }
                     }
                 } //fin else (delegar proyecto)

                 //pausa dentro del submenú (solo rank 0)
                 if (opcionSubMenu != 6) { // 6 es volver
                     printf("\n(Rank 0) Presione Enter para continuar en Proyectos...");
                     fflush(stdout);
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                 }

            } while (opcionSubMenu != 6);
            break; //fin case 4 (Proyectos Aceptados)
        case 5: //reportes
             do {
                mostrarMenuReportes(); //rank 0 muestra menú
                leerEntrada("Seleccione opción Reportes:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu == 6) break; //volver

                 if (rankTrabajador == 0) { // NP=1
                     printf("Ejecutando localmente (Rank 0)...\n");
                     tInicio = MPI_Wtime();
                     procesarOpcionReportesLocal(opcionSubMenu, conn_rank0); //llama a la función local
                     tFin = MPI_Wtime();
                     printf("Tiempo ejecución local: %.4f seg\n", tFin - tInicio);
                 } else { // NP > 1
                     bool tareaValidaParaEnviar = true;
                     //se determinar la tarea a enviar y si necesita parámetros
                     switch(opcionSubMenu) {
                         case 1: { //reporte de proyectos por periodo
                                tareaAEnviar = TAREA_REPORTE_PROYECTOS_PERIODO;
                                char fechaInicioStr[12];
                                char fechaFinStr[12];
                                printf("--- Reporte Proyectos por Periodo ---\n");
                                leerEntrada("Fecha Inicio (YYYY-MM-DD):", fechaInicioStr, sizeof(fechaInicioStr), esFechaValida);
                                leerEntrada("Fecha Fin (YYYY-MM-DD):", fechaFinStr, sizeof(fechaFinStr), esFechaValida);

                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                MPI_Send(fechaInicioStr, 12, MPI_CHAR, rankTrabajador, TAG_DATOS, MPI_COMM_WORLD);
                                MPI_Send(fechaFinStr, 12, MPI_CHAR, rankTrabajador, TAG_DATOS_EXTRA1, MPI_COMM_WORLD);
                                break;
                            }
                         case 2: //reporte Avance > 50%
                                tareaAEnviar = TAREA_REPORTE_PROYECTOS_AVANCE_50;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                //no necesita parámetros
                                break;
                         case 3: //reporte Empresas Canceladas
                                tareaAEnviar = TAREA_REPORTE_EMPRESAS_CANCELADAS;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                //no necesita parámetros
                                break;
                         case 4: //reporte Supervisor Avance < 20%
                                tareaAEnviar = TAREA_REPORTE_SUPERVISOR_AVANCE_20;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                //no necesita parámetros
                                break;
                         case 5: //reporte General Proyectos
                                tareaAEnviar = TAREA_REPORTE_PROYECTOS_GENERAL;
                                tInicio = MPI_Wtime();
                                MPI_Send(&tareaAEnviar, 1, MPI_INT, rankTrabajador, TAG_TAREA, MPI_COMM_WORLD);
                                //no necesita parámetros
                                break;



                         default:
                                printf("Opción inválida.\n");
                                tareaValidaParaEnviar = false;
                                break;
                     } //fin switch(opcionSubMenu) Reportes

                     //recibir solo el ACK del trabajador (ya que él imprime)
                     if (tareaValidaParaEnviar) {
                        MPI_Recv(&resultadoRecibido, 1, MPI_INT, rankTrabajador, TAG_RESULTADO, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                        tFin = MPI_Wtime();
                        //interpretar resultado (simple ACK)
                        if (resultadoRecibido == 0) {
                            printf("Rank 0: Reporte generado por Rank %d (ver salida del trabajador).\n", rankTrabajador);
                        } else {
                            printf("Rank 0: Rank %d reportó un ERROR al generar el reporte.\n", rankTrabajador);
                        }
                        printf("Tiempo de coordinación Reporte: %.4f seg\n", tFin - tInicio);
                     }
                 } //fin else (delegar reporte)

                 //pausa dentro del submenú (solo rank 0)
                 if (opcionSubMenu != 6) { // 6 es volver
                     printf("\n(Rank 0) Presione Enter para continuar en Reportes...");
                     fflush(stdout);
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                 }

            } while (opcionSubMenu != 6);
            break; //fin case 5 (Reportes)
        default:
            break;
    }
}


//funciones de procesamiento de submenús (SOLO PARA EJECUCIÓN LOCAL SI NP=1)

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
        case 2: mostrarSolicitudesPorEstado(conn); break;
        case 3: cancelarSolicitud(conn); break;
        case 4: aceptarSolicitud(conn); break;
        default: printf("Opción local inválida o pendiente.\n"); break;
    }
}

void procesarOpcionProyectosAceptadosLocal(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: mostrarProyectosAceptados(conn); break; 
        case 2: actualizarAvanceProyecto(conn); break;
        case 3: actualizarPrioridadProyecto(conn); break;
        case 4: reasignarSupervisorProyecto(conn); break;
        case 5: terminarProyecto(conn); break;
        // case 6 (volver) se maneja arriba
        default: printf("Opción local inválida.\n"); break;
    }
}


void procesarOpcionReportesLocal(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: { 
            char fechaInicioStr[12];
            char fechaFinStr[12];
            printf("--- Reporte Proyectos por Periodo (Local) ---\n");
            leerEntrada("Fecha Inicio (YYYY-MM-DD):", fechaInicioStr, sizeof(fechaInicioStr), esFechaValida);
            leerEntrada("Fecha Fin (YYYY-MM-DD):", fechaFinStr, sizeof(fechaFinStr), esFechaValida);
            reporteProyectosPorPeriodo(conn, fechaInicioStr, fechaFinStr); 
            break;
        }
        case 2: reporteProyectosAvanceMayor50(conn); break;
        case 3: reporteEmpresasCanceladas(conn); break;
        case 4: reporteSupervisorAvanceMenor20(conn); break;
        case 5: reporteProyectosAceptadosGeneral(conn); break;
        // case 6 (volver) se maneja arriba
        default: printf("Opción local inválida.\n"); break;
    }
}
