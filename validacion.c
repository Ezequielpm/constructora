#include "validacion.h"
#include <string.h> //para strlen, strchr
#include <ctype.h>  //para isdigit, isalpha, isspace
#include <stdio.h>  //oara sscanf
#include <stdlib.h> //para atoi

/**
 * @brief Verifica si una cadena está vacía o contiene solo espacios en blanco.
 */
bool esCampoVacio(const char* cadena) {
    if (cadena == NULL) {
        return true; 
    }
    int longitud = strlen(cadena);
    if (longitud == 0) {
        return true; //cadena de longitud 0 es vacia
    }
    //verificar si todos los caracteres son espacios
    for (int indice = 0; indice < longitud; indice++) {
        if (!isspace((unsigned char)cadena[indice])) {
            return false; //se encontró un caracter que no es espacio
        }
    }
    return true; //si llegó aqui, solo había espacios
}

/**
 * @brief Verifica si una cadena NO está vacía o NO contiene solo espacios en blanco.
 */
bool noEsVacio(const char* cadena) {
    //simplemente devuelve lo contrario de esCampoVacio
    return !esCampoVacio(cadena);
}

/**
 * @brief Verifica si una cadena contiene únicamente digitos numericos (0-9).
 */
bool sonSoloNumeros(const char* cadena) {
    if (esCampoVacio(cadena)) {
        return false; //campo vacío no es válido
    }
    int longitud = strlen(cadena);
    for (int indice = 0; indice < longitud; indice++) {
        //si cualquier caracter no es un dígito
        if (!isdigit((unsigned char)cadena[indice])) {
            return false;
        }
    }
    return true; //todos los caracteres son dígitos
}

/**
 * @brief Verifica si una cadena contiene únicamente letras del alfabeto y/o espacios.
 */
bool sonSoloLetras(const char* cadena) {
    if (esCampoVacio(cadena)) {
        return false; //campo vacio no es valido
    }
    int longitud = strlen(cadena);
    for (int indice = 0; indice < longitud; indice++) {
        unsigned char caracterActual = (unsigned char)cadena[indice];
        //si cualquier caracter no es una letra ni un espacio
        if (!isalpha(caracterActual) && !isspace(caracterActual)) {
            return false;
        }
    }
    return true; //todos los caracteres son letras o espacios
}

/**
 * @brief Verifica si una cadena parece tener un formato de fecha válido (YYYY-MM-DD).
 */
bool esFechaValida(const char* cadenaFecha) {
    if (esCampoVacio(cadenaFecha) || strlen(cadenaFecha) != 10) {
        return false; //longitud incorrecta
    }

    int anio, mes, dia;

    //intenta leer el formato en YYYY-MM-DD
    //%d consume un entero, %*c consume y descarta el '-'
    int elementosLeidos = sscanf(cadenaFecha, "%d%*c%d%*c%d", &anio, &mes, &dia);

    if (elementosLeidos != 3) {
         return false; //no se pudo leer el formato esperado
    }

    //verificacion de los separadores '-' en las posiciones correctas
    if (cadenaFecha[4] != '-' || cadenaFecha[7] != '-') {
         return false;
    }

    //verificación básica de rangos (no valida dias por mes)
    if (anio < 2025 || anio > 2100) return false;
    if (mes < 1 || mes > 12) return false;
    if (dia < 1 || dia > 31) return false; //chequeo simple

    return true; //formato y rangos basicos correctos
}

/**
 * @brief Verifica si una cadena parece tener un formato de correo electrónico básico.
 */
bool esCorreoValido(const char* correo) {
    if (esCampoVacio(correo)) {
        return false;
    }

    int longitud = strlen(correo);
    char* posicionArroba = strchr(correo, '@'); //encuentra la primera '@'

    //no hay '@' o está al principio o al final
    if (posicionArroba == NULL || posicionArroba == correo || posicionArroba == correo + longitud - 1) {
        return false;
    }

    //se buusca un '.' después de la '@'
    char* posicionPunto = strchr(posicionArroba + 1, '.');

    //no hay '.' después de '@', o está justo después de '@', o está al final
    if (posicionPunto == NULL || posicionPunto == posicionArroba + 1 || posicionPunto == correo + longitud - 1) {
        return false;
    }

    //validación muy básica, podría haber más de un '@' o puntos inválidos,
    // pero para simplicidad, esto es suficiente
    return true;
}

/**
 * @brief Verifica si una cadena parece tener un formato de RFC válido para México (12 o 13 caracteres).
 */
bool esRFCValido(const char* rfc) {
    if (esCampoVacio(rfc)) {
        return false;
    }

    int longitud = strlen(rfc);

    // Validar longitud
    if (longitud != 12 && longitud != 13) {
        return false;
    }

    //segun el servicio de administración tributaria (SAT), la estructura de un RFC es como se muestra a continuación
    //pagina web con la informacion(leer primer y segundo parrafo): 
    // https://www.gob.mx/cms/uploads/attachment/file/850663/Dato-6_RFC.pdf
    //se valida la estructura basica
    //persona física: LLLL######LLL (13) -> L=Letra, #=Número
    //persona moral: LLL######LL (12)  -> L=Letra, #=Número
    int letrasIniciales = (longitud == 13) ? 4 : 3;
    int letrasFinales = (longitud == 13) ? 3 : 2;
    int numeros = 6;

    //verificar letras iniciales
    for (int i = 0; i < letrasIniciales; i++) {
        if (!isalpha((unsigned char)rfc[i])) return false;
    }
    //verificar numeros (fecha)
    for (int i = letrasIniciales; i < letrasIniciales + numeros; i++) {
        if (!isdigit((unsigned char)rfc[i])) return false;
    }
    //verificar letras/numeros finales (homoclave - simplificado a cualquier alfanumérico)
    for (int i = letrasIniciales + numeros; i < longitud; i++) {
        if (!isalnum((unsigned char)rfc[i])) return false; //alnum = alpha OR numeric
    }

    return true; //pasó las validaciones básicas
}


/**
 * @brief Verifica si una cadena representa un número entero positivo válido.
 */
bool esNumeroEnteroPositivoValido(const char* cadenaNumero) {
    //reutilizamos sonSoloNumeros porque valida que no este vacío
    // y que todos los caracteres sean dígitos.
    return sonSoloNumeros(cadenaNumero);
}


/**
 * @brief Verifica si una cadena representa un número decimal válido.
 */
bool esDecimalValido(const char* cadenaNumero) {
    if (esCampoVacio(cadenaNumero)) {
        return false;
    }

    int longitud = strlen(cadenaNumero);
    bool puntoEncontrado = false;
    int indiceInicio = 0;

    //se permiti un signo opcional al inicio
    if (cadenaNumero[0] == '+' || cadenaNumero[0] == '-') {
        indiceInicio = 1;
        //un signo solo no es válido
        if (longitud == 1) return false;
    }

    for (int i = indiceInicio; i < longitud; i++) {
        if (isdigit((unsigned char)cadenaNumero[i])) {
            continue; //es un dígito, seguir
        } else if (cadenaNumero[i] == '.') {
            if (puntoEncontrado) {
                return false; //ya había un punto decimal
            }
            puntoEncontrado = true;
        } else {
            return false; //caracter no válido (ni digito, ni punto)
        }
    }
    //se asegura que no sea solo un punto o un signo y un punto
    if (longitud == indiceInicio + 1 && puntoEncontrado) {
         return false; //por ejemplo: "." o "-."
    }

    return true; //pasó todas las validaciones
}