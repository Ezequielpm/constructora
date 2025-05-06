#ifndef PROYECTO_H
#define PROYECTO_H

#include <libpq-fe.h> // para pgconn

/**
 * @brief Muestra en consola la lista de proyectos aceptados.
 * Permite al usuario filtrar por estado (en proceso, terminado, todos).
 * Muestra información relevante uniendo tablas (empresa, supervisor).
 * @param conn Puntero a la conexión activa de postgresql.
 */
void mostrarProyectosAceptados(PGconn *conn);

/**
 * @brief Permite al usuario actualizar el porcentaje de avance de un proyecto específico.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void actualizarAvanceProyecto(PGconn *conn);

/**
 * @brief Permite al usuario actualizar la prioridad (semaforización) de un proyecto.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void actualizarPrioridadProyecto(PGconn *conn);

/**
 * @brief Marca un proyecto como 'terminado' y establece su avance al 100%.
 * Solo afecta a proyectos que estén actualmente 'en proceso'.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void terminarProyecto(PGconn *conn);

/**
 * @brief Permite cambiar el supervisor asignado a un proyecto existente.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void reasignarSupervisorProyecto(PGconn *conn);



// --- Funciones Ejecutoras de BD para trabajadores MPI ---

/**
 * @brief Ejecuta la consulta y muestra los proyectos aceptados filtrados por estado.
 * Pensada para ser llamada por un proceso trabajador MPI. ¡Esta función imprime directamente!
 * @param conn Conexión a la BD del trabajador.
 * @param filtroEstatus El estado por el cual filtrar ('EN_PROCESO', 'TERMINADO', o NULL para todos).
 * @return 0 si la consulta se ejecutó, -1 si hubo error de BD.
 */
int ejecutarMostrarProyectosAceptadosDB(PGconn *conn, const char *filtroEstatus);

/**
 * @brief Ejecuta la actualización del porcentaje de avance de un proyecto en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idProyecto ID del proyecto a actualizar (como string).
 * @param nuevoPorcentajeStr Nuevo porcentaje (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de BD, -2 si no se afectaron filas (ID no encontrado).
 */
int ejecutarActualizarAvanceDB(PGconn *conn, const char *idProyecto, const char *nuevoPorcentajeStr);

/**
 * @brief Ejecuta la actualización de la prioridad de un proyecto en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idProyecto ID del proyecto a actualizar (como string).
 * @param nuevaPrioridadStr Nueva prioridad ("ROJO", "NARANJA", "AMARILLO").
 * @return 0 si éxito (1 fila afectada), -1 si error de BD, -2 si no se afectaron filas (ID no encontrado).
 */
int ejecutarActualizarPrioridadDB(PGconn *conn, const char *idProyecto, const char *nuevaPrioridadStr);

/**
 * @brief Ejecuta la finalización de un proyecto en la BD (cambia estado y avance).
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idProyecto ID del proyecto a terminar (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de BD, -2 si no se afectaron filas (ID no encontrado o no 'EN_PROCESO').
 */
int ejecutarTerminarProyectoDB(PGconn *conn, const char *idProyecto);

/**
 * @brief Ejecuta la reasignación de supervisor para un proyecto en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idProyecto ID del proyecto a modificar (como string).
 * @param nuevoIdSupervisorStr ID del nuevo supervisor (como string).
 * @return 0 si éxito (1 fila afectada), -1 si error de BD (ej. FK supervisor no existe), -2 si no se afectaron filas (ID proyecto no encontrado).
 */
int ejecutarReasignarSupervisorDB(PGconn *conn, const char *idProyecto, const char *nuevoIdSupervisorStr);



#endif // PROYECTO_H