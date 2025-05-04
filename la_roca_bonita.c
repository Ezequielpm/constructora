#define _GNU_SOURCE
#include "mpi.h"
#include <sched.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

/*
Este codigo es solo para pruebas, el codigo del proyecto se encuentra
en el archivo main.c y main_con_mpi.c
*/
#define TAMCADENA 100

PGconn* conectarPostgreSQL();
void menuPrincipal(PGconn *conn);
void mostrarProyectosAceptados(PGconn *conn);
void mostrarProyectosAceptadosPorPeriodo(PGconn *conn);
void mostrarProyectosConAvanceAlto(PGconn *conn);
void mostrarEmpresasConProyectosCancelados(PGconn *conn);
void mostrarSupervisoresConAvanceBajo(PGconn *conn);

int main(int argc, char **argv) {
    int id, numprocs;
    char nombreproc[MPI_MAX_PROCESSOR_NAME];
    int lnombreproc;
    double tmpinic = 0.0, tmpfin;
    clock_t t_ini, t_fin;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Get_processor_name(nombreproc, &lnombreproc);

    if (id == 0) {
        printf("\nNumero total de procesos: %d", numprocs);
        fprintf(stdout, "\nProceso %d en %s Encargado de la E/S\n\n", id, nombreproc);
    }

    tmpinic = MPI_Wtime();
    t_ini = clock();

    PGconn *conn = NULL;
    if (id == 0) {
        conn = conectarPostgreSQL();
        if (conn != NULL) {
            menuPrincipal(conn);
            PQfinish(conn);
        } else {
            fprintf(stderr, "No se pudo conectar a la base de datos.\n");
            fflush(stdout);
        }
    }

    t_fin = clock();
    tmpfin = MPI_Wtime();

    if (id == 0) {
        fprintf(stdout, "\n\nINFORMACION SOBRE LA EJECUCION COMPLETA DEL PROGRAMA\n");
        fprintf(stdout, "Numero Procesos: %d\n", numprocs);
        fprintf(stdout, "Tiempo Procesamiento: %f\n", tmpfin - tmpinic);
        fprintf(stdout, "Tiempo de Ejecución: %f \n", (double)(t_fin - t_ini) / CLOCKS_PER_SEC);
    }

    MPI_Finalize();
    return 0;
}

PGconn* conectarPostgreSQL() {
    PGconn *conn = PQsetdbLogin("127.0.0.1", "5432", NULL, NULL, "constructora", "postgres", "ezequielpm123");

    if (PQstatus(conn) == CONNECTION_OK) {
        puts("Conectado exitosamente a PostgreSQL.");
        return conn;
    } else {
        printf("Fallo en la conexión a PostgreSQL.\n");
        return NULL;
    }
}


int valSoloNumeros(char *cadena) {
    for (int i = 0; cadena[i]; i++) {
        if (!isdigit(cadena[i])) return 0;
    }
    return 1;
}


void menuPrincipal(PGconn *conn) {
    char res[10];
    int opcion = 0;

    do {
        fprintf(stdout, "\n------ MENU PRINCIPAL ------\n");
        fprintf(stdout, "[1].- Proyectos aceptados\n");
        fprintf(stdout, "[2].- Proyectos aceptados por periodo\n");
        fprintf(stdout, "[3].- Proyectos con avance > 50%%\n");
        fprintf(stdout, "[4].- Empresas con proyectos cancelados\n");
        fprintf(stdout, "[5].- Supervisores con avance < 20%%\n");
        fprintf(stdout, "[6].- Salir\n");
        
        
        do{
          fprintf(stdout, "Dame una opción:\n");
          fgets(res, sizeof(res), stdin);
          res[strcspn(res, "\n")] = 0;
          if (!valSoloNumeros(res)) {
            fprintf(stdout, "\nEntrada inválida. Solo números.\n");
            continue;
        } 
        }while(!valSoloNumeros(res));
        

        

        opcion = atoi(res); // Convertimos a entero

        switch (opcion) {
            case 1:
                mostrarProyectosAceptados(conn);
                break;
            case 2:
                mostrarProyectosAceptadosPorPeriodo(conn);
                break;
            case 3:
                mostrarProyectosConAvanceAlto(conn);
                break;
            case 4:
                mostrarEmpresasConProyectosCancelados(conn);
                break;
            case 5:
                mostrarSupervisoresConAvanceBajo(conn);
                break;
            case 6:
                break;
            default:
                printf("Opción no válida\n");
        }
    } while (opcion != 6);
}

void mostrarProyectosAceptados(PGconn *conn) {
    const char *consulta =
        "SELECT pa.id, pa.nombre_proyecto, pa.fecha_inicio, pa.ubicacion "
        "FROM proyecto_aceptado pa "
        "JOIN solicitud_proyecto sp ON pa.solicitud_id = sp.id "
        "WHERE sp.estado = 'ACEPTADO';";

    PGresult *res = PQexec(conn, consulta);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error al ejecutar consulta: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    printf("\n----- Proyectos Aceptados -------\n");
    for (int i = 0; i < PQntuples(res); i++) {
        printf("ID: %s | Nombre: %s | Inicio: %s | Ubicación: %s\n",
            PQgetvalue(res, i, 0),
            PQgetvalue(res, i, 1),
            PQgetvalue(res, i, 2),
            PQgetvalue(res, i, 3));
    }

    PQclear(res);
}

