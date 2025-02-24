/*
 *                    Funcionamento do programa 
 *O programa mostra números de 0 a 9 na matriz de lEDS 5x5, a depender da entrada de um desses 
 números no serial monitor,ao apertar o botão A o LED verde muda de estado, com a informação 
 sendo printada no serial monitor e no display da BitDogLab, o Botão B tem comportamento 
 similar ao botão A mas para funcionamento do LED Azul. Os caracteres inseridos via serial monitor 
 aparecem no Display 
 *                    Tratamento de deboucing com interrupção 
 * A ativação dos botões A e B são feitas através de uma rotina de interrupção, sendo
 * implementada condição para tratar o efeito boucing na rotina.
*/

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/adc.h"

//definições da i2c
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define Frames 10
#define LED_PIN_G 11
#define LED_PIN_B 12//LED que irá simular uma bonba de agua para irrigação 
#define Botao_A 5 // gpio do botão A na BitDogLab
#define Botao_B 6 // gpio do botão B na BitDogLab
#define JOY_Y 26//eixo  y segundo diagrama BitDogLab 
#define JOY_X 27//eixo  x segundo diagrama BitDogLab (Sensor de umidade do Solo resistivo Simulado)

//definição de variáveis que guardarão o estado atual de cada lED
bool led_on_G = false;// LED verde desligado
bool  aux_Bot_B= true;//aulixar para escolher o que mostrar na matriz de LEDs
int aux_G= 1;//variável auxiliar para indicar mudança de estado do LED no display 


// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0;  // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 5; // Intensidade do azul (inicia mostrando umidade do solo na matriz)

//variáveis globais 
static volatile int aux = 1; // posição do numero impresso na matriz, inicialmente imprime numero 5
static volatile uint32_t last_time_A = 0; // Armazena o tempo do último evento para Bot A(em microssegundos)
static volatile uint32_t last_time_B = 0; // Armazena o tempo do último evento para Bot B(em microssegundos)
char c=' ';

bool led_buffer[NUM_PIXELS];// Variável (protótipo)
bool buffer_Numeros[Frames][NUM_PIXELS];// // Variável (protótipo) 
    
//protótipo funções que ligam leds da matriz 5x5
void atualiza_buffer(bool buffer[], bool b[][NUM_PIXELS], int c); ///protótipo função que atualiza buffer
static inline void put_pixel(uint32_t pixel_grb);
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b);
void set_one_led(uint8_t r, uint8_t g, uint8_t b);//liga os LEDs escolhidos 

void gpio_irq_handler(uint gpio, uint32_t events);// protótipo função interrupção para os botões A e B com condição para deboucing

void Estado_LED_Display(int a , int b, ssd1306_t c);// protótipo função que imprime estado LEDs no Display

void Sensor_Matiz_5X5(uint joy_x, uint joy_y);//// protótipo função que escolhe qual sensor será representado na matriz 
void Imprime_5X5(uint rate);// protótipo função que mostra percentual do  sensor  na matriz 5x5
void Monitoramento(uint joy_x, uint joy_y, ssd1306_t c, bool b);// protótipo função que mostra monitoramento dos sensores no Display
void Irrigacao(uint joy_x);// Protótipo de função para acionar bomba d'água 

