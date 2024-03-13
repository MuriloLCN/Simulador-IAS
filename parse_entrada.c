/*
    SIMULADOR IAS
    DISCIPLINA: ARQUITETURA E ORGANIZAÇÃO DE COMPUTADORES I
    PROFESSOR: ANDERSON FAUSTINO DA SILVA
    ALUNOS: LEANDRO EUGÊNIO FARIAS BERTON RA: 129268
            FERNANDO SILVA GRANDE RA: 125294
            MURILO LUIS CALVO NEVES RA: 129037
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include "parse_entrada.h"

#define TAMANHO_MEMORIA 4096

typedef enum {False, True} booleano;

typedef enum {
  LOAD_MQ,
  LOAD_MQ_MX,
  STOR_MX,
  LOAD_MX,
  LOAD_MenosMX,
  LOAD_ABSMX,
  LOAD_MenosABSMX,
  JUMP_ESQ,
  JUMP_DIR,
  JUMPMais_ESQ,
  JUMPMais_DIR,
  ADD_MX,
  ADD_ABSMX,
  SUB_MX,
  SUB_ABSMX,
  MUL_MX,
  DIV_MX,
  LSH,
  RSH,
  STOR_MX_ESQ,
  STOR_MX_DIR,
  EXIT,
  NENHUMA
} Instrucao;

// ********************** Cabeçalhos de funções **********************
void printBits(int64_t n);
void printMemoria(uint8_t* memoria);
int pegaParametroInstrucao(char *instrucao, int index);
int stringParaInt(char *bin);
uint64_t montaLinhaDeInstrucao (int opcode1, int operand1, int opcode2, int operand2);
void armazenaNaMemoria (int posicao, uint64_t num, uint8_t *memoria);
uint64_t buscaNaMemoria (uint8_t *memoria, int posicao);
int64_t converteDado(uint64_t entrada);
booleano stringEhNumericaOuNula(char* str);
void carregaDados (FILE *arquivoEntrada,  uint8_t *memoria, int* ciclos_vetor, int* numeroLinhas);
void dumpDaMemoria(uint8_t *memoria, char nome_arq_saida[]);
booleano comecaCom(char* palavra, char* prefixo);
booleano terminaCom(char* palavra, char* sufixo);
void converteInstrucao(char* instrucao, char* opcode, int* endereco);
void completaMemoria (int PC, uint8_t *memoria);
char *cria_nome_saida(char *nome_entrada);
void tratamento_string(char *linha);
// *******************************************************************

int flagInstrucaoNaoReconhecida = 0;

void carregarMemoria(FILE* arquivoEntrada, uint8_t** memoria, int** ciclos_vetor, int* erroInstrucao)
{
    /*
        Função principal de carregamento de memória: Preenche a memória passada por referência com os valores
        lidos do arquivo de entrada, além de ler a quantidade de ciclos de cada instrução

        Entradas:
            FILE* arquivoEntrada: O arquivo contendo as intruções e diretivas do IAS
            uint8_t** memoria: O ponteiro para a memória que deverá ser preenchida
            int** ciclos_vetor: O ponteiro para o vetor que contém a quantidade de ciclos de clock de cada tipo de instrução
    */

    //printf("\nEntrou no carregar memoria");
    
    char instrucao[100], opcode[6], operand[6];

    // Completa a memória com zeros antes de qualquer coisa
    completaMemoria(0, *memoria);

    int linhaAtualDeLeitura = 0;

    // Usados para o controle durante a junção de duas instruções que devem ir na mesma linha
    int opcodeEsquerdo, enderecoEsquerdo;
    booleano controle = False;

    // printf("\nChamando carrega dados");
    carregaDados(arquivoEntrada, *memoria, *ciclos_vetor, &linhaAtualDeLeitura); // carrega os dados presentes no arquivo de entrada para a memória do IAS;

    // printf("\nDados carregados");
    int PC = linhaAtualDeLeitura;
    
    // Lê linha a linha do arquivo de entrada
    while(fgets(instrucao, 100, arquivoEntrada) != NULL)
    {
        linhaAtualDeLeitura++;

        char opcodeString[8];
        int endereco;
        uint64_t valor;  // Caso o endereço corresponda a um número


        // Removendo possíveis \n ou \n\r no final de cada linha

        // OBS: Essa ordem é necessária pois Windows e Linux usam terminadores de linha diferentes

        if (instrucao[strlen(instrucao) - 1] == '\n')
        {
            instrucao[strlen(instrucao) - 1] = '\0';
        }
        if (instrucao[strlen(instrucao) - 1] == '\r')
        {
            instrucao[strlen(instrucao) - 1] = '\0';
        }

        // dada a linha de instrução, é devolvida a string que representa o opcode e o local da memória onde será realizado aquela operação
        converteInstrucao(instrucao, opcodeString, &endereco); 

        int opcodeInt = stringParaInt(opcodeString);

        *erroInstrucao = flagInstrucaoNaoReconhecida;

        if (opcodeInt == 255 && controle == False) // se leu a última instrução e não há instrução da direita (número de instruções ímpar)
        {
            //printf("\n\nopcodeint final: %i\nPosicao %i\n", opcodeInt, PC);
            uint64_t word = montaLinhaDeInstrucao(opcodeInt, endereco, 0, 0);
            armazenaNaMemoria(PC, word, *memoria);
            PC++;
            break;
        }
        
        else 
        {
            if (controle == False)
            {
                // Caso a instrução lida tenha que ir na esquerda da seção de memória
                opcodeEsquerdo = opcodeInt;
                enderecoEsquerdo = endereco;
                controle = True;
            }
            else
            {
                //printf("\nprimeiro: %i, %i, pc=%i\n", opcodeEsquerdo, enderecoEsquerdo, PC);
                
                // Caso a instrução lida tenha que ir na direita da seção de memória, monta a linha e armazena
                int64_t word = montaLinhaDeInstrucao(opcodeEsquerdo, enderecoEsquerdo, opcodeInt, endereco);
                armazenaNaMemoria(PC, word, *memoria);
                PC++;
                controle = False;
            }
        }
    }
}

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
        index = 7 (57 começa na sétima posição da string)
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

