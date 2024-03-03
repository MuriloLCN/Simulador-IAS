#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "ias.h"

#define TAMANHO_MEMORIA 4096

uint8_t* memoria;

void executarBarramento();
void executarUla();
void limparPipeline();

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
  LOAD_MQ, // 0   
  LOAD_MQ_MX,
  STOR_MX,
  LOAD_MX,
  LOAD_MenosMX,
  LOAD_ABSMX, // 5
  LOAD_MenosABSMX,
  JUMP_ESQ,
  JUMP_DIR,
  JUMPMais_ESQ,
  JUMPMais_DIR, // 10
  ADD_MX,
  ADD_ABSMX,
  SUB_MX,
  SUB_ABSMX,
  MUL_MX, // 15
  DIV_MX,
  LSH,
  RSH,
  STOR_MX_ESQ,
  STOR_MX_DIR, // 20
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
        return NENHUMA;
        break;
    }
}

int* ciclosPorInstrucao;
// int ciclosPorInstrucao[] = {
//     1, // LOAD_MQ,
//     1, // LOAD_MQ_MX,
//     1, // STOR_MX,
//     1, // LOAD_MX,
//     1, // LOAD_MenosMX,
//     1, // LOAD_ABSMX,
//     1, // LOAD_MenosABSMX,
//     1, // JUMP_ESQ,
//     1, // JUMP_DIR,
//     1, // JUMPMais_ESQ,
//     1, // JUMPMais_DIR,
//     1, // ADD_MX,
//     1, // ADD_ABSMX,
//     1, // SUB_MX,
//     1, // SUB_ABSMX,
//     1, // MUL_MX,
//     1, // DIV_MX,
//     1, // LSH,
//     1, // RSH,
//     1, // STOR_MX_ESQ,
//     1, // STOR_MX_DIR,
//     1, // EXIT,
//     1  // NENHUMA
// };

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

uint64_t resultadoBusca = 0; // Resultado da fase de busca

// Resultados da fase de decodificação
Instrucao opcodeDecodificado = NENHUMA;
uint64_t enderecoDecodificado = 0;

// Resultados da fase da busca de operandos
Instrucao operacaoASerExecutada = NENHUMA;
uint64_t dadoParaExecucao = 0; // Em operações LOAD e STOR, representa o endereço de memória a ser acessado

// Resultados da fase de execução
uint64_t resultado = 0;
uint64_t resultado_auxiliar = 0;
Instrucao instrucao = NENHUMA;

booleano flagTerminou = False;
booleano flagCongelarTudo = False;

void pipelineBusca()
{
    // Lê o lugar de memória dito por PC (lembrando que pode ser esquerdo ou direito tbm)
    // e guarda o OpCode + dado em um lugar [A]

    if (flagEstagioCongelado[0] == True)
    {
        //printf("\nBusca congelada!");
        return;
    }

    //printf("\nExecutando busca");
    barramento.endereco = bancoRegistradores.PC;
    barramento.operacao = ler;
    executarBarramento();

    uint64_t enderecoBuscado = barramento.saida;


    uint64_t ladoEsquerdo = (enderecoBuscado & 0b1111111111111111111100000000000000000000) >> 20;
    uint64_t ladoDireito = enderecoBuscado & 0b11111111111111111111;

    if (ladoInstrucao == Esquerdo)
    {
        //printf("\nlado esquerdo");
        // bancoRegistradores.IBR = ladoDireito;
        
        resultadoBusca = ladoEsquerdo;
        ladoInstrucao = Direito;
    }
    else 
    {
        //printf("\nlado direito");
        ladoInstrucao = Esquerdo;
        resultadoBusca = ladoDireito;
        bancoRegistradores.PC =  bancoRegistradores.PC + 1;
    }


    if (flagCongelarTudo == True)
    {
        flagEstagioCongelado[0] = True;
        flagEstagioCongelado[1] = True;
        flagEstagioCongelado[2] = True;
        flagEstagioCongelado[4] = True;
        flagCongelarTudo = False;
    }
}

