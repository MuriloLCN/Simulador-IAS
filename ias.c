#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main ()
{
    char instruction[100], opcode[6], operand[6];
    FILE *f;
    f = fopen("instructions.txt", "r");
    while(fgets(instruction, 100, f) != NULL)
    {
        GetOpCode(instruction, opcode);
        printf("Opcode: %s\n", opcode);

        GetOperand(instruction, operand);
        printf("Operand: %s\n", operand);

        if (strcmp(instruction, "LOAD") == 0)  printf("000000001\n");
        else if (strcmp(instruction, "ADD") == 0) printf("000000010\n");
    }

    fclose(f);
    return 0;
}