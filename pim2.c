#include <stdio.h>
#include <windows.h>
#include <string.h>


 typedef struct 
    {   char nome[50];
        int numeroDaConta;
        float saldoDaConta;
        int numeroDeTransação;
        char senhaDaConta[6];
    } ContaUsuario; 

int main(void) {
    // Configuração do terminal Windows para UTF-8 
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    int opcaoMenu;
    int rodando = 1;
    
    // teste para ver se type vai funcionar 
    ContaUsuario contaTest;
     contaTest.numeroDaConta = 1001;
     contaTest.saldoDaConta = 500.50;
     strcpy(contaTest.nome, "Testeeee"); // Para texto, usamos strcpy


    printf("Conta: %d\n", contaTest.numeroDaConta);
    printf("Titular: %s\n", contaTest.nome);
    printf("Saldo: R$ %.2f\n", contaTest.saldoDaConta);

    do {
        printf("\n=================================\n");
        printf("        SISTEMA FINANCEIRO       \n");
        printf("=================================\n");
        printf(" 1 - Consultar Saldo\n");
        printf(" 2 - Registrar Entradas/Saídas\n");
        printf(" 3 - Histórico de Transações\n");
        printf(" 4 - Caixa Bank (Alerta de Contas)\n");
        printf(" 5 - Sair do Sistema\n");
        printf("---------------------------------\n");
        printf("Digite a sua opção: ");

        // Se o scanf conseguir gurda 1 dado na opcao menu então exeucuta o menu se não siginifica que algo foi digitado errado.
        if (scanf("%d", &opcaoMenu) != 1) {
            printf("\n⚠️ ERRO: Digite apenas números de 1 a 5!\n");
            
            //joga fora o que foi digitada para não travar o do menu loop
            while (getchar() != '\n'); 
            
            continue; //Pula o 'capítulo'. Ele vai ignora o que sobrou nesta parte, mas volta para o início do laço para testar a condição e continuar a próxima rodada.

        }

        switch (opcaoMenu) {
            case 1:
                printf("\n[Saldo] Seu saldo atual é R$ 0,00\n");
                break;

            case 2:
                printf("\n[Movimentação] Menu de Entradas e Saídas...\n");
                break;

            case 3:
                printf("\n[Histórico] Exibindo últimas transações...\n");
                break;

            case 4:
                printf("\n[Caixa Bank] Gerenciador de contas por data...\n");
                break;

            case 5:
                printf("\nEncerrando o sistema. Até logo!\n");
                rodando = 0; 
                break;

            default:
                printf("\n⚠️ Opção inválida! Escolha um número entre 1 e 5.\n");
                break;
        }

    } while (rodando == 1);

    return 0;
}
