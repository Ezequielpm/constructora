#ifndef SOLICITUD_H
#define SOLICITUD_H

#include <libpq-fe.h> //para PGconn
#include "structs.h"  // Para SolicitudProyecto, ProyectoAceptado
/**
 * @brief Registra una nueva solicitud de proyecto en la base de datos.
 * Pide al usuario los datos necesarios, incluyendo la selección de una empresa existente.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void registrarSolicitud(PGconn *conn);

/**
 * @brief Muestra las solicitudes de proyecto filtradas por el estado especificado
 * por el usuario (APERTURADO, ACEPTADO, CANCELADO).
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void mostrarSolicitudesPorEstado(PGconn *conn);

/**
 * @brief Permite al usuario cancelar una solicitud que esté en estado 'APERTURADO'
 * Solicita el ID de la solicitud y una razón para la cancelación
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void cancelarSolicitud(PGconn *conn);

/**
 * @brief Acepta una solicitud de proyecto en estado 'APERTURADO', la actualiza
 * y crea un nuevo registro correspondiente en la tabla 'proyecto_aceptado'.
 * Solicita al usuario los detalles adicionales necesarios para el proyecto aceptado.
 * Utiliza una transacción de base de datos.
 * @param conn Puntero a la conexión activa de PostgreSQL.
 */
void aceptarSolicitud(PGconn *conn);


// --- Funciones Ejecutoras de BD para trabajadores MPI ---

/**
 * @brief Ejecuta la inserción de una nueva solicitud en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idEmpresaStr ID de la empresa (como string).
 * @param fechaSolicitud Fecha de la solicitud (YYYY-MM-DD).
 * @param presupuestoStr Presupuesto (como string).
 * @param anticipoStr Anticipo (como string).
 * @param folio Folio de la solicitud.
 * @return 0 si éxito, -1 si hubo un error de BD (ej. FK no existe).
 */
int ejecutarRegistrarSolicitudDB(PGconn *conn, const char *idEmpresaStr, const char *fechaSolicitud,
                                 const char *presupuestoStr, const char *anticipoStr, const char *folio);

/**
 * @brief Ejecuta la consulta y muestra las solicitudes filtradas por estado.
 * Pensada para ser llamada por un proceso trabajador MPI. ¡Esta función imprime directamente!
 * @param conn Conexión a la BD del trabajador.
 * @param estadoFiltro El estado por el cual filtrar ('APERTURADO', 'ACEPTADO', 'CANCELADO').
 * @return 0 si la consulta se ejecutó (independiente de si encontró filas), -1 si hubo error de BD.
 */
int ejecutarMostrarSolicitudesPorEstadoDB(PGconn *conn, const char *estadoFiltro);

/**
 * @brief Ejecuta la cancelación de una solicitud en la BD.
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idSolicitud ID de la solicitud a cancelar (como string).
 * @param razonCancelacion Motivo de la cancelación.
 * @return 0 si éxito (1 fila afectada), -1 si error de BD, -2 si no se afectaron filas (ID no encontrado o no 'APERTURADO').
 */
int ejecutarCancelarSolicitudDB(PGconn *conn, const char *idSolicitud, const char *razonCancelacion);

/**
 * @brief Ejecuta la aceptación de una solicitud y creación del proyecto en la BD (transacción).
 * Pensada para ser llamada por un proceso trabajador MPI.
 * @param conn Conexión a la BD del trabajador.
 * @param idSolicitud ID de la solicitud a aceptar (como string).
 * @param datosProyecto Estructura con los datos del nuevo proyecto.
 * @param montoStr Monto del proyecto (como string).
 * @param idSupervisorStr ID del supervisor asignado (como string).
 * @param prioridadStr Prioridad del proyecto ("ROJO", "NARANJA", "AMARILLO").
 * @return 0 si la transacción fue exitosa (COMMIT), -1 si hubo error de BD en algún paso (ROLLBACK), -2 si la solicitud no se pudo actualizar (ID no existe o no 'APERTURADO', ROLLBACK).
 */
int ejecutarAceptarSolicitudDB(PGconn *conn, const char *idSolicitud,
                               const ProyectoAceptado *datosProyecto, const char *montoStr,
                               const char *idSupervisorStr, const char *prioridadStr);


#endif // SOLICITUD_H