//BIBLIOTECAS
#include <Drivers/port/port.h>
#include <Drivers/gpio/gpio.h>
#include <Libraries/delay/delay.h>
#include <Drivers/uart/uart0.h>
#include <Drivers/tpm/tpm.h>
#include "stdio.h"
#include "MKL05Z4.h"
#include <stdbool.h>

//******DEFINES******
// LEDS
#define RED_PIN1 GPIOB
#define RED1 8
#define GREEN_PIN1 GPIOB
#define GREEN1 9
#define RED_PIN2 GPIOA
#define RED2 8
#define GREEN_PIN2 GPIOA
#define GREEN2 9


// BOTÕES
#define BUTTON_PIN1 GPIOB
#define BUTTON1 1
#define BUTTON_PIN2 GPIOB
#define BUTTON2 2

//SOLENOIDES
#define SOLENOIDE_PIN1 GPIOB
#define SOLENOIDE1 6
#define SOLENOIDE_PIN2 GPIOB
#define SOLENOIDE2 7

//SENSOR DE FLUXO
#define SENSOR_PIN1 GPIOA
#define SENSOR1 12
#define SENSOR_PIN2 GPIOA
#define SENSOR2 10

//******VARIÁVEIS GLOBAIS******

uint32_t tpmModulo = 65535U; //VALOR DO MODULO DO TPM
float volatile countPulsos1; //Contador de pulsos
float volatile countPulsos2; //Contador de pulsos
float volatile pulsoConversorSensor1 = 0.3; //Multiplicador para conversão de pulsos para MLs
float volatile pulsoConversorSensor2 = 0.3; //Multiplicador para conversão de pulsos para MLs
int volatile conversorValorMLs1; //Conversor de valor de MLs para inteiro
int volatile conversorValorMLs2; //Conversor de valor de MLs para inteiro
int beer = 0;
volatile bool g_BUTTON1_PRESSED = false; //Variável para verificar se o botão 1 foi pressionado
volatile bool g_BUTTON2_PRESSED = false; //Variável para verificar se o botão 2 foi pressionado


typedef enum{    //CASES
	INIT_MCU,
	LOADING_BEER,
	BEER_TAP1,
	BEER_TAP2,
	BEEROPEN_TAP1,
	BEEROPEN_TAP2,
	CLOSE_TAPS,

}beer_t;

// ******ÁREA DE FUNÇÕES*******
void INIT_GPIO() //INICIALIZA GPIO E PINOS
 {
	PORT_Init( PORTA );
	PORT_Init( PORTB );
	//LEDs
	GPIO_InitOutputPin( RED_PIN1, RED1, 1 ); //LED VERMELHO TORNEIRA 1
	GPIO_InitOutputPin( GREEN_PIN1, GREEN1, 1 ); //LED VERDE TORNEIRA 1
	GPIO_InitOutputPin( RED_PIN2, RED2, 1 ); //LED VERMELHO TORNEIRA 2
	GPIO_InitOutputPin( GREEN_PIN2, GREEN2, 1 );  //LED VERDE TORNEIRA 2

	//SET MUX LEDS
	PORT_SetMux(PORTB, RED1, PORT_MUX_AS_GPIO); //LED VERMELHO TORNEIRA 1
	PORT_SetMux(PORTB, GREEN1, PORT_MUX_AS_GPIO); //LED VERDE TORNEIRA 1
	PORT_SetMux(PORTA, RED2, PORT_MUX_AS_GPIO); //LED VERMELHO TORNEIRA 2
	PORT_SetMux(PORTA, GREEN2, PORT_MUX_AS_GPIO); //LED VERDE TORNEIRA 2

	//BOTÕES
	GPIO_InitInputPin( GPIOB, BUTTON1 );
	GPIO_InitInputPin( GPIOB, BUTTON2 );

	//SET MUX BOTÕES
	PORT_SetMux( PORTB, BUTTON1, PORT_MUX_AS_GPIO );
	PORT_SetMux( PORTB, BUTTON2, PORT_MUX_AS_GPIO );
 }

