#include "lectura.h"
#include <stdio.h>  //para printf, fgets, getchar
#include <string.h> //para strlen, strcspn
#include <stdbool.h>
/**
 * @brief Muestra un mensaje, lee y valida la entrada del usuario.
 */
void leerEntrada(const char* mensajePrompt, char* bufferDestino, int tamanoBuffer, bool (*funcionValidadora)(const char*)) {
    bool entradaValida = false;
    char* resultadoFgets;

    do {
        //mostrar el mensaje al usuario
        printf("%s ", mensajePrompt);
        fflush(stdout); //asegurar que el mensaje se muestre antes de esperar la entrada
        //leer la entrada de forma segura
        resultadoFgets = fgets(bufferDestino, tamanoBuffer, stdin);

        if (resultadoFgets != NULL) {
            //eliminar el salto de línea final si existe
            // strcspn busca la primera ocurrencia de un caracter del segundo string ('\n')
            // y devuelve su índice, ponemos el terminador nulo '\0' ahi
            bufferDestino[strcspn(bufferDestino, "\n")] = '\0';

            //limpiar buffer de entrada si la línea fue más larga que el buffer
            //si el ultimo caracter leído NO es '\0' (después de quitar \n) Y TAMPOCO es
            // el '\n' original (que ya quitamos), significa que fgets se detuvo porque
            //llenó el buffer y quedaron caracteres extras en stdin
            int longitudLeida = strlen(bufferDestino);
            if (longitudLeida == tamanoBuffer - 1 && bufferDestino[longitudLeida-1] != '\0') {
                //se consumen los caracteres restantes hasta encontrar '\n' o EOF
                int caracterExtra;
                while ((caracterExtra = getchar()) != '\n' && caracterExtra != EOF);
            }

            //validar la entrada usando la función proporcionada
            entradaValida = funcionValidadora(bufferDestino);

            if (!entradaValida) {
                printf("\nEntrada inválida. Por favor, intente de nuevo.\n");
                fflush(stdout); 

            }

        } else {
            //error al leer la entrada
            printf("Error al leer la entrada.\n");
            entradaValida = false;
        }

    } while (!entradaValida); //se repite mientras la entrada no sea válida
}