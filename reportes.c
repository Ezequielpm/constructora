#include "reportes.h"
#include "lectura.h"    // para leerentrada
#include "validacion.h" // para esfechavalida
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// libpq-fe.h ya está incluido via reportes.h

// --- función auxiliar para imprimir encabezados ---
void imprimirEncabezadoSimple(const char* tituloReporte) {
    printf("\n------------------------------------------------------------\n");
    printf("        REPORTE: %s\n", tituloReporte);
    printf("------------------------------------------------------------\n");
}

/**
 * @brief Reporte de proyectos aceptados por periodo.
 * (Modificado para recibir fechas como parámetros)
 */
void reporteProyectosPorPeriodo(PGconn *conn, const char *fechaInicioStr, const char *fechaFinStr) { 
    if (conn == NULL) {
        fprintf(stderr, "error (reporteProyectosPorPeriodo): conexión a bd no válida.\n");
        return;
    }
     if (fechaInicioStr == NULL || fechaFinStr == NULL) {
        fprintf(stderr, "error (reporteProyectosPorPeriodo): Fechas de inicio o fin nulas.\n");
        return;
    }

    //ya no se lee la entrada aquí, se usan los parámetros
    // char fechaInicioStr[11]; // YYYY-MM-DD
    // char fechaFinStr[11];
    imprimirEncabezadoSimple("Proyectos Aceptados por Periodo");
    // leerEntrada("Ingrese Fecha de Inicio (YYYY-MM-DD):", fechaInicioStr, sizeof(fechaInicioStr), esFechaValida); 
    // leerEntrada("Ingrese Fecha de Fin (YYYY-MM-DD):", fechaFinStr, sizeof(fechaFinStr), esFechaValida);      

    const char *consulta =
        "SELECT p.id, p.nombre_proyecto, p.fecha_inicio, p.fecha_fin, p.estatus, e.nombre AS nombre_empresa "
        "FROM proyecto_aceptado p "
        "JOIN solicitud_proyecto sp ON p.solicitud_id = sp.id "
        "JOIN empresas e ON sp.empresa_id = e.id "
        "WHERE p.fecha_inicio BETWEEN $1 AND $2 " 
        "ORDER BY p.fecha_inicio, p.id;";

    //los valores ahora vienen de los parámetros de la función
    const char *valores[2] = {fechaInicioStr, fechaFinStr};
    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "error al generar reporte por periodo: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\nProyectos iniciados entre %s y %s (%d encontrados)\n", fechaInicioStr, fechaFinStr, numFilas);
    printf("| %-4s | %-30s | %-10s | %-10s | %-10s | %-25s |\n",
           "ID", "Nombre Proyecto", "F. Inicio", "F. Fin", "Estado", "Empresa");
    printf("--------------------------------------------------------------------------------------------------\n");

    if (numFilas == 0) {
         printf("| %-96s |\n", "No se encontraron proyectos en este periodo.");
    } else {
        for (int i = 0; i < numFilas; i++) {
            printf("| %-4s | %-30s | %-10s | %-10s | %-10s | %-25s |\n",
                   PQgetvalue(resultado, i, 0), // id
                   PQgetvalue(resultado, i, 1), // nombre_proyecto
                   PQgetvalue(resultado, i, 2), // fecha_inicio
                   PQgetvalue(resultado, i, 3), // fecha_fin
                   PQgetvalue(resultado, i, 4), // estatus
                   PQgetvalue(resultado, i, 5)  // nombre_empresa
                  );
        }
    }
    printf("--------------------------------------------------------------------------------------------------\n");
    printf("--- Fin del Reporte ---\n");

    PQclear(resultado);
}

/**
 * @brief Reporte de proyectos con avance > 50%.
 */
