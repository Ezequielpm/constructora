#ifndef REPORTES_H
#define REPORTES_H

#include <libpq-fe.h> //para pgconn

/**
 * @brief Genera y muestra el reporte de proyectos aceptados dentro de un rango de fechas de inicio.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void reporteProyectosPorPeriodo(PGconn *conn);

/**
 * @brief Genera y muestra el reporte de proyectos con un avance superior al 50%.
 * @param conn puntero a la conexión activa de postgresql.
 */
void reporteProyectosAvanceMayor50(PGconn *conn);

/**
 * @brief Genera y muestra el reporte de empresas a las que se les ha cancelado al menos una solicitud.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void reporteEmpresasCanceladas(PGconn *conn);

/**
 * @brief Genera y muestra el reporte de supervisores encargados de proyectos activos
 * ('en proceso') con menos del 20% de avance.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void reporteSupervisorAvanceMenor20(PGconn *conn);

/**
 * @brief Genera y muestra un listado general de todos los proyectos aceptados.
 * similar a mostrarproyectosaceptados pero en formato reporte.
 * @param conn Puntero a la conexión activa de postgresql.
 */
void reporteProyectosAceptadosGeneral(PGconn *conn);


#endif // REPORTES_H