#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "inc/ssd1306.h"
#include "ws2818b.pio.h"
#include "matrizes.c"

// Definições de pinos e parâmetros
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C  // Endereço do display OLED
#define LED_COUNT 25  // Número de LEDs no array
#define LED_PIN 7  // Pino de controle dos LEDs
#define BUTTON_A 5  // Pino do botão A
#define BUTTON_B 6  // Pino do botão B
#define LED_VERDE 11  // Pino do LED verde
#define LED_AZUL 12  // Pino do LED azul
#define TIME_DEBOUNCE 500  // Tempo de debounce para os botões (em milissegundos)

ssd1306_t ssd;  // Instância do display OLED
PIO np_pio;  // Instância do PIO (para controle de LEDs WS2818B)
uint sm;  // Estado da máquina de estados do PIO

// Variáveis de controle de botões e LEDs
volatile uint32_t last_press_time_A = 0;
volatile uint32_t last_press_time_B = 0;
volatile char ultimo_caractere = ' ';
volatile int numero_atual = -1;

volatile bool estado_led_verde = false;
volatile bool estado_led_azul = false;

// Estrutura para definir um pixel de LED RGB
struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t npLED_t;

// Array de LEDs com 25 LEDs (configuração para uma matriz 5x5)
npLED_t leds[LED_COUNT];

// Função para atualizar os LEDs
void npWrite();

// Função para configurar a cor de um LED específico
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

// Função para limpar todos os LEDs
void npClear() {
    for (uint i = 0; i < LED_COUNT; i++)
        npSetLED(i, 0, 0, 0);  // Define todos os LEDs como apagados
    npWrite();  // Aplica a alteração
}

// Função para inicializar o controle dos LEDs WS2818B
void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);  // Adiciona o programa WS2818 ao PIO
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);  // Reivindica a máquina de estados disponível

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);  // Inicializa o PIO com os parâmetros apropriados
    npClear();  // Limpa os LEDs
}

// Função que converte as coordenadas (x, y) para um índice no array de LEDs
int getIndex(int x, int y) {
    return (y % 2 == 0) ? (24 - (y * 5 + x)) : (24 - (y * 5 + (4 - x)));
}

// Função para enviar os dados dos LEDs para o hardware
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);  // Envia o valor do LED (G, R, B)
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);  // Aguarda um pequeno tempo para garantir que os LEDs recebam os dados
}

// Função para atualizar o display OLED com informações de estado
void atualizar_display() {
    ssd1306_fill(&ssd, false);  // Limpa o display
    ssd1306_draw_char(&ssd, ultimo_caractere, 60, 30);  // Desenha o último caractere
    ssd1306_draw_string(&ssd, estado_led_verde ? "LED Verde ON" : "LED Verde OFF", 5, 5);  // Exibe o estado do LED verde
    ssd1306_draw_string(&ssd, estado_led_azul ? "LED Azul ON" : "LED Azul OFF", 5, 15);  // Exibe o estado do LED azul
    ssd1306_send_data(&ssd);  // Envia os dados ao display
}

// Função de callback para lidar com pressionamento de botões
void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());  // Tempo atual (em milissegundos)

    // Botão A
    if (gpio == BUTTON_A && current_time - last_press_time_A > TIME_DEBOUNCE) {
        last_press_time_A = current_time;  // Atualiza o tempo do último pressionamento
        estado_led_verde = !estado_led_verde;  // Inverte o estado do LED verde
        gpio_put(LED_VERDE, estado_led_verde);  // Aplica o novo estado no LED
        atualizar_display();  // Atualiza o display OLED
    }

    // Botão B
    if (gpio == BUTTON_B && current_time - last_press_time_B > TIME_DEBOUNCE) {
        last_press_time_B = current_time;  // Atualiza o tempo do último pressionamento
        estado_led_azul = !estado_led_azul;  // Inverte o estado do LED azul
        gpio_put(LED_AZUL, estado_led_azul);  // Aplica o novo estado no LED
        atualizar_display();  // Atualiza o display OLED
    }
}

// Função para desenhar o número no display de LEDs
void npWriteNumber(int num) {
    if (num >= 0 && num <= 9) {  // Verifica se o número está dentro do intervalo válido
        npClear();  // Limpa os LEDs
        for (int linha = 0; linha < 5; linha++) {  // Itera sobre as linhas da matriz 5x5
            for (int coluna = 0; coluna < 5; coluna++) {  // Itera sobre as colunas
                int posicao = getIndex(linha, coluna);  // Calcula a posição do LED
                npSetLED(posicao, matrizes[num][linha][coluna][0],
                         matrizes[num][linha][coluna][1],
                         matrizes[num][linha][coluna][2]);  // Define a cor do LED
            }
        }
        npWrite();  // Atualiza os LEDs
    }
}

// Função para desenhar uma matriz personalizada de LEDs
void npDrawMatrix(int matriz[5][5][3]) {
    for (int linha = 0; linha < 5; linha++) {  // Itera sobre as linhas da matriz 5x5
        for (int coluna = 0; coluna < 5; coluna++) {  // Itera sobre as colunas
            int posicao = getIndex(linha, coluna);  // Calcula a posição do LED
            npSetLED(posicao, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);  // Define a cor do LED
        }
    }
    npWrite();  // Atualiza os LEDs
}

// Função principal
int main() {
    stdio_init_all();  // Inicializa a comunicação serial

    // Configura a interface I2C para o display OLED
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o display OLED
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Configura os LEDs e botões
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);  // Inicia o LED verde apagado

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);  // Inicia o LED azul apagado

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);  // Ativa o pull-up para o botão A

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);  // Ativa o pull-up para o botão B

    // Configura as interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    // Inicializa o controle dos LEDs WS2818B
    npInit(LED_PIN);
    npClear();  // Limpa os LEDs no início
    atualizar_display();  // Atualiza o display com o estado inicial

    while (true) {
        if (stdio_usb_connected()) {  // Verifica se a comunicação USB está conectada
            char c = getchar();  // Lê um caractere da entrada
            if (c != EOF) {  // Se for um caractere válido
                ultimo_caractere = c;  // Armazena o caractere
                if (c >= '0' && c <= '9') {  // Se for um número
                    numero_atual = c - '0';  // Converte o caractere para um número
                    //npWriteNumber(numero_atual);  // Desenha o número (comentado)
                    npDrawMatrix(matrizes[numero_atual]);  // Desenha a matriz associada ao número
                }
                atualizar_display();  // Atualiza o display OLED
            }
        }
        sleep_ms(100);  // Aguarda 100 ms antes de continuar o loop
    }
}
