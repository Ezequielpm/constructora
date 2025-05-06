#include "supervisor.h"
#include "structs.h"      
#include "lectura.h"      
#include "validacion.h"   //para las funciones de validación

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//libpq-fe.h ya está incluido en supervisor.h

/**
 * @brief Registra un nuevo supervisor en la base de datos.
 */
void registrarSupervisor(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (registrarSupervisor): Conexión a BD no válida.\n");
        return;
    }

    Supervisor nuevoSupervisor; //no necesitamos el ID para registrar porque se genera solo

    printf("\n--- Registro de Nuevo Supervisor ---\n");

    //se solcitan los datos usando leerEntrada y las validaciones correspondientes
    leerEntrada("Nombre(s):", nuevoSupervisor.nombre, sizeof(nuevoSupervisor.nombre), sonSoloLetras);
    leerEntrada("Apellidos:", nuevoSupervisor.apellidos, sizeof(nuevoSupervisor.apellidos), sonSoloLetras);
    leerEntrada("Teléfono (solo números):", nuevoSupervisor.telefono, sizeof(nuevoSupervisor.telefono), sonSoloNumeros);
    leerEntrada("Correo electrónico:", nuevoSupervisor.correo, sizeof(nuevoSupervisor.correo), esCorreoValido);

    //se prepara la consulta parametrizada
    const char *consulta = "INSERT INTO supervisores (nombre, apellidos, telefono, correo) VALUES ($1, $2, $3, $4);";
    const char *valores[4];
    valores[0] = nuevoSupervisor.nombre;
    valores[1] = nuevoSupervisor.apellidos;
    valores[2] = nuevoSupervisor.telefono;
    valores[3] = nuevoSupervisor.correo;

    //ejecutar la consulta
    PGresult *resultado = PQexecParams(conn, consulta, 4, NULL, valores, NULL, NULL, 0);

    //verificar el resultado
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        printf("Supervisor registrado exitosamente.\n");
    } else {
        fprintf(stderr, "Error al registrar supervisor: %s\n", PQerrorMessage(conn));
    }

    //liberar memoria del resultado
    PQclear(resultado);
}

/**
 * @brief Muestra la lista de todos los supervisores.
 */
void mostrarSupervisores(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "Error (mostrarSupervisores): Conexión a BD no válida.\n");
        return;
    }

    const char *consulta = "SELECT id, nombre, apellidos, telefono, correo FROM supervisores ORDER BY id;";
    PGresult *resultado = PQexecParams(conn, consulta, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error al consultar supervisores: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\n--- Lista de Supervisores (%d) ---\n", numFilas);
    printf("----------------------------------------------------------------------------------\n");
    printf("| %-4s | %-20s | %-20s | %-15s | %-25s |\n", "ID", "Nombre(s)", "Apellidos", "Teléfono", "Correo");
    printf("----------------------------------------------------------------------------------\n");

    for (int fila = 0; fila < numFilas; fila++) {
        printf("| %-4s | %-20s | %-20s | %-15s | %-25s |\n",
               PQgetvalue(resultado, fila, 0), // id
               PQgetvalue(resultado, fila, 1), // nombre
               PQgetvalue(resultado, fila, 2), // apellidos
               PQgetvalue(resultado, fila, 3), // telefono
               PQgetvalue(resultado, fila, 4)  // correo
              );
    }
    printf("----------------------------------------------------------------------------------\n");

    PQclear(resultado);
}

/**
 * @brief Actualiza un campo específico de un supervisor existente.
 */
