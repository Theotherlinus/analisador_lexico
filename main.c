#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

/* ==================== Definições de Token ==================== */
typedef enum {
    TK_KEYWORD = 1,
    TK_NUMBER = 2,
    TK_IDENTIFIER = 3,
    TK_OPERATOR = 4,
    TK_DELIMITER = 5,
    TK_SEPARATOR = 6,
    TK_STRING_LITERAL = 7
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line_number;
} Token;

/* ==================== Constantes ==================== */
#define KEYWORDS_COUNT 34
#define MAX_BUFFER 256
#define MAX_STACK 256

static const char *KEYWORDS[KEYWORDS_COUNT] = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline",
    "int", "long", "register", "restrict", "return", "short", "signed", "sizeof",
    "static", "struct", "switch", "typedef", "union", "unsigned", "void",
    "volatile", "while"
};

/* ==================== Estrutura do Analisador ==================== */
typedef struct {
    FILE *file;
    int current_line;
    char delimiter_stack[MAX_STACK];
    int delimiter_stack_top;
    int delimiter_lines[MAX_STACK];
} Lexer;

/* ==================== Funções Auxiliares ==================== */

/**
 * Lê um caractere do arquivo e rastreia as quebras de linha.
 */
int lexer_read_char(Lexer *lexer) {
    int ch = fgetc(lexer->file);
    if (ch == '\n') {
        lexer->current_line++;
    }
    return ch;
}

/**
 * Verifica se uma palavra é uma palavra-chave.
 */
