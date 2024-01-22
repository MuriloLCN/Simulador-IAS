#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "ias.h"

void simulacao(uint8_t** memoria)
{
    // Simulação vai aqui
}

int main (int argc, char *argv[])
{
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-m"))
    {
        printf("\nParametros incorretos");
        // TODO: Colocar mensagem mais detalhada aqui
    }

    FILE* arquivoEntrada;
    uint8_t* memoria = (uint8_t *) malloc (4096*40);

    arquivoEntrada = fopen(argv[2], "r");

    if (arquivoEntrada == NULL)
    {
        printf("Falha ao abrir o arquivo de entrada!\n");
        return 1;
    }
    
    carregarMemoria(arquivoEntrada, &memoria);

    simulacao(&memoria);   

    dumpDaMemoria(memoria, argv[4]);
        
    free(memoria);
    fclose(arquivoEntrada);
    return 0;
    
}
