# analisador lexico

este projeto implementa um analisador lexico simples em c.

## requisitos

- linux
- gcc instalado

## como compilar

no terminal, entre na pasta do projeto:

cd "/home/linus-501512/Área de trabalho/2026.1/compiladores/analisador_lexico"

depois compile:

gcc -Wall -Wextra -o analisador main.c

## como executar

o programa le o arquivo `arquivo.txt` na mesma pasta do projeto.

execute com:

./analisador

se quiser salvar a saida em um arquivo:

./analisador > resultado_teste.txt

## exemplo de entrada

crie ou edite o arquivo `arquivo.txt` com um conteudo como este:

```txt
int main() {
    int a = 10;
    float b = 3.14;
    if (a > 0) {
        a++;
    }
    return a;
}
```

## observacoes

- o arquivo de entrada deve se chamar `arquivo.txt`
- os resultados sao exibidos no terminal
- em caso de erro lexico, o programa informa a linha correspondente