void reporteProyectosAvanceMayor50(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (reporteproyectosavancemayor50): conexión a bd no válida.\n");
        return;
    }

    imprimirEncabezadoSimple("Proyectos con Avance Mayor al 50%");

    const char *consulta =
        "SELECT p.id, p.nombre_proyecto, p.porcentaje_avance, p.estatus, e.nombre AS nombre_empresa, sup.nombre || ' ' || sup.apellidos AS nombre_supervisor "
        "FROM proyecto_aceptado p "
        "JOIN solicitud_proyecto sp ON p.solicitud_id = sp.id "
        "JOIN empresas e ON sp.empresa_id = e.id "
        "JOIN supervisores sup ON p.id_supervisor = sup.id "
        "WHERE p.porcentaje_avance > 50.00 "
        "ORDER BY p.porcentaje_avance DESC, p.id;";

    PGresult *resultado = PQexecParams(conn, consulta, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "error al generar reporte de avance > 50%%: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\nProyectos encontrados: %d\n", numFilas);
    printf("| %-4s | %-30s | %-7s | %-10s | %-25s | %-25s |\n",
           "ID", "Nombre Proyecto", "Avance%", "Estado", "Empresa", "Supervisor");
    printf("------------------------------------------------------------------------------------------------------------------\n");

     if (numFilas == 0) {
         printf("|                             No se encontraron proyectos con avance > 50%%.                                |\n");
    } else {
        for (int i = 0; i < numFilas; i++) {
            printf("| %-4s | %-30s | %-7s | %-10s | %-25s | %-25s |\n",
                   PQgetvalue(resultado, i, 0), // id
                   PQgetvalue(resultado, i, 1), // nombre_proyecto
                   PQgetvalue(resultado, i, 2), // porcentaje_avance
                   PQgetvalue(resultado, i, 3), // estatus
                   PQgetvalue(resultado, i, 4), // nombre_empresa
                   PQgetvalue(resultado, i, 5)  // nombre_supervisor
                  );
        }
    }
     printf("------------------------------------------------------------------------------------------------------------------\n");
     printf("--- Fin del Reporte ---\n");

    PQclear(resultado);
}

/**
 * @brief Reporte de empresas con solicitudes canceladas.
 */
void reporteEmpresasCanceladas(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (reporteempresascanceladas): conexión a bd no válida.\n");
        return;
    }

     imprimirEncabezadoSimple("Empresas con Solicitudes Canceladas");

    // usamos distinct para no repetir empresas si tienen varias solicitudes canceladas
    const char *consulta =
        "SELECT DISTINCT e.id, e.nombre, e.rfc, e.telefono, e.correo, e.contacto_encargado "
        "FROM empresas e "
        "JOIN solicitud_proyecto s ON e.id = s.empresa_id "
        "WHERE s.estado = 'CANCELADO' "
        "ORDER BY e.nombre;";

    PGresult *resultado = PQexecParams(conn, consulta, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "error al generar reporte de empresas canceladas: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

     int numFilas = PQntuples(resultado);
    printf("\nEmpresas encontradas: %d\n", numFilas);
    printf("| %-4s | %-25s | %-15s | %-15s | %-25s | %-25s |\n",
           "ID", "Nombre", "RFC", "Teléfono", "Correo", "Contacto");
    printf("-----------------------------------------------------------------------------------------------------------------------------\n");

     if (numFilas == 0) {
         printf("|                           No se encontraron empresas con solicitudes canceladas.                                  |\n");
    } else {
        for (int i = 0; i < numFilas; i++) {
            printf("| %-4s | %-25s | %-15s | %-15s | %-25s | %-25s |\n",
                   PQgetvalue(resultado, i, 0), // id
                   PQgetvalue(resultado, i, 1), // nombre
                   PQgetvalue(resultado, i, 2), // rfc
                   PQgetvalue(resultado, i, 3), // telefono
                   PQgetvalue(resultado, i, 4), // correo
                   PQgetvalue(resultado, i, 5)  // contacto_encargado
                  );
        }
    }
     printf("-----------------------------------------------------------------------------------------------------------------------------\n");
     printf("--- Fin del Reporte ---\n");

    PQclear(resultado);
}

/**
 * @brief Reporte de supervisores con proyectos activos y avance < 20%.
 */
