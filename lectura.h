#ifndef LECTURA_H
#define LECTURA_H

#include <stdbool.h> //para bool

/**
 * @brief Muestra un mensaje al usuario, lee una línea de texto de la entrada estándar(teclado)
 * de forma segura, elimina el salto de línea final y valida la entrada usando
 * una función de validación proporcionada. Repite el proceso hasta que la entrada sea válida.
 *
 * @param mensajePrompt El mensaje que se mostrará al usuario para pedir la entrada.
 * @param bufferDestino El buffer (array de char) donde se almacenará la entrada válida.
 * @param tamanoBuffer El tamaño total del bufferDestino (para evitar desbordamientos).
 * @param funcionValidadora Un puntero a la función que se usará para validar la entrada.
 * Esta función debe recibir un 'const char*' y devolver 'bool'.
 */
void leerEntrada(const char* mensajePrompt, char* bufferDestino, int tamanoBuffer, bool (*funcionValidadora)(const char*));

#endif // LECTURA_H