void pipelineDecodificacao()
{
    if (flagEstagioCongelado[1] == True)
    {
        //printf("\nDecod. congelada!");
        return;
    }
    uint64_t opcode;
    uint64_t endereco;

    opcode = (resultadoBusca & 0b11111111000000000000) >> 12;

    endereco = resultadoBusca & 0b111111111111;

    opcodeDecodificado = opCodeParaInstrucao(opcode);
    enderecoDecodificado = endereco;
}

void pipelineBuscaOperandos()
{
    if (flagEstagioCongelado[2] == True)
    {
        //printf("\nBusca op. congelada!");
        return;
    }

    if (dependenciaRAW == True)
    {
        if (enderecoDecodificado == enderecoRAW)
        {
            printf("\nDEPENDENCIA RAW!!! INSERINDO BOLHA");
            // Inserindo bolha
            operacaoASerExecutada = NENHUMA;
            flagEstagioCongelado[0] = True;
            flagEstagioCongelado[1] = True;
            dependenciaRAW = False;
            return;
        }
    }

    switch (opcodeDecodificado)
    {
        case NENHUMA:
            //printf("\nOpcode era nenhum");
            // dadoParaExecucao = 0;
            break;
        case STOR_MX:
        case STOR_MX_DIR:
        case STOR_MX_ESQ:
            //printf("\nOpcode era de um stor");
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


booleano flagPegarNovoContador = False;


void pipelineExecucao() 
{
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
        flagCongelarTudo = True;
    }
    // Se a instrução precisa esperar ser feita
    if (contadorClockExecucao > 1)
    {
        contadorClockExecucao -= 1;

        // Congelando o pipeline após executar uma vez
        //flagEstagioCongelado[0] = True;
        //flagEstagioCongelado[1] = True;
        //flagEstagioCongelado[2] = True;
        //flagEstagioCongelado[4] = True;
        return;
    }

    instrucao = operacaoASerExecutada;
    flagPegarNovoContador = True;

    // Liberando o pipeline
    //printf("\nLiberando a escrita de resultados");
    flagEstagioCongelado[4] = False;

    switch (operacaoASerExecutada)
    {
    case ADD_MX:
        // printf("\nEX: Soma");
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.entrada2 = dadoParaExecucao;
        unidadeLogicaAritmetica.operacao = soma;
        executarUla();
        resultado = unidadeLogicaAritmetica.saida;
        break;
    case ADD_ABSMX:
        // printf("\nEX: Soma abs");
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.entrada2 = abs(dadoParaExecucao);
        unidadeLogicaAritmetica.operacao = soma;
        executarUla();
        resultado = unidadeLogicaAritmetica.saida;
        break;
    case SUB_MX:
        // printf("\nEX: SUB");
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.entrada2 = dadoParaExecucao;
        unidadeLogicaAritmetica.operacao = subtracao;
        executarUla();
        resultado = unidadeLogicaAritmetica.saida;
        break;
    case SUB_ABSMX:
        // printf("\nEX: SUB ABS");
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.entrada2 = abs(dadoParaExecucao);
        unidadeLogicaAritmetica.operacao = soma;
        executarUla();
        resultado = unidadeLogicaAritmetica.saida;
        break;
    case JUMPMais_DIR:
        // printf("\nEX: JUMP+ DIR");
        if (bancoRegistradores.AC < 0)
        {
            break;
        }
    case JUMP_DIR:
        // printf("\nEX: JUMP DIR");
        bancoRegistradores.PC = (int) dadoParaExecucao;
        limparPipeline();
        ladoInstrucao = Direito;
        instrucao = NENHUMA;
        // printf("\n\nJUMP FEITO\n\n");
        break;
    case JUMPMais_ESQ:
        // printf("\nEX: JUMP+ ESQ");
        if (bancoRegistradores.AC < 0)
        {
            break;
        }
    case JUMP_ESQ:
        // printf("\nEX: JUMP ESQ");
        bancoRegistradores.PC = (int) dadoParaExecucao;
        limparPipeline();
        ladoInstrucao = Esquerdo;
        instrucao = NENHUMA;
        // printf("\n\nJUMP FEITO\n\n");
        break;
    case RSH:
        // printf("\nEX: RSH");
        resultado = bancoRegistradores.AC / 2;
        break;
    case LSH:
        // printf("\nEX: LSH");
        resultado = bancoRegistradores.AC * 2;
        break;
    case DIV_MX:
        // printf("\nEX: DIV");
        resultado = bancoRegistradores.AC % dadoParaExecucao;
        resultado_auxiliar = bancoRegistradores.AC / dadoParaExecucao;
        break;
    case MUL_MX:
        // printf("\nEX: MUL");
        resultado = (dadoParaExecucao * bancoRegistradores.MQ) >> 39;
        resultado_auxiliar = (dadoParaExecucao * bancoRegistradores.MQ) & 0b111111111111111111111111111111111111111;
        break;
    case LOAD_ABSMX:
        // printf("\nEX: LOAD ABS");
        resultado = abs(dadoParaExecucao);
        break;
    case LOAD_MenosABSMX:
        // printf("\nEX: LOD -ABS");
        resultado = -1 * abs(dadoParaExecucao);
        break;
    case LOAD_MenosMX:
        // printf("\nEX: LOAD MENOS");
        resultado = -1 * dadoParaExecucao;
        break;
    case LOAD_MQ_MX:
    case LOAD_MX:
        // printf("\nEX: LOAD OU LOAD MQ");
        resultado = dadoParaExecucao;
        break;
    case STOR_MX:
        // printf("\nEX: STOR");
        resultado = dadoParaExecucao;
        break;
    case STOR_MX_ESQ:
        // printf("\nEX: STOR ESQ");
        barramento.operacao = ler;
        barramento.endereco = dadoParaExecucao;
        executarBarramento();

        resultado = (barramento.saida & 0b1111111100000000000011111111111111111111);
        resultado_auxiliar = dadoParaExecucao;
        // barramento.operacao = escrever;
        // barramento.endereco = dadoParaExecucao;
        // barramento.entrada = (barramento.saida & 0b1111111100000000000011111111111111111111)| (bancoRegistradores.AC << 20);
        // executarBarramento();
        break;
    case STOR_MX_DIR:
        // printf("\nEX: STOR DIR");
        barramento.operacao = ler;
        barramento.endereco = dadoParaExecucao;
        executarBarramento();

        resultado = (barramento.saida & 0b1111111111111111111111111111000000000000);
        resultado_auxiliar = dadoParaExecucao;
        // barramento.operacao = escrever;
        // barramento.endereco = dadoParaExecucao;
        // barramento.entrada = (barramento.saida & 0b1111111111111111111111111111000000000000) | bancoRegistradores.AC;
        // executarBarramento();
    case EXIT:
        // printf("\nEX: EXIT");
        flagTerminou = True;
        break;
    case NENHUMA:
    case LOAD_MQ:
    default:
        // printf("\nEXEC: Nada feito");
        break;
    }

    printf("\nOperacao feita: %d   resultado: %d   resultado_aux: %d", instrucao, resultado, resultado_auxiliar);
}  

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
    flagEstagioCongelado[3] = False;

    switch (instrucao)
    {
        // Essa instrução faz AC <- MQ
        case LOAD_MQ:
            bancoRegistradores.AC = bancoRegistradores.MQ;
            break;
        // Essas instruções fazem MQ <- res
        case LOAD_MQ_MX:
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
        case ADD_MX:
        case ADD_ABSMX:
        case SUB_MX:
        case SUB_ABSMX:
        case LSH:
        case RSH:
            bancoRegistradores.AC = resultado;
            break;
        case STOR_MX:
            barramento.endereco = resultado;
            barramento.operacao = escrever;
            barramento.entrada = bancoRegistradores.AC;
            executarBarramento();
            break;    
        case STOR_MX_DIR:
            barramento.operacao = escrever;
            barramento.endereco = resultado_auxiliar;
            barramento.entrada = resultado | bancoRegistradores.AC;
            executarBarramento();
            break;
        case STOR_MX_ESQ:
            barramento.operacao = escrever;
            barramento.endereco = resultado_auxiliar;
            barramento.entrada = resultado | (bancoRegistradores.AC << 20);
            executarBarramento();
            break;
        case JUMP_DIR:
        case JUMP_ESQ:
        case JUMPMais_DIR:
        case JUMPMais_ESQ:
        case NENHUMA:
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
    
    while (flagTerminou != True)
    {
        //printf("\n--------------------------------");
        //printf("\nFazendo a escrita de resultados");
        //printf("\nEntradas: %d %d %d", resultado, resultado_auxiliar, instrucao);
        pipelineEscritaResultados();
        ///printf("\n----------------");
        ///printf("\nFazendo a execucao");
        ///printf("\nEntradas: %d %d", operacaoASerExecutada, dadoParaExecucao);
        pipelineExecucao();
        ///printf("\nSaidas: %d %d %d", resultado, resultado_auxiliar, instrucao);
        ///printf("\n----------------");
        ///printf("\nFazendo a busca de operandos");
        //printf("\nEntradas: %d %d", opcodeDecodificado, enderecoDecodificado);
        pipelineBuscaOperandos();
        ///printf("\nSaidas: %d %d", operacaoASerExecutada, dadoParaExecucao);
        ///printf("\n----------------");
        ///printf("\nFazendo a decodificacao");
        //printf("\nEntrada: %d", resultadoBusca);
        pipelineDecodificacao();
        ///printf("\nSaidas: %d %d", opcodeDecodificado, enderecoDecodificado);
        ///printf("\n----------------");
        ///printf("\nFazendo a busca, PC = %d", bancoRegistradores.PC);
        //printf("\nSaida: %d", resultadoBusca);
        pipelineBusca();

        // printf("\nAC: %d  MQ: %d", bancoRegistradores.AC, bancoRegistradores.MQ);
    }
}

void limparPipeline()
{
    resultado = 0;
    resultado_auxiliar = 0;
    instrucao = NENHUMA;
    
    dadoParaExecucao = 0;
    operacaoASerExecutada = NENHUMA;

    flagPegarNovoContador = False;

    opcodeDecodificado = NENHUMA;
    enderecoDecodificado = 0;

    resultadoBusca = 0;

    flagEstagioCongelado[0] = False;
    flagEstagioCongelado[1] = False;
    flagEstagioCongelado[2] = False;
    flagEstagioCongelado[3] = False;
    flagEstagioCongelado[4] = False;

    flagCongelarTudo = False;
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

    ciclosPorInstrucao = (int*)malloc(sizeof(int) * 23);

    for (int i = 0; i < 23; i++)
    {
        ciclosPorInstrucao[i] = 1;
    }

    bancoRegistradores.AC = 0;
    bancoRegistradores.MQ = 0;
    bancoRegistradores.MBR = 0;
    bancoRegistradores.PC = 4;
    bancoRegistradores.MAR = 0;
    bancoRegistradores.IBR = 0;
    bancoRegistradores.IR = 0;

    dadoParaExecucao = 0;

    unidadeLogicaAritmetica.entrada1 = 0;
    unidadeLogicaAritmetica.entrada2 = 0;
    unidadeLogicaAritmetica.operacao = soma;
    unidadeLogicaAritmetica.saida = 0;

    barramento.operacao = ler;
    barramento.entrada = 0;
    barramento.endereco = 0;
    barramento.saida = 0;
    
    carregarMemoria(arquivoEntrada, &memoria, &ciclosPorInstrucao);

    simulacao();   

    char *novo_nome = cria_nome_saida(argv[2]);

    dumpDaMemoria(memoria, novo_nome);
        
    free(memoria);
    free(ciclosPorInstrucao);
    fclose(arquivoEntrada);
    return 0;
    
}