void reporteSupervisorAvanceMenor20(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (reportesupervisoravancemenor20): conexión a bd no válida.\n");
        return;
    }

     imprimirEncabezadoSimple("Supervisores con Proyectos Activos Avance < 20%");

    // usamos distinct por si un supervisor tiene varios proyectos en esta condición
    const char *consulta =
        "SELECT DISTINCT sup.id, sup.nombre, sup.apellidos, sup.telefono, sup.correo "
        "FROM supervisores sup "
        "JOIN proyecto_aceptado p ON sup.id = p.id_supervisor "
        "WHERE p.porcentaje_avance < 20.00 AND p.estatus = 'EN_PROCESO' " //se flitra por estado
        "ORDER BY sup.apellidos, sup.nombre;";

    PGresult *resultado = PQexecParams(conn, consulta, 0, NULL, NULL, NULL, NULL, 0);

     if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "error al generar reporte de supervisores avance < 20%%: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\nSupervisores encontrados: %d\n", numFilas);
    printf("| %-4s | %-20s | %-20s | %-15s | %-25s |\n",
           "ID", "Nombre", "Apellidos", "Teléfono", "Correo");
    printf("----------------------------------------------------------------------------------------\n");

    if (numFilas == 0) {
         printf("|        No se encontraron supervisores con proyectos activos de bajo avance.        |\n");
    } else {
        for (int i = 0; i < numFilas; i++) {
            printf("| %-4s | %-20s | %-20s | %-15s | %-25s |\n",
                   PQgetvalue(resultado, i, 0), // id
                   PQgetvalue(resultado, i, 1), // nombre
                   PQgetvalue(resultado, i, 2), // apellidos
                   PQgetvalue(resultado, i, 3), // telefono
                   PQgetvalue(resultado, i, 4)  // correo
                  );
        }
    }
    printf("----------------------------------------------------------------------------------------\n");
    printf("--- Fin del Reporte ---\n");


    PQclear(resultado);
}

/**
 * @brief Reporte general de proyectos aceptados.
 */
void reporteProyectosAceptadosGeneral(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (reporteproyectosaceptadosgeneral): conexión a bd no válida.\n");
        return;
    }

    imprimirEncabezadoSimple("Listado General de Proyectos Aceptados");

    // usamos la misma consulta que en mostrarproyectosaceptados
    const char *consulta =
        "SELECT "
        "    p.id, p.nombre_proyecto, p.estatus, p.prioridad, p.porcentaje_avance, " // 0-4
        "    p.fecha_inicio, p.fecha_fin, p.monto, "                                // 5-7
        "    e.nombre AS nombre_empresa, "                                           // 8
        "    sup.nombre || ' ' || sup.apellidos AS nombre_supervisor "              // 9
        "FROM "
        "    proyecto_aceptado p "
        "JOIN "
        "    solicitud_proyecto sp ON p.solicitud_id = sp.id "
        "JOIN "
        "    empresas e ON sp.empresa_id = e.id "
        "JOIN "
        "    supervisores sup ON p.id_supervisor = sup.id "
        "ORDER BY p.estatus, p.prioridad, p.id;"; //se ordena por estado y prioridad


    PGresult *resultado = PQexecParams(conn, consulta, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "error al generar reporte general de proyectos: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\nProyectos encontrados: %d\n", numFilas);
    //usamos el mismo formato que mostrarproyectosaceptados
    printf("| %-4s | %-25s | %-10s | %-8s | %-7s | %-10s | %-10s | %-15s | %-20s | %-25s |\n",
           "ID", "Nombre Proyecto", "Estado", "Prioridad", "Avance%", "F. Inicio", "F. Fin", "Monto", "Empresa", "Supervisor");
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    if (numFilas == 0) {
         printf("|                                              No hay proyectos aceptados registrados.                                                                                       |\n");
    } else {
        for (int fila = 0; fila < numFilas; fila++) {
            printf("| %-4s | %-25s | %-10s | %-8s | %-7s | %-10s | %-10s | %-15s | %-20s | %-25s |\n",
                   PQgetvalue(resultado, fila, 0), PQgetvalue(resultado, fila, 1),
                   PQgetvalue(resultado, fila, 2), PQgetvalue(resultado, fila, 3),
                   PQgetvalue(resultado, fila, 4), PQgetvalue(resultado, fila, 5),
                   PQgetvalue(resultado, fila, 6), PQgetvalue(resultado, fila, 7),
                   PQgetvalue(resultado, fila, 8), PQgetvalue(resultado, fila, 9)
                  );
        }
     }
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    printf("--- Fin del Reporte ---\n");


    PQclear(resultado);
}