uint64_t montaLinhaDeInstrucao (int opcode1, int operand1, int opcode2, int operand2)
{
    /*
        Monta uma linha de memória de instrução IAS, sendo composta de duas instruções individuais justapostas
        Entradas:
        int opcode1: O inteiro representando o opcode da instrução à esquerda da linha
        int operand1: O inteiro representando o argumento da instrução à esquerda da linha
        int opcode2: O inteiro representando o opcode da instrução à direita da linha
        int operand2: O inteiro representando o argumento da instrução à direita da linha
    */

    uint64_t final = 0;
    final = final | opcode1;
    final = final << 12;
    final = final | operand1;
    final = final << 8;
    final = final | opcode2;
    final = final << 12;
    final = final | operand2;

    return final;
}

void armazenaNaMemoria (int posicao, uint64_t num, uint8_t *memoria)
{
    /*
        Armazena um valor uint64_t na memória

        Entradas:
            int posicao: A posição da memória a inserir o dado
            uint64_t num: O valor a ser inserido na memória
            uint8_t* memoria: A memória para inserir o valor 
    */

    // Valor de inserção inválido
    if (posicao < 0 || posicao >= TAMANHO_MEMORIA)
    {
        return;
    }

    memoria += posicao*5;  // Posiciona o ponteiro da memórica no local correto da gravação na memória
    for (int i = 0; i < 5; i++) 
    {
        /*
            Em cada iteração, é lido um dos cinco blocos que compões uma dada palavra da memória, então é realizado um
            deslocamento para a direita a fim de posicionar nos 8 bits menos significativos da variável num a respectiva parte que se
            deseja escrever no momento para a memória. Ainda, é feita uma operação de máscara para isolar os 8 bits menos significativos de
            num (i.e, os bits que serão escritos na memória naquela iteração)
        */
        memoria[i] = (num >> (i * 8)) & 0xFF;
    }
}

