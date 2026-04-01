#include <stdio.h>
#include <ctype.h>
#include <string.h>

//definicao dos tokens
enum {
    tk_palavra_reservada = 1,
    tk_numero,
    tk_identificador,
    tk_operador,
    tk_delimitador,
    tk_separador,
    tk_literal
};

#define MAX_TOKENS 1024

typedef struct {
    char tipo[32];
    char lexema[256];
    int id;
    int linha;
    int coluna;
} Token;

static int ultima_linha_lida = 1;
static int ultima_coluna_lida = 0;

static const char *palavras_reservadas[] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while"
};

//entrada de dados
int ler_caractere(FILE *arquivo, int *linha, int *coluna) {
    int c = fgetc(arquivo);

    ultima_linha_lida = *linha;
    ultima_coluna_lida = *coluna;

    if (c == '\n') {
        (*linha)++;
        *coluna = 0;
    } else if (c != EOF) {
        (*coluna)++;
    }

    return c;
}
//devolucao de dados
void devolver_caractere(int c, FILE *arquivo, int *linha, int *coluna) {
    if (c == EOF) return;

    *linha = ultima_linha_lida;
    *coluna = ultima_coluna_lida;
    ungetc(c, arquivo);
}

void registrar_token(Token tokens[], int *total, const char *tipo, const char *lexema,
                     int id, int linha, int coluna) {
    if (*total >= MAX_TOKENS) return;

    strncpy(tokens[*total].tipo, tipo, sizeof(tokens[*total].tipo) - 1);
    tokens[*total].tipo[sizeof(tokens[*total].tipo) - 1] = '\0';

    strncpy(tokens[*total].lexema, lexema, sizeof(tokens[*total].lexema) - 1);
    tokens[*total].lexema[sizeof(tokens[*total].lexema) - 1] = '\0';

    tokens[*total].id = id;
    tokens[*total].linha = linha;
    tokens[*total].coluna = coluna;
    (*total)++;
}

void imprimir_tabela_tokens(const Token tokens[], int total) {
    int i;

    printf("\n%-22s %-20s %-4s %-7s %-7s\n", "tipo", "lexema", "id", "linha", "coluna");
    printf("-------------------------------------------------------------------------------\n");

    for (i = 0; i < total; i++) {
        printf("%-22s %-20s %-4d %-7d %-7d\n",
               tokens[i].tipo,
               tokens[i].lexema,
               tokens[i].id,
               tokens[i].linha,
               tokens[i].coluna);
    }
}

//verificacao de palavras reservadas
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

    int caractere, linha = 1, coluna = 0, topo = -1;
    int linhas[256] = {0}, colunas_abertura[256] = {0};
    int total_tokens = 0;
    char texto[256], pilha[256] = {0};
    Token tokens[MAX_TOKENS];
//leitura do arquivo
    while ((caractere = ler_caractere(arquivo, &linha, &coluna)) != EOF) {
        int i = 0;
        int linha_token = linha;
        int coluna_token = coluna;

        if (isspace((unsigned char)caractere)) continue;

        if (caractere == '"') {
            int fechado = 0, linha_inicial = linha_token, coluna_inicial = coluna_token;

            texto[i++] = '"';
//leitura de literais
            while ((caractere = ler_caractere(arquivo, &linha, &coluna)) != EOF && i < 255) {
                texto[i++] = (char)caractere;

                if (caractere == '\\') {
                    caractere = ler_caractere(arquivo, &linha, &coluna);
                    if (caractere == EOF) break;
                    if (i < 255) texto[i++] = (char)caractere;
                } else if (caractere == '"') {
                    fechado = 1;
                    break;
                }
            }

            texto[i] = '\0';

            if (fechado) {
                registrar_token(tokens, &total_tokens, "literal", texto, tk_literal,
                                linha_inicial, coluna_inicial);
            } else {
                printf("linha %d, coluna %d\nerro: literal nao fechada\n",
                       linha_inicial, coluna_inicial);
                while (caractere != '\n' && caractere != EOF) {
                    caractere = ler_caractere(arquivo, &linha, &coluna);
                }
            }
            continue;
        }

        if (caractere == '(' || caractere == '{' || caractere == '[') {
            char lexema[2] = {(char)caractere, '\0'};
//verificacao de delimitadores
            if (topo < 255) {
                pilha[++topo] = (char)caractere;
                linhas[topo] = linha_token;
                colunas_abertura[topo] = coluna_token;
            }

            registrar_token(tokens, &total_tokens, "delimitador", lexema, tk_delimitador,
                            linha_token, coluna_token);
            continue;
        }

        if (caractere == ')' || caractere == '}' || caractere == ']') {
            char lexema[2] = {(char)caractere, '\0'};

            if (topo < 0) {
                printf("linha %d, coluna %d\nerro: delimitador sem abertura correspondente\n",
                       linha_token, coluna_token);
            } else {
                char abertura = pilha[topo];

                if ((abertura == '(' && caractere == ')') ||
                    (abertura == '{' && caractere == '}') ||
                    (abertura == '[' && caractere == ']')) {
                    topo--;
                    registrar_token(tokens, &total_tokens, "delimitador", lexema,
                                    tk_delimitador, linha_token, coluna_token);
                } else {
                    printf("linha %d, coluna %d\nerro: delimitador incompativel (aberto com '%c' na linha %d, coluna %d; fechado com '%c')\n",
                           linha_token, coluna_token, abertura, linhas[topo],
                           colunas_abertura[topo], caractere);
                }
            }
            continue;
        }
//verificacao de separadores
        if (caractere == ';' || caractere == ',' || caractere == '.') {
            char lexema[2] = {(char)caractere, '\0'};

            registrar_token(tokens, &total_tokens, "separador", lexema, tk_separador,
                            linha_token, coluna_token);
            continue;
        }

        if (isdigit((unsigned char)caractere)) {
            do {
                if (i < 255) texto[i++] = (char)caractere;
                caractere = ler_caractere(arquivo, &linha, &coluna);
            } while (isdigit((unsigned char)caractere) || caractere == '.');

            texto[i] = '\0';
            registrar_token(tokens, &total_tokens, "numero", texto, tk_numero,
                            linha_token, coluna_token);
            devolver_caractere(caractere, arquivo, &linha, &coluna);
            continue;
        }

        if (isalpha((unsigned char)caractere) || caractere == '_') {
            int reservada;

            do {
                if (i < 255) texto[i++] = (char)caractere;
                caractere = ler_caractere(arquivo, &linha, &coluna);
            } while (isalnum((unsigned char)caractere) || caractere == '_');

            texto[i] = '\0';
            reservada = eh_palavra_reservada(texto);

            registrar_token(tokens, &total_tokens,
                            reservada ? "palavra reservada" : "identificador",
                            texto,
                            reservada ? tk_palavra_reservada : tk_identificador,
                            linha_token, coluna_token);

            devolver_caractere(caractere, arquivo, &linha, &coluna);
            continue;
        }

        if (strchr("=!<>+-*/%&|^", caractere)) {
            int proximo = ler_caractere(arquivo, &linha, &coluna);

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
                devolver_caractere(proximo, arquivo, &linha, &coluna);
            }

            registrar_token(tokens, &total_tokens, "operador", texto, tk_operador,
                            linha_token, coluna_token);
        }
    }

    imprimir_tabela_tokens(tokens, total_tokens);

    while (topo >= 0) {
        printf("linha %d, coluna %d\nerro: delimitador '%c' nao fechado\n",
               linhas[topo], colunas_abertura[topo], pilha[topo]);
        topo--;
    }

    fclose(arquivo);
    return 0;
}
