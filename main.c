#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

#include "parse_entrada.h"

#define TAMANHO_MEMORIA 4096

uint8_t* memoria;

// Protótipos de funções
void executarBarramento();
void executarUla();
void limparPipeline();

// Auxiliar na organização do código
typedef enum {False, True} booleano;

// Operações que a ULA realiza
typedef enum {
    soma,              // C = A + B
    somam,             // C = A + |B|
    subtracao,         // C = A - B
    subtracaom,        // C = A - |B|
    multiplicacao,     // C = A * B
    divisao,           // C = A / B
    shiftParaEsquerda, // C = A << 1
    shiftParaDireita   // C = A >> 1
} OperacaoULA;

// Operações que o barramento realiza
typedef enum {
    ler,
    escrever
} OperacaoBarramento;

// Barramento de memória
typedef struct {
    int endereco;
    OperacaoBarramento operacao;
    uint64_t entrada;
    uint64_t saida;
} BarramentoMemoria;

// Unidade Lógica Aritmética (ULA)
typedef struct {
    uint64_t entrada1;
    uint64_t entrada2;
    OperacaoULA operacao;
    uint64_t saida;
} ULA;

// Lados que podem ser lidos de memória
typedef enum {
    Esquerdo,
    Direito
} Lado;

// Unidade de controle (UC)

typedef struct {
    // Flags normais de controle
    booleano carry;
    booleano par;
    booleano zero;
    booleano sinal;

    // Flags extras de controle

    // Indicam quais estágios estão congelados
    booleano flagEstagioCongelado[5];

    // Indica se há uma possível dependência RAW
    booleano dependenciaRAW;
    int enderecoRAW;

    // Indica se há uma possível dependência STOR
    booleano dependenciaStorInstrucao;
    Lado dependenciaStorLado;
    int pcAlterado;

    // Lado atual para ler a instrução
    Lado ladoInstrucao;

    // Indica se a simulação terminou
    booleano flagTerminou;

    // Indica se tudo deve ser congelado no próximo ciclo
    booleano flagCongelarTudo;

    booleano dependenciaJump;

    // Flags extras para controle e tratamento de dependências
    booleano flagAnularBusca;
    booleano flagEvitarSobreescrita;

    // Flag que indica quando uma execução terminou após N ciclos de clock
    booleano flagPegarNovoContador;
} UC;

// Instruções que o IAS pode realizar
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


// Banco de registradores
typedef struct {
    uint64_t AC;
    uint64_t MQ;
    uint64_t MBR;
    uint64_t PC;
    uint64_t MAR;
    uint64_t IBR;
    uint64_t IR;
} BR;

// Banco de registradores intermediários (interfaces de comunicação entre os estágios do pipeline)
typedef struct {
    // Resultados da busca
    uint64_t resultadoBusca;

    // Resultados da decodificação
    Instrucao opcodeDecodificado;
    uint64_t enderecoDecodificado;

    // Resultados da busca de operandos
    Instrucao operacaoASerExecutada;
    uint64_t dadoParaExecucao;

    // Resultados da execução
    uint64_t resultado;
    uint64_t resultado_auxiliar;
    Instrucao instrucao;
} BRIntermediario;

