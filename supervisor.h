#ifndef SUPERVISOR_H
#define SUPERVISOR_H
#include <libpq-fe.h> //para PGconn
#include "structs.h"

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


// --- Funciones Ejecutoras de BD para trabajadores MPI ---
// (Estas funciones reciben datos ya validados y solo interactúan con la BD)

/**
 * @brief Ejecuta la inserción de un nuevo supervisor en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param datosSupervisor Puntero a la estructura Supervisor con los datos a insertar.
 * @return 0 si éxito, -1 si hubo un error de BD.
 */
int ejecutarRegistrarSupervisorDB(PGconn *conn, const Supervisor *datosSupervisor);

/**
 * @brief Ejecuta la actualización de un campo específico de un supervisor en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idSupervisor ID del supervisor a actualizar (como string).
 * @param nombreCampo Nombre de la columna a actualizar (ej: "nombre", "telefono").
 * @param nuevoValor El nuevo valor para el campo (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de BD, -2 si no se afectaron filas (ID no encontrado).
 */
int ejecutarActualizarCampoSupervisorDB(PGconn *conn, const char *idSupervisor, const char *nombreCampo, const char *nuevoValor);

/**
 * @brief Ejecuta la eliminación de un supervisor de la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idSupervisor ID del supervisor a eliminar (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de BD (ej: foreign key), -2 si no se afectaron filas (ID no encontrado).
 */
int ejecutarEliminarSupervisorDB(PGconn *conn, const char *idSupervisor);

#endif // SUPERVISOR_H