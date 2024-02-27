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

int ciclosPorInstrucao[] = {
    1, // LOAD_MQ,
    1, // LOAD_MQ_MX,
    1, // STOR_MX,
    1, // LOAD_MX,
    1, // LOAD_MenosMX,
    1, // LOAD_ABSMX,
    1, // LOAD_MenosABSMX,
    1, // JUMP_ESQ,
    1, // JUMP_DIR,
    1, // JUMPMais_ESQ,
    1, // JUMPMais_DIR,
    1, // ADD_MX,
    1, // ADD_ABSMX,
    1, // SUB_MX,
    1, // SUB_ABSMX,
    1, // MUL_MX,
    1, // DIV_MX,
    1, // LSH,
    1, // RSH,
    1, // STOR_MX_ESQ,
    1, // STOR_MX_DIR,
    1, // EXIT,
    1  // NENHUMA
};

BR bancoRegistradores;
ULA unidadeLogicaAritmetica;
BarramentoMemoria barramento;

// Contadores de clock para que seja simulado o pipeline
int contadorClockExecucao = 1;

//                                   B      D      Bo     Ex     Er
booleano flagEstagioCongelado[] = {False, False, False, False, False};

// Vira true toda vez que é feito uma escrita
booleano dependenciaRAW = False;

// Indica o endereço que foi feita a escrita
int enderecoRAW = 0;

Lado ladoInstrucao = Esquerdo;

void pipelineBusca()
{
    // Lê o lugar de memória dito por PC (lembrando que pode ser esquerdo ou direito tbm)
    // e guarda o OpCode + dado em um lugar [A]

    if (flagEstagioCongelado[0] == True)
    {
        return;
    }

    barramento.endereco = bancoRegistradores.PC;
    barramento.operacao = ler;
    executarBarramento();

    bancoRegistradores.MBR = barramento.saida;
    
    uint64_t ladoEsquerdo;
    uint64_t ladoDireito;

    if (ladoInstrucao == Esquerdo)
    {
        bancoRegistradores.IBR = ladoDireito;
        // divide lado esquerdo entre IR (opcode) e MAR (dado)
        ladoInstrucao = Direito;
    }
    else 
    {
        // divide o lado direito entre IR e IBR
        ladoInstrucao = Esquerdo;
    }
}

uint64_t resultadoBusca; 

void pipelineDecodificacao()
{
    if (flagEstagioCongelado[1] == True)
    {
        return;
    }
    uint64_t opcode;
    uint64_t endereco;

    // Opcode <- Opcode de (resultadobusca)
    // endereco <- Dado de (resultado busca)

    opcodeDecodificado = opCodeParaInstrucao(opcode);
    enderecoDecodificado = endereco;
}

Instrucao opcodeDecodificado;
uint64_t enderecoDecodificado;


void pipelineBuscaOperandos()
{
    if (flagEstagioCongelado[2] == True)
    {
        return;
    }

    if (dependenciaRAW == True)
    {
        if (enderecoDecodificado == enderecoRAW)
        {
            // Inserindo bolha
            opcodeDecodificado = NENHUMA;
            flagEstagioCongelado[0] = True;
            flagEstagioCongelado[1] = True;
            // return;
        }
    }

    switch (opcodeDecodificado)
    {
        case STOR_MX:
        case STOR_MX_DIR:
        case STOR_MX_ESQ:
            dadoParaExecucao = enderecoDecodificado;
            break;
        default:
            barramento.endereco = enderecoDecodificado;
            barramento.operacao = ler;
            executarBarramento();
            dadoParaExecucao = barramento.saida;
            break;
    }

    operacaoASerExecutada = opcodeDecodificado;
}

// B | D | Bo | Ex | Er

booleano flagPegarNovoContador = False;

// Instrução que será executada
Instrucao operacaoASerExecutada;

// Dado para ser executado
// Em operações LOAD e STOR, representa o endereço de memória a ser acessado
uint64_t dadoParaExecucao;

