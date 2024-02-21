#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "ias.h"

#define TAMANHO_MEMORIA 4096

uint8_t* memoria;

typedef enum {False, True} booleano;

typedef enum {
    soma,
    subtracao,
    multiplicacao,
    divisao,
    shiftParaEsquerda,
    shiftParaDireita
} OperacaoULA;

typedef enum {
    ler,
    escrever
} OperacaoBarramento;

typedef struct {
    int endereco;
    OperacaoBarramento operacao;
    uint64_t entrada;
    uint64_t saida;
} BarramentoMemoria;

typedef struct {
    uint64_t entrada1;
    uint64_t entrada2;
    OperacaoULA operacao;
    uint64_t saida;
} ULA;

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

typedef enum {
    Esquerdo,
    Direito
} Lado;

typedef struct {
    uint64_t AC;
    uint64_t MQ;
    uint64_t MBR;
    uint64_t PC;
    uint64_t MAR;
    uint64_t IBR;
    uint64_t IR;
} BR;

Instrucao opCodeParaInstrucao(uint64_t opCode)
{
    /*
        Converte um OpCode para a sua instrução no enum de acordo com a tabela abaixo

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
    
    switch (opCode)
    {
    case 0b00001010:
        return LOAD_MQ;
        break;
    case 0b00001001:
        return LOAD_MQ_MX;
        break;
    case 0b00100001:
        return STOR_MX;
        break;
    case 0b00000001:
        return LOAD_MX;
        break;
    case 0b00000010:
        return LOAD_MenosMX;
        break;
    case 0b00000011:
        return LOAD_ABSMX;
        break;
    case 0b00000100:
        return LOAD_MenosABSMX;
        break;
    case 0b00001101:
        return JUMP_ESQ;
        break;
    case 0b00001110:
        return JUMP_DIR;
        break;
    case 0b00001111:
        return JUMPMais_ESQ;
        break;
    case 0b00010000:
        return JUMPMais_DIR;
        break;
    case 0b00000101:
        return ADD_MX;
        break;
    case 0b00000111:
        return ADD_ABSMX;
        break;
    case 0b00000110:
        return SUB_MX;
        break;
    case 0b00001000:
        return SUB_ABSMX;
        break;
    case 0b00001011:
        return MUL_MX;
        break;
    case 0b00001100:
        return DIV_MX;
        break;
    case 0b00010100:
        return LSH;
        break;
    case 0b00010101:
        return RSH;
        break;
    case 0b00010010:
        return STOR_MX_ESQ;
        break;
    case 0b00010011:
        return STOR_MX_DIR;
        break;
    case 0b11111111:
        return EXIT;
        break;
    default:
        return EXIT;
        break;
    }
}


BR bancoRegistradores;
ULA unidadeLogicaAritmetica;
BarramentoMemoria barramento;

// Contadores de clock para que seja simulado o pipeline
//int contadorClockEscritaResultado = 1;
int contadorClockExecucao = 1;
// int contadorBuscaOperandos = 1;
// int contadorDecodificacao = 1;
// int contadorBusca = 1;

// Indicam se os estágios podem receber os dados anteriores
booleano flagPipelineVazio[] = {True, True, True, True, True};

// Indicam se os dados anteriores podem ser lidos e executados
booleano flagBuscarDadoAnterior[] = {True, True, True, True};

// Vira true toda vez que é feito uma escrita
booleano dependenciaRAW = False;

// Indica o endereço que foi feita a escrita
int enderecoRAW = 0;

// Assume que a gente criou algumas variáveis [A],[B],...,[G], e também algumas variáveis que indicam
// se o estágio tá congelado ou não

// Antes de realizar cada etapa, primeiro faz o seguinte:
// se o estágio tá congelado, não faz nada
// se não está congelado:
// === se não for execução:
// ====== roda o estágio e continua
// === se for execução:
// ====== vê se o contador chegou a 1:
// ========= se sim, executa a instrução e descongela
// ========= se não, diminui 1 do contador e congela todos os anteriores

// Tem que lembrar que, depois que a execução terminar, os estágios anteriores continuam congelados por mais um
// ciclo, pq tem que escrever os resultados primeiro (dependência RAW)

Lado ladoInstrucao = Esquerdo;

void pipelineBusca()
{
    // Lê o lugar de memória dito por PC (lembrando que pode ser esquerdo ou direito tbm)
    // e guarda o OpCode + dado em um lugar [A]


    // OBS: O MBR, IR e IBR aqui são só para decoração. O IAS não funciona com pipeline.
    // a gente usa ladoInstrucao e busca toda vez. Isso é só pra fazer parecer que a gente tá fazendo algo.

    // Busca o endereço de PC na memória

    // se  lado==esquerdo:
    //     salva o lado direito em MBR
    //     divide o lado esquerdo em IR e IBR
    //     marca flag como "direito"
    // senão:
    //     salva o lado direito em IR e IBR
    //     marca flag como "esquerdo"
}

uint64_t resultadoBusca;  // [A]

void pipelineDecodificacao()
{
    // Opcode <- Opcode de (resultadobusca)
    // endereco <- Dado de (resultado busca)
    // enum operacao op <- opCodeParaInstrucao(Opcode)
    
    // [B] <- op
    // [C] <- endereco

    // Lê o opcode e o dado do lugar [A]
    // Armazena a instrução (já em forma enum) em [B]
    // Armazena o endereço do operando em [C]
}

void pipelineBuscaOperandos()
{
    // Lê o endereço do operando de [C] e busca ele na memória
    // Copia a instrução de [B] para [D]
    // Armazena o dado em [E]

    // se for uma operacao normal
    //  dado <- buscaNaMemoria([C])
    // se for um stor
    //  dado <- [C]
    // [D] <- [B]
    // [E] <- Dado
    // 
}

booleano flagPegarNovoContador = False;

void pipelineExecucao()
{
    if (flagPegarNovoContador == True)
    {
        // contador clock = quantidade de ciclos de clock daquela instrucao
        flagPegarNovoContador = False;
    }
    if (contadorClockExecucao > 1)
    {
        contadorClockExecucao -= 1;
        // Para que a escrita de resultados não escreva dados antigos
        flagBuscarDadoAnterior[3] = False;
        return;
    }

    flagPegarNovoContador = True;
    // Lê o enum de [D] e o dado de [E], e faz a execução dependendo do tipo de instrução
    // Armazena o resultado em [F]
    // Armazena a instrução em [G]

    // resultado
    // switch [D]
    //    faz cada tipo de operacao e arnazena no lugar correspondente

    // load MQ : Nada
    // load MQ,MX : Resultado <- [MX]
    // stor MX: Guarda resultado na memória
    // load M(X) (e variacoes): Resultado <- [MX]

    // jump:
    //      pc <- valor
    //      lado <- direito ou esquerdo
    //      limpar o pipeline

    // jump+:
    //      se o pulo for tomado:
    //          faz que nem um jump normal
    //      se não foi tomado, continua

    // add e sub:
    //      resultado <- resultado da operacao

    // mul e div:
    //      colocar partes dos dados de AC em resultado
    //      colocar partes dos dados de MQ em resultado_auxiliar
    
    // lsh e rsh:
    //      resultado <- resultado da operacao
}  

uint64_t resultado;
uint64_t resultado_auxiliar;
Instrucao instrucao;

void pipelineEscritaResultados()
{
    // Se o dado anterior não estiver pronto, saia
    if (flagBuscarDadoAnterior[3] == False)
    {
        return;
    }

    switch (instrucao)
    {
        case LOAD_MQ:
            bancoRegistradores.AC = bancoRegistradores.MQ;
        case LOAD_MQ_MX:
        case ADD_MX:
        case ADD_ABSMX:
        case SUB_MX:
        case SUB_ABSMX:
            bancoRegistradores.MQ = resultado;
            break;
        case MUL_MX:
        case DIV_MX:
            bancoRegistradores.AC = resultado;
            bancoRegistradores.MQ = resultado_auxiliar;
        case LOAD_MX:
        case LOAD_MenosMX:
        case LOAD_ABSMX:
        case LOAD_MenosABSMX:
        case LSH:
        case RSH:
            bancoRegistradores.AC = resultado;

        case STOR_MX:
        case STOR_MX_DIR:
        case STOR_MX_ESQ:
        case JUMP_DIR:
        case JUMP_ESQ:
        case JUMPMais_DIR:
        case JUMPMais_ESQ:
        default:
            break;
        }

    // Como esse estágio leva só 1 ciclo de clock, não é necessário contador
    // Lê o dado a ser armazenado de [F] e a instrução de [G]
    // escreve o resultado na memória dependendo do tipo de instrução
    // STOR e LOAD meio que não precisam dessa parte, pq são feitos na execução
    // como outras instruções escrevem no AC ou no MQ, não precisamos dos endereços de memória
}

void executarBarramento()
{
    if (barramento.operacao == ler)
    {
        uint64_t res = buscaNaMemoria(memoria, barramento.endereco);
        barramento.saida = res;
    }
    else 
    {
        armazenaNaMemoria(barramento.endereco, barramento.entrada, memoria);
    }
}

void executarUla()
{
    /*
        Executa a operação atual da ULA com as suas entradas atuais e devolve o resultado em sua saída
    */
    switch (unidadeLogicaAritmetica.operacao)
    {
    case soma:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 + unidadeLogicaAritmetica.entrada2;
        break;
    case subtracao:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 - unidadeLogicaAritmetica.entrada2;
        break;
    case multiplicacao:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 * unidadeLogicaAritmetica.entrada2;
        break;
    case divisao:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 / unidadeLogicaAritmetica.entrada2;
        break;
    case shiftParaDireita:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 >> 1;
        break;
    case shiftParaEsquerda:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 << 1;
        break;
    default:
        break;
    }
}

