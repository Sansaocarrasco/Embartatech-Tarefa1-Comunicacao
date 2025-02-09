# Tarefa 1: Comunicação

## 📌 Introdução
Este projeto implementa um sistema de comunicação e controle utilizando um display OLED e uma matriz de LEDs WS2818B, permitindo a exibição de caracteres e números baseados na entrada serial.

## 🛠 Componentes Utilizados
A tarefa requer os seguintes componentes conectados à placa **BitDogLab**:

| Componente                | Conexão à GPIO |
|---------------------------|---------------|
| Display OLED SSD1306      | GPIOs 14, 15  |
| Matriz de LEDs WS2818B    | GPIO 7        |
| Botão A                   | GPIO 5        |
| Botão B                   | GPIO 6        |
| LED Verde                 | GPIO 11       |
| LED Azul                  | GPIO 12       |

## 📌 Requisitos da Atividade

1. O display OLED exibe o último caractere recebido via comunicação serial.
2. Os botões físicos controlam o acionamento dos LEDs Verde e Azul.
3. A matriz de LEDs WS2818B exibe padrões conforme números recebidos via comunicação serial.
4. A rotina principal mantém um loop contínuo verificando entradas e atualizando o display e LEDs.
5. A comunicação serial permite o envio de comandos para exibição no display OLED e controle da matriz de LEDs.

## 💻 Instruções de Uso

1. Clone este repositório:
   ```sh
   git clone https://github.com/Sansaocarrasco/Embartatech-Tarefa1-Comunicacao.git
   Abra o projeto no VS Code.

2. Conecte a placa Raspberry Pi Pico W ao computador no modo BOOTSEL (pressionando o botão BOOTSEL ao conectar via USB).

3. Compile o arquivo "comunicacao.c" e carregue o projeto para a placa.

## 🎥 Vídeo Demonstrativo

O vídeo associado a esta prática pode ser acessado no link a seguir:

https://youtu.be/T8cTxiIBelk

*Fonte: autor*

📜 Licença
Este projeto está licenciado sob a Licença MIT. Veja o arquivo LICENSE para mais detalhes.
