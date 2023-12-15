#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#define TAMANHO_MEMORIA 4096

typedef enum {False, True} booleano;

int memoria[TAMANHO_MEMORIA] = {0};

int pegaParametroInstrucao(char *instrucao, int index)
{
    /*
        Obtém o endereço de memória inscrito em uma instrução
        Entradas:
        char* instrucao: A string contendo a instrução
        int index: O índice onde o valor tem início

        Retorno:
        O valor lido da instrução

        Exemplo:
        instrução = "LOAD M(57)"
        index = 7 (57 começa na sétima posição)
        retorno: 57
    */

    // String que ira conter o valor lido, ex: "57"
    char num[5];

    int i = 0, int_num;

    while (instrucao[index] == '0' ||
           instrucao[index] == '1' ||
           instrucao[index] == '2' ||
           instrucao[index] == '3' ||
           instrucao[index] == '4' ||
           instrucao[index] == '5' ||
           instrucao[index] == '6' ||
           instrucao[index] == '7' ||
           instrucao[index] == '8' ||
           instrucao[index] == '9')
    {
        if (i > 3) return -1; // Erro, endereço de memória de tamanho maior que 4 digitos
        else
        {
            num[i] = instrucao[index];
            i++;
            index++;
        }
    }

    num[i] = '\0';

    int_num = atoi(num);

    if (int_num <= 4095) return int_num; // verifica se o endereço de memória está dentro dos valores permitidos
    return -1;
}

int stringParaInt(char *bin)
{
    /*
        Converte uma string contendo um valor binário para um inteiro
        Entradas:
            char* bin: A string a ser convertida
        Retorno:
            A string convertida para um inteiro

        Exemplo:
        "1010" retorna 10
    */
    int i=0, num=0;
    for (int k = strlen(bin)-1; k >= 0; k--)
    {
        if (bin[k] == '1')
        {
            num += (int)pow(2, i);
        }
        i++;
    }
    return num;
}

int64_t montaLinhaDeInstrucao (int opcode1, int operand1, int opcode2, int operand2)
{
    /*
        Monta uma linha de memória de instrução IAS, sendo composta de duas instruções individuais justapostas
        Entradas:
        int opcode1: O inteiro representando o opcode da instrução à esquerda da linha
        int operand1: O inteiro representando o argumento da instrução à esquerda da linha
        int opcode2: O inteiro representando o opcode da instrução à direita da linha
        int operand2: O inteiro representando o argumento da instrução à direita da linha
    */

    int64_t final = 0;
    final = final | opcode1;
    final = final << 12;
    final = final | operand1;
    final = final << 8;
    final = final | opcode2;
    final = final << 12;
    final = final | operand2;

    return final;
}

void armazenaNaMemoria(int endereco, int dado)
{
    /*
        Guarda um valor na memoria - usado como encapsulamento
        Entradas:
            int endereco: O endereço de memória para se guardar o dado
            int dado: O dado a ser armazenado
    */
    if(endereco <= TAMANHO_MEMORIA)
    {
        memoria[endereco] = dado;
    }
}

void dumpDaMemoria() {
    /*
        Realiza um dump de todo o conteúdo da memória em um arquivo output.txt
    */

    FILE *outFile;
    outFile = fopen("output.txt", "w");

    int i=0;
    while (memoria[i] != 0) {
        fwrite(memoria[i], 1, sizeof(int), outFile);
    }

    fclose(outFile);
}

/*
Instruções - Opcode
LOAD MQ - 00001010 (10)
LOAD MQ,M(X) - 00001001 (9)
STOR M(X) - 00100001 (33)
LOAD M(X) - 00000001 (1)
LOAD -M(X) - 00000010 (2)
LOAD |M(X)| - 00000011 (3)
LOAD -|M(X)| - 00000100 (4)

JUMP M(X,0:19) - 00001101 (13)
JUMP M(X,20:39) - 00001110 (14)

JUMP +M(X,0:19) - 00001111 (15)
JUMP +M(X,20:39) - 00010000 (16)

ADD M(X) - 00000101 (5)
ADD |M(X)| - 00000111 (7)
SUB M(X) - 00000110 (6)
SUB |M(X)| - 00001000 (8)
MUL M(X) - 00001011 (11)
DIV M(X) - 00001100 (12)
LSH - 00010100 (20)
RSH - 00010101 (21)
STOR M(X,8:19) - 00010010 (18)
STOR M(X,28:39) - 00010011 (19)

EXIT - 11111111 (255)
*/

