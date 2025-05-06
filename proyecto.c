#include "proyecto.h"
#include "structs.h"
#include "lectura.h"
#include "validacion.h"
#include "supervisor.h" //para mostrarSupervisores() en reasignacion

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>     //para tolower
// libpq-fe.h ya está incluido via proyecto.h


// -------------------------------------------------------------------------
// Funciones usadas por rank 0
// -------------------------------------------------------------------------

/**
 * @brief Muestra la lista de proyectos aceptados, con opción de filtro.
 */
void mostrarProyectosAceptados(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "error (mostrarProyectosAceptados): conexión a bd no válida.\n");
        return;
    }

    char opcionFiltroStr[5];
    int opcionFiltro;
    const char *filtroEstatus = NULL; //null para mostrar todos por defecto

    printf("\n--- Mostrar Proyectos Aceptados ---\n");
    printf("Filtrar por estado:\n");
    printf("[1] EN PROCESO\n");
    printf("[2] TERMINADO\n");
    printf("[3] TODOS\n");
    leerEntrada("Seleccione filtro:", opcionFiltroStr, sizeof(opcionFiltroStr), esNumeroEnteroPositivoValido);
    opcionFiltro = atoi(opcionFiltroStr);

    switch (opcionFiltro) {
        case 1: filtroEstatus = "EN_PROCESO"; break;
        case 2: filtroEstatus = "TERMINADO"; break;
        case 3: filtroEstatus = NULL; break; //NULL para mostrar todos
        default:
            printf("Opción de filtro inválida. Mostrando todos.\n");
            filtroEstatus = NULL;
            break;
    }

    //llamar a la función que ejecuta la consulta y muestra
    ejecutarMostrarProyectosAceptadosDB(conn, filtroEstatus);
}


/**
 * @brief Actualiza el porcentaje de avance de un proyecto.
 */
