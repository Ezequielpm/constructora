#ifndef EMPRESA_H
#define EMPRESA_H
#include <libpq-fe.h> //para PGconn
#include "structs.h"
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


//Funciones Ejecutoras de BD para trabajadores MPI
// (Estas funciones reciben datos ya validados y solo interactúan con la BD)

/**
 * @brief EJecuta la inserción de una nueva empresa en la bd.
 * Pensada para ser llamada por un proceso trabajador mpi.
 * @param conn Conexión a la bd del trabajador.
 * @param datosEmpresa Puntero a la estructura empresa con los datos a insertar.
 * @return 0 si éxito, -1 si hubo un error de bd.
 */
int ejecutarRegistrarEmpresaDB(PGconn *conn, const Empresa *datosEmpresa);

/**
 * @brief Ejecuta la actualización de un campo específico de una empresa en la bd.
 * pensada Para ser llamada por un proceso trabajador mpi.
 * @param conn Conexión a la bd del trabajador.
 * @param idEmpresa id de la empresa a actualizar (como string).
 * @param nombreCampo Nombre de la columna a actualizar (ej: "nombre", "telefono").
 * @param nuevoValor El nuevo valor para el campo (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de bd, -2 si no se afectaron filas (id no encontrado).
 */
int ejecutarActualizarCampoEmpresaDB(PGconn *conn, const char *idEmpresa, const char *nombreCampo, const char *nuevoValor);

/**
 * @brief Ejecuta la eliminación de una empresa de la bd.
 * Pensada para ser llamada por un proceso trabajador mpi.
 * @param conn Conexión a la bd del trabajador.
 * @param idEmpresa id de la empresa a eliminar (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de bd (ej: foreign key), -2 si no se afectaron filas (id no encontrado).
 */
int ejecutarEliminarEmpresaDB(PGconn *conn, const char *idEmpresa);

#endif // EMPRESA_H