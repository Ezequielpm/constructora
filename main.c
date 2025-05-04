#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> //para bool, true, false

#include "base_de_datos.h"   //para conectarDB, desconectarDB
#include "lectura.h"    //para leerEntrada
#include "validacion.h" //para esNumeroEnteroPositivoValido
#include "empresa.h"    //funciones de empresa
#include "supervisor.h" //funciones de supervisor
#include "solicitud.h"  //funciones de solicitud (las implementadas)

//declaraciones de funciones de menu (prototipos)
void mostrarMenuPrincipal();
void mostrarMenuEmpresas();
void mostrarMenuSupervisores();
void mostrarMenuSolicitudes();

void procesarOpcionPrincipal(int opcion, PGconn *conn, bool *salir);
void procesarOpcionEmpresas(int opcion, PGconn *conn);
void procesarOpcionSupervisores(int opcion, PGconn *conn);
void procesarOpcionSolicitudes(int opcion, PGconn *conn);

//función principal
int main() {
    //conexión a la base de datos
    PGconn *conexion = conectarDB();

    if (conexion == NULL) {
        //conectarDB ya debería haber impreso un error
        return 1; //salir si no se pudo conectar
    }

    //bucle principal del menu
    bool salir = false;
    char opcionStr[5];
    int opcionSeleccionada;

    while (!salir) {
        mostrarMenuPrincipal();
        leerEntrada("Seleccione una opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
        opcionSeleccionada = atoi(opcionStr);
        procesarOpcionPrincipal(opcionSeleccionada, conexion, &salir);

        //pausa simple para que el usuario vea el resultado antes de volver al menu
        if (!salir) {
             printf("\nPresione Enter para continuar...");
             //limpiar buffer antes de esperar enter
             int c; while ((c = getchar()) != '\n' && c != EOF);
             getchar(); //esperar Enter
        }
    }

    //desconexión de la base de datos
    desconectarDB(conexion);
    printf("Programa terminado. ¡Hasta luego!\n");

    return 0;
}

//implementación de funciones de menu

void mostrarMenuPrincipal() {
    printf("\n----- CONSTRUCTORA LA ROCA BONITA - MENÚ PRINCIPAL -----\n");
    printf("[1] Gestionar Empresas\n");
    printf("[2] Gestionar Supervisores\n");
    printf("[3] Gestionar Solicitudes de Proyecto\n");
    printf("[4] Gestionar Proyectos Aceptados (Pendiente)\n");
    printf("[5] Generar Reportes (Pendiente)\n");
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
                    //pausa dentro del submenú
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 5); //repetir hasta que elija volver
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
        case 3: // Solicitudes
             do {
                mostrarMenuSolicitudes();
                leerEntrada("Seleccione una opción del menú Solicitudes:", opcionSubMenuStr, sizeof(opcionSubMenuStr), esNumeroEnteroPositivoValido);
                opcionSubMenu = atoi(opcionSubMenuStr);
                 //asumimos que la opción para volver será la última definida en mostrarMenuSolicitudes
                 // (por ahora, la última opción implementada es la 3, así que usamos 4 para volver)
                if (opcionSubMenu != 4) {
                    procesarOpcionSolicitudes(opcionSubMenu, conn);
                     printf("\nPresione Enter para continuar...");
                     int c; while ((c = getchar()) != '\n' && c != EOF);
                     getchar();
                }
            } while (opcionSubMenu != 4);
            break;
        case 4:
            printf("Funcionalidad de Proyectos Aceptados aún no implementada.\n");
            break;
        case 5:
            printf("Funcionalidad de Reportes aún no implementada.\n");
            break;
        case 6:
            *salir = true; //bandera para salir del bucle principal
            break;
        default:
            printf("Opción no válida. Por favor, intente de nuevo.\n");
            break;
    }
}

//submenu de empresas

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
        case 1:
            registrarEmpresa(conn);
            break;
        case 2:
            mostrarEmpresas(conn);
            break;
        case 3:
            actualizarEmpresa(conn);
            break;
        case 4:
            eliminarEmpresa(conn);
            break;
        //el caso 5 (volver) se maneja en el bucle de procesarOpcionPrincipal
        default:
            printf("Opción no válida.\n");
            break;
    }
}

//submenu de supervisores

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
        case 1:
            registrarSupervisor(conn);
            break;
        case 2:
            mostrarSupervisores(conn);
            break;
        case 3:
            actualizarSupervisor(conn);
            break;
        case 4:
            eliminarSupervisor(conn);
            break;
        //el caso 5 (volver) se maneja en el bucle de procesarOpcionPrincipal
        default:
            printf("Opción no válida.\n");
            break;
    }
}

//submenu de solicitudes

void mostrarMenuSolicitudes() {
    printf("\n--- Menú Solicitudes de Proyecto ---\n");
    printf("[1] Registrar Nueva Solicitud\n");
    printf("[2] Mostrar Solicitudes por Estado\n");
    printf("[3] Cancelar Solicitud (Estado APERTURADO)\n");
    printf("[4] Volver al Menú Principal\n");
    //pciones endientes:
    // printf("[5] Aceptar Solicitud\n");
    printf("------------------------------------\n");
}

void procesarOpcionSolicitudes(int opcion, PGconn *conn) {
     switch (opcion) {
        case 1:
            registrarSolicitud(conn);
            break;
        case 2:
            mostrarSolicitudesPorEstado(conn);
            break;
        case 3:
            cancelarSolicitud(conn);
            break;
        //el caso 4 (volver) se maneja en el bucle de procesarOpcionPrincipal
        default:
            printf("Opción no válida o pendiente de implementación.\n");
            break;
    }
}