uint64_t buscaNaMemoria (uint8_t *memoria, int posicao)
{
    /*
        Busca um valor armazenado na memória a partir de sua posição
        Entradas:
            uint8_t* memoria: A memória para buscar o valor
            int posicao: A posição da memória no qual o valor se encontra
    */

    if (posicao < 0 || posicao >= TAMANHO_MEMORIA)
    {
        return 0;
    }

    uint64_t num = 0; 

    memoria += posicao*5; // Posição que o ponteiro de leitura deve ficar na memória

    for (int i = 0; i < 5; i++) // 5 "blocos" de 8 bits - a maior unidade que pode ser lida de forma eficiente
    {
        /*
            A cada iteração, é lido um dos cinco blocos que compões uma palavra da memória e, simultaneamente, é feito
            um deslocamento para a esquerda para posicionar cada bloco em seu local final
        */
        num |= ((uint64_t)memoria[i] << (i * 8));
    }
    memoria -= posicao*5;  // Devolve a memória com o ponteiro posicionado na mesma posição que recebeu

    return num;
}

int64_t converteDado(uint64_t entrada)
{
    /*
        Converte um valor uint64_t da memória para um int64_t normal para que possa ser trabalhado com operações em C
        Isso pois a representação de negativos pedida foi sinal magnitude porém a linguagem C utiliza complemento de dois
        Entrada:
            uint64_t entrada: O valor que se quer converter
        Retorna o valor convertido para complemento de dois

        OBS: 549755813887 = 0111111111111111111111111111111111111111
             549755813888 = 1000000000000000000000000000000000000000
    */

    int64_t magnitude = entrada & 549755813887; // máscara que extrai a magnitude do número vindo pela entrada, o bit de sinal é desconsiderado
    int sinal = (entrada & 549755813888) >> 39; // máscara que filtra o bit de sinal, também é feito bitshift para que o sinal ocupe a posição do bit menos significativo

    if (sinal == 1)
    {
        magnitude *= -1; // se for negativo, é necessário multiplicar a magnitude por -1 para a representação correta do número decimal
    }

    return magnitude;
}

uint64_t inverteDado(int64_t entrada)
{
    /*
        Inverte um dado para ser armazenado na memória no formato sinal magnitude
        Entrada:
            int64_t entrada: O valor que se quer converter
        Retorna o valor convertido para sinal magnitude

        Usado como função reversa à "converteDado()"
    */

    if (entrada >= 0)
    {
        return entrada;
    }

    entrada *= -1;

    entrada = entrada | 549755813888;

    return entrada;
}

booleano stringEhNumericaOuNula(char* str)
{
    /*
        Verifica se uma string representa um valor numérico ou uma string nula

        Entrada:
            char* str: A string a ser checada
        Retorno:
            True caso a string seja numérica/nula
            False caso a string possua símbolos diferentes
    */
    for (int i = 0; i < strlen(str); i++)
    {
        // printf("* %c - %i * ", str[i], str[i]);
        switch (str[i])
        {
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '\0':
            break;
        
        default:
            return False;
        }
    }
    return True;
}

