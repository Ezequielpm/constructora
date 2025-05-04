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
    leerEntrada("Ingrese el ID del supervisor a eliminar:", idSupervisorStr, sizeof(idSupervisorStr), esNumeroEnteroPositivoValido);

    //pedir confirmación
    printf("¿Está seguro que desea eliminar al supervisor con ID %s? (s/n): ", idSupervisorStr);
    //podríamos crear un validador específico para s/n si quisiéramos ser estrictos
    //por simplicidad, leemos sin validador estricto y comprobamos manualmente
    fgets(confirmacion, sizeof(confirmacion), stdin);
    confirmacion[strcspn(confirmacion, "\n")] = '\0'; //quitar salto de línea
    //limpiar el buffer por si acaso
     int c; while ((c = getchar()) != '\n' && c != EOF);


    //convertir a minuscula para comparar sin importar mayusculas o minusculas
    if (tolower(confirmacion[0]) != 's') {
        printf("Eliminación cancelada.\n");
        return;
    }

    //preparar consulta y parámetros
    const char *consulta = "DELETE FROM supervisores WHERE id = $1;";
    const char *valores[1];
    valores[0] = idSupervisorStr;

    //ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadasStr = PQcmdTuples(resultado);
        if (filasAfectadasStr != NULL && strcmp(filasAfectadasStr, "1") == 0) {
             printf("Supervisor eliminado correctamente.\n");
        } else {
            printf("Operación completada, pero no se encontró un supervisor con el ID %s (0 filas actualizadas).\n", idSupervisorStr);
        }
    } else {
        fprintf(stderr, "Error al eliminar supervisor: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}