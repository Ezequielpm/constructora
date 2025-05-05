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


#endif // PROYECTO_H