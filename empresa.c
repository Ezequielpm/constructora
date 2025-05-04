#include "empresa.h"
#include "structs.h"      //para el struct de la empresa
#include "lectura.h"      //para leerEntrada
#include "validacion.h"   //para las funciones de validación

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      //para tolower
//libpq-fe.h ya está incluido en empresa.h

/**
 * @brief Registra una nueva empresa en la base de datos.
 */
void registrarEmpresa(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (registrarEmpresa): Conexión a BD no válida.\n");
        return;
    }

    Empresa nuevaEmpresa; //no necesitamos ID para registrar porque se genera solo

    printf("\n--- Registro de Nueva Empresa ---\n");

    //se solciitan los datos usando leerEntrada y validaciones
    //para campos de texto libre como nombre/dirección, podemos usar un validador simple
    leerEntrada("Nombre de la empresa:", nuevaEmpresa.nombre, sizeof(nuevaEmpresa.nombre), noEsVacio); 
    leerEntrada("Dirección:", nuevaEmpresa.direccion, sizeof(nuevaEmpresa.direccion), noEsVacio);     
    leerEntrada("Teléfono (solo números):", nuevaEmpresa.telefono, sizeof(nuevaEmpresa.telefono), sonSoloNumeros);
    leerEntrada("Correo electrónico:", nuevaEmpresa.correo, sizeof(nuevaEmpresa.correo), esCorreoValido);
    leerEntrada("RFC (formato mexicano):", nuevaEmpresa.rfc, sizeof(nuevaEmpresa.rfc), esRFCValido);
    leerEntrada("Nombre del Contacto Encargado:", nuevaEmpresa.contacto_encargado, sizeof(nuevaEmpresa.contacto_encargado), sonSoloLetras);

    //preparar consulta parametrizada
    const char *consulta = "INSERT INTO empresas (nombre, direccion, telefono, correo, rfc, contacto_encargado) VALUES ($1, $2, $3, $4, $5, $6);";
    const char *valores[6];
    valores[0] = nuevaEmpresa.nombre;
    valores[1] = nuevaEmpresa.direccion;
    valores[2] = nuevaEmpresa.telefono;
    valores[3] = nuevaEmpresa.correo;
    valores[4] = nuevaEmpresa.rfc;
    valores[5] = nuevaEmpresa.contacto_encargado;

    //ejecutar consulta
    PGresult *resultado = PQexecParams(conn, consulta, 6, NULL, valores, NULL, NULL, 0);

    //verificar resultado
    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        printf("Empresa registrada exitosamente.\n");
    } else {
        fprintf(stderr, "Error al registrar empresa: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}

/**
 * @brief Muestra la lista de todas las empresas registradas.
 */
void mostrarEmpresas(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (mostrarEmpresas): Conexión a BD no válida.\n");
        return;
    }

    const char *consulta = "SELECT id, nombre, direccion, telefono, correo, rfc, contacto_encargado FROM empresas ORDER BY id;";
    PGresult *resultado = PQexecParams(conn, consulta, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(resultado) != PGRES_TUPLES_OK) {
        fprintf(stderr, "Error al consultar empresas: %s\n", PQerrorMessage(conn));
        PQclear(resultado);
        return;
    }

    int numFilas = PQntuples(resultado);
    printf("\n--- Lista de Empresas (%d) ---\n", numFilas);
    printf("| %-4s | %-25s | %-30s | %-15s | %-25s | %-15s | %-25s |\n",
           "ID", "Nombre", "Dirección", "Teléfono", "Correo", "RFC", "Contacto");
    printf("----------------------------------------------------------------------------------------------------------------------------------------------------------\n");


    for (int fila = 0; fila < numFilas; fila++) {
        printf("| %-4s | %-25s | %-30s | %-15s | %-25s | %-15s | %-25s |\n",
               PQgetvalue(resultado, fila, 0), // id
               PQgetvalue(resultado, fila, 1), // nombre
               PQgetvalue(resultado, fila, 2), // direccion
               PQgetvalue(resultado, fila, 3), // telefono
               PQgetvalue(resultado, fila, 4), // correo
               PQgetvalue(resultado, fila, 5), // rfc
               PQgetvalue(resultado, fila, 6)  // contacto_encargado
              );
    }
     printf("----------------------------------------------------------------------------------------------------------------------------------------------------------\n");


    PQclear(resultado);
}

/**
 * @brief Actualiza un campo específico de una empresa existente.
 */
void actualizarEmpresa(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (actualizarEmpresa): Conexión a BD no válida.\n");
        return;
    }

    mostrarEmpresas(conn);

    char idEmpresaStr[10];
    char opcionStr[5];
    char nuevoValor[151]; 
    int opcion;
    const char *consulta = NULL;
    const char *valores[2]; // [0]=nuevoValor, [1]=idEmpresaStr

    printf("\n--- Actualizar Empresa ---\n");
    leerEntrada("Ingrese el ID de la empresa a actualizar:", idEmpresaStr, sizeof(idEmpresaStr), esNumeroEnteroPositivoValido);
    valores[1] = idEmpresaStr;

    printf("\nSeleccione el campo a actualizar:\n");
    printf("[1] Nombre\n");
    printf("[2] Dirección\n");
    printf("[3] Teléfono\n");
    printf("[4] Correo\n");
    printf("[5] RFC\n");
    printf("[6] Contacto Encargado\n");
    printf("[7] Cancelar\n");
    leerEntrada("Opción:", opcionStr, sizeof(opcionStr), esNumeroEnteroPositivoValido);
    opcion = atoi(opcionStr);

    //apuntar a la función de validación correcta según la opción
    bool (*funcionValidadora)(const char*) = NULL;

    switch (opcion) {
        case 1:
            consulta = "UPDATE empresas SET nombre = $1 WHERE id = $2;";
            funcionValidadora = noEsVacio; // Mejorar a noEsVacio
            leerEntrada("Nuevo Nombre:", nuevoValor, sizeof(nuevoValor), funcionValidadora);
            break;
        case 2:
            consulta = "UPDATE empresas SET direccion = $1 WHERE id = $2;";
             funcionValidadora = noEsVacio; // Mejorar a noEsVacio
            leerEntrada("Nueva Dirección:", nuevoValor, sizeof(nuevoValor), funcionValidadora);
            break;
        case 3:
            consulta = "UPDATE empresas SET telefono = $1 WHERE id = $2;";
             funcionValidadora = sonSoloNumeros;
            leerEntrada("Nuevo Teléfono:", nuevoValor, sizeof(nuevoValor), funcionValidadora);
            break;
        case 4:
            consulta = "UPDATE empresas SET correo = $1 WHERE id = $2;";
             funcionValidadora = esCorreoValido;
            leerEntrada("Nuevo Correo:", nuevoValor, sizeof(nuevoValor), funcionValidadora);
            break;
        case 5:
            consulta = "UPDATE empresas SET rfc = $1 WHERE id = $2;";
             funcionValidadora = esRFCValido;
            leerEntrada("Nuevo RFC:", nuevoValor, sizeof(nuevoValor), funcionValidadora);
            break;
        case 6:
            consulta = "UPDATE empresas SET contacto_encargado = $1 WHERE id = $2;";
             funcionValidadora = sonSoloLetras;
            leerEntrada("Nuevo Contacto Encargado:", nuevoValor, sizeof(nuevoValor), funcionValidadora);
            break;
        case 7:
            printf("Actualización cancelada.\n");
            return;
        default:
            printf("Opción no válida.\n");
            return;
    }

    //asignar el nuevo valor para la consulta
    valores[0] = nuevoValor;

    //ejecutar
    PGresult *resultado = PQexecParams(conn, consulta, 2, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
        char *filasAfectadasStr = PQcmdTuples(resultado);
        if (filasAfectadasStr != NULL && strcmp(filasAfectadasStr, "1") == 0) {
             printf("Empresa actualizada correctamente.\n");
        } else {
            printf("Empresa actualizada (o ID no encontrado, 0 filas afectadas).\n");
        }
    } else {
        fprintf(stderr, "Error al actualizar empresa: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}

/**
 * @brief Elimina una empresa de la base de datos, previa confirmación.
 */
void eliminarEmpresa(PGconn *conn) {
    if (conn == NULL) {
        fprintf(stderr, "Error (eliminarEmpresa): Conexión a BD no válida.\n");
        return;
    }

    mostrarEmpresas(conn);

    char idEmpresaStr[10];
    char confirmacion[5];

    printf("\n--- Eliminar Empresa ---\n");
    leerEntrada("Ingrese el ID de la empresa a eliminar:", idEmpresaStr, sizeof(idEmpresaStr), esNumeroEnteroPositivoValido);

    printf("¿Está seguro que desea eliminar la empresa con ID %s? (s/n): ", idEmpresaStr);
    printf("\nADVERTENCIA: Esto fallará si la empresa tiene solicitudes de proyecto asociadas.\nConfirmar (s/n):");
    fgets(confirmacion, sizeof(confirmacion), stdin);
    confirmacion[strcspn(confirmacion, "\n")] = '\0';
    //limpiar buffer
    int c; while ((c = getchar()) != '\n' && c != EOF);

    if (tolower(confirmacion[0]) != 's') {
        printf("Eliminación cancelada.\n");
        return;
    }

    const char *consulta = "DELETE FROM empresas WHERE id = $1;";
    const char *valores[1];
    valores[0] = idEmpresaStr;

    PGresult *resultado = PQexecParams(conn, consulta, 1, NULL, valores, NULL, NULL, 0);

    if (PQresultStatus(resultado) == PGRES_COMMAND_OK) {
         char *filasAfectadasStr = PQcmdTuples(resultado);
        if (filasAfectadasStr != NULL && strcmp(filasAfectadasStr, "1") == 0) {
             printf("Empresa eliminada correctamente.\n");
        } else {
             //esto también puede ocurrir si el ID no existe
            printf("No se eliminó la empresa (ID no encontrado o restricción de clave foránea).\n");
        }
    } else {
        //aquí es probable que caiga un error de Foreign Key si hay solicitudes
        fprintf(stderr, "Error al eliminar empresa: %s\n", PQerrorMessage(conn));
    }

    PQclear(resultado);
}