int main()
{
    //Inicializando I2C . Usando 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);// Definindo a função do pino GPIO para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);// Definindo a função do pino GPIO para I2C
    gpio_pull_up(I2C_SDA);// definindo como resistência de pull up
    gpio_pull_up(I2C_SCL);// definindo como resistência de pull up
    ssd1306_t ssd;// Inicializa a estrutura do display
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);// Inicializa o display
    ssd1306_config(&ssd);// Configura o display
    ssd1306_send_data(&ssd);// Envia os dados para o display
    bool cor = true;
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
  
    //configuração PIO
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    
    stdio_init_all(); // Inicializa comunicação USB CDC para monitor serial
    adc_init();//inicializando adc
    adc_gpio_init(JOY_Y);//inicializando pino direção y 
    adc_gpio_init(JOY_X);//inicializando pino direção x

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    // configuração led RGB verde
    gpio_init(LED_PIN_G);              // inicializa pino do led verde
    gpio_set_dir(LED_PIN_G, GPIO_OUT); // configrua como saída
    gpio_put(LED_PIN_G, 0);            // led inicia apagado
    
    // configuração led RGB azul
    gpio_init(LED_PIN_B);              // inicializa pino do led azul
    gpio_set_dir(LED_PIN_B, GPIO_OUT); // configrua como saída
    gpio_put(LED_PIN_B, 0);            // led inicia apagado
    
    // configuração botão A
    gpio_init(Botao_A);             // inicializa pino do botão A
    gpio_set_dir(Botao_A, GPIO_IN); // configura como entrada
    gpio_pull_up(Botao_A);          // Habilita o pull-up interno

    // configuração botão B
    gpio_init(Botao_B);             // inicializa pino do botão B
    gpio_set_dir(Botao_B, GPIO_IN); // configura como entrada
    gpio_pull_up(Botao_B);          // Habilita o pull-up interno

    // configurando a interrupção com botão na descida para o botão A
    gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // configurando a interrupção com botão na descida para o botão B
    gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    bool collor = true;
    while (1)
    {
        
        adc_select_input(0);//canal adc JOY para eixo y
        uint16_t JOY_Y_value = adc_read(); // Lê o valor do eixo y, de 0 a 4095.
        adc_select_input(1);//canal adc JOY para eixo x
        uint16_t JOY_X_value = (adc_read()/4095.0)*100;// Lê o valor do eixo x, de 0 a 4095.
        printf("Umidade:%d  y:%d\n",JOY_X_value, JOY_Y_value );


        Sensor_Matiz_5X5(JOY_X_value, JOY_Y_value);
       /* if(aux_Bot_B){//mostra percentual de umidade na matriz de LEDs 
            led_b=5;
            led_r=0;
            Imprime_5X5(JOY_X_value);//imprime umidade do solo Matriz de LEDs
        }else{//mostra percentual de temperatura na matriz de LEDs
            led_b=0;
            led_r=5;
            Imprime_5X5(JOY_Y_value/4095.0*100);//imprime umidade do solo Matriz de LEDs
        }*/

        Irrigacao(JOY_X_value);//Liga LED azul simulando acionamento de bomba D'água
        sleep_ms(411);
        Monitoramento(JOY_X_value, JOY_Y_value, ssd, collor);//

    }

    return 0;
}

bool led_buffer[NUM_PIXELS] = { //Buffer para armazenar quais LEDs estão ligados matriz 5x5
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0};