void INIT_PERIF() //INICIALIZA PERIFÉRICOS E DELAY
 {
	//SOLENOIDE
	GPIO_InitOutputPin( SOLENOIDE_PIN1, SOLENOIDE1, 1 ); //SOLENOIDE TORNEIRA 1
	GPIO_InitOutputPin( SOLENOIDE_PIN2, SOLENOIDE2, 1 ); //SOLENOIDE TORNEIRA 2

	//SET MUX SOLENOIDE
	PORT_SetMux(PORTB, SOLENOIDE1, PORT_MUX_AS_GPIO); //SOLENOIDE TORNEIRA 1
	PORT_SetMux(PORTB, SOLENOIDE2, PORT_MUX_AS_GPIO); //SOLENOIDE TORNEIRA 2

	//SENSOR DE FLUXO
	GPIO_InitInputPin( GPIOA, SENSOR1 );
	GPIO_InitInputPin( GPIOA, SENSOR2 );

	//SET MUX SENSOR
	PORT_SetMux(PORTA, SENSOR1, PORT_MUX_AS_GPIO); //SENSOR TORNEIRA 1
	PORT_SetMux(PORTA, SENSOR2, PORT_MUX_AS_GPIO); //SENSOR TORNEIRA 2

	//INICIA DELAY
	Delay_Init();
 }

void INIT_UART0()
{
	UART0_SetClkSrc( UART0_CLOCK_FLL );
	UART0_Init( 115200, UART0_TX_RX_ENABLE, UART0_NO_PARITY, UART0_ONE_STOP_BIT ); //INICIALIZA UART0 COM 115200 BAUDRATE, TRANSMISSÃO E RECEPÇÃO HABILITADOS, PARIDADE DESABILITADA E 1 STOP BIT
}

void INIT_TPM() //INICIALIZA TPM
 {
	TPM_SetCounterClkSrc( TPM0, TPM_CNT_CLOCK_FLL );
	TPM_Init( TPM0, tpmModulo, TPM_PRESCALER_DIV_128 );
	TPM_EnableIRQ( TPM0 );
	NVIC_EnableIRQ( TPM0_IRQn );

	TPM_SetCounterClkSrc( TPM1, TPM_CNT_CLOCK_FLL );
	TPM_Init( TPM1, tpmModulo, TPM_PRESCALER_DIV_128 );
	TPM_EnableIRQ( TPM1 );
	NVIC_EnableIRQ( TPM1_IRQn );
 }

void INIT_IRC()//INICIALIZA INTERRUPÇÕES
 {
	// INTERRUPÇÃO BOTÃO
	PORT_EnablePull( PORTB, BUTTON1 );
	PORT_EnableIRQ( PORTB, BUTTON1, PORT_IRQ_ON_RISING_EDGE );
	PORT_EnablePull( PORTB, BUTTON2 );
	PORT_EnableIRQ( PORTB, BUTTON2, PORT_IRQ_ON_RISING_EDGE );
    NVIC_EnableIRQ(PORTB_IRQn);

    // INTERRUPÇÃO SENSOR
    PORT_EnablePull( PORTA, SENSOR1 );
    PORT_EnableIRQ( PORTA, SENSOR1, PORT_IRQ_ON_RISING_EDGE );
    PORT_EnablePull( PORTA, SENSOR2 );
    PORT_EnableIRQ( PORTA, SENSOR2, PORT_IRQ_ON_RISING_EDGE );
    NVIC_EnableIRQ(PORTA_IRQn);

 }

void RED1_OFF() // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 1
{
	RED_PIN1->PDOR &= ~(1 << RED1);
}

void RED2_OFF() // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 2
{
	RED_PIN2->PDOR &= ~(1 << RED2);
}

void GREEN1_OFF() // FUNÇÃO PARA LIGAR LED VERDE TORNEIRA 1
{
	GREEN_PIN1->PDOR &= ~(1 << GREEN1);
}

void GREEN2_OFF() // FUNÇÃO PARA LIGAR LED VERDE TORNEIRA 2
{
	GREEN_PIN2->PDOR &= ~(1 << GREEN2);
}

void RED1_ON() // FUNÇÃO PARA DESLIGAR LED VERMELHO TORNEIRA 1
{
	RED_PIN1->PDOR |= (1 << RED1);
}

void RED2_ON() // FUNÇÃO PARA DESLIGAR LED VERMELHO TORNEIRA 2
{
	RED_PIN2->PDOR |= (1 << RED2);
}