void actualizarAvanceProyecto(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "error (actualizarAvanceProyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char nuevoPorcentajeStr[10];
    double nuevoPorcentaje;

    printf("\n--- Actualizar Avance de Proyecto ---\n");
    //se muestran los proyectos para facilitar la selección del ID
    ejecutarMostrarProyectosAceptadosDB(conn, NULL); //mostrar todos

    leerEntrada("Ingrese el ID del proyecto a actualizar:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);
    leerEntrada("Ingrese el nuevo porcentaje de avance (0-100):", nuevoPorcentajeStr, sizeof(nuevoPorcentajeStr), esDecimalValido);

    //se vaida el rango del porcentaje
    nuevoPorcentaje = atof(nuevoPorcentajeStr);
    if (nuevoPorcentaje < 0.0 || nuevoPorcentaje > 100.0) {
        printf("error: el porcentaje debe estar entre 0 y 100.\n");
        return;
    }

    //llamar a la función ejecutora
    int resultadoDB = ejecutarActualizarAvanceDB(conn, idProyectoStr, nuevoPorcentajeStr);

    //interpretar el resultado
    if (resultadoDB == 0) {
        printf("Avance del proyecto %s actualizado correctamente a %.2f%%.\n", idProyectoStr, nuevoPorcentaje);
    } else if (resultadoDB == -2) {
        printf("No se actualizó el avance (ID: %s no encontrado).\n", idProyectoStr);
    } else {
        printf("Fallo al actualizar el avance del proyecto.\n");
    }
}

/**
 * @brief Actualiza la prioridad (semaforización) de un proyecto.
 */
void actualizarPrioridadProyecto(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (actualizarPrioridadProyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char opcionPrioridadStr[5];
    int opcionPrioridad;
    char nuevaPrioridadStr[10]; // "ROJO", "NARANJA", "AMARILLO"

    printf("\n--- Actualizar Prioridad de Proyecto ---\n");
    ejecutarMostrarProyectosAceptadosDB(conn, NULL); //mostrar todos

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

    //llamar a la función ejecutora
    int resultadoDB = ejecutarActualizarPrioridadDB(conn, idProyectoStr, nuevaPrioridadStr);

    //interpretar el resultado
    if (resultadoDB == 0) {
        printf("Prioridad del proyecto %s actualizada a %s.\n", idProyectoStr, nuevaPrioridadStr);
    } else if (resultadoDB == -2) {
        printf("No se actualizó la prioridad (ID: %s no encontrado).\n", idProyectoStr);
    } else {
        printf("Fallo al actualizar la prioridad del proyecto.\n");
    }
}

/**
 * @brief Marca un proyecto como terminado.
 */
void terminarProyecto(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (terminarProyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char confirmacion[5];

    printf("\n--- Terminar Proyecto ---\n");
    printf("Se mostrarán los proyectos EN PROCESO:\n");
    ejecutarMostrarProyectosAceptadosDB(conn, "EN_PROCESO"); //mostrar solo en proceso

    leerEntrada("Ingrese el ID del proyecto a marcar como TERMINADO:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);

    //limpiar buffer antes de pedir confirmación
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    // confirmación
    printf("¿Está seguro que desea marcar el proyecto %s como TERMINADO? (s/n): ", idProyectoStr);
    fflush(stdout);
    if (fgets(confirmacion, sizeof(confirmacion), stdin) != NULL) {
        confirmacion[strcspn(confirmacion, "\n")] = '\0';
        int len = strlen(confirmacion);
        if (len == sizeof(confirmacion) - 1 && confirmacion[len-1] != '\0') { while ((c = getchar()) != '\n' && c != EOF); }
        else if (len == 0) { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }
    } else { confirmacion[0] = 'n'; confirmacion[1] = '\0'; }


    if (tolower(confirmacion[0]) != 's') {
        printf("Operación cancelada.\n");
        return;
    }

    //llamar a la función ejecutora
    int resultadoDB = ejecutarTerminarProyectoDB(conn, idProyectoStr);

    //interpretar el resultado
    if (resultadoDB == 0) {
        printf("Proyecto %s marcado como TERMINADO y avance al 100%%.\n", idProyectoStr);
    } else if (resultadoDB == -2) {
        printf("No se pudo terminar el proyecto (ID: %s no encontrado o no estaba 'EN PROCESO').\n", idProyectoStr);
    } else {
        printf("Fallo al terminar el proyecto.\n");
    }
}


/**
 * @brief Permite reasignar un supervisor a un proyecto.
 */
void reasignarSupervisorProyecto(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "error (reasignarSupervisorProyecto): conexión a bd no válida.\n");
        return;
    }

    char idProyectoStr[10];
    char nuevoIdSupervisorStr[10];

    printf("\n--- Reasignar Supervisor a Proyecto ---\n");
    ejecutarMostrarProyectosAceptadosDB(conn, NULL); //mostrar todos los proyectos
    leerEntrada("Ingrese el ID del proyecto:", idProyectoStr, sizeof(idProyectoStr), esNumeroEnteroPositivoValido);

    printf("\nSeleccione el nuevo supervisor de la lista:\n");
    mostrarSupervisores(conn); //mostrar supervisores disponibles
    leerEntrada("Ingrese el ID del nuevo supervisor:", nuevoIdSupervisorStr, sizeof(nuevoIdSupervisorStr), esNumeroEnteroPositivoValido);

    //llamar a la función ejecutora
    int resultadoDB = ejecutarReasignarSupervisorDB(conn, idProyectoStr, nuevoIdSupervisorStr);

    //interpretar el resultado
    if (resultadoDB == 0) {
        printf("Supervisor reasignado correctamente para el proyecto %s.\n", idProyectoStr);
    } else if (resultadoDB == -2) {
        printf("No se pudo reasignar supervisor (ID de proyecto: %s no encontrado).\n", idProyectoStr);
    } else {
        // Error de BD (FK supervisor no existe), la función ejecutora ya imprimió detalles
        printf("Fallo al reasignar supervisor.\n");
    }
}


// --------------------------------------------------------------------
//  Funciones ejecutoras de BD para trabajadores MPI
// --------------------------------------------------------------------

/**
 * @brief Ejecuta la consulta y muestra los proyectos aceptados filtrados por estado.
 */
int ejecutarMostrarProyectosAceptadosDB(PGconn *conn, const char *filtroEstatus) {
    if (conn == NULL) {
        fprintf(stderr, "Rank Worker BD Error (ejecutarMostrarProyectosAceptadosDB): Conexión nula.\n");
        return -1;
    }

    // Base de la consulta con todos los JOINs necesarios
    const char *consultaBase =
        "SELECT "
        "  p.id, p.nombre_proyecto, p.estatus, p.prioridad, p.porcentaje_avance, " // 0-4
        "  p.fecha_inicio, p.fecha_fin, p.monto, "                                 // 5-7
        "  e.nombre AS nombre_empresa, "                                           // 8
        "  sup.nombre || ' ' || sup.apellidos AS nombre_supervisor "              // 9
        "FROM "
        "  proyecto_aceptado p "
        "JOIN "
        "  solicitud_proyecto sp ON p.solicitud_id = sp.id "
        "JOIN "
        "  empresas e ON sp.empresa_id = e.id "
        "JOIN "
        "  supervisores sup ON p.id_supervisor = sup.id ";

    char consultaCompleta[1024];
    const char *valores[1];
    int numParams = 0;

    // Construir la consulta final añadiendo el filtro si existe
    if (filtroEstatus != NULL) {
        snprintf(consultaCompleta, sizeof(consultaCompleta), "%s WHERE p.estatus = $1 ORDER BY p.prioridad, p.id;", consultaBase);
        valores[0] = filtroEstatus;
        numParams = 1;
    } else {
        // Sin filtro, mostrar todos ordenados
        snprintf(consultaCompleta, sizeof(consultaCompleta), "%s ORDER BY p.estatus, p.prioridad, p.id;", consultaBase);
    }

    // Ejecutar la consulta construida
    PGresult *resultado = PQexecParams(conn, consultaCompleta, numParams, NULL, (numParams > 0 ? valores : NULL), NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Rank Worker BD Error (ejecutarMostrarProyectosAceptadosDB): %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return -1; // Error de BD
    }

    //imprimir resultados (esta parte la hace el trabajador)
    int numFilas = PQntuples(resultado);
    printf("\n--- Lista de Proyectos Aceptados (%s) (%d) --- (Reportado por Rank Worker)\n", (filtroEstatus ? filtroEstatus : "TODOS"), numFilas);
    printf("| %-4s | %-25s | %-10s | %-8s | %-7s | %-10s | %-10s | %-15s | %-20s | %-25s |\n",
           "ID", "Nombre Proyecto", "Estado", "Prioridad", "Avance%", "F. Inicio", "F. Fin", "Monto", "Empresa", "Supervisor");
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    if (numFilas == 0) {
         printf("| %-165s |\n", "No se encontraron proyectos con los criterios seleccionados.");
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

    PQclear(resultado);
    return 0; //exito
}


/**
 * @brief Ejecuta la actualización del porcentaje de avance de un proyecto en la BD.
 */
int ejecutarActualizarAvanceDB(PGconn *conn, const char *idProyecto, const char *nuevoPorcentajeStr) {
    if (conn == NULL || idProyecto == NULL || nuevoPorcentajeStr == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarActualizarAvanceDB): Conexión o datos nulos.\n");
        return -1;
    }

    //se valida el rango aquí también por seguridad, aunque rank 0 ya lo hizo
    double nuevoPorcentaje = atof(nuevoPorcentajeStr);
    if (nuevoPorcentaje < 0.0 || nuevoPorcentaje > 100.0) {
        fprintf(stderr, "Rank Worker (ejecutarActualizarAvanceDB): Porcentaje inválido (%.2f).\n", nuevoPorcentaje);
        return -1; 
    }

    const char *consulta = "UPDATE proyecto_aceptado SET porcentaje_avance = $1 WHERE id = $2;";
    const char *valores[2];
    valores[0] = nuevoPorcentajeStr; // Enviar como string
    valores[1] = idProyecto;

    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0;

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            estadoFinal = 0; // Éxito
        } else {
            estadoFinal = -2; // ID no encontrado
        }
    } else {
        fprintf(stderr, "Rank Worker BD Error (ejecutarActualizarAvanceDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; //error de BD
    }

    PQclear(resultado);
    return estadoFinal;
}

/**
 * @brief Ejecuta la actualización de la prioridad de un proyecto en la BD.
 */
int ejecutarActualizarPrioridadDB(PGconn *conn, const char *idProyecto, const char *nuevaPrioridadStr) {
     if (conn == NULL || idProyecto == NULL || nuevaPrioridadStr == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarActualizarPrioridadDB): Conexión o datos nulos.\n");
        return -1;
    }

    // Validar prioridad por seguridad
    if (strcmp(nuevaPrioridadStr, "ROJO") != 0 && strcmp(nuevaPrioridadStr, "NARANJA") != 0 && strcmp(nuevaPrioridadStr, "AMARILLO") != 0) {
         fprintf(stderr, "Rank Worker (ejecutarActualizarPrioridadDB): Prioridad inválida '%s'.\n", nuevaPrioridadStr);
         return -1;
    }

    const char *consulta = "UPDATE proyecto_aceptado SET prioridad = $1 WHERE id = $2;";
    const char *valores[2];
    valores[0] = nuevaPrioridadStr;
    valores[1] = idProyecto;

    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0;

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            estadoFinal = 0; // Éxito
        } else {
            estadoFinal = -2; // ID no encontrado
        }
    } else {
        fprintf(stderr, "Rank Worker BD Error (ejecutarActualizarPrioridadDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; // Error de BD
    }

    PQclear(resultado);
    return estadoFinal;
}

