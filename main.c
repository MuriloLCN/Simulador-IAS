// Arquivo principal

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define int64 int64_t

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

        __int64 j = _atoi64(linha);
        printf("\n[tam: %d] '%s' -> %" PRIi64 "", tam_palavra, linha, j);
    }
    int t;
    scanf("%d", &t);
}