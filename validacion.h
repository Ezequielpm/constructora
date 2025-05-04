#ifndef VALIDACION_H
#define VALIDACION_H

#include <stdbool.h> //para usar bool, true, false

/**
 * @brief Verifica si una cadena está vacía o contiene solo espacios en blanco.
 * @param cadena La cadena a verificar.
 * @return true si la cadena está vacía o solo contiene espacios, false en caso contrario.
 */
bool esCampoVacio(const char* cadena);

/**
 * @brief Verifica si una cadena NO está vacía o NO contiene solo espacios en blanco.
 * Es la negación lógica de esCampoVacio.
 * @param cadena La cadena a verificar.
 * @return true si la cadena contiene al menos un caracter visible, false en caso contrario.
 */
bool noEsVacio(const char* cadena);

/**
 * @brief Verifica si una cadena contiene únicamente dígitos numéricos (0-9).
 * @param cadena La cadena a verificar.
 * @return true si la cadena contiene solo números y no está vacía, false en caso contrario.
 */
bool sonSoloNumeros(const char* cadena);

/**
 * @brief Verifica si una cadena contiene únicamente letras del alfabeto y/o espacios.
 * (Nota: Validación simple, no incluye acentos u otros caracteres especiales).
 * @param cadena La cadena a verificar.
 * @return true si la cadena contiene solo letras/espacios y no está vacía, false en caso contrario.
 */
bool sonSoloLetras(const char* cadena);

/**
 * @brief Verifica si una cadena parece tener un formato de fecha válido (YYYY-MM-DD).
 * (Nota: Validación simple de formato y rangos básicos, no valida días por mes o años bisiestos).
 * @param cadenaFecha La cadena con la fecha a verificar.
 * @return true si el formato y rangos básicos son correctos, false en caso contrario.
 */
bool esFechaValida(const char* cadenaFecha);

/**
 * @brief Verifica si una cadena parece tener un formato de correo electrónico básico.
 * (Nota: Validación muy simple, busca un '@' y un '.' después).
 * @param correo La cadena con el correo a verificar.
 * @return true si la estructura básica parece correcta, false en caso contrario.
 */
bool esCorreoValido(const char* correo);

/**
 * @brief Verifica si una cadena parece tener un formato de RFC válido para México (12 o 13 caracteres).
 * (Nota: Validación simple de longitud y tipo de caracteres, no valida dígito verificador).
 * @param rfc La cadena con el RFC a verificar.
 * @return true si la longitud y estructura básica parecen correctas, false en caso contrario.
 */
bool esRFCValido(const char* rfc);

/**
 * @brief Verifica si una cadena representa un número entero positivo válido.
 * Útil para entradas como IDs o opciones de menú.
 * @param cadenaNumero La cadena a verificar.
 * @return true si la cadena contiene solo dígitos y no está vacía, false en caso contrario.
 */
bool esNumeroEnteroPositivoValido(const char* cadenaNumero);

/**
 * @brief Verifica si una cadena representa un número decimal válido (positivo o negativo).
 * Permite un punto decimal opcional.
 * @param cadenaNumero La cadena a verificar.
 * @return true si la cadena es un número decimal válido y no está vacía, false en caso contrario.
 */
bool esDecimalValido(const char* cadenaNumero);

#endif // VALIDACION_H