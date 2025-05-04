#ifndef SUPERVISOR_H
#define SUPERVISOR_H

#include <libpq-fe.h> //para PGconn

/**
 * @brief Solicita al usuario los datos de un nuevo supervisor y los inserta en la base de datos.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void registrarSupervisor(PGconn *conn);

/**
 * @brief Consulta y muestra en consola la lista de todos los supervisores registrados.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void mostrarSupervisores(PGconn *conn);

/**
 * @brief Permite al usuario seleccionar un supervisor por ID y actualizar uno de sus campos.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void actualizarSupervisor(PGconn *conn);

/**
 * @brief Permite al usuario seleccionar un supervisor por ID y eliminarlo de la base de datos,
 * previa confirmación.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void eliminarSupervisor(PGconn *conn);

#endif // SUPERVISOR_H