booleano comecaCom(char* palavra, char* prefixo)
{
    /*
        Verifica se uma palavra começa com algum prefixo específico
        Parâmetros:
            char* palavra: A string a qual se quer ver o prefixo
            char* prefixo: O prefixo a ser comparado com o início da palavra
        Retorno:
            True: Caso a palavra tenha o prefixo
            False: Caso a palavra não tenha o prefixo
    */
    int tamanho_palavra = strlen(palavra);
    int tamanho_prefixo = strlen(prefixo);

    // printf("\nComparando %s com prefixo %s...", palavra, prefixo);

    if (tamanho_prefixo > tamanho_palavra)
    {
        return False;
    }

    int i = 0;
    while (i < tamanho_prefixo)
    {
        if (palavra[i] != prefixo[i])
        {
            return False;
        }
        i++;
    }

    // printf(": True");
    return True;
}

booleano terminaCom(char* palavra, char* sufixo)
{
    /*
        Verifica se uma palavra termina com algum sufixo específico
        Parâmetros:
            char* palavra: A string a qual se quer ver o prefixo
            char* sufixo: O sufixo a ser comparado com o fim da palavra
        Retorno:
            True: Caso a palavra tenha o sufixo
            False: Caso a palavra não tenha o sufixo
    */
    int tamanho_palavra = strlen(palavra);
    int tamanho_sufixo = strlen(sufixo);

    if (tamanho_sufixo > tamanho_palavra)
    {
        return False;
    }

    int i = tamanho_palavra - 1;
    int j = tamanho_sufixo - 1;
    while (j >= 0)
    {
        if (palavra[i] != sufixo[j])
        {
            return False;
        }
        i--;
        j--;
    }
    return True;
}

void converteInstrucao(char* instrucao, char* opcode, int* endereco)
{
    /*
        Converte uma instrução em seu respectivo OPCODE e endereço
        Parâmetros:
        char* instrucao: A instrução a ser convertida
        char* opcode: Uma string para receber o opcode convertido
        int* endereco: Um inteiro para receber o endereço convertido
        Exemplo:
        instrucao = LOAD-M(57) -> opcode = "00000001" & endereco = 57

        Em caso de erro (instrução não reconhecida), retorna "00000000" para o opcode e -1 para o endereço
    */
    if (comecaCom(instrucao,"LOAD-MQ,M("))
    {
        // Opcode: 00001001 (9)
        // Inicio endereço: posicao 10
        strcpy(opcode, "00001001");
        *endereco = pegaParametroInstrucao(instrucao, 10);
    }
    else if (comecaCom(instrucao,"LOAD-MQ"))
    {
        // Opcode: 00001010 (10)
        // Não há endereço a ser lido
        strcpy(opcode, "00001010");
        *endereco = 0;
    }
    else if (comecaCom(instrucao,"LOAD-M("))
    {
        // Opcode 00000001 (1)
        // Início endereço: 7
        strcpy(opcode, "00000001");
        *endereco = pegaParametroInstrucao(instrucao, 7);
    }
    else if (comecaCom(instrucao,"LOAD--M("))
    {
        // Opcode 00000010 (2)
        // Início endereço: 8
        strcpy(opcode, "000010010");
        *endereco = pegaParametroInstrucao(instrucao, 8);
    }
    else if (comecaCom(instrucao,"LOAD-|M("))
    {
        // Opcode 00000011 (3)
        // Início endereço: 8
        strcpy(opcode, "00000011");
        *endereco = pegaParametroInstrucao(instrucao, 8);
    }
    else if (comecaCom(instrucao,"LOAD--|M("))
    {
        // Opcode 00000100 (4)
        // Início endereço: 9
        strcpy(opcode, "00000100");
        *endereco = pegaParametroInstrucao(instrucao, 9);
    }
    else if (comecaCom(instrucao,"STOR-M("))
    {
        // Início endereço: 7
        if (terminaCom(instrucao,"8:19)"))
        {
            // Opcode 00010010 (18)
            strcpy(opcode, "00010010");
            *endereco = pegaParametroInstrucao(instrucao, 7);
        }
        else if (terminaCom(instrucao, "28:39)"))
        {
            // Opcode 00010011 (19)
            strcpy(opcode, "00010011");
            *endereco = pegaParametroInstrucao(instrucao, 7);
        }
        else
        {
            // Opcode 00100001 (33)
            strcpy(opcode, "00100001");
            *endereco = pegaParametroInstrucao(instrucao, 7);
        }
    }
    else if (comecaCom(instrucao,"JUMP-M("))
    {
        // Início endereço: 7
        if (terminaCom(instrucao,"0:19)"))
        {
            // Opcode 00001101 (13)
            strcpy(opcode, "00001101");
            *endereco = pegaParametroInstrucao(instrucao, 7);
        }
        else if (terminaCom(instrucao, "20:39)"))
        {
            // Opcode 00001110 (14)
            strcpy(opcode, "00001110");
            *endereco = pegaParametroInstrucao(instrucao, 7);
        }
    }
    else if (comecaCom(instrucao,"JUMP+-M("))
    {
        // Início endereço: 8
        if (terminaCom(instrucao,"0:19)"))
        {
            // Opcode 00001111 (15)
            strcpy(opcode, "00001111");
            *endereco = pegaParametroInstrucao(instrucao, 8);
        }
        else if (terminaCom(instrucao, "20:39)"))
        {
            // Opcode 00010000 (16)
            strcpy(opcode, "00010000");
            *endereco = pegaParametroInstrucao(instrucao, 8);
        }
    }
    else if (comecaCom(instrucao,"ADD-M("))
    {
        // Opcode 00000101 (5)
        // Início endereço: 6
        strcpy(opcode, "00000101");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"ADD-|M("))
    {
        // Opcode 00000111 (7)
        // Início endereço: 7
        strcpy(opcode, "00000111");
        *endereco = pegaParametroInstrucao(instrucao, 7);
    }
    else if (comecaCom(instrucao,"SUB-M("))
    {
        // Opcode 00000110 (6)
        // Início endereço: 6
        strcpy(opcode, "00000110");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"SUB-|M("))
    {
        // Opcode 00001000 (8)
        // Início endereço: 7
        strcpy(opcode, "00001000");
        *endereco = pegaParametroInstrucao(instrucao, 7);
    }
    else if (comecaCom(instrucao,"MUL-M("))
    {
        // Opcode 00001011 (11)
        // Início endereço: 6
        strcpy(opcode, "00001011");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"DIV-M("))
    {
        // Opcode 00001100 (12)
        // Início endereço: 6
        strcpy(opcode, "00001100");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"LSH"))
    {
        // Opcode 00010100 (20)
        // Início endereço: Não há
        strcpy(opcode, "00010100");
        *endereco = 0;
    }
    else if (comecaCom(instrucao,"RSH"))
    {
        // Opcode 00010101 (21)
        // Início endereço: Não há
        strcpy(opcode, "00010101");
        *endereco = 0;
    }
    else if (comecaCom(instrucao,"EXIT"))
    {
        // Opcode 11111111 (255)
        // Início endereço: Não há
        strcpy(opcode, "11111111");
        *endereco = 0;
    }
    else
    {
        strcpy(opcode, "00000000");
        *endereco = -1;
    }
}