void pegaCiclo (char *linha, Instrucao *instrucao, int *ciclos)
{
    char instrucao_buffer[10], ciclos_buffer[5];
    int indice = 0, ciclos_indice=0;
    instrucao_buffer[indice] = linha[indice];

    while (linha[indice] != ':') // pega a instrução, contida antes do sinal :
    {
        instrucao_buffer[indice] = linha[indice];
        indice++;
    }
    instrucao_buffer[indice] = '\0';        	
    indice++;

    //printf("\n%s.", instrucao_buffer);

    while (linha[indice] != '\0') // pega a quantiade de ciclos, contida antes do fim da string
    {
        ciclos_buffer[ciclos_indice] = linha[indice];
        indice++;
        ciclos_indice++;
    }
    ciclos_buffer[ciclos_indice] = '\0';
    indice++;

    //("\nbuffer ciclos: %s\n", ciclos_buffer);
    *ciclos = atoi(ciclos_buffer);

    if (strcmp(instrucao_buffer, "loadm")==0 || strcmp(instrucao_buffer, "LOADM")==0)
    {
        *instrucao = LOAD_MQ;
    }

    else if (strcmp(instrucao_buffer, "loadmm")==0 || strcmp(instrucao_buffer, "LOADMM")==0)
    {
        *instrucao = LOAD_MQ_MX;
    }    

    else if (strcmp(instrucao_buffer, "stor")==0 || strcmp(instrucao_buffer, "STOR")==0)
    {
        *instrucao = STOR_MX;
    }    

    else if (strcmp(instrucao_buffer, "load")==0 || strcmp(instrucao_buffer, "LOAD")==0)
    {
        *instrucao = LOAD_MX;
    }    

    else if (strcmp(instrucao_buffer, "load-m")==0 || strcmp(instrucao_buffer, "LOAD-M")==0)
    {
        *instrucao = LOAD_MenosMX;
    }    

    else if (strcmp(instrucao_buffer, "load|m")==0 || strcmp(instrucao_buffer, "LOAD|M")==0)
    {
        *instrucao = LOAD_ABSMX;
    }   

    else if (strcmp(instrucao_buffer, "load-|m")==0 || strcmp(instrucao_buffer, "LOAD-|M")==0)
    {
        *instrucao = LOAD_MenosABSMX;
    }   

    else if (strcmp(instrucao_buffer, "jump")==0 || strcmp(instrucao_buffer, "JUMP")==0)
    {
        *instrucao = JUMP_DIR;
    }   

    else if (strcmp(instrucao_buffer, "jump+")==0 || strcmp(instrucao_buffer, "JUMP+")==0)
    {
        *instrucao = JUMPMais_DIR;
    }   

    else if (strcmp(instrucao_buffer, "add")==0 || strcmp(instrucao_buffer, "ADD")==0)
    {
        *instrucao = ADD_MX;
    }   

    else if (strcmp(instrucao_buffer, "add|")==0 || strcmp(instrucao_buffer, "ADD|")==0)
    {
        *instrucao = ADD_ABSMX;
    }   

    else if (strcmp(instrucao_buffer, "sub")==0 || strcmp(instrucao_buffer, "SUB")==0)
    {
        *instrucao = SUB_MX;
    }   

    else if (strcmp(instrucao_buffer, "sub|")==0 || strcmp(instrucao_buffer, "SUB|")==0)
    {
        *instrucao = SUB_ABSMX;
    } 

    else if (strcmp(instrucao_buffer, "mul")==0 || strcmp(instrucao_buffer, "MUL")==0)
    {
        *instrucao = MUL_MX;
    }     

    else if (strcmp(instrucao_buffer, "div")==0 || strcmp(instrucao_buffer, "DIV")==0)
    {
        *instrucao = DIV_MX;
    }   

    else if (strcmp(instrucao_buffer, "lsh")==0 || strcmp(instrucao_buffer, "LSH")==0)
    {
        *instrucao = LSH;
    }   

    else if (strcmp(instrucao_buffer, "rsh")==0 || strcmp(instrucao_buffer, "RSH")==0)
    {
        *instrucao = RSH;
    }   

    else if (strcmp(instrucao_buffer, "storm")==0 || strcmp(instrucao_buffer, "STORM")==0)
    {
        *instrucao = STOR_MX_DIR;
    }   

    else
    {
        *instrucao = NENHUMA;
    }

}

