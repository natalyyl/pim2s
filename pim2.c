/*
 * PIM - Banco Digital (protótipo acadêmico)
 * Linguagem: C
 * Banco de dados: MySQL local
 *
 * Requer MySQL Connector/C (headers e biblioteca de desenvolvimento).
 * Ajuste as constantes DB_USER, DB_PASSWORD e DB_NAME para seu ambiente.
 * Use apenas dados fictícios; este programa NÃO é um sistema bancário real.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mysql.h>
#include <locale.h>
#include <windows.h>

#define DB_HOST "localhost"
#define DB_USER "root"
#define DB_PASSWORD "59252328"
#define DB_NAME "banco_digital"
#define MAX_HISTORICO 100
#define MAX_CATEGORIAS 20



typedef struct {
    int idConta;
    char numeroDaConta[21];
    char nome[101];
    double saldoDaConta;
    char statusConta[16];
} ContaUsuario;

typedef struct {
    int idTransacao;
    char tipo[21];
    double valor;
    char descricao[201];
    char categoria[51];
    char dataHora[20];
} Transacao;

/* Lê uma linha de texto e remove a quebra de linha. */
void lerTexto(const char *mensagem, char *destino, size_t tamanho) {
    printf("%s", mensagem);
    if (fgets(destino, (int)tamanho, stdin) != NULL) {
        destino[strcspn(destino, "\r\n")] = '\0';
    } else {
        destino[0] = '\0';
    }
}

/* Lê inteiro com validação simples. */
int lerInteiro(const char *mensagem) {
    char linha[80];
    char *fim;
    long valor;

    for (;;) {
        lerTexto(mensagem, linha, sizeof(linha));
        valor = strtol(linha, &fim, 10);
        while (isspace((unsigned char)*fim)) fim++;

        if (linha[0] != '\0' && *fim == '\0') {
            return (int)valor;
        }
        printf("Entrada inválida. Digite um número inteiro.\n");
    }
}

/* Lê valor monetário positivo. O separador decimal deve ser ponto, por exemplo 25.50. */
double lerValorPositivo(const char *mensagem) {
    char linha[80];
    char *fim;
    double valor;

    for (;;) {
        lerTexto(mensagem, linha, sizeof(linha));
        valor = strtod(linha, &fim);
        while (isspace((unsigned char)*fim)) fim++;

        if (linha[0] != '\0' && *fim == '\0' && valor > 0.0) {
            return valor;
        }
        printf("Valor inválido. Digite um valor maior que zero (ex.: 25.50).\n");
    }
}

int conectarBanco(MYSQL **conexao) {
    *conexao = mysql_init(NULL);
    if (*conexao == NULL) {
        fprintf(stderr, "Erro ao inicializar o MySQL.\n");
        return 0;
    }

    if (mysql_real_connect(*conexao, DB_HOST, DB_USER, DB_PASSWORD,
                           DB_NAME, 0, NULL, 0) == NULL) {
        fprintf(stderr, "Não foi possível conectar ao MySQL: %s\n",
                mysql_error(*conexao));
        mysql_close(*conexao);
        *conexao = NULL;
        return 0;
    }

    mysql_set_character_set(*conexao, "utf8mb4");
    printf("Conexão com o banco de dados estabelecida.\n");
    return 1;
}

/* Busca a conta pelo número informado e carrega os dados da tabela Conta/Cliente. */
int buscarConta(MYSQL *conexao, const char *numero, ContaUsuario *conta) {
    char numeroEscapado[128];
    char consulta[512];
    MYSQL_RES *resultado;
    MYSQL_ROW linha;

    mysql_real_escape_string(conexao, numeroEscapado, numero,
                             (unsigned long)strlen(numero));
    snprintf(consulta, sizeof(consulta),
        "SELECT c.id_conta, c.numero_conta, cl.nome, c.saldo, c.status_conta "
        "FROM Conta c INNER JOIN Cliente cl ON cl.id_cliente = c.id_cliente "
        "WHERE c.numero_conta = '%s' LIMIT 1", numeroEscapado);

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao consultar conta: %s\n", mysql_error(conexao));
        return 0;
    }

    resultado = mysql_store_result(conexao);
    if (resultado == NULL) {
        printf("Erro ao ler resultado: %s\n", mysql_error(conexao));
        return 0;
    }

    linha = mysql_fetch_row(resultado);
    if (linha == NULL) {
        mysql_free_result(resultado);
        return 0;
    }

    conta->idConta = atoi(linha[0]);
    snprintf(conta->numeroDaConta, sizeof(conta->numeroDaConta), "%s", linha[1]);
    snprintf(conta->nome, sizeof(conta->nome), "%s", linha[2]);
    conta->saldoDaConta = atof(linha[3]);
    snprintf(conta->statusConta, sizeof(conta->statusConta), "%s", linha[4]);

    mysql_free_result(resultado);
    return 1;
}