void simulacao()
{
    // Ciclos de clock
    booleano flagTerminou = False;
    while (flagTerminou != True)
    {
        // Aqui é simulado o funcionamento do IAS clock-a-clock
    }
}

int main (int argc, char *argv[])
{
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-m"))
    {
        printf("\nParametros incorretos");
        // TODO: Colocar mensagem mais detalhada aqui
    }

    FILE* arquivoEntrada;
    memoria = (uint8_t *) malloc (4096*40);

    arquivoEntrada = fopen(argv[2], "r");

    if (arquivoEntrada == NULL)
    {
        printf("Falha ao abrir o arquivo de entrada!\n");
        return 1;
    }

    bancoRegistradores.AC = 0;
    bancoRegistradores.MQ = 0;
    bancoRegistradores.MBR = 0;
    bancoRegistradores.PC = 0;
    bancoRegistradores.MAR = 0;
    bancoRegistradores.IBR = 0;
    bancoRegistradores.IR = 0;

    unidadeLogicaAritmetica.entrada1 = 0;
    unidadeLogicaAritmetica.entrada2 = 0;
    unidadeLogicaAritmetica.operacao = soma;
    unidadeLogicaAritmetica.saida = 0;

    barramento.operacao = ler;
    barramento.entrada = 0;
    barramento.endereco = 0;
    barramento.saida = 0;
    
    carregarMemoria(arquivoEntrada, &memoria);

    simulacao();   

    dumpDaMemoria(memoria, argv[4]);
        
    free(memoria);
    fclose(arquivoEntrada);
    return 0;
    
}
