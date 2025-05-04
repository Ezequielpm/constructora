#ifndef SOLICITUD_H
#define SOLICITUD_H

#include <libpq-fe.h> //para PGconn

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

// void aceptarSolicitud(PGconn *conn);


#endif // SOLICITUD_H