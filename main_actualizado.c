#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> // para bool, true, false

#include "base_de_datos.h"   // para conectarDB, desconectarDB
#include "lectura.h"    // para leerEntrada
#include "validacion.h" // para esNumeroEnteroPositivoValido
#include "empresa.h"    // funciones de empresa
#include "supervisor.h" // funciones de supervisor
#include "solicitud.h"  // funciones de solicitud
#include "proyecto.h"   // funciones de proyecto aceptado 
#include "reportes.h"   // funciones de reportes        

// Declaraciones de Funciones de Menú (Prototipos)
// Menús de Mostrar
void mostrarMenuPrincipal();
void mostrarMenuEmpresas();
void mostrarMenuSupervisores();
void mostrarMenuSolicitudes();
void mostrarMenuProyectosAceptados(); 
void mostrarMenuReportes();         

// funciones de procesamiento de opciones
void procesarOpcionPrincipal(int opcion, PGconn *conn, bool *salir);
void procesarOpcionEmpresas(int opcion, PGconn *conn);
void procesarOpcionSupervisores(int opcion, PGconn *conn);
void procesarOpcionSolicitudes(int opcion, PGconn *conn);
void procesarOpcionProyectosAceptados(int opcion, PGconn *conn); 
void procesarOpcionReportes(int opcion, PGconn *conn);         