void actualizarSupervisor(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "Error (actualizarSupervisor): Conexión a BD no válida.\n");
        return;
    }

    //mostrar lista para que el usuario elija ID
    mostrarSupervisores(conn);

    char idSupervisorStr[10];
    char opcionStr[5];
    char nuevoValor[101];
    int opcion;
    const char *consulta = NULL;
    const char *valores[2]; // [0] = nuevoValor, [1] = idSupervisorStr

    printf("\n--- Actualizar Supervisor ---\n");
    leerEntrada("Ingrese el ID del supervisor a actualizar:", idSupervisorStr, sizeof(idSupervisorStr), esNumeroEnteroPositivoValido);
    valores[1] = idSupervisorStr; //el ID siempre será el segundo parámetro

    printf("\nSeleccione el campo a actualizar:\n");
    printf("[1] Nombre(s)\n");
    printf("[2] Apellidos\n");
    printf("[3] Teléfono\n");
    printf("[4] Correo\n");
    printf("[5] Cancelar\n");
    leerEntrada("Opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
    opcion = atoi(opcionStr);

    bool (*funcionValidadora)(const char*) = NULL; //puntero a la función de validación

    switch (opcion) {
        case 1:
            consulta = "UPDATE supervisores SET nombre = $1 WHERE id = $2;";
            leerEntrada("Nuevo Nombre(s):", nuevoValor, sizeof(nuevoValor), sonSoloLetras);
            funcionValidadora = sonSoloLetras; //se guarda para revalidar si es necesario
            break;
        case 2:
            consulta = "UPDATE supervisores SET apellidos = $1 WHERE id = $2;";
            leerEntrada("Nuevos Apellidos:", nuevoValor, sizeof(nuevoValor), sonSoloLetras);
             funcionValidadora = sonSoloLetras;
            break;
        case 3:
            consulta = "UPDATE supervisores SET telefono = $1 WHERE id = $2;";
            leerEntrada("Nuevo Teléfono:", nuevoValor, sizeof(nuevoValor), sonSoloNumeros);
             funcionValidadora = sonSoloNumeros;
            break;
        case 4:
            consulta = "UPDATE supervisores SET correo = $1 WHERE id = $2;";
            leerEntrada("Nuevo Correo:", nuevoValor, sizeof(nuevoValor), esCorreoValido);
             funcionValidadora = esCorreoValido;
            break;
        case 5:
            printf("Actualización cancelada.\n");
            return; //salir de la función
        default:
            printf("Opción no válida.\n");
            return; //salir de la función
    }

    //asignar el nuevo valor al array de parámetros
    valores[0] = nuevoValor;

    //ejecutar la consulta
    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        //verificar cuantas filas fueron afectadas
        char *filasAfectadasStr = PQcmdTuples(resultado);
        if (filasAfectadasStr != NULL && strcmp(filasAfectadasStr, "1") == 0) {
             printf("Supervisor actualizado correctamente.\n");
        } else {
            printf("Operación completada, pero no se encontró un supervisor con el ID %s (0 filas actualizadas).\n", idSupervisorStr);
        }
    } else {
        fprintf(stderr, "Error al actualizar supervisor: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}

/**
 * @brief Elimina un supervisor de la base de datos, previa confirmación.
 */
void eliminarSupervisor(PGconn *conn) {
     if (conn == NULL) {
        fprintf(stderr, "Error (eliminarSupervisor): Conexión a BD no válida.\n");
        return;
    }

    //mostrar lista para que el usuario elija ID
    mostrarSupervisores(conn);

    char idSupervisorStr[10];
    char confirmacion[5]; //para "s" o "n" + '\0'

    printf("\n--- Eliminar Supervisor ---\n");
    // Leer el ID usando la función segura
    leerEntrada("Ingrese el ID del supervisor a eliminar:", idSupervisorStr, sizeof(idSupervisorStr), esNumeroEnteroPositivoValido);

    // --- CORRECCIÓN BUG v2: Limpieza de buffer más simple y robusta ---
    // Después de leer el ID con leerEntrada (que usa fgets), el '\n' ya fue consumido por fgets.
    // El problema suele ser si *otras* lecturas previas dejaron algo.
    // Una forma más directa es intentar leer la confirmación y luego limpiar si es necesario.

    //pedir confirmación
    printf("¿Está seguro que desea eliminar al supervisor con ID %s? (s/n): ", idSupervisorStr);
    printf("\nADVERTENCIA: Esto fallará si el supervisor tiene proyectos asignados.\nConfirmar (s/n):");
    fflush(stdout); // Asegurar que el prompt se muestre antes de leer

    // Leer la confirmación directamente
    if (fgets(confirmacion, sizeof(confirmacion), stdin) != NULL) {
        // Quitar el salto de línea si fgets lo leyó
        confirmacion[strcspn(confirmacion, "\n")] = '\0';

        // Limpiar el buffer DESPUÉS de leer la confirmación,
        // SOLO si la entrada llenó el buffer (no se encontró '\n')
        int len = strlen(confirmacion);
        if (len == sizeof(confirmacion) - 1 && confirmacion[len-1] != '\0') {
             int c;
             //printf("\nLimpiando buffer post-confirmación...\n"); // Debug
             while ((c = getchar()) != '\n' && c != EOF); // Consumir el resto
        }
         // Si la longitud es 0, significa que fgets probablemente solo leyó el '\n' residual.
         // En este caso, la confirmación no es válida.
         else if (len == 0) {
             //printf("\nEntrada de confirmación vacía detectada, cancelando...\n"); // Debug
             confirmacion[0] = 'n'; // Forzar cancelación
             confirmacion[1] = '\0';
         }

    } else {
        // Error al leer la confirmación, asignar un valor por defecto para cancelar
        confirmacion[0] = 'n';
        confirmacion[1] = '\0';
        fprintf(stderr, "Error al leer la confirmación.\n");
    }


    //convertir a minuscula para comparar sin importar mayusculas o minusculas
    if (tolower(confirmacion[0]) != 's') {
        printf("Eliminación cancelada.\n");
        return;
    }

    // --- El resto de la lógica de eliminación ---
    //preparar consulta y parámetros
    const char *consulta = "DELETE FROM supervisores WHERE id = $1;";
    const char *valores[1];
    valores[0] = idSupervisorStr;

    //ejecutar (lógica secuencial)
    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadasStr = PQcmdTuples(resultado);
        if (filasAfectadasStr != NULL && strcmp(filasAfectadasStr, "1") == 0) {
             printf("Supervisor eliminado correctamente.\n");
        } else {
            // Puede ser ID no encontrado o FK constraint
            printf("No se eliminó el supervisor (ID no encontrado o tiene proyectos asignados).\n");
        }
    } else {
        fprintf(stderr, "Error al eliminar supervisor: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}



// --------------------------------------------------------------------
// --- Funciones Ejecutoras de BD para Trabajadores MPI ---
// --------------------------------------------------------------------

/**
 * @brief Ejecuta la inserción de un nuevo supervisor en la BD.
 */
int ejecutarRegistrarSupervisorDB(PGconn *conn, const Supervisor *datosSupervisor) {
    if (conn == NULL || datosSupervisor == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarRegistrarSupervisorDB): Conexión o datos nulos.\n");
        return -1;
    }

    const char *consulta = "INSERT INTO supervisores (nombre, apellidos, telefono, correo) VALUES ($1, $2, $3, $4);";
    const char *valores[4];
    valores[0] = datosSupervisor->nombre;
    valores[1] = datosSupervisor->apellidos;
    valores[2] = datosSupervisor->telefono;
    valores[3] = datosSupervisor->correo;

    PGresult *resultado = PQexecParams(conn, consulta, 4, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0; // Asumimos éxito

    if (PQresultStatus(resultado) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Rank Worker BD Error (ejecutarRegistrarSupervisorDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; // Indica error de BD
    }

    PQclear(resultado);
    return estadoFinal;
}

/**
 * @brief Ejecuta la actualización de un campo específico de un supervisor en la BD.
 */
int ejecutarActualizarCampoSupervisorDB(PGconn *conn, const char *idSupervisor, const char *nombreCampo, const char *nuevoValor) {
    if (conn == NULL || idSupervisor == NULL || nombreCampo == NULL || nuevoValor == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarActualizarCampoSupervisorDB): Conexión o datos nulos.\n");
        return -1;
    }

    const char *columnaSQL = NULL;
    char consultaBuffer[256];

    // Mapeo seguro de nombreCampo a columna SQL
    if (strcmp(nombreCampo, "nombre") == 0) columnaSQL = "nombre";
    else if (strcmp(nombreCampo, "apellidos") == 0) columnaSQL = "apellidos";
    else if (strcmp(nombreCampo, "telefono") == 0) columnaSQL = "telefono";
    else if (strcmp(nombreCampo, "correo") == 0) columnaSQL = "correo";
    else {
        fprintf(stderr, "Rank Worker Error (ejecutarActualizarCampoSupervisorDB): Nombre de campo no válido '%s'.\n", nombreCampo);
        return -1; // Error, campo inválido
    }

    // Construir la consulta de forma segura
    snprintf(consultaBuffer, sizeof(consultaBuffer), "UPDATE supervisores SET %s = $1 WHERE id = $2;", columnaSQL);

    const char *valores[2];
    valores[0] = nuevoValor;
    valores[1] = idSupervisor;

    PGresult *resultado = PQexecParams(conn, consultaBuffer, 2, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0;

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            estadoFinal = 0; // Éxito, 1 fila afectada
        } else {
            estadoFinal = -2; // Éxito en ejecución, pero 0 filas afectadas (ID no encontrado?)
        }
    } else {
        fprintf(stderr, "Rank Worker BD Error (ejecutarActualizarCampoSupervisorDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; // Error de BD
    }

    PQclear(resultado);
    return estadoFinal;
}

/**
 * @brief Ejecuta la eliminación de un supervisor de la BD.
 */
int ejecutarEliminarSupervisorDB(PGconn *conn, const char *idSupervisor) {
    if (conn == NULL || idSupervisor == NULL) {
        fprintf(stderr, "Rank Worker (ejecutarEliminarSupervisorDB): Conexión o ID nulo.\n");
        return -1;
    }

    const char *consulta = "DELETE FROM supervisores WHERE id = $1;";
    const char *valores[1];
    valores[0] = idSupervisor;

    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);
    int estadoFinal = 0;

     if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadas = PQcmdTuples(resultado);
        if (filasAfectadas != NULL && strcmp(filasAfectadas, "1") == 0) {
            estadoFinal = 0; // Éxito, 1 fila afectada
        } else {
            estadoFinal = -2; // Éxito en ejecución, 0 filas afectadas (ID no encontrado?)
        }
    } else {
        // Puede ser error de foreign key si tiene proyectos asignados
        fprintf(stderr, "Rank Worker BD Error (ejecutarEliminarSupervisorDB): %s\n", PQerrorMessage(conn));
        estadoFinal = -1; // Error de BD
    }

    PQclear(resultado);
    return estadoFinal;
}