bool is_keyword(const char *word) {
    for (int i = 0; i < KEYWORDS_COUNT; i++) {
        if (strcmp(word, KEYWORDS[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Processa uma literal de string.
 */
bool process_string_literal(Lexer *lexer, Token *token) {
    int literal_start_line = lexer->current_line;
    int index = 0;
    token->value[index++] = '"';

    int ch;
    while ((ch = lexer_read_char(lexer)) != EOF && index < MAX_BUFFER - 1) {
        token->value[index++] = (char)ch;

        if (ch == '\\') {
            int escaped = lexer_read_char(lexer);
            if (escaped == EOF) break;
            token->value[index++] = (char)escaped;
        } else if (ch == '"') {
            token->value[index] = '\0';
            token->type = TK_STRING_LITERAL;
            token->line_number = literal_start_line;
            return true;
        }
    }

    /* Literal não fechada */
    printf("Linha %d\nERRO: literal de string não fechada\n", literal_start_line);
    while ((ch = lexer_read_char(lexer)) != '\n' && ch != EOF);
    return false;
}

/**
 * Processa um número (inteiro ou decimal).
 */
bool process_number(Lexer *lexer, int first_char, Token *token) {
    int index = 0;
    token->value[index++] = (char)first_char;

    int ch;
    while ((ch = lexer_read_char(lexer)) != EOF) {
        if (isdigit(ch) || ch == '.') {
            token->value[index++] = (char)ch;
        } else {
            ungetc(ch, lexer->file);
            break;
        }
    }

    token->value[index] = '\0';
    token->type = TK_NUMBER;
    return true;
}

/**
 * Processa uma palavra (identificador ou palavra-chave).
 */
bool process_word(Lexer *lexer, int first_char, Token *token) {
    int index = 0;
    token->value[index++] = (char)first_char;

    int ch;
    while ((ch = lexer_read_char(lexer)) != EOF) {
        if (isalnum(ch) || ch == '_') {
            token->value[index++] = (char)ch;
        } else {
            ungetc(ch, lexer->file);
            break;
        }
    }

    token->value[index] = '\0';

    if (is_keyword(token->value)) {
        token->type = TK_KEYWORD;
    } else {
        token->type = TK_IDENTIFIER;
    }

    return true;
}

/**
 * Processa operadores (incluindo operadores compostos).
 */
bool process_operator(Lexer *lexer, int first_char, Token *token) {
    char op[3] = {(char)first_char, '\0', '\0'};
    int next_ch = lexer_read_char(lexer);

    /* Operadores de dois caracteres */
    static const char *COMPOUND_OPS[] = {
        "==", "!=", "<=", ">=", "++", "--", "&&", "||", "<<", ">>", "+=", "-=",
        "*=", "/=", "%=", "&=", "|=", "^="
    };

    op[1] = (char)next_ch;

    bool found = false;
    for (size_t i = 0; i < sizeof(COMPOUND_OPS) / sizeof(COMPOUND_OPS[0]); i++) {
        if (strcmp(op, COMPOUND_OPS[i]) == 0) {
            found = true;
            break;
        }
    }

    if (!found) {
        ungetc(next_ch, lexer->file);
        op[1] = '\0';
    }

    strcpy(token->value, op);
    token->type = TK_OPERATOR;
    return true;
}

/**
 * Gerencia a pilha de delimitadores para validação.
 */
void handle_opening_delimiter(Lexer *lexer, int ch, Token *token) {
    if (lexer->delimiter_stack_top < MAX_STACK - 1) {
        lexer->delimiter_stack[++lexer->delimiter_stack_top] = (char)ch;
        lexer->delimiter_lines[lexer->delimiter_stack_top] = lexer->current_line;
    }
    token->value[0] = (char)ch;
    token->value[1] = '\0';
    token->type = TK_DELIMITER;
}

/**
 * Valida delimitadores de fechamento.
 */
void handle_closing_delimiter(Lexer *lexer, int ch, Token *token) {
    token->value[0] = (char)ch;
    token->value[1] = '\0';
    token->type = TK_DELIMITER;

    if (lexer->delimiter_stack_top < 0) {
        printf("Linha %d\nERRO: delimitador sem abertura correspondente\n",
               lexer->current_line);
        return;
    }

    char opening = lexer->delimiter_stack[lexer->delimiter_stack_top];
    bool valid = (opening == '(' && ch == ')') ||
                 (opening == '{' && ch == '}') ||
                 (opening == '[' && ch == ']');

    if (valid) {
        lexer->delimiter_stack_top--;
    } else {
        printf("Linha %d\nERRO: delimitador incompatível (aberto com '%c', fechado com '%c')\n",
               lexer->current_line, opening, ch);
    }
}

/**
 * Obtém o próximo token do arquivo.
 */
bool lexer_next_token(Lexer *lexer, Token *token) {
    token->line_number = lexer->current_line;
    int ch;

    /* Ignora espaços em branco */
    while ((ch = lexer_read_char(lexer)) != EOF && isspace(ch));

    if (ch == EOF) {
        return false;
    }

    /* Literais de string */
    if (ch == '"') {
        return process_string_literal(lexer, token);
    }

    /* Delimitadores de abertura */
    if (ch == '(' || ch == '{' || ch == '[') {
        handle_opening_delimiter(lexer, ch, token);
        return true;
    }

    /* Delimitadores de fechamento */
    if (ch == ')' || ch == '}' || ch == ']') {
        handle_closing_delimiter(lexer, ch, token);
        return true;
    }

    /* Separadores */
    if (ch == ';' || ch == ',' || ch == '.') {
        token->value[0] = (char)ch;
        token->value[1] = '\0';
        token->type = TK_SEPARATOR;
        return true;
    }

    /* Números */
    if (isdigit(ch)) {
        return process_number(lexer, ch, token);
    }

    /* Identificadores e palavras-chave */
    if (isalpha(ch) || ch == '_') {
        return process_word(lexer, ch, token);
    }

    /* Operadores */
    if (strchr("=!<>+-*/%&|^", ch)) {
        return process_operator(lexer, ch, token);
    }

    return false;
}

/**
 * Inicializa o analisador léxico.
 */
Lexer* lexer_create(const char *filename) {
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    if (lexer == NULL) {
        perror("Erro ao alocar memória para o analisador");
        return NULL;
    }

    lexer->file = fopen(filename, "r");
    if (lexer->file == NULL) {
        perror("Erro ao abrir o arquivo");
        free(lexer);
        return NULL;
    }

    lexer->current_line = 1;
    lexer->delimiter_stack_top = -1;
    return lexer;
}

/**
 * Libera recursos do analisador léxico.
 */
void lexer_destroy(Lexer *lexer) {
    if (lexer == NULL) return;

    /* Verifica delimitadores não fechados */
    while (lexer->delimiter_stack_top >= 0) {
        printf("Linha %d\nERRO: delimitador '%c' não fechado\n",
               lexer->delimiter_lines[lexer->delimiter_stack_top],
               lexer->delimiter_stack[lexer->delimiter_stack_top]);
        lexer->delimiter_stack_top--;
    }

    if (lexer->file != NULL) {
        fclose(lexer->file);
    }

    free(lexer);
}

/**
 * Converte o tipo de token em string legível.
 */
const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TK_KEYWORD:
            return "Palavra reservada";
        case TK_NUMBER:
            return "Número";
        case TK_IDENTIFIER:
            return "Identificador";
        case TK_OPERATOR:
            return "Operador";
        case TK_DELIMITER:
            return "Delimitador";
        case TK_SEPARATOR:
            return "Separador";
        case TK_STRING_LITERAL:
            return "Literal";
        default:
            return "Desconhecido";
    }
}

int main() {
    Lexer *lexer = lexer_create("arquivo.txt");
    if (lexer == NULL) {
        return 1;
    }

    Token token;

    while (lexer_next_token(lexer, &token)) {
        printf("%s: %s | ID: %d\n", token_type_to_string(token.type), token.value, token.type);
    }

    lexer_destroy(lexer);
    return 0;
}