void carregaDados (FILE *arquivoEntrada,  uint8_t *memoria, int *ciclos_vetor, int* numeroLinhas)
{
    /*
        Carrega as primeiras linhas do arquivo de entrada (i.e, que contém dados) para a memória
        Linhas vazias antes da instrução são assumidas como valor zero

        Entradas:
            FILE* arquivoEntrada: O descritor do arquivo de entrada
            uint8_t* memoria: A memória para armazenar os dados lidos
    */
    char linha[30];
    booleano sec_ciclos = False;
    uint64_t numero_convertido;
    rewind(arquivoEntrada); // para garantir que irá ser lida as primeiras linhas do arquivo de entrada
    FILE ultima_leitura; // o algoritmo só para de ler os números quando encontra uma instrução, então é necessário voltar uma linha no descritor para que a leirura das strings seja feita corretamente

    fgets(linha, 30, arquivoEntrada);

    if (linha[strlen(linha) - 1] == '\n')
    {
        linha[strlen(linha) - 1] = '\0';
    }
    if (linha[strlen(linha) - 1] == '\r')
    {
        linha[strlen(linha) - 1] = '\0';
    }

    if (strcmp(linha, "/*") == 0) // verifica, primeiramente, se existe a seção de declaração dos ciclos de clock
    {
        //linhaAtual = linhaAtual + 1;
        sec_ciclos = True;
    }

    while (sec_ciclos == True)
    {
        fgets(linha, 30, arquivoEntrada);

        if (linha[strlen(linha) - 1] == '\n')
        {
            linha[strlen(linha) - 1] = '\0';
        }
        if (linha[strlen(linha) - 1] == '\r')
        {
            linha[strlen(linha) - 1] = '\0';
        }
        if (linha[strlen(linha) - 1] == ' ')
        {
            linha[strlen(linha) - 1] = '\0';
        }

        if (strcmp(linha, "*/")==0)
        {
            // printf("\n ** Fim da leitura dos ciclos de clock **\n");
            sec_ciclos = False;
        }

        else 
        {   
            Instrucao instrucao;
            int ciclo;
            pegaCiclo(linha, &instrucao, &ciclo);
            //printf("\n%d: %d", instrucao, ciclo);
            if (instrucao == JUMP_DIR) ciclos_vetor[JUMP_ESQ] = ciclo;
            if (instrucao == JUMPMais_DIR) ciclos_vetor[JUMPMais_ESQ] = ciclo;
            if (instrucao == STOR_MX_DIR) ciclos_vetor[STOR_MX_ESQ] = ciclo;
            ciclos_vetor[instrucao] = ciclo;
        }

        //linhaAtual++;
    } 

    // for (int i=0; i<23; i++) printf("ciclos_vetor[%i] = %i\n", i, ciclos_vetor[i]);

    *numeroLinhas = 0;

    fgets(linha, 30, arquivoEntrada);
    tratamento_string(linha);
    /*
    if (linha[strlen(linha) - 1] == '\n')
    {
        linha[strlen(linha) - 1] = '\0';
    }
    if (linha[strlen(linha) - 1] == '\r')
    {
        linha[strlen(linha) - 1] = '\0';
    }
    */
    
    while (stringEhNumericaOuNula(linha) && (feof(arquivoEntrada) == 0))
    {
        if (linha[0] == '-')
        {
            numero_convertido = strtoll(linha+1, NULL, 10); // converte a string para inteiro (pulando o caractere -)
            numero_convertido |= 549755813888; // faz com que o bit mais significativo assuma valor 1, indicando que o número é negativo
        }
        else numero_convertido = strtoll(linha, NULL, 10); // converte a string para inteiro
        
        armazenaNaMemoria(*numeroLinhas, numero_convertido, memoria); // grava na memória o dado lido
        //linhaAtual += 1;
        *numeroLinhas += 1;
        //printf("Leitura: %s\n", linha);
        ultima_leitura = *arquivoEntrada;
        fgets(linha, 30, arquivoEntrada);
        tratamento_string(linha);
        /*
        if (linha[strlen(linha) - 1] == '\n')
        {
            linha[strlen(linha) - 1] = '\0';
        }
        if (linha[strlen(linha) - 1] == '\r')
        {
            linha[strlen(linha) - 1] = '\0';
        }
        */
        //printf("Leitura2: %s\n", linha);
        //printf("Eh numero: %i\n", stringEhNumericaOuNula(linha));
    }
    *arquivoEntrada = ultima_leitura;
    // printf("Saindo dos numeros linha %i\n", *numeroLinhas);
    //*numeroLinhas -= 1; // corrige a posição atual na leitura
    /*
        Como o algoritmo tem que ler a linha para saber se é um dado numérico/nulo, o laço acaba sendo finalizado na linha em que a condição é quebrada
        com isso, é necessário retroceder (com o descritor do arquivo de entrada) para a posição de leitura antes da última linha ser processada
    */
    //fseek(arquivoEntrada, -(strlen(linha)+1), SEEK_CUR);
}