/**
 * @brief Ejecuta la finalización de un proyecto en la BD (cambia estado y avance).
 */
int ejecutarTerminarProyectoDB(PGconn *conn, const char *idProyecto) {
    if (conn == NULL || idProyecto == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarTerminarProyectoDB): Conexión o ID nulo.\n");
        return -1;
    }

    //actualizar estado a 'TERMINADO' y avance a 100% solo si está 'EN_PROCESO'
    const char *consulta = "UPDATE proyecto_aceptado SET estatus = 'TERMINADO', porcentaje_avance = 100.00 "
                           "WHERE id = $1 AND estatus = 'EN_PROCESO';";
    const char *valores[1] = {idProyecto};

    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0;

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            estadoFinal = 0; //exito
        } else {
            // ID no encontrado o no estaba 'EN_PROCESO'
            estadoFinal = -2;
        }
    } else {
        fprintf(stderr, "Rank Worker BD Error (ejecutarTerminarProyectoDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; //error de BD
    }

    PQclear(resultado);
    return estadoFinal;
}

/**
 * @brief Ejecuta la reasignación de supervisor para un proyecto en la BD.
 */
int ejecutarReasignarSupervisorDB(PGconn *conn, const char *idProyecto, const char *nuevoIdSupervisorStr) {
    if (conn == NULL || idProyecto == NULL || nuevoIdSupervisorStr == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarReasignarSupervisorDB): Conexión o datos nulos.\n");
        return -1;
    }

    const char *consulta = "UPDATE proyecto_aceptado SET id_supervisor = $1 WHERE id = $2;";
    const char *valores[2];
    valores[0] = nuevoIdSupervisorStr;
    valores[1] = idProyecto;

    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0;

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            estadoFinal = 0; // Éxito
        } else {
            estadoFinal = -2; // ID de proyecto no encontrado
        }
    } else {
        //error común: nuevo id_supervisor no existe (falla foreign key)
        fprintf(stderr, "Rank Worker BD Error (ejecutarReasignarSupervisorDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; // Error de BD
    }

     PQclear(resultado);
    return estadoFinal;
}