void mostrarProyectosAceptadosPorPeriodo(PGconn *conn) {
    char fecha_inicio[20], fecha_fin[20];

    int c;
    
    printf("Introduce la fecha de inicio (YYYY-MM-DD): ");
    fflush(stdout);
    fgets(fecha_inicio, sizeof(fecha_inicio), stdin);
    fecha_inicio[strcspn(fecha_inicio, "\n")] = 0;  

    printf("Introduce la fecha de fin (YYYY-MM-DD): ");
    fflush(stdout);
    fgets(fecha_fin, sizeof(fecha_fin), stdin);
    fecha_fin[strcspn(fecha_fin, "\n")] = 0;  

    char consulta[500];
    snprintf(consulta, sizeof(consulta),
        "SELECT pa.id, pa.nombre_proyecto, pa.fecha_inicio, pa.fecha_fin "
        "FROM proyecto_aceptado pa "
        "JOIN solicitud_proyecto sp ON pa.solicitud_id = sp.id "
        "WHERE sp.estado = 'ACEPTADO' AND pa.fecha_inicio BETWEEN '%s' AND '%s';",
        fecha_inicio, fecha_fin
    );

    PGresult *res = PQexec(conn, consulta);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error en consulta: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    int filas = PQntuples(res);
    if (filas == 0) {
        printf("No hay proyectos aceptados en ese periodo.\n");
    } else {
        printf("\n=== Proyectos aceptados en el periodo ===\n");
        for (int i = 0; i < filas; i++) {
            printf("ID: %s | Nombre: %s | Inicio: %s | Fin: %s\n",
                   PQgetvalue(res, i, 0),
                   PQgetvalue(res, i, 1),
                   PQgetvalue(res, i, 2),
                   PQgetvalue(res, i, 3));
        }
    }

    PQclear(res);
}


void mostrarProyectosConAvanceAlto(PGconn *conn) {
    const char *consulta = 
        "SELECT id, nombre_proyecto, porcentaje_avance FROM proyecto_aceptado "
        "WHERE porcentaje_avance > 50;";

    PGresult *res = PQexec(conn, consulta);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error en consulta: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    int filas = PQntuples(res);
    if (filas == 0) {
        printf("No hay proyectos con más del 50%% de avance.\n");
    } else {
        printf("\n=== Proyectos con avance mayor al 50%% ===\n");
        for (int i = 0; i < filas; i++) {
            printf("ID: %s | Nombre: %s | Avance: %s%%\n",
                   PQgetvalue(res, i, 0),
                   PQgetvalue(res, i, 1),
                   PQgetvalue(res, i, 2));
        }
    }

    PQclear(res);
}

void mostrarEmpresasConProyectosCancelados(PGconn *conn) {
    const char *consulta = 
        "SELECT DISTINCT e.id, e.nombre, e.rfc "
        "FROM empresas e "
        "JOIN solicitud_proyecto sp ON e.id = sp.empresa_id "
        "WHERE sp.estado = 'CANCELADO';";

    PGresult *res = PQexec(conn, consulta);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error en consulta: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    int filas = PQntuples(res);
    if (filas == 0) {
        printf("No hay empresas con proyectos cancelados.\n");
    } else {
        printf("\n------ Empresas con proyectos cancelados --------\n");
        for (int i = 0; i < filas; i++) {
            printf("ID: %s | Nombre: %s | RFC: %s\n",
                   PQgetvalue(res, i, 0),
                   PQgetvalue(res, i, 1),
                   PQgetvalue(res, i, 2));
        }
    }

    PQclear(res);
}

void mostrarSupervisoresConAvanceBajo(PGconn *conn) {
    const char *consulta = 
        "SELECT DISTINCT s.id, s.nombre, s.apellidos, s.correo "
        "FROM supervisores s "
        "JOIN proyecto_aceptado pa ON s.id = pa.id_supervisor "
        "WHERE pa.porcentaje_avance < 20;";

    PGresult *res = PQexec(conn, consulta);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error en consulta: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    int filas = PQntuples(res);
    if (filas == 0) {
        printf("No hay supervisores con proyectos debajo del 20%% de avance.\n");
    } else {
        printf("\n=== Supervisores con proyectos con avance < 20%% ===\n");
        for (int i = 0; i < filas; i++) {
            printf("ID: %s | Nombre: %s %s | Correo: %s\n",
                   PQgetvalue(res, i, 0),
                   PQgetvalue(res, i, 1),
                   PQgetvalue(res, i, 2),
                   PQgetvalue(res, i, 3));
        }
    }

    PQclear(res);
}

// mpicc -o laRocaBonita la_roca_bonita.c  -I/Library/PostgreSQL/16/include -L/Library/PostgreSQL/16/lib -lpq