// --- Función Principal ---
int main() {
    // conexión a la base de datos
    PGconn *conexion = conectarDB();

    if (conexion == NULL) {
        // conectarDB ya debería haber impreso un error
        return 1; // salir si no se pudo conectar
    }

    // bucle principal del menu
    bool salir = false;
    char opcionStr[5];
    int opcionSeleccionada;

    while (!salir) {
        mostrarMenuPrincipal();
        leerEntrada("Seleccione una opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
        opcionSeleccionada = atoi(opcionStr);
        procesarOpcionPrincipal(opcionSeleccionada, conexion, &salir);

        // pausa simple para que el usuario vea el resultado antes de volver al menu
        if (!salir) {
             printf("\nPresione Enter para continuar...");
             // limpiar buffer antes de esperar enter
             int c; while ((c = getchar()) != '\n' && c != EOF);
             getchar(); // esperar Enter
        }
    }

    // desconexión de la base de datos
    desconectarDB(conexion);
    printf("Programa terminado. ¡Hasta luego!\n");

    return 0;
}

// --- Implementación de funciones del Menu ---

void mostrarMenuPrincipal() {
    printf("\n----- CONSTRUCTORA LA ROCA BONITA - MENÚ PRINCIPAL -----\n");
    printf("[1] Gestionar Empresas\n");
    printf("[2] Gestionar Supervisores\n");
    printf("[3] Gestionar Solicitudes de Proyecto\n");
    printf("[4] Gestionar Proyectos Aceptados\n");
    printf("[5] Generar Reportes\n");             
    printf("[6] Salir\n");
    printf("-------------------------------------------------------------\n");
}

void procesarOpcionPrincipal(int opcion, PGconn *conn, bool *salir) {
    char opcionSubMenuStr[5];
    int opcionSubMenu;

    switch (opcion) {
        case 1: // Empresas
            do {
                mostrarMenuEmpresas();
                leerEntrada("Seleccione una opción del menú Empresas:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu != 5) { // 5 es volver
                    procesarOpcionEmpresas(opcionSubMenu, conn);
                    // pausa dentro del submenú
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 5); // repetir hasta que elija volver
            break;
        case 2: // Supervisores
             do {
                mostrarMenuSupervisores();
                leerEntrada("Seleccione una opción del menú Supervisores:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu != 5) { // 5 es volver
                    procesarOpcionSupervisores(opcionSubMenu, conn);
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 5);
            break;
        case 3: // solicitudes
             do {
                mostrarMenuSolicitudes();
                leerEntrada("Seleccione una opción del menú Solicitudes:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu != 5) {
                    procesarOpcionSolicitudes(opcionSubMenu, conn);
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 5); 
            break;
        case 4: 
             do {
                mostrarMenuProyectosAceptados();
                leerEntrada("Seleccione una opción del menú Proyectos:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu != 6) {
                    procesarOpcionProyectosAceptados(opcionSubMenu, conn);
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 6);
            break;
        case 5: 
             do {
                mostrarMenuReportes();
                leerEntrada("Seleccione una opción del menú Reportes:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                if (opcionSubMenu != 6) {
                    procesarOpcionReportes(opcionSubMenu, conn);
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 6);
            break;
        case 6:
            *salir = true; // bandera para salir del bucle principal
            break;
        default:
            printf("Opción no válida. Por favor, intente de nuevo.\n");
            break;
    }
}

// --- Sub Menu Empresas ---

void mostrarMenuEmpresas() {
    printf("\n--- Menú Empresas ---\n");
    printf("[1] Registrar Nueva Empresa\n");
    printf("[2] Mostrar Todas las Empresas\n");
    printf("[3] Actualizar Datos de Empresa\n");
    printf("[4] Eliminar Empresa\n");
    printf("[5] Volver al Menú Principal\n");
    printf("---------------------\n");
}

void procesarOpcionEmpresas(int opcion, PGconn *conn) {
    switch (opcion) {
        case 1: registrarEmpresa(conn); break;
        case 2: mostrarEmpresas(conn); break;
        case 3: actualizarEmpresa(conn); break;
        case 4: eliminarEmpresa(conn); break;
        default: printf("Opción no válida.\n"); break;
    }
}

// --- Sub Menu Supervisores ---

void mostrarMenuSupervisores() {
    printf("\n--- Menú Supervisores ---\n");
    printf("[1] Registrar Nuevo Supervisor\n");
    printf("[2] Mostrar Todos los Supervisores\n");
    printf("[3] Actualizar Datos de Supervisor\n");
    printf("[4] Eliminar Supervisor\n");
    printf("[5] Volver al Menú Principal\n");
    printf("-------------------------\n");
}

void procesarOpcionSupervisores(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: registrarSupervisor(conn); break;
        case 2: mostrarSupervisores(conn); break;
        case 3: actualizarSupervisor(conn); break;
        case 4: eliminarSupervisor(conn); break;
        default: printf("Opción no válida.\n"); break;
    }
}

// --- Sub Menu Solicitudes ---

void mostrarMenuSolicitudes() {
    printf("\n--- Menú Solicitudes de Proyecto ---\n");
    printf("[1] Registrar Nueva Solicitud\n");
    printf("[2] Mostrar Solicitudes por Estado\n");
    printf("[3] Cancelar Solicitud (Estado APERTURADO)\n");
    printf("[4] Aceptar Solicitud (Crear Proyecto)\n"); 
    printf("[5] Volver al Menú Principal\n");        
    printf("------------------------------------\n");
}

void procesarOpcionSolicitudes(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: registrarSolicitud(conn); break;
        case 2: mostrarSolicitudesPorEstado(conn); break;
        case 3: cancelarSolicitud(conn); break;
        case 4: aceptarSolicitud(conn); break; 
        // caso 5 (volver) se maneja en el bucle principal
        default: printf("Opción no válida.\n"); break;
    }
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
}

void procesarOpcionProyectosAceptados(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: mostrarProyectosAceptados(conn); break;
        case 2: actualizarAvanceProyecto(conn); break;
        case 3: actualizarPrioridadProyecto(conn); break;
        case 4: reasignarSupervisorProyecto(conn); break;
        case 5: terminarProyecto(conn); break;
        // caso 6 (volver) se maneja en el bucle principal
        default: printf("Opción no válida.\n"); break;
    }
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
}

void procesarOpcionReportes(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1: reporteProyectosPorPeriodo(conn); break;
        case 2: reporteProyectosAvanceMayor50(conn); break;
        case 3: reporteEmpresasCanceladas(conn); break;
        case 4: reporteSupervisorAvanceMenor20(conn); break;
        case 5: reporteProyectosAceptadosGeneral(conn); break;
         // caso 6 (volver) se maneja en el bucle principal
        default: printf("Opción no válida.\n"); break;
    }
}