int main ()
{
    /*
        Ponto de entrada principal do programa
    */

    char instrucao[100], opcode[6], operand[6];

    FILE *arquivoEntrada, *arquivoSaida;
    arquivoEntrada = fopen("instructions.txt", "r");
    arquivoSaida = fopen("out.txt", "w");

    int linhaAtualDeLeitura = 0;

    // Usados para o controle durante a junção de duas instruções que devem ir na mesma linha
    int opcodeEsquerdo, enderecoEsquerdo;
    booleano controle = False;

    // Lê linha a linha do arquivo de entrada
    while(fgets(instrucao, 100, arquivoEntrada) != NULL)
    {
        linhaAtualDeLeitura++;

        char opcodeString[8];
        int endereco;
        int64_t valor;  // Caso o endereço corresponda a um número


        // Removendo possíveis \n ou \n\r no final de cada linha
        if (instrucao[strlen(instrucao) - 1] == '\n')
        {
            instrucao[strlen(instrucao) - 1] = '\0';
        }
        if (instrucao[strlen(instrucao) - 1] == '\r')
        {
            instrucao[strlen(instrucao) - 1] = '\0';
        }

        // A fazer: leitura **integral** das 500 primeiras linhas aqui

        converteInstrucao(instrucao, opcodeString, &endereco);

        int opcodeInt = stringParaInt(opcodeString);

        //armazenaNaMemoria(endereco, opcode);

        printf("\nOpcode: %s endereco: %d instrucao: \"%s\"", opcodeString, endereco, instrucao);

        if (!controle)
        {
            // Caso a instrução lida tenha que ir na esquerda da seção de memória
            opcodeEsquerdo = opcodeInt;
            enderecoEsquerdo = endereco;
            controle = True;
        }
        else
        {
            // Caso a instrução lida tenha que ir na direita da seção de memória, monta a linha e armazena
            int64_t word = montaLinhaDeInstrucao(opcodeEsquerdo, enderecoEsquerdo, opcodeInt, endereco);
            fprintf(arquivoSaida, "%"PRId64"\n", word);
            controle = False;
        }

    }

    //dumpDaMemoria();

    fclose(arquivoSaida);
    fclose(arquivoEntrada);
    return 0;
}