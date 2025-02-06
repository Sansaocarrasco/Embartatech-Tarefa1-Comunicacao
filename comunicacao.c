#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "inc/ssd1306.h"
#include "ws2818b.pio.h"
#include "matrizes.c"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C
#define LED_COUNT 25
#define LED_PIN 7
#define BUTTON_A 5
#define BUTTON_B 6
#define LED_VERDE 11
#define LED_AZUL 12
#define TIME_DEBOUNCE 500

ssd1306_t ssd;
PIO np_pio;
uint sm;

volatile uint32_t last_press_time_A = 0;
volatile uint32_t last_press_time_B = 0;
volatile char ultimo_caractere = ' ';
volatile int numero_atual = -1;

volatile bool estado_led_verde = false;
volatile bool estado_led_azul = false;

struct pixel_t {
    uint8_t G, R, B;
};
typedef struct pixel_t npLED_t;

npLED_t leds[LED_COUNT];
void npWrite();

void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    leds[index].R = r;
    leds[index].G = g;
    leds[index].B = b;
}

void npClear() {
    for (uint i = 0; i < LED_COUNT; i++)
        npSetLED(i, 0, 0, 0);
    npWrite();
}

void npInit(uint pin) {
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, true);

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);
    npClear();
}

int getIndex(int x, int y) {
    return (y % 2 == 0) ? (24 - (y * 5 + x)) : (24 - (y * 5 + (4 - x)));
}

void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

void atualizar_display() {
    ssd1306_fill(&ssd, false);
    ssd1306_draw_char(&ssd, ultimo_caractere, 60, 30);
    ssd1306_draw_string(&ssd, estado_led_verde ? "LED Verde ON" : "LED Verde OFF", 5, 5);
    ssd1306_draw_string(&ssd, estado_led_azul ? "LED Azul ON" : "LED Azul OFF", 5, 15);
    ssd1306_send_data(&ssd);
}

void button_callback(uint gpio, uint32_t events) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());

    if (gpio == BUTTON_A && current_time - last_press_time_A > TIME_DEBOUNCE) {
        last_press_time_A = current_time;
        estado_led_verde = !estado_led_verde;
        gpio_put(LED_VERDE, estado_led_verde);
        atualizar_display();
    }

    if (gpio == BUTTON_B && current_time - last_press_time_B > TIME_DEBOUNCE) {
        last_press_time_B = current_time;
        estado_led_azul = !estado_led_azul;
        gpio_put(LED_AZUL, estado_led_azul);
        atualizar_display();
    }
}

void npWriteNumber(int num) {
    if (num >= 0 && num <= 9) {
        npClear();
        for (int linha = 0; linha < 5; linha++) {
            for (int coluna = 0; coluna < 5; coluna++) {
                int posicao = getIndex(linha, coluna);
                npSetLED(posicao, matrizes[num][linha][coluna][0],
                         matrizes[num][linha][coluna][1],
                         matrizes[num][linha][coluna][2]);
            }
        }
        npWrite();
    }
}

void npDrawMatrix(int matriz[5][5][3]) {
    for (int linha = 0; linha < 5; linha++) {
        for (int coluna = 0; coluna < 5; coluna++) {
            int posicao = getIndex(linha, coluna);
            npSetLED(posicao, matriz[coluna][linha][0], matriz[coluna][linha][1], matriz[coluna][linha][2]);
        }
    }
    npWrite();
}

int main() {
    stdio_init_all();

    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_put(LED_VERDE, 0);

    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);
    gpio_put(LED_AZUL, 0);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    npInit(LED_PIN);
    npClear();
    atualizar_display();

    while (true) {
        if (stdio_usb_connected()) {
            char c = getchar();
            if (c != EOF) {
                ultimo_caractere = c;
                if (c >= '0' && c <= '9') {
                    numero_atual = c - '0';
                    npWriteNumber(numero_atual);
                    npDrawMatrix(matrizes[numero_atual]);
                }
                atualizar_display();
            }
        }
        sleep_ms(100);
    }
}