void GREEN1_ON() // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 1
{
	GREEN_PIN1->PDOR |= (1 << GREEN1);
}

void GREEN2_ON()  // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 2
{
	GREEN_PIN2->PDOR |= (1 << GREEN2);
}

void SOLENOIDE1_OPEN(void) // FUNÇÃO PARA ABRIR SOLENOIDE TORNEIRA 1
{
	SOLENOIDE_PIN1->PDOR &= ~(1 << SOLENOIDE1);
}

void SOLENOIDE2_OPEN(void) // FUNÇÃO PARA ABRIR SOLENOIDE TORNEIRA 2
{
	SOLENOIDE_PIN2->PDOR &= ~(1 << SOLENOIDE2);
}

void SOLENOIDE1_CLOSE(void) // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 1
{
	SOLENOIDE_PIN1->PDOR |= (1 << SOLENOIDE1);
}

void SOLENOIDE2_CLOSE(void) // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 2
{
	SOLENOIDE_PIN2->PDOR |= (1 << SOLENOIDE2);
}

void TPM0_IRQHandler(void)
{
	if( TPM_GetIRQFlag( TPM0 ) )
    {
		Delay_Waitms(20);

	printf("CONTADOR:\n");
	printf("%d",conversorValorMLs1);

    }

	TPM_ClearIRQFlag( TPM0 );

}

void TPM1_IRQHandler(void)
{
	if( TPM_GetIRQFlag( TPM1 ) )
    {
		Delay_Waitms(20);

	printf("CONTADOR 2:\n");
	printf("%d",conversorValorMLs2);

    }

	TPM_ClearIRQFlag( TPM1 );

}

void PORTB_IRQHandler( void ) // INTERRUPÇÃO BOTÃO
{
	if ( PORT_GetIRQFlag( PORTB, BUTTON1 ) )  //INTERRUPÇÃO BOTÃO 1
    {
		Delay_Waitms(50); //DELAY DEBOUNCE
        PORT_ClearIRQFlag( PORTB, 1 ); //LIMPA FLAG DE INTERRUPÇÃO
        GPIO_TogglePin( GPIOB, 9 );
        Delay_Waitms(500);
        GPIO_TogglePin( GPIOB, 9 );
        beer = BEER_TAP1;
    }
	if (PORT_GetIRQFlag( PORTB, BUTTON2)) {
		Delay_Waitms(50); //DELAY DEBOUNCE
		PORT_ClearIRQFlag( PORTB, 2);
		GPIO_TogglePin( GPIOB, 9);
		Delay_Waitms(500);
		GPIO_TogglePin( GPIOB, 9);
        beer = BEER_TAP2;
	}
}

void PORTA_IRQHandler( void ) // INTERRUPÇÃO SENSOR
{
	if ( PORT_GetIRQFlag( PORTA, 10 ) )// PTA10 causou interrupção?
    {
        //Limpa a flag de ISR do PTA10
        PORT_ClearIRQFlag( PORTA, 10 );

        //Inverte valor lógico de PTB9
        Delay_Waitms(10);
        GPIO_TogglePin( GPIOA, 9 );
        countPulsos1++;
    }
	if ( PORT_GetIRQFlag( PORTA, 12 ) )// PTA10 causou interrupção?
    {
        //Limpa a flag de ISR do PTA10
        PORT_ClearIRQFlag( PORTA, 12 );

        //Inverte valor lógico de PTB9
        Delay_Waitms(10);
        GPIO_TogglePin( GPIOB, 9 );
        countPulsos2++;
    }
}

