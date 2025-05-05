#include "proyecto.h"
#include "structs.h"    // para tamaños de buffer si es necesario
#include "lectura.h"    // para leerentrada
#include "validacion.h" // para validadores
#include "supervisor.h" // para mostrarsupervisores() en reasignacion

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>     //para tolower
// libpq-fe.h ya está incluido via proyecto.h

/**
 * @brief Muestra la lista de proyectos aceptados, con opción de filtro.
 */
void mostrarProyectosAceptados(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "error (mostrarproyectosaceptados): conexión a bd no válida.\n");
        return;
    }

    char opcionFiltroStr[5];
    int opcionFiltro;
    const char *filtroEstatus = NULL; // null para mostrar todos por defecto
    char *consultaBase =
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
        "    supervisores sup ON p.id_supervisor = sup.id "; 

    char consultaCompleta[1024]; // buffer para la consulta final
    const char *valores[1]; // solo necesitamos 1 parametro para el filtro de estado
    int numParams = 0;

    printf("\n--- Mostrar Proyectos Aceptados ---\n");
    printf("Filtrar por estado:\n");
    printf("[1] EN PROCESO\n");
    printf("[2] TERMINADO\n");
    printf("[3] TODOS\n");
    leerEntrada("Seleccione filtro:", opcionFiltroStr, sizeof(opcionFiltroStr), esNumeroEnteroPositivoValido);
    opcionFiltro = atoi(opcionFiltroStr);

    switch (opcionFiltro) {
        case 1:
            filtroEstatus = "EN_PROCESO";
            snprintf(consultaCompleta, sizeof(consultaCompleta), "%s WHERE p.estatus = $1 ORDER BY p.prioridad, p.id;", consultaBase);
            valores[0] = filtroEstatus;
            numParams = 1;
            break;
        case 2:
            filtroEstatus = "TERMINADO";
             snprintf(consultaCompleta, sizeof(consultaCompleta), "%s WHERE p.estatus = $1 ORDER BY p.id;", consultaBase);
            valores[0] = filtroEstatus;
            numParams = 1;
            break;
        case 3:
             snprintf(consultaCompleta, sizeof(consultaCompleta), "%s ORDER BY p.estatus, p.prioridad, p.id;", consultaBase);
            break;
        default:
            printf("Opción de filtro inválida. Mostrando todos.\n");
             snprintf(consultaCompleta, sizeof(consultaCompleta), "%s ORDER BY p.estatus, p.prioridad, p.id;", consultaBase);
            break;
    }

    // ejecutar la consulta construida
    PGresult *resultado = PQexecParams(conn, consultaCompleta, numParams, NULL, (numParams > 0 ? valores : NULL), NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "error al consultar proyectos aceptados: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\n--- Lista de Proyectos Aceptados (%s) (%d) ---\n", (filtroEstatus ? filtroEstatus : "TODOS"), numFilas);
    printf("| %-4s | %-25s | %-10s | %-8s | %-7s | %-10s | %-10s | %-15s | %-20s | %-25s |\n",
           "ID", "Nombre Proyecto", "Estado", "Prioridad", "Avance%", "F. Inicio", "F. Fin", "Monto", "Empresa", "Supervisor");
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    for (int fila = 0; fila < numFilas; fila++) {
        printf("| %-4s | %-25s | %-10s | %-8s | %-7s | %-10s | %-10s | %-15s | %-20s | %-25s |\n",
               PQgetvalue(resultado, fila, 0), // id
               PQgetvalue(resultado, fila, 1), // nombre_proyecto
               PQgetvalue(resultado, fila, 2), // estatus
               PQgetvalue(resultado, fila, 3), // prioridad
               PQgetvalue(resultado, fila, 4), // porcentaje_avance
               PQgetvalue(resultado, fila, 5), // fecha_inicio
               PQgetvalue(resultado, fila, 6), // fecha_fin
               PQgetvalue(resultado, fila, 7), // monto
               PQgetvalue(resultado, fila, 8), // nombre_empresa
               PQgetvalue(resultado, fila, 9)  // nombre_supervisor
              );
    }
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    PQclear(resultado);
}


/**
 * @brief Actualiza el porcentaje de avance de un proyecto.
 */
