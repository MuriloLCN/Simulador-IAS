#ifndef IAS_H
#define IAS_H

#include <stdio.h>
#include <inttypes.h>

void carregarMemoria(FILE* arquivoEntrada, uint8_t** memoria, int** ciclos_vetor);

void armazenaNaMemoria (int posicao, uint64_t num, uint8_t *memoria);

uint64_t buscaNaMemoria (uint8_t *memoria, int posicao);

int64_t converteDado(uint64_t entrada);

void dumpDaMemoria(uint8_t *memoria, char nome_arq_saida[]);

void printMemoria(uint8_t* memoria);

void printBits(int64_t n); // Para debug

char *cria_nome_saida(char *nome_entrada);

#endif