Instrucao opCodeParaInstrucao(uint64_t opCode)
{
    /*
        Converte um opCode para a sua devida instrução (enum) de acordo com a tabela abaixo

        Entrada:
            uint64_t opCode: O opCode a ser convertido
        Saída:
            Instrucao i: A instrução que representa o opcode convertido

        Caso o opCode não bata com nenhuma instrução conhecida, retorna-se NENHUMA

        Tabela de opcodes:
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

// Vetor que armazena a quantidade de ciclos de clock que uma determinada instrução deve levar
int* ciclosPorInstrucao;

// Contador de clock (para que seja simulado o pipeline)
int contadorClockExecucao = 1;

// Declarações de componentes
BR bancoRegistradores;
ULA unidadeLogicaAritmetica;
BarramentoMemoria barramento;
BRIntermediario bancoIntermediario;
UC unidadeDeControle;

/*
    Dependências identificadas e corrigidas:

    1 - Dependência RAW

    Ocorre quando há uma operação que utiliza o resultado de uma escrita logo após uma modificação no endereço
    Exemplo:

    STOR M(5)
    LOAD M(5)

    2 - Dependência JUMP+

    Ocorre quando há uma operação de JUMP+ que necessita de uma operação ainda em andamento
    Exemplo:
    
    LOAD M(5)
    JUMP+ M(7,20:39)

    3 - Dependência STOR M(X, [Esq, Dir])
    
    Ocorre quando há uma operação de STOR em uma linha de instrução que será lida antes da operação de STOR ter sido concluída
    Ao contrário das demais dependências, é detectada e resolvida na decodificação e na busca, respectivamente
    Exemplo:

    Linha: Instrução
    5-0: LOAD M(0)
    5-1: STOR M(6,8:19)
    6-0: LOAD M(999)
*/

void pipelineBusca()
{
    /*
        Executa a fase de busca do pipeline

        ... Mais documentação vem aqui
    */
    
    // Caso o a busca esteja congelada, não faça nada
    if (unidadeDeControle.flagEstagioCongelado[0] == True)
    {
        return;
    }
    
    // Caso todo o pipeline deva ser congelado após a execução dessa fase
    if (unidadeDeControle.flagCongelarTudo == True)
    {
        unidadeDeControle.flagEstagioCongelado[0] = True;
        unidadeDeControle.flagEstagioCongelado[1] = True;
        unidadeDeControle.flagEstagioCongelado[2] = True;
        unidadeDeControle.flagEstagioCongelado[4] = True;
        unidadeDeControle.flagCongelarTudo = False;
    }

    // Caso deva ser inserida uma bolha na busca [Dependência STOR M(X, [Esq, Dir])]
    if (unidadeDeControle.flagAnularBusca == True)
    {
        bancoIntermediario.resultadoBusca = 0;    
        return;
    }

    // Identificando a dependência de STOR M(X, [Esq, Dir])
    if (unidadeDeControle.dependenciaStorInstrucao == True)
    {
        if (bancoRegistradores.PC == unidadeDeControle.pcAlterado && unidadeDeControle.ladoInstrucao == unidadeDeControle.dependenciaStorLado)
        {
            // Dependência STOR identificada
            unidadeDeControle.flagAnularBusca = True;
            unidadeDeControle.flagEvitarSobreescrita = True;
            bancoIntermediario.resultadoBusca = 0;
            return;
        }
    }
    
    // Buscando o dado de M(PC)
    // Barramento sempre lê uma linha inteira de memória
    barramento.endereco = bancoRegistradores.PC;
    barramento.operacao = ler;
    executarBarramento();

    //uint64_t enderecoBuscado = barramento.saida;
    bancoRegistradores.MBR = barramento.saida;

    uint64_t ladoEsquerdo = (bancoRegistradores.MBR & 0b1111111111111111111100000000000000000000) >> 20;
    uint64_t ladoDireito = bancoRegistradores.MBR & 0b11111111111111111111;
    
    if (unidadeDeControle.ladoInstrucao == Esquerdo)
    {
        // Fazer coisas do lado esquerdo aqui...
        bancoIntermediario.resultadoBusca = ladoEsquerdo;
        unidadeDeControle.ladoInstrucao = Direito;
    }
    else 
    {
        // Fazer coisas do lado direito aqui...
        unidadeDeControle.ladoInstrucao = Esquerdo;
        bancoIntermediario.resultadoBusca = ladoDireito;
        bancoRegistradores.PC =  bancoRegistradores.PC + 1;
    }
}

void pipelineDecodificacao()
{
    /*
        Executa a fase de decodificação do pipeline

        ... Mais documentação vem aqui
    */
    
    // Caso o estágio esteja congelado ou seja necessário evitar a sobreescrita por dependência STOR
    // Obs: A sobreescrita foi corrigida experimentalmente com essa flag
    if (unidadeDeControle.flagEstagioCongelado[1] == True || unidadeDeControle.flagEvitarSobreescrita == True)
    {
        return;
    }

    uint64_t opcode;
    uint64_t endereco;

    opcode = (bancoIntermediario.resultadoBusca & 0b11111111000000000000) >> 12;

    endereco = bancoIntermediario.resultadoBusca & 0b111111111111;

    bancoIntermediario.opcodeDecodificado = opCodeParaInstrucao(opcode);
    bancoIntermediario.enderecoDecodificado = endereco;

    // Marcando possível dependência STOR
    if (bancoIntermediario.opcodeDecodificado == STOR_MX_DIR)
    {
        unidadeDeControle.dependenciaStorInstrucao = True;
        unidadeDeControle.dependenciaStorLado = Direito;
        unidadeDeControle.pcAlterado = (int) bancoIntermediario.enderecoDecodificado;
    }
    if (bancoIntermediario.opcodeDecodificado == STOR_MX_ESQ)
    {
        unidadeDeControle.dependenciaStorInstrucao = True;
        unidadeDeControle.dependenciaStorLado = Esquerdo;
        unidadeDeControle.pcAlterado = (int) bancoIntermediario.enderecoDecodificado;
    }
}

void pipelineBuscaOperandos()
{
    /*
        Executa a fase de busca de operandos do pipeline
    */

    if (unidadeDeControle.flagEstagioCongelado[2] == True || unidadeDeControle.flagEvitarSobreescrita == True)
    {
        return;
    }

    // Identificando dependências RAW
    if (unidadeDeControle.dependenciaRAW == True)
    {
        if (bancoIntermediario.enderecoDecodificado == unidadeDeControle.enderecoRAW)
        {
            // Inserindo bolha
            bancoIntermediario.operacaoASerExecutada = NENHUMA;
            unidadeDeControle.flagEstagioCongelado[0] = True;
            unidadeDeControle.flagEstagioCongelado[1] = True;
            unidadeDeControle.dependenciaRAW = False;
            return;
        }
    }

    // Identificando dependências JUMP+
    if (unidadeDeControle.dependenciaJump == True)
    {
        switch (bancoIntermediario.opcodeDecodificado)
        {
        case JUMPMais_DIR:
        case JUMPMais_ESQ:
            // Inserindo bolha
            bancoIntermediario.operacaoASerExecutada = NENHUMA;
            unidadeDeControle.flagEstagioCongelado[0] = True;
            unidadeDeControle.flagEstagioCongelado[1] = True;
            unidadeDeControle.dependenciaJump = False;
            break;
        
        default:
            break;
        }
    }

    switch (bancoIntermediario.opcodeDecodificado)
    {
        // Casos em que o valor passado significa um endereço a ser usado
        case NENHUMA:
            break;
        case STOR_MX:
        case STOR_MX_DIR:
        case STOR_MX_ESQ:
        case JUMP_DIR:
        case JUMP_ESQ:
        case JUMPMais_DIR:
        case JUMPMais_ESQ:
            bancoIntermediario.dadoParaExecucao = bancoIntermediario.enderecoDecodificado;
            break;
        // Casos em que o valor passado significa um endereço a ser lido, cujo valor será usado
        default:
            barramento.endereco = bancoIntermediario.enderecoDecodificado;
            barramento.operacao = ler;
            executarBarramento();
            bancoIntermediario.dadoParaExecucao = converteDado(barramento.saida);
            break;
    }
    bancoIntermediario.operacaoASerExecutada = bancoIntermediario.opcodeDecodificado;
}

void pipelineExecucao() 
{
    /*
        Executa a fase de Execução do pipeline

        ... Mais documentação vem aqui
    */

    // Caso a instrução anterior tenha terminado
    if (unidadeDeControle.flagPegarNovoContador == True)
    {
        // Pega o novo contador
        contadorClockExecucao = ciclosPorInstrucao[bancoIntermediario.operacaoASerExecutada];

        // Busca dependências RAW
        switch (bancoIntermediario.operacaoASerExecutada)
        {
            case STOR_MX:
                unidadeDeControle.dependenciaRAW = True;
                unidadeDeControle.enderecoRAW = bancoIntermediario.dadoParaExecucao;
                break;
            default:
                unidadeDeControle.dependenciaRAW = False;
                break;
        }

        // Busca dependências JUMP (operações que alteram AC)
        switch (bancoIntermediario.operacaoASerExecutada)
        {
            case LOAD_ABSMX:
            case LOAD_MenosABSMX:
            case LOAD_MenosMX:
            case LOAD_MQ:
            case LOAD_MQ_MX:
            case LOAD_MX:
            case MUL_MX:
            case DIV_MX:
            case ADD_MX:
            case ADD_ABSMX:
            case SUB_MX:
            case SUB_ABSMX:
            case LSH:
            case RSH:
                unidadeDeControle.dependenciaJump = True;
                break;
            default:
                unidadeDeControle.dependenciaJump = False;
        }

        unidadeDeControle.flagPegarNovoContador = False;
        unidadeDeControle.flagCongelarTudo = True;
    }

    // Se a instrução ainda não terminou...
    if (contadorClockExecucao > 1)
    {
        contadorClockExecucao -= 1;
        return;
    }

    bancoIntermediario.resultado_auxiliar = 0;

    bancoIntermediario.instrucao = bancoIntermediario.operacaoASerExecutada;
    unidadeDeControle.flagPegarNovoContador = True;

    // Liberando a escrita de resultado
    unidadeDeControle.flagEstagioCongelado[4] = False;

    switch (bancoIntermediario.operacaoASerExecutada)
    {
    case ADD_MX:
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.entrada2 = bancoIntermediario.dadoParaExecucao;
        unidadeLogicaAritmetica.operacao = soma;
        executarUla();
        bancoIntermediario.resultado = unidadeLogicaAritmetica.saida;
        break;
    case ADD_ABSMX:
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        //unidadeLogicaAritmetica.entrada2 = abs(dadoParaExecucao);
        unidadeLogicaAritmetica.entrada2 = bancoIntermediario.dadoParaExecucao;
        unidadeLogicaAritmetica.operacao = somam;
        executarUla();
        bancoIntermediario.resultado = unidadeLogicaAritmetica.saida;
        break;
    case SUB_MX:
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        //unidadeLogicaAritmetica.entrada2 = dadoParaExecucao;
        unidadeLogicaAritmetica.operacao = subtracao;
        executarUla();
        bancoIntermediario.resultado = unidadeLogicaAritmetica.saida;
        break;
    case SUB_ABSMX:
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        //unidadeLogicaAritmetica.entrada2 = abs(dadoParaExecucao);
        unidadeLogicaAritmetica.entrada2 = bancoIntermediario.dadoParaExecucao;
        unidadeLogicaAritmetica.operacao = subtracaom;
        executarUla();
        bancoIntermediario.resultado = unidadeLogicaAritmetica.saida;
        break;
    case JUMP_DIR:
        bancoRegistradores.PC = (int) bancoIntermediario.dadoParaExecucao;
        limparPipeline();
        unidadeDeControle.ladoInstrucao = Direito;
        bancoIntermediario.instrucao = NENHUMA;
        break;
    case JUMP_ESQ:
        bancoRegistradores.PC = (int) bancoIntermediario.dadoParaExecucao;
        limparPipeline();
        unidadeDeControle.ladoInstrucao = Esquerdo;
        bancoIntermediario.instrucao = NENHUMA;
        break;
    case RSH:
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.operacao = shiftParaDireita;
        executarUla();
        bancoIntermediario.resultado = unidadeLogicaAritmetica.saida;
        // resultado = bancoRegistradores.AC / 2;
        break;
    case LSH:
        unidadeLogicaAritmetica.entrada1 = bancoRegistradores.AC;
        unidadeLogicaAritmetica.operacao = shiftParaEsquerda;
        executarUla();
        bancoIntermediario.resultado = unidadeLogicaAritmetica.saida;
        // resultado = bancoRegistradores.AC * 2;
        break;
    case DIV_MX:
        bancoIntermediario.resultado = bancoRegistradores.AC % bancoIntermediario.dadoParaExecucao;
        bancoIntermediario.resultado_auxiliar = bancoRegistradores.AC / bancoIntermediario.dadoParaExecucao;
        bancoRegistradores.AC = bancoIntermediario.resultado;
        bancoRegistradores.MQ = bancoIntermediario.resultado_auxiliar;
        break;
    case MUL_MX:
        bancoIntermediario.resultado = (bancoIntermediario.dadoParaExecucao * bancoRegistradores.MQ) >> 39;
        bancoIntermediario.resultado_auxiliar = (bancoIntermediario.dadoParaExecucao * bancoRegistradores.MQ) & 0b111111111111111111111111111111111111111;
        break;
    case LOAD_ABSMX:
        bancoIntermediario.resultado = abs(bancoIntermediario.dadoParaExecucao);
        break;
    case LOAD_MenosABSMX:
        bancoIntermediario.resultado = -1 * abs(bancoIntermediario.dadoParaExecucao);
        break;
    case LOAD_MenosMX:
        bancoIntermediario.resultado = -1 * bancoIntermediario.dadoParaExecucao;
        break;
    case LOAD_MQ_MX:
        bancoIntermediario.resultado = bancoIntermediario.dadoParaExecucao;
        break;
    case LOAD_MX:
        bancoIntermediario.resultado = bancoIntermediario.dadoParaExecucao;
        break;
    case STOR_MX:
        bancoIntermediario.resultado = bancoIntermediario.dadoParaExecucao;
        break;
    case STOR_MX_ESQ:
        barramento.operacao = ler;
        barramento.endereco = bancoIntermediario.dadoParaExecucao;
        executarBarramento();

        bancoIntermediario.resultado = (barramento.saida & 0b1111111100000000000011111111111111111111);
        bancoIntermediario.resultado_auxiliar = bancoIntermediario.dadoParaExecucao;
        break;
    case STOR_MX_DIR:
        barramento.operacao = ler;
        barramento.endereco = bancoIntermediario.dadoParaExecucao;
        executarBarramento();

        bancoIntermediario.resultado = (barramento.saida & 0b1111111111111111111111111111000000000000);
        bancoIntermediario.resultado_auxiliar = bancoIntermediario.dadoParaExecucao;
        // barramento.operacao = escrever;
        // barramento.endereco = dadoParaExecucao;
        // barramento.entrada = (barramento.saida & 0b1111111111111111111111111111000000000000) | bancoRegistradores.AC;
        // executarBarramento();
        break;
    case EXIT:
        unidadeDeControle.flagTerminou = True;
        break;
    case NENHUMA:
    case LOAD_MQ:
    default:
        break;
    }

    // Casos com IF tem que ficar fora do switch...
    if ((bancoIntermediario.operacaoASerExecutada == JUMPMais_DIR && (int64_t) bancoRegistradores.AC >= 0))  // || operacaoASerExecutada == JUMP_DIR)
    {
        bancoRegistradores.PC = (int) bancoIntermediario.dadoParaExecucao;
        limparPipeline();
        unidadeDeControle.ladoInstrucao = Direito;
        bancoIntermediario.instrucao = NENHUMA;
    }

    if ((bancoIntermediario.operacaoASerExecutada == JUMPMais_ESQ && (int64_t) bancoRegistradores.AC >= 0))  // || operacaoASerExecutada == JUMP_ESQ)
    {
        bancoRegistradores.PC = (int) bancoIntermediario.dadoParaExecucao;
        limparPipeline();
        unidadeDeControle.ladoInstrucao = Esquerdo;
        bancoIntermediario.instrucao = NENHUMA;
    }

    printf("\nOperacao feita: %d   resultado: %d   resultado_aux: %d", bancoIntermediario.instrucao, bancoIntermediario.resultado, bancoIntermediario.resultado_auxiliar);
}  

void pipelineEscritaResultados()
{
    /*
        Executa a fase de escrita dos resultados do pipeline
    */

    // Se o dado anterior não estiver pronto
    if (unidadeDeControle.flagEstagioCongelado[4] == True)
    {
        return;
    }

    // Descongelando o pipeline :D
    unidadeDeControle.flagEstagioCongelado[0] = False;
    unidadeDeControle.flagEstagioCongelado[1] = False;
    unidadeDeControle.flagEstagioCongelado[2] = False;
    unidadeDeControle.flagEstagioCongelado[3] = False;
    
    unidadeDeControle.flagEvitarSobreescrita = False;
    
    switch (bancoIntermediario.instrucao)
    {
        // Essas instruções fazem AC <- MQ
        case LOAD_MQ:
            bancoRegistradores.AC = bancoRegistradores.MQ;
            break;
        // Essas instruções fazem MQ <- res
        case LOAD_MQ_MX:
            bancoRegistradores.MQ = bancoIntermediario.resultado;
            break;
        // Essas instruções fazem MQ <- res_aux e AC <- res
        case MUL_MX:
        case DIV_MX:
            bancoRegistradores.AC = bancoIntermediario.resultado;
            bancoRegistradores.MQ = bancoIntermediario.resultado_auxiliar;
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
            bancoRegistradores.AC = bancoIntermediario.resultado;
            break;
        case STOR_MX:
            barramento.endereco = bancoIntermediario.resultado;
            barramento.operacao = escrever;
            barramento.entrada = inverteDado(bancoRegistradores.AC);
            executarBarramento();
            break;    
        case STOR_MX_DIR:
            barramento.operacao = escrever;
            barramento.endereco = bancoIntermediario.resultado_auxiliar;
            barramento.entrada = bancoIntermediario.resultado | bancoRegistradores.AC;
            executarBarramento();
            unidadeDeControle.dependenciaStorInstrucao = False;
            unidadeDeControle.flagAnularBusca = False;
            break;
        case STOR_MX_ESQ:
            barramento.operacao = escrever;
            barramento.endereco = bancoIntermediario.resultado_auxiliar;
            barramento.entrada = bancoIntermediario.resultado | (bancoRegistradores.AC << 20);
            executarBarramento();
            unidadeDeControle.dependenciaStorInstrucao = False;
            unidadeDeControle.flagAnularBusca = False;
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
    /*
        Realiza a operação atualmente setada no barramento com os operandos necessários
    */
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
    case somam:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 + abs(unidadeLogicaAritmetica.entrada2);
        break;
    case subtracao:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 - unidadeLogicaAritmetica.entrada2;
        break;
    case subtracaom:
        unidadeLogicaAritmetica.saida = unidadeLogicaAritmetica.entrada1 - abs(unidadeLogicaAritmetica.entrada2);
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

    // Flags da UC
    unidadeDeControle.par = (unidadeLogicaAritmetica.saida % 2 == 0) ? True : False;
    unidadeDeControle.zero = (unidadeLogicaAritmetica.saida == 0) ? True : False;
    unidadeDeControle.sinal = (unidadeLogicaAritmetica.saida < 0) ? True : False;
}

void simulacao()
{
    // Cada vez que é percorrido esse laço é simulado um ciclo de clock do processador
    while (unidadeDeControle.flagTerminou != True)
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
    /*
        Limpa o pipeline completamente
    */
    bancoIntermediario.resultado = 0;
    bancoIntermediario.resultado_auxiliar = 0;
    bancoIntermediario.instrucao = NENHUMA;
    
    bancoIntermediario.dadoParaExecucao = 0;
    bancoIntermediario.operacaoASerExecutada = NENHUMA;

    unidadeDeControle.flagPegarNovoContador = False;

    bancoIntermediario.opcodeDecodificado = NENHUMA;
    bancoIntermediario.enderecoDecodificado = 0;

    bancoIntermediario.resultadoBusca = 0;

    unidadeDeControle.flagEstagioCongelado[0] = False;
    unidadeDeControle.flagEstagioCongelado[1] = False;
    unidadeDeControle.flagEstagioCongelado[2] = False;
    unidadeDeControle.flagEstagioCongelado[3] = False;
    unidadeDeControle.flagEstagioCongelado[4] = False;

    unidadeDeControle.flagCongelarTudo = False;
}

int main (int argc, char *argv[])
{
    if (argc != 5 || strcmp(argv[1], "-p") != 0 || strcmp(argv[3], "-i") != 0)
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

    int endIncialPC = atoi(argv[4]);
    
    // BR
    bancoRegistradores.AC = 0;
    bancoRegistradores.MQ = 0;
    bancoRegistradores.MBR = 0;
    bancoRegistradores.PC = endIncialPC;
    bancoRegistradores.MAR = 0;
    bancoRegistradores.IBR = 0;
    bancoRegistradores.IR = 0;

    // print(bancoRegistradores.PC);
    // ULA
    unidadeLogicaAritmetica.entrada1 = 0;
    unidadeLogicaAritmetica.entrada2 = 0;
    unidadeLogicaAritmetica.operacao = soma;
    unidadeLogicaAritmetica.saida = 0;

    // Barramento
    barramento.operacao = ler;
    barramento.entrada = 0;
    barramento.endereco = 0;
    barramento.saida = 0;

    // BR Intermediário
    bancoIntermediario.opcodeDecodificado = NENHUMA;
    bancoIntermediario.enderecoDecodificado = 0;
    bancoIntermediario.operacaoASerExecutada = NENHUMA;
    bancoIntermediario.dadoParaExecucao = 0;
    bancoIntermediario.resultadoBusca = 0;
    bancoIntermediario.resultado = 0;
    bancoIntermediario.resultado_auxiliar = 0;
    bancoIntermediario.instrucao = NENHUMA;


    // UC
    for (int i = 0; i < 5; i++)
    {
        unidadeDeControle.flagEstagioCongelado[i] = False;
    }
    unidadeDeControle.dependenciaRAW = False;
    unidadeDeControle.enderecoRAW = 0;
    unidadeDeControle.pcAlterado = 0;
    unidadeDeControle.ladoInstrucao = Esquerdo;
    unidadeDeControle.flagTerminou = False;
    unidadeDeControle.flagCongelarTudo = False;
    unidadeDeControle.dependenciaJump = False;
    unidadeDeControle.dependenciaStorInstrucao = False;
    unidadeDeControle.dependenciaStorLado = Esquerdo;
    unidadeDeControle.flagAnularBusca = False;
    unidadeDeControle.flagEvitarSobreescrita = False;
    unidadeDeControle.flagPegarNovoContador = False;
    unidadeDeControle.carry = False; // Sem carry
    unidadeDeControle.par = True; // Ula começa com zero
    unidadeDeControle.zero = True; // Ula começa com zero
    unidadeDeControle.sinal = False; // Par
    
    carregarMemoria(arquivoEntrada, &memoria, &ciclosPorInstrucao);
    char *novo_nome = cria_nome_saida(argv[2]);

    simulacao();   

    dumpDaMemoria(memoria, novo_nome);
        
    free(memoria);
    free(ciclosPorInstrucao);
    fclose(arquivoEntrada);
    return 0;
}
