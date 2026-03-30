#include <stdio.h>
#include <ctype.h>
#include <string.h>

enum {
    tk_palavra_reservada = 1,
    tk_numero,
    tk_identificador,
    tk_operador,
    tk_delimitador,
    tk_separador,
    tk_literal
};

static const char *palavras_reservadas[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while"
};

int ler_caractere(FILE *arquivo, int *linha) {
    int c = fgetc(arquivo);
    if (c == '\n') (*linha)++;
    return c;
}

void devolver_caractere(int c, FILE *arquivo, int *linha) {
    if (c == EOF) return;
    if (c == '\n') (*linha)--;
    ungetc(c, arquivo);
}

int eh_palavra_reservada(const char *texto) {
    for (size_t i = 0; i < sizeof(palavras_reservadas) / sizeof(*palavras_reservadas); i++) {
        if (strcmp(texto, palavras_reservadas[i]) == 0) return 1;
    }
    return 0;
}

int main(void) {
    FILE *arquivo = fopen("arquivo.txt", "r");
    if (!arquivo) {
        perror("erro ao abrir o arquivo");
        return 1;
    }

    int caractere, linha = 1, topo = -1, linhas[256] = {0};
    char texto[256], pilha[256] = {0};

    while ((caractere = ler_caractere(arquivo, &linha)) != EOF) {
        int i = 0;

        if (isspace((unsigned char)caractere)) continue;

        if (caractere == '"') {
            int fechado = 0, linha_inicial = linha;
            texto[i++] = '"';

            while ((caractere = ler_caractere(arquivo, &linha)) != EOF && i < 255) {
                texto[i++] = (char)caractere;

                if (caractere == '\\') {
                    caractere = ler_caractere(arquivo, &linha);
                    if (caractere == EOF) break;
                    if (i < 255) texto[i++] = (char)caractere;
                } else if (caractere == '"') {
                    fechado = 1;
                    break;
                }
            }

            texto[i] = '\0';

            if (fechado) {
                printf("literal: %s | id: %d\n", texto, tk_literal);
            } else {
                printf("linha %d\nerro: literal nao fechada\n", linha_inicial);
                while (caractere != '\n' && caractere != EOF) {
                    caractere = ler_caractere(arquivo, &linha);
                }
            }
            continue;
        }

        if (caractere == '(' || caractere == '{' || caractere == '[') {
            if (topo < 255) {
                pilha[++topo] = (char)caractere;
                linhas[topo] = linha;
            }
            printf("delimitador: %c | id: %d\n", caractere, tk_delimitador);
            continue;
        }

        if (caractere == ')' || caractere == '}' || caractere == ']') {
            if (topo < 0) {
                printf("linha %d\nerro: delimitador sem abertura correspondente\n", linha);
            } else {
                char abertura = pilha[topo];

                if ((abertura == '(' && caractere == ')') ||
                    (abertura == '{' && caractere == '}') ||
                    (abertura == '[' && caractere == ']')) {
                    topo--;
                    printf("delimitador: %c | id: %d\n", caractere, tk_delimitador);
                } else {
                    printf("linha %d\nerro: delimitador incompativel (aberto com '%c', fechado com '%c')\n",
                           linha, abertura, caractere);
                }
            }
            continue;
        }

        if (caractere == ';' || caractere == ',' || caractere == '.') {
            printf("separador: %c | id: %d\n", caractere, tk_separador);
            continue;
        }

        if (isdigit((unsigned char)caractere)) {
            do {
                if (i < 255) texto[i++] = (char)caractere;
                caractere = ler_caractere(arquivo, &linha);
            } while (isdigit((unsigned char)caractere) || caractere == '.');

            texto[i] = '\0';
            printf("numero: %s | id: %d\n", texto, tk_numero);
            devolver_caractere(caractere, arquivo, &linha);
            continue;
        }

        if (isalpha((unsigned char)caractere) || caractere == '_') {
            int reservada;

            do {
                if (i < 255) texto[i++] = (char)caractere;
                caractere = ler_caractere(arquivo, &linha);
            } while (isalnum((unsigned char)caractere) || caractere == '_');

            texto[i] = '\0';
            reservada = eh_palavra_reservada(texto);

            printf("%s: %s | id: %d\n",
                   reservada ? "palavra reservada" : "identificador",
                   texto,
                   reservada ? tk_palavra_reservada : tk_identificador);

            devolver_caractere(caractere, arquivo, &linha);
            continue;
        }

        if (strchr("=!<>+-*/%&|^", caractere)) {
            int proximo = ler_caractere(arquivo, &linha);

            texto[0] = (char)caractere;
            texto[1] = '\0';

            if ((strchr("=!<>", caractere) && proximo == '=') ||
                ((caractere == '+' || caractere == '-' || caractere == '&' ||
                  caractere == '|' || caractere == '<' || caractere == '>') &&
                 proximo == caractere) ||
                (strchr("+-*/%&|^", caractere) && proximo == '=')) {
                texto[1] = (char)proximo;
                texto[2] = '\0';
            } else {
                devolver_caractere(proximo, arquivo, &linha);
            }

            printf("operador: %s | id: %d\n", texto, tk_operador);
        }
    }

    while (topo >= 0) {
        printf("linha %d\nerro: delimitador '%c' nao fechado\n", linhas[topo], pilha[topo]);
        topo--;
    }

    fclose(arquivo);
    return 0;
}
