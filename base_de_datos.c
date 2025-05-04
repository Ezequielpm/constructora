#include "base_de_datos.h"
#include <stdio.h> //para fprintf y printf
#include <stdlib.h>


/**
 * @brief Establece la conexión a la base de datos.
 */
PGconn* conectarDB() {
    //en una aplicación real, esto debería leerse de forma segura (variables de entorno, archivo, etc.)
    const char *infoConexion = "postgresql://postgres:ezequielpm123@127.0.0.1:5432/constructora";

    //intentar conectar a la base de datos
    PGconn *nuevaConexion = PQconnectdb(infoConexion);

    //verificar el estado de la conexión
    if (PQstatus(nuevaConexion) != CONNECTION_OK) {
        //hubo un error al conectar
        fprintf(stderr, "Error al conectar a la base de datos: %s\n", PQerrorMessage(nuevaConexion));

        //liberar el objeto de conexión aunque haya fallado
        PQfinish(nuevaConexion);

        return NULL; //devolver NULL para indicar el fallo
    }

    //si llegamos aquí, la conexión fue exitosa
    printf("Conexión a la base de datos 'constructora' establecida exitosamente.\n");
    return nuevaConexion; // devolver el puntero a la conexión
}

/**
 * @brief Cierra la conexión a la base de datos.
 */
void desconectarDB(PGconn *conexion) {
    //verificar si el puntero de conexión es válido antes de intentar cerrar
    if (conexion != NULL) {
        PQfinish(conexion);
        printf("Conexión a la base de datos cerrada.\n");
    }
}