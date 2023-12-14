#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#define TAM_MEM 4096

typedef enum {False, True} booleano;

int memory[TAM_MEM] = {0};

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

int binToDec (char *bin)
{
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

int64_t joinElements (int opcode1, int operand1, int opcode2, int operand2)
{
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

void storeInMemory(int endereco, int dado) 
{
    if(endereco <= TAM_MEM)
    {
        memory[endereco] = dado;
    }
}

void printMemoryDec() {
    FILE *outFile;
    outFile = fopen("output.txt", "w");

    int i=0;
    while (memory[i] != 0) {
        fwrite(memory[i], 1, sizeof(int), outFile);
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

void converteInstrucao(char* instruction, char* opcode, int* endereco)
{
    /*
        Converte uma instrução em seu respectivo OPCODE e endereço
        Parâmetros:
        char* instruction: A instrução a ser convertida
        char* opcode: Uma string para receber o opcode convertido
        int* endereco: Um inteiro para receber o endereço convertido
        Exemplo:
        instruction = LOAD-M(57) -> opcode = "00000001" & endereco = 57

        Em caso de erro (instrução não reconhecida), retorna "00000000" para o opcode e -1 para o endereço
    */
    if (startsWith(instruction,"LOAD-MQ,M("))
    {
        // Opcode: 00001001 (9)
        // Inicio endereço: posicao 10
        strcpy(opcode, "00001001");
        *endereco = memLocation(instruction, 10);
    }
    else if (startsWith(instruction,"LOAD-MQ"))
    {
        // Opcode: 00001010 (10)
        // Não há endereço a ser lido
        strcpy(opcode, "00001010");
        *endereco = 0;
    }
    else if (startsWith(instruction,"LOAD-M("))
    {
        // Opcode 00000001 (1)
        // Início endereço: 7
        strcpy(opcode, "00000001");
        *endereco = memLocation(instruction, 7);
    }
    else if (startsWith(instruction,"LOAD--M("))
    {
        // Opcode 00000010 (2)
        // Início endereço: 8
        strcpy(opcode, "000010010");
        *endereco = memLocation(instruction, 8);
    }
    else if (startsWith(instruction,"LOAD-|M("))
    {
        // Opcode 00000011 (3)
        // Início endereço: 8
        strcpy(opcode, "00000011");
        *endereco = memLocation(instruction, 8);
    }
    else if (startsWith(instruction,"LOAD--|M("))
    {
        // Opcode 00000100 (4)
        // Início endereço: 9
        strcpy(opcode, "00000100");
        *endereco = memLocation(instruction, 9);
    }
    else if (startsWith(instruction,"STOR-M("))
    {
        // Início endereço: 7
        if (endsWith(instruction,"8:19)"))
        {
            // Opcode 00010010 (18)
            strcpy(opcode, "00010010");
            *endereco = memLocation(instruction, 7);
        }
        else if (endsWith(instruction, "28:39)"))
        {
            // Opcode 00010011 (19)
            strcpy(opcode, "00010011");
            *endereco = memLocation(instruction, 7);
        }
        else
        {
            // Opcode 00100001 (33)
            strcpy(opcode, "00100001");
            *endereco = memLocation(instruction, 7);
        }
    }
    else if (startsWith(instruction,"JUMP-M("))
    {
        // Início endereço: 7
        if (endsWith(instruction,"0:19)"))
        {
            // Opcode 00001101 (13)
            strcpy(opcode, "00001101");
            *endereco = memLocation(instruction, 7);
        }
        else if (endsWith(instruction, "20:39)"))
        {
            // Opcode 00001110 (14)
            strcpy(opcode, "00001110");
            *endereco = memLocation(instruction, 7);
        }
    }
    else if (startsWith(instruction,"JUMP+-M("))
    {
        // Início endereço: 8
        if (endsWith(instruction,"0:19)"))
        {
            // Opcode 00001111 (15)
            strcpy(opcode, "00001111");
            *endereco = memLocation(instruction, 8);
        }
        else if (endsWith(instruction, "20:39)"))
        {
            // Opcode 00010000 (16)
            strcpy(opcode, "00010000");
            *endereco = memLocation(instruction, 8);
        }
    }
    else if (startsWith(instruction,"ADD-M("))
    {
        // Opcode 00000101 (5)
        // Início endereço: 6
        strcpy(opcode, "00000101");
        *endereco = memLocation(instruction, 6);
    }
    else if (startsWith(instruction,"ADD-|M("))
    {
        // Opcode 00000111 (7)
        // Início endereço: 7
        strcpy(opcode, "00000111");
        *endereco = memLocation(instruction, 7);
    }
    else if (startsWith(instruction,"SUB-M("))
    {
        // Opcode 00000110 (6)
        // Início endereço: 6
        strcpy(opcode, "00000110");
        *endereco = memLocation(instruction, 6);
    }
    else if (startsWith(instruction,"SUB-|M("))
    {
        // Opcode 00001000 (8)
        // Início endereço: 7
        strcpy(opcode, "00001000");
        *endereco = memLocation(instruction, 7);
    }
    else if (startsWith(instruction,"MUL-M("))
    {
        // Opcode 00001011 (11)
        // Início endereço: 6 
        strcpy(opcode, "00001011");
        *endereco = memLocation(instruction, 6);
    }
    else if (startsWith(instruction,"DIV-M("))
    {
        // Opcode 00001100 (12)
        // Início endereço: 6
        strcpy(opcode, "00001100");
        *endereco = memLocation(instruction, 6);
    }
    else if (startsWith(instruction,"LSH"))
    {
        // Opcode 00010100 (20)
        // Início endereço: Não há
        strcpy(opcode, "00010100");
        *endereco = 0;
    }
    else if (startsWith(instruction,"RSH"))
    {
        // Opcode 00010101 (21)
        // Início endereço: Não há
        strcpy(opcode, "00010101");
        *endereco = 0;
    }
    else if (startsWith(instruction,"EXIT"))
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
    char instruction[100], opcode[6], operand[6];
    FILE *f, *out;
    f = fopen("instructions.txt", "r");
    out = fopen("out.txt", "w");
    int linha = 0, opcode_memo, adress_memo, control=0;
    while(fgets(instruction, 100, f) != NULL)
    {
        linha++;
        printf("\n");

        char opcode[8];
        int endereco;

        if (instruction[strlen(instruction) - 1] == '\n')
        {
            instruction[strlen(instruction) - 1] = '\0';
        }
        if (instruction[strlen(instruction) - 1] == '\r')
        {
            instruction[strlen(instruction) - 1] = '\0';
        }

        converteInstrucao(instruction, opcode, &endereco);

        int opcode_dec = binToDec(opcode);
        
        //storeInMemory(endereco, opcode);

        printf("\nOpcode: %s endereco: %d instrucao: \"%s\"", opcode, endereco, instruction);
        
        if (control == 0)
        {
            opcode_memo = opcode_dec;
            adress_memo = endereco;
            control = 1;
        }
        else if (control == 1)
        {
            int64_t word = joinElements(opcode_memo, adress_memo, opcode_dec, endereco);
            fprintf(out, "%"PRId64"\n", word);
            control = 0;
        }
        //+/+/+/+/+/+/ CASOS LOAD /+/+/+/+/+/+/+/+/+/+/+/+/
        // if (startsWith(instruction,"LOAD-MQ,M("))
        // {
        //     // printf("> linha %d era um LOAD MQ,M(%d)", linha, memLocation(instruction, 10));
        //     // Opcode: 00001001
        //     // Inicio endereço: posicao 10
        // }
        // else if (startsWith(instruction,"LOAD-MQ"))
        // {
        //     // Opcode: 00001010
        //     // Não há endereço a ser lido
        // }
        // else if (startsWith(instruction,"LOAD-M("))
        // {
        //     // Opcode 00000001
        //     // Início endereço: 7
        // }
        // else if (startsWith(instruction,"LOAD--M("))
        // {
        //     // Opcode 00000010
        //     // Início endereço: 8
        // }
        // else if (startsWith(instruction,"LOAD-|M("))
        // {
        //     // Opcode 00000011
        //     // Início endereço: 8
        // }
        // else if (startsWith(instruction,"LOAD--|M("))
        // {
        //     // Opcode 00000100
        //     // Início endereço: 9
        // }
        // else if (startsWith(instruction,"STOR-M("))
        // {
        //     // Início endereço: 7
        //     if (endsWith(instruction,"8:19)"))
        //     {
        //         // Opcode 00010010
        //     }
        //     else if (endsWith(instruction, "28:39)"))
        //     {
        //         // Opcode 00010011
        //     }
        //     else
        //     {
        //         // Opcode 00100001
        //     }
        // }
        // else if (startsWith(instruction,"JUMP-M("))
        // {
        //     // Início endereço: 7
        //     if (endsWith(instruction,"0:19)"))
        //     {
        //         // Opcode 00001101
        //     }
        //     else if (endsWith(instruction, "20:39)"))
        //     {
        //         // Opcode 00001110
        //     }
        // }
        // else if (startsWith(instruction,"JUMP+-M("))
        // {
        //     // Início endereço: 8
        //     if (endsWith(instruction,"0:19)"))
        //     {
        //         // Opcode 00001111
        //     }
        //     else if (endsWith(instruction, "20:39)"))
        //     {
        //         // Opcode 00010000
        //     }
        // }
        // else if (startsWith(instruction,"ADD-M("))
        // {
        //     // Opcode 00000101
        //     // Início endereço: 6
        // }
        // else if (startsWith(instruction,"ADD-|M("))
        // {
        //     // Opcode 00000111
        //     // Início endereço: 7
        // }
        // else if (startsWith(instruction,"SUB-M("))
        // {
        //     // Opcode 00000110
        //     // Início endereço: 6
        // }
        // else if (startsWith(instruction,"SUB-|M("))
        // {
        //     // Opcode 00001000
        //     // Início endereço: 7
        // }
        // else if (startsWith(instruction,"MUL-M("))
        // {
        //     // Opcode 00001011
        //     // Início endereço: 6 
        // }
        // else if (startsWith(instruction,"DIV-M("))
        // {
        //     // Opcode 00001100
        //     // Início endereço: 6
        // }
        // else if (startsWith(instruction,"LSH"))
        // {
        //     // Opcode 00010100
        //     // Início endereço: Não há
        // }
        // else if (startsWith(instruction,"RSH"))
        // {
        //     // Opcode 00010101
        //     // Início endereço: Não há
        // }
        // else if (startsWith(instruction,"EXIT"))
        // {
        //     // Opcode 11111111
        //     // Início endereço: Não há
        // }
        // else
        // {
        //     printf("\n> Erro: Comando '%s' nao reconhecido, linha %d", opcode, linha);
        // }
        // if (strcmp(instruction, "LOAD") == 0)  printf("000000001\n");
        // else if (strcmp(instruction, "ADD") == 0) printf("000000010\n");
    }

    //printMemoryDec();


    fclose(out);
    fclose(f);
    return 0;
}