#include "solicitud.h"
#include "structs.h"    //para SolicitudProyecto
#include "lectura.h"    //para leerEntrada
#include "validacion.h" //para validadores (esNumeroEnteroPositivoValido, esFechaValida, esDecimalValido, etc.)
#include "empresa.h"    //para mostrarEmpresas()

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//libpq-fe.h ya está incluido en solicitud.h

/**
 * @brief Registra una nueva solicitud de proyecto.
 */
void registrarSolicitud(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (registrarSolicitud): Conexión a BD no válida.\n");
        return;
    }

    SolicitudProyecto nuevaSolicitud; //no usamos todos los campos aquí
    char empresaIdStr[10];
    char presupuestoStr[20];
    char anticipoStr[20];

    printf("\n--- Registro de Nueva Solicitud de Proyecto ---\n");

    //seleccionar Empresa
    printf("Seleccione la empresa solicitante de la lista:\n");
    mostrarEmpresas(conn); //se muestran las empresas para elegir ID
    leerEntrada("ID de la Empresa:", empresaIdStr, sizeof(empresaIdStr), esNumeroEnteroPositivoValido);
    //Nota: No estamos validando aquí si el ID de empresa realmente existe en la BD
    //la restricción FOREIGN KEY lo hará al intentar insertar.

    //resto de los datos
    leerEntrada("Fecha de Solicitud (YYYY-MM-DD):", nuevaSolicitud.fecha_solicitud, sizeof(nuevaSolicitud.fecha_solicitud), esFechaValida);
    leerEntrada("Folio de la Solicitud:", nuevaSolicitud.folio, sizeof(nuevaSolicitud.folio), noEsVacio); 
    leerEntrada("Monto del Presupuesto:", presupuestoStr, sizeof(presupuestoStr), esDecimalValido);
    leerEntrada("Monto del Anticipo:", anticipoStr, sizeof(anticipoStr), esDecimalValido);

    //preparar consulta (estado siempre 'APERTURADO' al registrar)
    const char *consulta = "INSERT INTO solicitud_proyecto "
                           "(empresa_id, fecha_solicitud, presupuesto, anticipo, folio, estado) "
                           "VALUES ($1, $2, $3, $4, $5, 'APERTURADO');";
    const char *valores[5];
    valores[0] = empresaIdStr;
    valores[1] = nuevaSolicitud.fecha_solicitud;
    valores[2] = presupuestoStr; //se inserta como string, PG lo convierte a NUMERIC
    valores[3] = anticipoStr;    //se inserta como string, PG lo convierte a NUMERIC
    valores[4] = nuevaSolicitud.folio;

    //ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 5, NULL, valores, NULL, NULL, 0);

    //verificar
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        printf("Solicitud registrada exitosamente con estado APERTURADO.\n");
    } else {
        //error: empresa_id no existe (violación de Foreign Key)
        fprintf(stderr, "Error al registrar solicitud: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}

/**
 * @brief Muestra solicitudes filtradas por estado.
 */
void mostrarSolicitudesPorEstado(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (mostrarSolicitudesPorEstado): Conexión a BD no válida.\n");
        return;
    }

    char opcionStr[5];
    int opcion;
    const char *estadoFiltro = NULL;

    printf("\n--- Mostrar Solicitudes por Estado ---\n");
    printf("[1] APERTURADO\n");
    printf("[2] ACEPTADO\n");
    printf("[3] CANCELADO\n");
    leerEntrada("Seleccione el estado a mostrar:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
    opcion = atoi(opcionStr);

    switch (opcion) {
        case 1: estadoFiltro = "APERTURADO"; break;
        case 2: estadoFiltro = "ACEPTADO"; break;
        case 3: estadoFiltro = "CANCELADO"; break;
        default:
            printf("Opción no válida.\n");
            return;
    }

    //consulta con JOIN para obtener el nombre de la empresa
    const char *consulta = "SELECT s.id, s.fecha_solicitud, s.folio, s.presupuesto, s.anticipo, s.estado, e.nombre AS nombre_empresa, s.razon_cancelacion "
                           "FROM solicitud_proyecto s "
                           "JOIN empresas e ON s.empresa_id = e.id "
                           "WHERE s.estado = $1 ORDER BY s.id;";
    const char *valores[1];
    valores[0] = estadoFiltro;

    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error al consultar solicitudes: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\n--- Solicitudes con Estado: %s (%d) ---\n", estadoFiltro, numFilas);
    printf("| %-4s | %-10s | %-15s | %-15s | %-15s | %-10s | %-25s | %-30s |\n",
           "ID", "Fecha Sol.", "Folio", "Presupuesto", "Anticipo", "Estado", "Empresa", "Razón Cancelación");
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    for (int fila = 0; fila < numFilas; fila++) {
        const char* razon = PQgetisnull(resultado, fila, 7) ? "" : PQgetvalue(resultado, fila, 7); // Manejar NULL para razón
        printf("| %-4s | %-10s | %-15s | %-15s | %-15s | %-10s | %-25s | %-30s |\n",
               PQgetvalue(resultado, fila, 0), // id
               PQgetvalue(resultado, fila, 1), // fecha_solicitud
               PQgetvalue(resultado, fila, 2), // folio
               PQgetvalue(resultado, fila, 3), // presupuesto
               PQgetvalue(resultado, fila, 4), // anticipo
               PQgetvalue(resultado, fila, 5), // estado
               PQgetvalue(resultado, fila, 6), // nombre_empresa
               razon //razon_cancelacion (puede ser vacio)
              );
    }
    printf("-------------------------------------------------------------------------------------------------------------------------------------------------------------\n");

    PQclear(resultado);
}

/**
 * @brief Cancela una solicitud que esté en estado 'APERTURADO'.
 */
void cancelarSolicitud(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (cancelarSolicitud): Conexión a BD no válida.\n");
        return;
    }

    char idSolicitudStr[10];
    char razonCancelacion[256];

    printf("\n--- Cancelar Solicitud de Proyecto ---\n");
    printf("Se mostrarán las solicitudes en estado APERTURADO:\n");

    //mostrar solicitudes 'APERTURADO' para facilitar la selección
    const char *estadoAperturado = "APERTURADO";
    const char *consultaMostrar = "SELECT s.id, s.fecha_solicitud, s.folio, e.nombre AS nombre_empresa "
                                  "FROM solicitud_proyecto s JOIN empresas e ON s.empresa_id = e.id "
                                  "WHERE s.estado = $1 ORDER BY s.id;";
    const char *valoresMostrar[1] = {estadoAperturado};
    PGresult *resMostrar = PQexecParams(conn, consultaMostrar, 1, NULL, valoresMostrar, NULL, NULL, 0);
    if (PQresultStatus(resMostrar) == PGRES_TUPLES_OK) {
         int numFilas = PQntuples(resMostrar);
         printf("--- Solicitudes APERTURADAS (%d) ---\n", numFilas);
         printf("| %-4s | %-10s | %-15s | %-25s |\n", "ID", "Fecha Sol.", "Folio", "Empresa");
         printf("----------------------------------------------------------------\n");
         for (int i=0; i<numFilas; ++i) {
             printf("| %-4s | %-10s | %-15s | %-25s |\n",
                    PQgetvalue(resMostrar, i, 0), PQgetvalue(resMostrar, i, 1),
                    PQgetvalue(resMostrar, i, 2), PQgetvalue(resMostrar, i, 3));
         }
         printf("----------------------------------------------------------------\n");
    } else {
        fprintf(stderr,"Error al mostrar solicitudes aperturadas: %s\n", PQerrorMessage(conn));
    }
    PQclear(resMostrar); //limpiar resultado de la muestra


    //pedir ID a cancelar
    leerEntrada("Ingrese el ID de la solicitud a cancelar:", idSolicitudStr, sizeof(idSolicitudStr), esNumeroEnteroPositivoValido);

    //pedir razón de cancelación
    leerEntrada("Ingrese la razón de la cancelación:", razonCancelacion, sizeof(razonCancelacion), noEsVacio);//por ahora pedimos que la razon no este vacia, se modificará luego


    //preparar consulta UPDATE (solo si está APERTURADO)
    const char *consultaUpdate = "UPDATE solicitud_proyecto SET estado = 'CANCELADO', razon_cancelacion = $1 "
                                 "WHERE id = $2 AND estado = 'APERTURADO';";
    const char *valoresUpdate[2];
    valoresUpdate[0] = razonCancelacion;
    valoresUpdate[1] = idSolicitudStr;

    //ejecutar
    PGresult *resultadoUpdate = PQexecParams(conn, consultaUpdate, 2, NULL, valoresUpdate, NULL, NULL, 0);

    //verificar
    if (PQresultStatus(resultadoUpdate) == PGRES_COMMAND_OK) {
        char *filasAfectadasStr = PQcmdTuples(resultadoUpdate);
        if (filasAfectadasStr != NULL && strcmp(filasAfectadasStr, "1") == 0) {
            printf("Solicitud %s cancelada correctamente.\n", idSolicitudStr);
        } else {
            //esto puede ocurrir si el ID no existe o si la solicitud no estaba en estado 'APERTURADO'
            printf("No se pudo cancelar la solicitud (ID: %s). Verifique que exista y esté en estado 'APERTURADO'.\n", idSolicitudStr);
        }
    } else {
        fprintf(stderr, "Error al cancelar solicitud: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultadoUpdate);
}