void actualizarAvanceProyecto(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "error (actualizaravanceproyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char nuevoPorcentajeStr[10];
    double nuevoPorcentaje;

    printf("\n--- Actualizar Avance de Proyecto ---\n");
    // podríamos mostrar solo los 'en proceso' aquí para facilitar
    mostrarProyectosAceptados(conn); //se muestran todos por ahora

    leerEntrada("Ingrese el ID del proyecto a actualizar:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);
    leerEntrada("Ingrese el nuevo porcentaje de avance (0-100):", nuevoPorcentajeStr, sizeof(nuevoPorcentajeStr), esDecimalValido);

    //validar rango del porcentaje
    nuevoPorcentaje = atof(nuevoPorcentajeStr);
    if (nuevoPorcentaje < 0.0 || nuevoPorcentaje > 100.0) {
        printf("error: el porcentaje debe estar entre 0 y 100.\n");
        return;
    }

    // preparar consulta update
    const char *consulta = "UPDATE proyecto_aceptado SET porcentaje_avance = $1 WHERE id = $2;";
    const char *valores[2];
    valores[0] = nuevoPorcentajeStr; // enviar como string
    valores[1] = idProyectoStr;

    // ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);

    // verificar
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            printf("Avance del proyecto %s actualizado correctamente a %.2f%%.\n", idProyectoStr, nuevoPorcentaje);
        } else {
            printf("No se actualizó el avance (ID: %s no encontrado).\n", idProyectoStr);
        }
    } else {
        fprintf(stderr, "error al actualizar avance: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}

/**
 * @brief Actualiza la prioridad (semaforización) de un proyecto.
 */
void actualizarPrioridadProyecto(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (actualizarprioridadproyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char opcionPrioridadStr[5];
    int opcionPrioridad;
    char nuevaPrioridadStr[10];

    printf("\n--- Actualizar Prioridad de Proyecto ---\n");
    mostrarProyectosAceptados(conn); //mostrar todos por ahora

    leerEntrada("Ingrese el ID del proyecto a actualizar:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);

    // seleccionar nueva prioridad
    printf("Seleccione la nueva prioridad:\n");
    printf("[1] ROJO (Alta)\n");
    printf("[2] NARANJA (Media)\n");
    printf("[3] AMARILLO (Baja)\n");
    leerEntrada("Opción de Prioridad:", opcionPrioridadStr, sizeof(opcionPrioridadStr), esNumeroEnteroPositivoValido);
    opcionPrioridad = atoi(opcionPrioridadStr);

    switch(opcionPrioridad) {
        case 1: strcpy(nuevaPrioridadStr, "ROJO"); break;
        case 2: strcpy(nuevaPrioridadStr, "NARANJA"); break;
        case 3: strcpy(nuevaPrioridadStr, "AMARILLO"); break;
        default:
            printf("Opción inválida. No se realizaron cambios.\n");
            return;
    }

    // preparar update
    const char *consulta = "UPDATE proyecto_aceptado SET prioridad = $1 WHERE id = $2;";
    const char *valores[2];
    valores[0] = nuevaPrioridadStr;
    valores[1] = idProyectoStr;

    // ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);

    // verificar
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            printf("Prioridad del proyecto %s actualizada a %s.\n", idProyectoStr, nuevaPrioridadStr);
        } else {
            printf("No se actualizó la prioridad (ID: %s no encontrado).\n", idProyectoStr);
        }
    } else {
         fprintf(stderr, "error al actualizar prioridad: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}

/**
 * @brief Marca un proyecto como terminado.
 */
void terminarProyecto(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (terminarproyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char confirmacion[5];

    printf("\n--- Terminar Proyecto ---\n");
    printf("Se mostrarán los proyectos EN PROCESO:\n");
    // podríamos filtrar mostrarproyectosaceptados por "en proceso" 

    // mostrar todos por ahora, el usuario debe elegir uno en proceso
     mostrarProyectosAceptados(conn);

    leerEntrada("Ingrese el ID del proyecto a marcar como TERMINADO:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);

    // confirmación
    printf("¿Está seguro que desea marcar el proyecto %s como TERMINADO? (s/n): ", idProyectoStr);
    fgets(confirmacion, sizeof(confirmacion), stdin);
    confirmacion[strcspn(confirmacion, "\n")] = '\0';
    //limpiar buffer
    int c; while ((c = getchar()) != '\n' && c != EOF);

    if (tolower(confirmacion[0]) != 's') {
        printf("Operación cancelada.\n");
        return;
    }

    // preparar update (solo si está 'en proceso')
    const char *consulta = "UPDATE proyecto_aceptado SET estatus = 'TERMINADO', porcentaje_avance = 100.00 "
                           "WHERE id = $1 AND estatus = 'EN_PROCESO';";
    const char *valores[1] = {idProyectoStr};

    // ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);

     // verificar
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            printf("Proyecto %s marcado como TERMINADO y avance al 100%%.\n", idProyectoStr);
        } else {
            // id no encontrado o ya estaba terminado
            printf("No se pudo terminar el proyecto (ID: %s no encontrado o no estaba 'EN PROCESO').\n", idProyectoStr);
        }
    } else {
        fprintf(stderr, "error al terminar proyecto: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}


/**
 * @brief Permite reasignar un supervisor a un proyecto.
 */
void reasignarSupervisorProyecto(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (reasignarsupervisorproyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char nuevoIdSupervisorStr[10];

    printf("\n--- Reasignar Supervisor a Proyecto ---\n");
    mostrarProyectosAceptados(conn); //mostrar proyectos
    leerEntrada("Ingrese el ID del proyecto:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);

    printf("\nSeleccione el nuevo supervisor de la lista:\n");
    mostrarSupervisores(conn); //mostrar supervisores
    leerEntrada("Ingrese el ID del nuevo supervisor:", nuevoIdSupervisorStr, sizeof(nuevoIdSupervisorStr), esNumeroEnteroPositivoValido);
    //no validamos si los ids existen aquí, la bd lo hará

    // preparar update
    const char *consulta = "UPDATE proyecto_aceptado SET id_supervisor = $1 WHERE id = $2;";
    const char *valores[2];
    valores[0] = nuevoIdSupervisorStr;
    valores[1] = idProyectoStr;

    // ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);

    // verificar
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            printf("Supervisor reasignado correctamente para el proyecto %s.\n", idProyectoStr);
        } else {
            printf("No se pudo reasignar supervisor (ID de proyecto: %s no encontrado).\n", idProyectoStr);
        }
    } else {
        //nuevo id de supervisor no existe (falla foreign key)
        fprintf(stderr, "error al reasignar supervisor: %s\n", PQerrorMessage(conn));
    }

     PQclear(resultado);
}