void pipelineExecucao() 
{
    // Se a instrução antiga acabou, pegue a quantidade de ciclos de clock para a instrução atual
    if (flagPegarNovoContador == True)
    {
        contadorClockExecucao = ciclosPorInstrucao[operacaoASerExecutada];
        
        switch (operacaoASerExecutada)
        {
        case STOR_MX:
        case STOR_MX_DIR:
        case STOR_MX_ESQ:
            dependenciaRAW = True;
            enderecoRAW = dadoParaExecucao;
            break;
        default:
            dependenciaRAW = False;
            break;
        }
        flagPegarNovoContador = False;
    }
    // Se a instrução precisa esperar ser feita
    if (contadorClockExecucao > 1)
    {
        contadorClockExecucao -= 1;

        // Congelando o pipeline
        flagEstagioCongelado[0] = True;
        flagEstagioCongelado[1] = True;
        flagEstagioCongelado[2] = True;
        flagEstagioCongelado[4] = True;

        return;
    }

    instrucao = operacaoASerExecutada;
    flagPegarNovoContador = True;

    // Liberando o pipeline
    flagEstagioCongelado[4] = False;

    switch (operacaoASerExecutada)
    {
    case ADD_MX:
        bancoRegistradores.AC += dadoParaExecucao;
        break;
    case ADD_ABSMX:
        bancoRegistradores.AC += abs(dadoParaExecucao);
        break;
    case SUB_MX:
        bancoRegistradores.AC -= dadoParaExecucao;
        break;
    case SUB_ABSMX:
        bancoRegistradores.AC -= abs(dadoParaExecucao);
        break;
    case JUMPMais_DIR:
        if (bancoRegistradores.AC < 0)
        {
            break;
        }
    case JUMP_DIR:
        // talvez isso deva ir para a ER?
        // PC <- (int) dado para execucao
        // limpar pipeline
        // lado = dir
        break;
    case JUMPMais_ESQ:
        if (bancoRegistradores.AC < 0)
        {
            break;
        }
    case JUMP_ESQ:
        bancoRegistradores.PC = (int) dadoParaExecucao;
        limparPipeline();
        break;
    case RSH:
        resultado = bancoRegistradores.AC / 2;
        break;
    case LSH:
        resultado = bancoRegistradores.AC * 2;
        break;
    case DIV_MX:
        uint64_t res = bancoRegistradores.AC / dadoParaExecucao;
        resultado = bancoRegistradores.AC % dadoParaExecucao;
        resultado_auxiliar = res;
        break;
    case MUL_MX:
        uint64_t res = dadoParaExecucao * bancoRegistradores.MQ;
        uint64_t res1 = res & 0b111111111111111111111111111111111111111;
        res = res >> 39;
        resultado = res; 
        resultado_auxiliar = res1;
        break;
    case LOAD_ABSMX:
        resultado = abs(dadoParaExecucao);
        break;
    case LOAD_MenosABSMX:
        resultado = -1 * abs(dadoParaExecucao);
        break;
    case LOAD_MenosMX:
        resultado = -1 * dadoParaExecucao;
        break;
    case LOAD_MQ_MX:
    case LOAD_MX:
        resultado = dadoParaExecucao;
        break;
    case STOR_MX:
        barramento.endereco = dadoParaExecucao;
        barramento.operacao = escrever;
        barramento.entrada = bancoRegistradores.AC;
        executarBarramento();
        break;
        resultado = dadoParaExecucao;
        break;
    case STOR_MX_ESQ:
        barramento.operacao = ler;
        barramento.endereco = dadoParaExecucao;
        executarBarramento();
        uint64_t dado = barramento.saida;

        dado = dado & 0b1111111100000000000011111111111111111111;

        uint64_t novaParte = bancoRegistradores.AC;

        novaParte = novaParte << 20;

        dado = dado | novaParte;

        barramento.operacao = escrever;
        barramento.endereco = dadoParaExecucao;
        barramento.entrada = dado;
        executarBarramento();
        break;
    case STOR_MX_DIR:
        barramento.operacao = ler;
        barramento.endereco = dadoParaExecucao;
        executarBarramento();
        uint64_t dado = barramento.saida; 

        dado = dado & 0b1111111111111111111111111111000000000000;

        uint64_t novaParte = bancoRegistradores.AC;

        dado = dado | novaParte;
        barramento.operacao = escrever;
        barramento.endereco = dadoParaExecucao;
        barramento.entrada = dado;
        executarBarramento();
    case EXIT:
        flagTerminou = True;
        break;
    case NENHUMA:
    case LOAD_MQ:
    default:
        break;
    }
}  

uint64_t resultado;
uint64_t resultado_auxiliar;
Instrucao instrucao;

void pipelineEscritaResultados()
{
    // Se o dado anterior não estiver pronto, saia
    if (flagEstagioCongelado[4] == True)
    {
        return;
    }

    flagEstagioCongelado[0] = False;
    flagEstagioCongelado[1] = False;
    flagEstagioCongelado[2] = False;

    switch (instrucao)
    {
        // Essa instrução faz AC <- MQ
        case LOAD_MQ:
            bancoRegistradores.AC = bancoRegistradores.MQ;
            break;
        // Essas instruções fazem MQ <- res
        case LOAD_MQ_MX:
        case ADD_MX:
        case ADD_ABSMX:
        case SUB_MX:
        case SUB_ABSMX:
            bancoRegistradores.MQ = resultado;
            break;
        // Essas instruções fazem MQ <- res_aux e AC <- res
        case MUL_MX:
        case DIV_MX:
            bancoRegistradores.AC = resultado;
            bancoRegistradores.MQ = resultado_auxiliar;
            break;
        // Essas instruções fazem AC <- res
        case LOAD_MX:
        case LOAD_MenosMX:
        case LOAD_ABSMX:
        case LOAD_MenosABSMX:
        case LSH:
        case RSH:
            bancoRegistradores.AC = resultado;
            break;
        // Essas instruções não fazem nada
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
        pipelineEscritaResultados();
        pipelineExecucao();
        pipelineBuscaOperandos();
        pipelineDecodificacao();
        pipelineBusca();
    }
}

void limparPipeline()
{
    resultado = 0;
    resultado_auxiliar = 0;
    instrucao = NENHUMA
    
    dadoParaExecucao = 0;
    operacaoASerExecutada = NENHUMA;

    flagPegarNovoContador = True;

    opcodeDecodificado = NENHUMA;
    enderecoDecodificado = 0;

    resultadoBusca = 0;

    flagEstagioCongelado[0] = False;
    flagEstagioCongelado[1] = False;
    flagEstagioCongelado[2] = False;
    flagEstagioCongelado[3] = False;
    flagEstagioCongelado[4] = False;
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

    dumpDaMemoria(memoria, argv[2]);
        
    free(memoria);
    fclose(arquivoEntrada);
    return 0;
    
}