void consultarSaldo(MYSQL *conexao, ContaUsuario *conta) {
    char consulta[256];
    MYSQL_RES *resultado;
    MYSQL_ROW linha;

    snprintf(consulta, sizeof(consulta),
             "SELECT saldo, status_conta FROM Conta WHERE id_conta = %d",
             conta->idConta);

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao consultar saldo: %s\n", mysql_error(conexao));
        return;
    }

    resultado = mysql_store_result(conexao);
    if (resultado == NULL) {
        printf("Não foi possível obter o saldo.\n");
        return;
    }

    linha = mysql_fetch_row(resultado);
    if (linha != NULL) {
        conta->saldoDaConta = atof(linha[0]);
        snprintf(conta->statusConta, sizeof(conta->statusConta), "%s", linha[1]);
        printf("\nTitular: %s\n", conta->nome);
        printf("Conta: %s\n", conta->numeroDaConta);
        printf("Status: %s\n", conta->statusConta);
        printf("Saldo atual: R$ %.2f\n", conta->saldoDaConta);
    } else {
        printf("Conta não encontrada.\n");
    }
    mysql_free_result(resultado);
}

/* Exibe categorias e retorna o ID real da categoria escolhida.
   Os vetores e a matriz armazenam temporariamente os dados recebidos do BD. */
int selecionarCategoria(MYSQL *conexao, const char *tipoDesejado) {
    MYSQL_RES *resultado;
    MYSQL_ROW linha;
    char consulta[256];
    int ids[MAX_CATEGORIAS];
    char nomes[MAX_CATEGORIAS][51];
    int quantidade = 0;
    int escolha;

    snprintf(consulta, sizeof(consulta),
        "SELECT id_categoria, nome_categoria FROM Categoria "
        "WHERE UPPER(tipo_categoria) = '%s' ORDER BY nome_categoria",
        tipoDesejado);

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao consultar categorias: %s\n", mysql_error(conexao));
        return -1;
    }

    resultado = mysql_store_result(conexao);
    if (resultado == NULL) return -1;

    while ((linha = mysql_fetch_row(resultado)) != NULL &&
           quantidade < MAX_CATEGORIAS) {
        ids[quantidade] = atoi(linha[0]);
        snprintf(nomes[quantidade], sizeof(nomes[quantidade]), "%s", linha[1]);
        quantidade++;
    }
    mysql_free_result(resultado);

    if (quantidade == 0) {
        printf("Não existem categorias cadastradas para %s.\n", tipoDesejado);
        return -1;
    }

    printf("\nCategorias disponíveis:\n");
    for (int i = 0; i < quantidade; i++) {
        printf("%d - %s\n", i + 1, nomes[i]);
    }

    escolha = lerInteiro("Escolha a categoria: ");
    if (escolha < 1 || escolha > quantidade) {
        printf("Categoria inválida.\n");
        return -1;
    }
    return ids[escolha - 1];
}