bool buffer_Numeros[Frames][NUM_PIXELS] =//Frames que formam nível de umidade no solo na matriz
    {
      //{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24} referência para posição na BitDogLab matriz 5x5
        {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // para o número zero (até 20% de umidade)
        {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // para o número 1 (entre 20% e 40% de umidade)
        {0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // para o número 2 (entre 40% e 60% de umidade)
        {0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // para o número 3 (entre 60% e 80% de umidade)
        {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // para o número 4 (acima de 80% de umidade)
        {0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0}, // para o número 5
        {0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 1, 0}, // para o número 6
        {0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0}, // para o número 7
        {0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0}, // para o número 8
        {0, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0}  // para o número 9
};

// função que atualiza o buffer de acordo o número de 0 a 9
void atualiza_buffer(bool buffer[],bool b[][NUM_PIXELS], int c)
{
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        buffer[i] = b[c][i];
    }
}

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void set_one_led(uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (led_buffer[i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0); // Desliga os LEDs com zero no buffer
        }
    }
}

// função interrupção para os botões A e B com condição para deboucing
void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());//// Obtém o tempo atual em microssegundos
    if (gpio_get(Botao_A) == 0 &&  (current_time - last_time_A) > 200000)//200ms de boucing adiconado como condição 
    { // se botão A for pressionado e aux<9 incrementa aux em 1(próximo número) 
        last_time_A = current_time; // Atualiza o tempo do último evento
        //led_on_G= !led_on_G;//altera estado LED
        //gpio_put(LED_PIN_G, led_on_G);
        if(led_on_G){
            printf("LED verde ligado\n");//printa estado do LED serial monitor
            aux_G++;//auxiliar para imprimir estado LED display
        }else{
            printf("LED verde desligado\n");//printa estado do LED serial monitor
            aux_G--;//auxiliar para imprimir estado LED display
        }
    }
    if (gpio_get(Botao_B) == 0  && (current_time - last_time_B) > 300000)//200ms de boucing adiconado como condição 
    { // se botão B for pressionado e aux>0 decrementa aux em 1(número anterior)
        last_time_B = current_time; // Atualiza o tempo do último evento
        //led_on_B= !led_on_B;//altera estado LED
        //gpio_put(LED_PIN_B, led_on_B);
        aux_Bot_B=!aux_Bot_B;
       
    }
}
//função que coloca percentual do sensor na matriz de LEDs
void Imprime_5X5(uint rate){//
    if(rate <=20){
        atualiza_buffer(led_buffer, buffer_Numeros, 0); 
        set_one_led(led_r, led_g, led_b);
    }else if(rate >20 && rate<=40){
        atualiza_buffer(led_buffer, buffer_Numeros, 1); 
        set_one_led(led_r, led_g, led_b);
    }else if(rate >40 && rate<=60){
        atualiza_buffer(led_buffer, buffer_Numeros, 2); 
        set_one_led(led_r, led_g, led_b); 
    }else if(rate >60 && rate<80){
        atualiza_buffer(led_buffer, buffer_Numeros, 3); 
        set_one_led(led_r, led_g, led_b); 
    }else if(rate >=80){
        atualiza_buffer(led_buffer, buffer_Numeros, 4); 
        set_one_led(led_r, led_g, led_b);
    }
}
// função que imprime estado LEDs no Display
void Estado_LED_Display(int a , int b, ssd1306_t c){
    if (a == 1)
    {
        ssd1306_draw_string(&c, "LED azul off", 8, 10); // Desenha uma string
    }
    else
    {
        ssd1306_draw_string(&c, "LED azul on  ", 8, 10); // Desenha uma string;
    }
    if (b== 1)
    {
        ssd1306_draw_string(&c, "LED verde off", 8, 20); // Desenha uma string
    }
    else
    {
        ssd1306_draw_string(&c, "LED verde on  ", 8, 20); // Desenha uma string;
    }
}
//Função que mostra monitoramente de temperatura e umidade no display
void Monitoramento(uint joy_x, uint joy_y, ssd1306_t c, bool b){
    char str_x[5];  // Buffer para armazenar a string
        char str_y[5];  // Buffer para armazenar a string
        sprintf(str_x, "%d", joy_x);  // Converte o inteiro em string
        sprintf(str_y, "%d", joy_y);  // Converte o inteiro em string
        ssd1306_fill(&c, !b);                                      // Limpa o display
        ssd1306_rect(&c, 3, 3, 122, 60, b, !b);                  // Desenha um retângulo
        ssd1306_line(&c, 3, 14, 123, 14, b);                   // Desenha uma linha
        ssd1306_draw_string(&c, "MONITORAMENTO", 12, 5);           // Desenha uma string
        ssd1306_draw_string(&c, "UMIDADE", 8, 16);     
        ssd1306_draw_string(&c, " T yC", 77, 16);         // Desenha uma string (y equivale a ° na font.h)
        ssd1306_line(&c, 75, 15, 75, 60, b);                       // Desenha uma linha vertical
        ssd1306_draw_string(&c, str_x, 21, 35);                       // Desenha uma string
        ssd1306_draw_string(&c, "z", 37, 35);//z equivale a % na font.h
        ssd1306_draw_string(&c, str_y, 81, 35);                      // Desenha uma string
        ssd1306_send_data(&c);                                       // Atualiza o display
}
// Função para simular acionamento de bomba d'água com LED azul
void Irrigacao(uint joy_x){
    if(joy_x<=20){//aciona bomda d'agua para irrigação se umidade chegar a 20%
        gpio_put(LED_PIN_B, 1);
    }else if(joy_x>=60){//desliga bomba d'agua para irrigação se umidade chegar a 60%
        gpio_put(LED_PIN_B, 0);
    }
}
//função que escolhe qual sensor será representado na matriz de LEDs 
void Sensor_Matiz_5X5(uint joy_x, uint joy_y){
    if(aux_Bot_B){//mostra percentual de umidade na matriz de LEDs 
        led_b=5;
        led_r=0;
        Imprime_5X5(joy_x);//imprime umidade do solo Matriz de LEDs
    }else{//mostra percentual de temperatura na matriz de LEDs
        led_b=0;
        led_r=5;
        Imprime_5X5(joy_y/4095.0*100);//imprime umidade do solo Matriz de LEDs
        }
}