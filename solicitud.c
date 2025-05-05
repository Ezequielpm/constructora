#include "solicitud.h"
#include "structs.h"    //para SolicitudProyecto
#include "lectura.h"    //para leerEntrada
#include "validacion.h" //para validadores (esNumeroEnteroPositivoValido, esFechaValida, esDecimalValido, etc.)
#include "empresa.h"    //para mostrarEmpresas()
#include "supervisor.h"
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

/**
 * @brief Acepta una solicitud y crea el proyecto correspondiente.
 */
void aceptarSolicitud(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "error (aceptarSolicitud): conexión a bd no válida.\n");
        return;
    }

    char idSolicitudStr[10];
    ProyectoAceptado nuevoProyecto; // usaremos los campos char[] para leerEntrada
    char montoStr[20];
    char idSupervisorStr[10];
    char prioridadStr[10]; // "ROJO", "NARANJA", "AMARILLO"
    char opcionPrioridadStr[5];
    int opcionPrioridad;

    PGresult *resultado = NULL; // para resultados de consultas
    bool transaccionOk = true; //bandera para controlar la transacción

    printf("\n--- Aceptar Solicitud y Crear Proyecto ---\n");

    // 1. mostrar solicitudes en estado 'aperturado' para elegir
    printf("Se mostrarán las solicitudes en estado APERTURADO:\n");
    // (reutilizamos la lógica/query de cancelarSolicitud para mostrar)
    const char *estadoAperturado = "APERTURADO";
    const char *consultaMostrar = "SELECT s.id, s.fecha_solicitud, s.folio, e.nombre AS nombre_empresa "
                                  "FROM solicitud_proyecto s JOIN empresas e ON s.empresa_id = e.id "
                                  "WHERE s.estado = $1 ORDER BY s.id;";
    const char *valoresMostrar[1] = {estadoAperturado};
    PGresult *resMostrar = PQexecParams(conn, consultaMostrar, 1, NULL, valoresMostrar, NULL, NULL, 0);
    if (PQresultStatus(resMostrar) == PGRES_TUPLES_OK) {
        int numFilas = PQntuples(resMostrar);
         if (numFilas == 0) {
            printf("No hay solicitudes en estado APERTURADO para aceptar.\n");
            PQclear(resMostrar);
            return;
         }
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
        fprintf(stderr,"error al mostrar solicitudes aperturadas: %s\n", PQerrorMessage(conn));
        // no necesariamente retornamos, el usuario puede intentar ingresar un id de todos modos
    }
    PQclear(resMostrar); // limpiar resultado de la muestra

    // 2. pedir id de la solicitud a aceptar
    leerEntrada("Ingrese el ID de la solicitud a aceptar:", idSolicitudStr, sizeof(idSolicitudStr), esNumeroEnteroPositivoValido);

    // 3. pedir datos para el nuevo proyecto aceptado
    printf("\n--- Ingrese los datos para el nuevo proyecto aceptado ---\n");
    leerEntrada("Nombre del Proyecto:", nuevoProyecto.nombre_proyecto, sizeof(nuevoProyecto.nombre_proyecto), noEsVacio);
    leerEntrada("Fecha de Inicio (YYYY-MM-DD):", nuevoProyecto.fecha_inicio, sizeof(nuevoProyecto.fecha_inicio), esFechaValida);
    leerEntrada("Fecha de Fin (YYYY-MM-DD):", nuevoProyecto.fecha_fin, sizeof(nuevoProyecto.fecha_fin), esFechaValida);
    // (validación fecha fin >= fecha inicio omitida por simplicidad)
    leerEntrada("Monto del Proyecto:", montoStr, sizeof(montoStr), esDecimalValido);
    leerEntrada("Ubicación del Proyecto:", nuevoProyecto.ubicacion, sizeof(nuevoProyecto.ubicacion), noEsVacio);
    // la descripción es opcional, se lee directamente con fgets
    printf("Descripción del Proyecto (opcional, presione Enter para omitir): ");
    fflush(stdout);
    fgets(nuevoProyecto.descripcion, sizeof(nuevoProyecto.descripcion), stdin);
    nuevoProyecto.descripcion[strcspn(nuevoProyecto.descripcion, "\n")] = '\0'; // quitar salto de línea
    // no limpiar buffer aquí, fgets lo maneja diferente si la línea es larga

    // 4. seleccionar supervisor
    printf("\nSeleccione el supervisor para el proyecto:\n");
    mostrarSupervisores(conn); // mostrar lista de supervisores
    leerEntrada("ID del Supervisor:", idSupervisorStr, sizeof(idSupervisorStr), esNumeroEnteroPositivoValido);
    // (no validamos si el id existe aquí, la bd lo hará)

    // 5. seleccionar prioridad
    printf("\nSeleccione la prioridad del proyecto:\n");
    printf("[1] ROJO (Alta)\n");
    printf("[2] NARANJA (Media)\n");
    printf("[3] AMARILLO (Baja)\n");
    leerEntrada("Opción de Prioridad:", opcionPrioridadStr, sizeof(opcionPrioridadStr), esNumeroEnteroPositivoValido);
    opcionPrioridad = atoi(opcionPrioridadStr);
    switch(opcionPrioridad) {
        case 1: strcpy(prioridadStr, "ROJO"); break;
        case 2: strcpy(prioridadStr, "NARANJA"); break;
        case 3: strcpy(prioridadStr, "AMARILLO"); break;
        default:
            printf("Opción de prioridad inválida. Se asignará AMARILLO por defecto.\n");
            strcpy(prioridadStr, "AMARILLO");
            break;
    }

    // --- Inicio de la Transacción ---
    printf("\nIniciando transacción para aceptar solicitud...\n");
    resultado = PQexec(conn, "BEGIN");
    if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "error al iniciar transacción: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return; // no se puede continuar
    }
    PQclear(resultado); // limpiar resultado de begin

    // --- Paso 1 de la Transacción: Actualizar Solicitud ---
    const char *consultaUpdate = "UPDATE solicitud_proyecto SET estado = 'ACEPTADO' "
                                 "WHERE id = $1 AND estado = 'APERTURADO';";
    const char *valoresUpdate[1] = {idSolicitudStr};

    resultado = PQexecParams(conn, consultaUpdate, 1, NULL, valoresUpdate, NULL, NULL, 0);

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        // verificar si realmente se actualizó una fila
        char *filasAfectadasStr = PQcmdTuples(resultado);
        if (filasAfectadasStr == NULL || strcmp(filasAfectadasStr, "1") != 0) {
            // id no encontrado o no estaba 'aperturado'
            fprintf(stderr, "error: no se pudo actualizar la solicitud (ID: %s). Verifique que exista y esté en estado APERTURADO.\n", idSolicitudStr);
            transaccionOk = false; // marcar para rollback
        } else {
             printf("Solicitud %s actualizada a ACEPTADO.\n", idSolicitudStr);
        }
    } else {
        fprintf(stderr, "error al actualizar solicitud: %s\n", PQerrorMessage(conn));
        transaccionOk = false; // marcar para rollback
    }
    PQclear(resultado); // limpiar resultado del update

    // --- Paso 2 de la Transacción: Insertar Proyecto Aceptado (solo si el update fue bien) ---
    if (transaccionOk) {
        const char *consultaInsert = "INSERT INTO proyecto_aceptado "
                                     "(solicitud_id, nombre_proyecto, fecha_inicio, fecha_fin, monto, "
                                     "estatus, porcentaje_avance, ubicacion, descripcion, prioridad, id_supervisor) "
                                     "VALUES ($1, $2, $3, $4, $5, 'EN_PROCESO', 0.00, $6, $7, $8, $9);";
        // ¡Son 9 parámetros!
        const char *valoresInsert[9];
        valoresInsert[0] = idSolicitudStr;
        valoresInsert[1] = nuevoProyecto.nombre_proyecto;
        valoresInsert[2] = nuevoProyecto.fecha_inicio;
        valoresInsert[3] = nuevoProyecto.fecha_fin;
        valoresInsert[4] = montoStr; // monto como string
        valoresInsert[5] = nuevoProyecto.ubicacion;
        valoresInsert[6] = (strlen(nuevoProyecto.descripcion) > 0) ? nuevoProyecto.descripcion : NULL; //se envia null si está vacío
        valoresInsert[7] = prioridadStr;
        valoresInsert[8] = idSupervisorStr;

        resultado = PQexecParams(conn, consultaInsert, 9, NULL, valoresInsert, NULL, NULL, 0);

        if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
            // error común: id_supervisor no existe, o solicitud_id no existe (raro si update funcionó)
            fprintf(stderr, "error al insertar en proyecto_aceptado: %s\n", PQerrorMessage(conn));
            transaccionOk = false; // marcar para rollback
        } else {
             printf("Proyecto aceptado creado exitosamente.\n");
        }
         PQclear(resultado); // limpiar resultado del insert
    }

    // --- Finalizar Transacción ---
    if (transaccionOk) {
        printf("Confirmando transacción (COMMIT)...\n");
        resultado = PQexec(conn, "COMMIT");
        if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
            // error muy grave si commit falla después de operaciones exitosas
            fprintf(stderr, "¡¡¡ERROR CRÍTICO AL HACER COMMIT!!! %s\n", PQerrorMessage(conn));
        } else {
            printf("¡Solicitud aceptada y proyecto creado con éxito!\n");
        }
    } else {
        printf("Revirtiendo transacción (ROLLBACK) debido a errores...\n");
        resultado = PQexec(conn, "ROLLBACK");
         if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
            fprintf(stderr, "Error al hacer ROLLBACK: %s\n", PQerrorMessage(conn));
        } else {
             printf("Cambios revertidos.\n");
        }
    }
    PQclear(resultado); // limpiar resultado de commit/rollback
}
