#ifndef BASE_DE_DATOS_H
#define BASE_DE_DATOS_H
#include <libpq-fe.h>

/**
 * @brief Establece una conexión con la base de datos PostgreSQL predefinida.
 * La información de conexión está hardcodeada por ahora dentro de la función.
 * @return Un puntero a la estructura PGconn que representa la conexión exitosa,
 * o NULL si la conexión falla. Imprime un mensaje de error en caso de fallo.
 */
PGconn* conectarDB();

/**
 * @brief Cierra una conexión existente con la base de datos PostgreSQL.
 * @param conexion Puntero a la conexión PGconn que se desea cerrar.
 * La función verifica internamente si el puntero es NULL.
 */
void desconectarDB(PGconn *conexion);

#endif // BASE_DE_DATOS_H