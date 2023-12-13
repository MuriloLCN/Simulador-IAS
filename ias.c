#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {False, True} booleano;

void OffSpace (char line[])
{
    for (int i = 0; i < strlen(line); i++)
    {
        if (line[i] == ' ')
        {
            for (int j = i+1; j < strlen(line); j++)
            {
                line[j-1] = line[j];
            }
            line[strlen(line)-1] = '\0';
        }
    }
}

int GetOpCode (char instruction[], char opcode[])
{
    int i = 0;
    while (instruction[i] != 32)
    {
        opcode[i] = instruction[i];
        i++;
    }
    opcode[i] = '\0';
    return 0;
}

int GetOperand (char instruction[], char operand[])
{
    int i = 0;
    while (instruction[i] != 32) {i++;} // encontra o primeiro espaço
    while (instruction[i] == 32) {i++;} // passa por todos os espaços
    int j = 0;
    while (instruction[i] != '\n')
    {
        operand[j] = instruction[i];
        i++;
        j++;
    }
    operand[j] = '\0';    

    return 0;
}

int memLocation (char *instruction, int index)
{
    char num[5];
    int i = 0, int_num;
    while (instruction[index] == '0' ||
           instruction[index] == '1' ||
           instruction[index] == '2' ||
           instruction[index] == '3' ||
           instruction[index] == '4' ||
           instruction[index] == '5' ||
           instruction[index] == '6' ||
           instruction[index] == '7' ||
           instruction[index] == '8' ||
           instruction[index] == '9')
    {
        if (i > 3) return -1; // erro, endereço de memória de tamanho maior que 4 digitos
        else
        {
            num[i] = instruction[index];
            i++; 
            index++;
        }
    }
    num[i] = '\0';

    int_num = atoi(num);
    if (int_num <= 4095) return int_num; // verifica se o endereço de memória tá dentro dos valores permitidos
    else return -1;
}

/*
Instruções - Opcode
LOAD MQ - 00001010
LOAD MQ,M(X) - 00001001
STOR M(X) - 00100001
LOAD M(X) - 00000001
LOAD -M(X) - 00000010
LOAD |M(X)| - 00000011
LOAD -|M(X)| - 00000100

JUMP M(X,0:19) - 00001101
JUMP M(X,20:39) - 00001110

JUMP +M(X,0:19) - 00001111
JUMP +M(X,20:39) - 00010000

ADD M(X) - 00000101
ADD |M(X)| - 00000111
SUB M(X) - 00000110
SUB |M(X)| - 00001000
MUL M(X) - 00001011
DIV M(X) - 00001100
LSH - 00010100
RSH - 00010101
STOR M(X,8:19) - 00010010
STOR M(X,28:39) - 00010011

EXIT - 11111111
*/

booleano startsWith(char* palavra, char* prefixo)
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

booleano endsWith(char* palavra, char* sufixo)
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

int main ()
{
    char instruction[100], opcode[6], operand[6];
    FILE *f;
    f = fopen("instructions.txt", "r");
    int linha = 0;
    while(fgets(instruction, 100, f) != NULL)
    {
        linha++;
        printf("\n");

        //+/+/+/+/+/+/ CASOS LOAD /+/+/+/+/+/+/+/+/+/+/+/+/
        if (startsWith(instruction,"LOAD-MQ,M("))
        {
            // printf("> linha %d era um LOAD MQ,M(%d)", linha, memLocation(instruction, 10));
            // Opcode: 00001001
            // Inicio endereço: posicao 10
        }
        else if (startsWith(instruction,"LOAD-MQ"))
        {
            // Opcode: 00001010
            // Não há endereço a ser lido
        }
        else if (startsWith(instruction,"LOAD-M("))
        {
            // Opcode 00100001
            // Início endereço: 7
        }
        else if (startsWith(instruction,"LOAD--M("))
        {
            // Opcode 00000010
            // Início endereço: 8
        }
        else if (startsWith(instruction,"LOAD-|M("))
        {
            // Opcode 00000011
            // Início endereço: 8
        }
        else if (startsWith(instruction,"LOAD--|M("))
        {
            // Opcode 00000100
            // Início endereço: 9
        }
        else if (startsWith(instruction,"STOR-M("))
        {
            // Início endereço: 7
            if (endsWith(instruction,"8:19)"))
            {
                // Opcode 00010010
            }
            else if (endsWith(instruction, "28:39)"))
            {
                // Opcode 00010011
            }
            else
            {
                // Opcode 00100001
            }
        }
        else if (startsWith(instruction,"JUMP-M("))
        {
            // Início endereço: 7
            if (endsWith(instruction,"0:19)"))
            {
                // Opcode 00001101
            }
            else if (endsWith(instruction, "20:39)"))
            {
                // Opcode 00001110
            }
        }
        else if (startsWith(instruction,"JUMP+-M("))
        {
            // Início endereço: 8
            if (endsWith(instruction,"0:19)"))
            {
                // Opcode 00001111
            }
            else if (endsWith(instruction, "20:39)"))
            {
                // Opcode 00010000
            }
        }
        else if (startsWith(instruction,"ADD-M("))
        {
            // Opcode 00000101
            // Início endereço: 6
        }
        else if (startsWith(instruction,"ADD-|M("))
        {
            // Opcode 00000111
            // Início endereço: 7
        }
        else if (startsWith(instruction,"SUB-M("))
        {
            // Opcode 00000110
            // Início endereço: 6
        }
        else if (startsWith(instruction,"SUB-|M("))
        {
            // Opcode 00001000
            // Início endereço: 7
        }
        else if (startsWith(instruction,"MUL-M("))
        {
            // Opcode 00001011
            // Início endereço: 6 
        }
        else if (startsWith(instruction,"DIV-M("))
        {
            // Opcode 00001100
            // Início endereço: 6
        }
        else if (startsWith(instruction,"LSH"))
        {
            // Opcode 00010100
            // Início endereço: Não há
        }
        else if (startsWith(instruction,"RSH"))
        {
            // Opcode 00010101
            // Início endereço: Não há
        }
        else if (startsWith(instruction,"EXIT"))
        {
            // Opcode 11111111
            // Início endereço: Não há
        }
        else
        {
            printf("\n> Erro: Comando '%s' nao reconhecido, linha %d", opcode, linha);
        }
        // if (strcmp(instruction, "LOAD") == 0)  printf("000000001\n");
        // else if (strcmp(instruction, "ADD") == 0) printf("000000010\n");
    }

    fclose(f);
    return 0;
}