void dumpDaMemoria(uint8_t *memoria, char nome_arq_saida[]) 
{
    /*
        Realiza um dump de todo o conteúdo da memória em um arquivo saida_binario.txt

        Útil para debug
    */
    FILE *saida;
    FILE *saidaBinaria;

    int64_t palavra;
    int64_t dado;

    saida = fopen(nome_arq_saida, "w");
    saidaBinaria = fopen("saida_binario.txt", "w");

    for (int i = 0; i < TAMANHO_MEMORIA; i++)
    {
        palavra = converteDado(buscaNaMemoria(memoria, i));
        fprintf(saida, "%"PRId64"\n", palavra);
        /*
        if (i <= 499)
        {
            dado = converteDado(palavra);
            fprintf(saida, "%"PRId64"\n", dado);
        }
        else {
            fprintf(saida, "%"PRId64"\n", palavra);
        }
        */
        char str[41];
        for (int i = 39; i >= 0; i--)
        {
            if (palavra % 2 == 0)
            {
                str[i] = '0';
            }
            else
            {
                str[i] = '1';
            }
            palavra = palavra/2;
        }
        str[40] = '\0';
        fprintf(saidaBinaria, "%s\n", str);
    }

    fclose(saida);
    fclose(saidaBinaria);   
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
    if (comecaCom(instrucao,"LOAD MQ,"))
    {
        // Opcode: 00001001 (9)
        // Inicio endereço: posicao 10
        strcpy(opcode, "00001001");
        //devido as diferenças no espaçamento, pode ser necessário um espaço a mais para o início do endereço
        if (comecaCom(instrucao, "LOAD MQ,M(")) *endereco = pegaParametroInstrucao(instrucao, 10);
        else if (comecaCom(instrucao,"LOAD MQ, M(")) *endereco = pegaParametroInstrucao(instrucao, 11);
    }
    else if (comecaCom(instrucao,"LOAD MQ"))
    {
        // Opcode: 00001010 (10)
        // Não há endereço a ser lido
        strcpy(opcode, "00001010");
        *endereco = 0;
    }
    else if (comecaCom(instrucao,"LOAD M("))
    {
        // Opcode 00000001 (1)
        // Início endereço: 7
        strcpy(opcode, "00000001");
        *endereco = pegaParametroInstrucao(instrucao, 7);
    }
    else if (comecaCom(instrucao,"LOAD- M(") || comecaCom(instrucao,"LOAD -M("))
    {
        // Opcode 00000010 (2)
        // Início endereço: 8
        strcpy(opcode, "00000010");
        *endereco = pegaParametroInstrucao(instrucao, 8);
    }
    else if (comecaCom(instrucao,"LOAD |M("))
    {
        // Opcode 00000011 (3)
        // Início endereço: 8
        strcpy(opcode, "00000011");
        *endereco = pegaParametroInstrucao(instrucao, 8);
    }
    else if (comecaCom(instrucao,"LOAD- |M(") || comecaCom(instrucao,"LOAD -|M("))
    {
        // Opcode 00000100 (4)
        // Início endereço: 9
        strcpy(opcode, "00000100");
        *endereco = pegaParametroInstrucao(instrucao, 9);
    }
    else if (comecaCom(instrucao,"STOR M("))
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
    else if (comecaCom(instrucao,"JUMP M("))
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
    else if (comecaCom(instrucao,"JUMP+ M("))
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
    else if (comecaCom(instrucao,"ADD M("))
    {
        // Opcode 00000101 (5)
        // Início endereço: 6
        strcpy(opcode, "00000101");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"ADD |M("))
    {
        // Opcode 00000111 (7)
        // Início endereço: 7
        strcpy(opcode, "00000111");
        *endereco = pegaParametroInstrucao(instrucao, 7);
    }
    else if (comecaCom(instrucao,"SUB M("))
    {
        // Opcode 00000110 (6)
        // Início endereço: 6
        strcpy(opcode, "00000110");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"SUB |M("))
    {
        // Opcode 00001000 (8)
        // Início endereço: 7
        strcpy(opcode, "00001000");
        *endereco = pegaParametroInstrucao(instrucao, 7);
    }
    else if (comecaCom(instrucao,"MUL M("))
    {
        // Opcode 00001011 (11)
        // Início endereço: 6
        strcpy(opcode, "00001011");
        *endereco = pegaParametroInstrucao(instrucao, 6);
    }
    else if (comecaCom(instrucao,"DIV M("))
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
        printf("\nInstrucao nao reconhecida: %s", instrucao);
        strcpy(opcode, "00000000");
        *endereco = -1;
        flagInstrucaoNaoReconhecida = 1;
    }
}