/* Atualiza o saldo e insere a transação de forma atômica. */
void registrarTransacao(MYSQL *conexao, ContaUsuario *conta, const char *tipo) {
    double valor;
    int idCategoria;
    char descricao[201];
    char descricaoEscapada[402];
    char consulta[1200];
    char tipoSql[21];
    int atualizou;

    if (strcmp(conta->statusConta, "ATIVA") != 0) {
        printf("Esta conta não está ativa. Não é possível movimentá-la.\n");
        return;
    }

    idCategoria = selecionarCategoria(conexao, tipo);
    if (idCategoria < 0) return;

    valor = lerValorPositivo("Informe o valor (ex.: 25.50): ");
    lerTexto("Descrição da movimentação: ", descricao, sizeof(descricao));
    if (descricao[0] == '\0') {
        snprintf(descricao, sizeof(descricao), "%s", "Sem descrição");
    }

    mysql_real_escape_string(conexao, descricaoEscapada, descricao,
                             (unsigned long)strlen(descricao));
    snprintf(tipoSql, sizeof(tipoSql), "%s", tipo);

    if (mysql_query(conexao, "START TRANSACTION") != 0) {
        printf("Não foi possível iniciar a transação: %s\n", mysql_error(conexao));
        return;
    }

    if (strcmp(tipo, "ENTRADA") == 0) {
        snprintf(consulta, sizeof(consulta),
            "UPDATE Conta SET saldo = saldo + %.2f "
            "WHERE id_conta = %d AND status_conta = 'ATIVA'",
            valor, conta->idConta);
    } else {
        snprintf(consulta, sizeof(consulta),
            "UPDATE Conta SET saldo = saldo - %.2f "
            "WHERE id_conta = %d AND status_conta = 'ATIVA' AND saldo >= %.2f",
            valor, conta->idConta, valor);
    }

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao atualizar saldo: %s\n", mysql_error(conexao));
        mysql_query(conexao, "ROLLBACK");
        return;
    }

    atualizou = (int)mysql_affected_rows(conexao);
    if (atualizou != 1) {
        printf("Operação não realizada. Verifique o status da conta e, "
               "no caso de saída, se há saldo suficiente.\n");
        mysql_query(conexao, "ROLLBACK");
        return;
    }

    snprintf(consulta, sizeof(consulta),
        "INSERT INTO Transacao "
        "(id_conta, id_categoria, tipo_transacao, valor, descricao, data_hora, status_transacao) "
        "VALUES (%d, %d, '%s', %.2f, '%s', NOW(), 'CONCLUIDA')",
        conta->idConta, idCategoria, tipoSql, valor, descricaoEscapada);

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao registrar transação: %s\n", mysql_error(conexao));
        mysql_query(conexao, "ROLLBACK");
        return;
    }

    if (mysql_query(conexao, "COMMIT") != 0) {
        printf("Erro ao confirmar operação: %s\n", mysql_error(conexao));
        mysql_query(conexao, "ROLLBACK");
        return;
    }

    printf("Movimentação registrada com sucesso!\n");
    consultarSaldo(conexao, conta);
}

/* Consulta até 100 movimentações, guarda em vetor e permite exportar CSV. */
void consultarHistorico(MYSQL *conexao, const ContaUsuario *conta) {
    MYSQL_RES *resultado;
    MYSQL_ROW linha;
    char consulta[512];
    Transacao historico[MAX_HISTORICO];
    int quantidade = 0;
    int exportar;

    snprintf(consulta, sizeof(consulta),
        "SELECT t.id_transacao, t.tipo_transacao, t.valor, "
        "COALESCE(t.descricao, ''), cat.nome_categoria, "
        "DATE_FORMAT(t.data_hora, '%%Y-%%m-%%d %%H:%%i') "
        "FROM Transacao t "
        "INNER JOIN Categoria cat ON cat.id_categoria = t.id_categoria "
        "WHERE t.id_conta = %d ORDER BY t.data_hora DESC LIMIT %d",
        conta->idConta, MAX_HISTORICO);

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao consultar histórico: %s\n", mysql_error(conexao));
        return;
    }

    resultado = mysql_store_result(conexao);
    if (resultado == NULL) {
        printf("Não foi possível obter o histórico.\n");
        return;
    }

    while ((linha = mysql_fetch_row(resultado)) != NULL &&
           quantidade < MAX_HISTORICO) {
        historico[quantidade].idTransacao = atoi(linha[0]);
        snprintf(historico[quantidade].tipo, sizeof(historico[quantidade].tipo), "%s", linha[1]);
        historico[quantidade].valor = atof(linha[2]);
        snprintf(historico[quantidade].descricao, sizeof(historico[quantidade].descricao), "%s", linha[3]);
        snprintf(historico[quantidade].categoria, sizeof(historico[quantidade].categoria), "%s", linha[4]);
        snprintf(historico[quantidade].dataHora, sizeof(historico[quantidade].dataHora), "%s", linha[5]);
        quantidade++;
    }
    mysql_free_result(resultado);

    printf("\n========== HISTÓRICO ==========\n");
    if (quantidade == 0) {
        printf("Nenhuma transação encontrada.\n");
        return;
    }

    for (int i = 0; i < quantidade; i++) {
        printf("ID: %d | %s | R$ %.2f | %s | %s | %s\n",
            historico[i].idTransacao, historico[i].tipo,
            historico[i].valor, historico[i].categoria,
            historico[i].dataHora, historico[i].descricao);
    }

    exportar = lerInteiro("Exportar este histórico para CSV? (1=Sim, 0=Não): ");
    if (exportar == 1) {
        FILE *arquivo = fopen("historico_transacoes.csv", "w");
        if (arquivo == NULL) {
            printf("Não foi possível criar o arquivo CSV.\n");
            return;
        }
        fprintf(arquivo, "id_transacao;tipo;valor;descricao;categoria;data_hora\n");
        for (int i = 0; i < quantidade; i++) {
            /* Substitui ponto e vírgula da descrição para preservar as colunas. */
            for (char *p = historico[i].descricao; *p; p++) {
                if (*p == ';') *p = ',';
            }
            fprintf(arquivo, "%d;%s;%.2f;%s;%s;%s\n",
                historico[i].idTransacao, historico[i].tipo,
                historico[i].valor, historico[i].descricao,
                historico[i].categoria, historico[i].dataHora);
        }
        fclose(arquivo);
        printf("Arquivo historico_transacoes.csv criado na pasta do programa.\n");
    }
}

