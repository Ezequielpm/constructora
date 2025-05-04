#ifndef EMPRESA_H
#define EMPRESA_H

#include <libpq-fe.h> //para PGconn

/**
 * @brief Solicita al usuario los datos de una nueva empresa y la inserta en la base de datos.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void registrarEmpresa(PGconn *conn);

/**
 * @brief Consulta y muestra en consola la lista de todas las empresas registradas.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void mostrarEmpresas(PGconn *conn);

/**
 * @brief Permite al usuario seleccionar una empresa por ID y actualizar uno de sus campos.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void actualizarEmpresa(PGconn *conn);

/**
 * @brief Permite al usuario seleccionar una empresa por ID y eliminarla de la base de datos,
 * previa confirmación. (Nota: fallará si la empresa tiene solicitudes asociadas).
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void eliminarEmpresa(PGconn *conn);

#endif // EMPRESA_H