void completaMemoria (int PC, uint8_t *memoria)
{
    /*
        Preenche a memória a partir de um dado ponto com o valor zero

        Entradas:
            int PC: O valor a partir do qual a memória será preenchido com zeros
            uint8_t* memoria: A memória a ser preenchida com zeros
    */
    for (int i = PC; i < 4095; i++)
    {
        armazenaNaMemoria(i, 0, memoria);
    }
}

void printBits(int64_t n)
{
    /*
        Imprime os últimos 40 bits de um valor int64_t como uma string de 0's e 1's
        Entradas:
            int64_t n: Um valor de entrada a ser impresso
    */

    char str[41];
    for (int i = 39; i >= 0; i--)
    {
        if (n % 2 == 0)
        {
            str[i] = '0';
        }
        else
        {
            str[i] = '1';
        }
        n = n/2;
    }
    str[40] = '\0';
    printf("%s", str);
}

void printMemoria(uint8_t* memoria)
{
    /*
        Imprime o estado atual da memória na saída padrão
        Entrada:
            uint64_t* memoria: A memória simulada que será impressa
    */
   
    int64_t palavra;

    for (int i = 0; i < 4095; i++)
    {
        palavra = buscaNaMemoria(memoria, i);
        printf("%d: ", i);
        printBits(palavra);
    }
}

char *cria_nome_saida(char *nome_entrada)
{
    /*
        Cria o nome do arquivo de saída a partir do nome do arquivo de entrada
        Entrada:
            char* nome_entrada: O nome do arquivo de instruções passados pelo usuário
        Saída:
            Um ponteiro para uma string que representa o nome de entrada acrescido de ".out"
    */
    int tamanho = strlen(nome_entrada);
    // printf("tamanho: %i\n", tamanho);
    
    int novo_tamanho = tamanho+4;
    char *nome_saida = malloc(novo_tamanho); //tamanho do nome de entrada mais 4
    for (int i=0; i<tamanho; i++) nome_saida[i] = nome_entrada[i];
    nome_saida[tamanho] = '.';
    nome_saida[tamanho+1] = 'o';
    nome_saida[tamanho+2] = 'u';
    nome_saida[tamanho+3] = 't';
    nome_saida[tamanho+4] = '\0';
    return nome_saida;
}

void tratamento_string(char *linha)
{
    /*
        Como a leitura da linha pode conter conteúdo indesejado, como espaços no fim ou um \n, 
        é necessário fazer um tratamento para deixá-la em um formato padronizado de processamento.
        Entrada: char *linha - string lida em uma linha do arquivo de instruções
    */    
    int tamanho = strlen(linha);
    int i = 0;
    while ((linha[i] != ' ') && (i < tamanho)) i++; // percorre o vetor da linha até encontrar um espaço ou chegar no fim
    if (i == tamanho) linha[i-1] = '\0'; // se chegou no fim, então não houve espaço e o procedimento é apenas colocar o \0 no lugar do \n ou \r
    else linha[i] = '\0'; // se não chegou no fim, então houve espaço, portanto deve-se colocar o \0 na posição do espaço, finalizando ali a string
}