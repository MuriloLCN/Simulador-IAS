// Arquivo principal

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

int main()
{
    FILE* arq_dados = fopen("teste.txt", "r");
    char linha[500];
    while (fgets(linha, 500, arq_dados) != NULL)
    {
        int tam_palavra = strlen(linha);

        if (linha[tam_palavra - 1] == '\n') {
            linha[tam_palavra - 1] = '\0';
        }

        //__int64 j = _atoi64(linha);
        int64_t j = strtoimax(linha, NULL, 10);
        printf("\n[tam: %d] '%s' -> %" PRIi64 "", tam_palavra, linha, j);
    }

    // Só pra não fechar imediatamente o prompt de comando no windows
    int t;
    scanf("%d", &t);
}