/* Demonstra consulta de contas que não estão ativas. */
void alertarContas(MYSQL *conexao) {
    MYSQL_RES *resultado;
    MYSQL_ROW linha;
    const char *consulta =
        "SELECT c.numero_conta, cl.nome, c.status_conta "
        "FROM Conta c INNER JOIN Cliente cl ON cl.id_cliente = c.id_cliente "
        "WHERE c.status_conta <> 'ATIVA' ORDER BY cl.nome";

    if (mysql_query(conexao, consulta) != 0) {
        printf("Erro ao consultar contas: %s\n", mysql_error(conexao));
        return;
    }

    resultado = mysql_store_result(conexao);
    if (resultado == NULL) return;

    printf("\n====== ALERTA DE CONTAS ======\n");
    if (mysql_num_rows(resultado) == 0) {
        printf("Nenhuma conta com status diferente de ATIVA foi encontrada.\n");
    } else {
        while ((linha = mysql_fetch_row(resultado)) != NULL) {
            printf("Conta: %s | Titular: %s | Status: %s\n",
                   linha[0], linha[1], linha[2]);
        }
    }
    mysql_free_result(resultado);
}

int main(void) {
    //confuguração necessaria para os caracteres adptada para compilar com -I e -L
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setlocale(LC_ALL, ".UTF-8");    MYSQL *conexao = NULL;
    setlocale(LC_NUMERIC, "C");
    ContaUsuario conta;
    char numeroConta[21];
    int opcaoMenu;
    int rodando = 1;


    if (!conectarBanco(&conexao)) {
        printf("Confira se o serviço MySQL está ativo e se as configurações "
               "de conexão estão corretas.\n");
        return 1;
    }

    printf("\n===== ACESSO AO BANCO DIGITAL =====\n");
    lerTexto("Digite o número da conta cadastrada: ",
             numeroConta, sizeof(numeroConta));

    if (!buscarConta(conexao, numeroConta, &conta)) {
        printf("Conta não encontrada. Confira o número e o cadastro no banco.\n");
        mysql_close(conexao);
        return 1;
    }

    printf("\nBem-vindo(a), %s!\n", conta.nome);

    do {
        printf("\n=================================\n");
        printf("        SISTEMA FINANCEIRO       \n");
        printf("=================================\n");
        printf(" 1 - Consultar Saldo\n");
        printf(" 2 - Registrar Entrada\n");
        printf(" 3 - Registrar Saída\n");
        printf(" 4 - Histórico de Transações\n");
        printf(" 5 - Caixa Bank (Alerta de Contas)\n");
        printf(" 6 - Sair do Sistema\n");
        printf("---------------------------------\n");

        opcaoMenu = lerInteiro("Digite sua opção: ");

        switch (opcaoMenu) {
            case 1:
                consultarSaldo(conexao, &conta);
                break;
            case 2:
                registrarTransacao(conexao, &conta, "ENTRADA");
                break;
            case 3:
                registrarTransacao(conexao, &conta, "SAIDA");
                break;
            case 4:
                consultarHistorico(conexao, &conta);
                break;
            case 5:
                alertarContas(conexao);
                break;
            case 6:
                printf("Encerrando o sistema. Até logo!\n");
                rodando = 0;
                break;
            default:
                printf("Opção inválida! Escolha um número de 1 a 6.\n");
                break;
        }
    } while (rodando == 1);

    mysql_close(conexao);
    return 0;
}