int main(void)
{

	//inicialize as funções de perifericos
	INIT_GPIO();
	INIT_PERIF();
	INIT_UART0();
	INIT_TPM();
	INIT_IRC();

	 for ( ; ; )
    {
		 switch (beer) { //ESTADOS

		 	case INIT_MCU: //ESTADO INICIAL

		 		RED1_ON(); // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 1
		 		RED2_ON(); // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 2
		 		GREEN1_OFF(); // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 1
		 		GREEN2_OFF(); // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 2
		 		SOLENOIDE1_CLOSE(); // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 1
		 		SOLENOIDE2_CLOSE(); // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 2
		 		printf ("MHL SOLUCOES \n ");
		 		g_BUTTON1_PRESSED = false;
		 		g_BUTTON2_PRESSED = false;
		 		Delay_Waitms(4000); //DELAY DE 4 SEGUNDOS
		 		beer = LOADING_BEER; //CASO TIMEOUT = 4 SEGUNDOS, VAI PARA ESTADO LOADING_BEER
		 		break;

		 	case LOADING_BEER: //ESTADO AGUARDANDO BOTÃO PRESSIONADO

		 		 RED1_ON(); // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 1
		 		 RED2_ON(); // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 2
		 		 GREEN1_OFF(); // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 1
		 		 GREEN2_OFF(); // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 2
		 		printf("DISPONIVEL     DISPONIVEL\n");
		 		NVIC_EnableIRQ(PORTB_IRQn);
		 		Delay_Waitms(500);
		 		 SOLENOIDE1_CLOSE(); // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 1
		 		 SOLENOIDE2_CLOSE(); // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 2
		 		break;


		 	case BEER_TAP1:

			if (!g_BUTTON1_PRESSED) {
				RED1_OFF(); // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 1
				GREEN1_ON(); // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 1
				printf("LOADING time     AGUARDE\n");
				 NVIC_DisableIRQ(PORTB_IRQn);
				SOLENOIDE1_CLOSE(); // FUNÇÃO PARA abrir SOLENOIDE TORNEIRA 1
				printf("3 segundos\n");
				Delay_Waitms(1000); //DELAY DE 3 SEGUNDOS
				printf("2 segundos\n");
				Delay_Waitms(1000); //DELAY DE 2 SEGUNDOS
				printf("1 segundo\n");
				Delay_Waitms(1000); //DELAY DE 1 SEGUNDOS
				printf("SIRVA-SE TORNEIRA 1\n");
				Delay_Waitms(1000); //DELAY DE 1 SEGUNDOS
				NVIC_EnableIRQ(PORTA_IRQn);
				SOLENOIDE2_OPEN();
				TPM_InitCounter(TPM0);
				beer = BEEROPEN_TAP1;
			}
			break;

		 	case BEER_TAP2:

			if (!g_BUTTON2_PRESSED) {
				RED2_OFF(); // FUNÇÃO PARA LIGAR LED VERMELHO TORNEIRA 1
				GREEN2_ON(); // FUNÇÃO PARA DESLIGAR LED VERDE TORNEIRA 1
				printf("LOADING time     AGUARDE\n");
				NVIC_DisableIRQ(PORTB_IRQn);
				SOLENOIDE2_CLOSE(); // FUNÇÃO PARA FECHAR SOLENOIDE TORNEIRA 1
				printf("3 segundos\n");
				Delay_Waitms(1000); //DELAY DE 3 SEGUNDOS
				printf("2 segundos\n");
				Delay_Waitms(1000); //DELAY DE 2 SEGUNDOS
				printf("1 segundo\n");
				Delay_Waitms(1000); //DELAY DE 1 SEGUNDOS
				printf("SIRVA-SE TORNEIRA 2\n");
				Delay_Waitms(1000); //DELAY DE 1 SEGUNDOS
				NVIC_EnableIRQ(PORTA_IRQn);
				SOLENOIDE1_OPEN();
				TPM_InitCounter(TPM1);
				beer = BEEROPEN_TAP2;
			}
			break;
		case BEEROPEN_TAP1:

			conversorValorMLs1 = pulsoConversorSensor1 * countPulsos1;
			if (conversorValorMLs1 >= 200) {
				TPM_StopCounter(TPM0);
				Delay_Waitms(100);
				printf("CONSUMO FINALIZADO 1\n");
				Delay_Waitms(1000);
				conversorValorMLs1 = 0;
				countPulsos1 = 0;
				beer = LOADING_BEER;

			}
			break;

		case BEEROPEN_TAP2:

			conversorValorMLs2 = pulsoConversorSensor2 * countPulsos2;
			if (conversorValorMLs2 == 200) {
				TPM_StopCounter(TPM1);
				Delay_Waitms(100);
				printf("CONSUMO FINALIZADO 2\n");
				Delay_Waitms(1000);
				conversorValorMLs2 = 0;
				countPulsos2 = 0;
				beer = LOADING_BEER;
			}
			break;

		 }


	}
}
