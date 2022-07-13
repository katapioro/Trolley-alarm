/*********************************************************************************************
 * Systemy mikroprocesorowe 2
 * Elektronika rok 3, semestr 5
 * Projekt: Trolley alarm
 * Autorzy: Katarzyna Pióro, Bartosz Sajdak
 * Uwagi: Dla celów projektu skorzystano z bibliotek udostepnionych w ramach laboratorium SM2
 *********************************************************************************************/
					
#include "MKL05Z4.h"
#include "frdm_bsp.h" 
#include "led.h" 
#include "uart.h" 
#include "stdio.h"
#include "i2c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//--------------UART--------------//
#define LF	0xa		// Enter
char rx_buf[16];
char alarm_ON[]="SET ON";
char alarm_OFF[]="SET OFF";
char alarm_RESET[]="RESET";
char Error[]="Zla komenda";
char Too_Long[]="Zbyt dlugi ciag";
uint8_t rx_buf_pos=0;
char temp,buf;
uint8_t rx_FULL=0;
uint8_t too_long=0;
//--------------Zegar--------------//
static uint8_t msTicks = 0;
static uint8_t newTick = 0;
//----------Akcelerometr----------//
static char tmpOut[99];
static uint8_t readout = 0;
static uint8_t arrayXYZ[6];

//----------Przerwanie od UART0---------//
void UART0_IRQHandler()
{
	if(UART0->S1 & UART0_S1_RDRF_MASK)
	{
		temp=UART0->D;	// Odczyt wartosci z bufora odbiornika i skasowanie flagi RDRF
		if(!rx_FULL)
		{
			if(temp!=LF)
			{
				if(!too_long)	// Jesli za dlugi ciag, ignoruj reszte znaków
				{
					rx_buf[rx_buf_pos] = temp;	// Kompletuj komende
					rx_buf_pos++;
					if(rx_buf_pos==16)
						too_long=1;		// Za dlugi ciag
				}
			}
			else
			{
				if(!too_long)	// Jesli za dlugi ciag, porzuc tablice
					rx_buf[rx_buf_pos] = 0;
				rx_FULL=1;
			}
		}
	NVIC_EnableIRQ(UART0_IRQn);
	}
}

//------------------------------Main------------------------------//
int main (void)
{	
	double X_axis, Y_axis, Z_axis; // inicjalizacja zmiennych dla akcelerometru
	uint8_t i;
	uint8_t alarmFlag = 0; //flaga czy alarm jest aktywny
	uint8_t alarmSet = 0; //flaga czy alarm jest uzbrojony
	
	SysTick_Config(1000000); 								/* initialize system timer */
	
	LED_Init ();	 													/* initialize all LEDs */ 
	LED_Welcome();  												/* blink with all LEDs */
	
	UART_Init(9600);												// Inicjalizacja portu szeregowego UART0
	UART_Println("\n\rSystemy mikroprocesorowe 2: Projekt - Trolley alarm");
	
	I2C_Init();		// initalize I2C
	
	//-----------------------Ustawienia poczatkowe-----------------------//
	if(alarmSet == 1) {
			LED_Ctrl(LED_RED, LED_ON); // LED RED ON
		}
	else {
			LED_Ctrl(LED_GREEN, LED_ON);  // LED GREEN ON
	}
		
	//-------------------------------------------PETLA GLÓWNA-------------------------------------------//
	while(1)
	{
		
		if (alarmFlag != 1 && alarmSet == 1) { //Jesli alarm jest uzbrojony, ale nieaktywny
			if (newTick) {
				newTick = 0;
				
				//----------------Cykliczne wypisywainie pomiarów z akcelerometru na UARTa----------------//
				if( msTicks%50 == 0) {
					I2C_ReadRegBlock(0x1D, 0x01, 8, arrayXYZ);
					X_axis = ((double)((int16_t)((arrayXYZ[0]<<8)|arrayXYZ[1])>>2)/4096);
					sprintf(tmpOut,"X: %+1.6fg",X_axis); // default 4096 counts/g sensitivity
					UART_Println(tmpOut);
					
					Y_axis = ((double)((int16_t)((arrayXYZ[2]<<8)|arrayXYZ[3])>>2)/4096);
					sprintf(tmpOut,"Y: %+1.6fg", Y_axis); // default 4096 counts/g sensitivity
					UART_Println(tmpOut);
					
					Z_axis = ((double)((int16_t)((arrayXYZ[4]<<8)|arrayXYZ[5])>>2)/4096);
					sprintf(tmpOut,"Z: %+1.6fg", Z_axis); // default 4096 counts/g sensitivity
					UART_Println(tmpOut);
					
					if(alarmSet==1 && ((X_axis > 0.15 || X_axis < -0.15) || (Y_axis > 0.15 || Y_axis < -0.15)))
						alarmFlag = 1;
				}
			}
			
		} //------------------------------------------Aktywcja alarmu------------------------------------------//
		else if(alarmFlag == 1) {
				UART_Println("!!!------------Alarm------------!!!"); //komunikat o alarmie
				LED_Ctrl(LED_GREEN, LED_OFF);
				LED_Ctrl(LED_BLUE, LED_OFF);
				LED_Ctrl(LED_RED, LED_OFF);
				LED_Blink(LED_RED, 70);  //RED LED - blink - bedzie migac cyklicznie dopoki nie zmieni sie flaga alarmFlag
		}
		
		//------------------------------------------Reakcja na komendy z terminala------------------------------------------//
		if(rx_FULL)		// Czy dana gotowa?
		{
			if(too_long)
			{
				for(i=0;Too_Long[i]!=0;i++)	// Zbyt dlugi ciag
					{
						while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Czy nadajnik gotowy?
						UART0->D = Too_Long[i];
					}
					while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Czy nadajnik gotowy?
					UART0->D = 0xa;		// Nastepna linia
					too_long=0;
			}
			else 
			{	//////------------------------------------------UART USB------------------------------------------//////
				if(strcmp (rx_buf,alarm_ON)==0)	//"SET ON" - uzbraja alarm, bedzie czekac na warunek alarmu zeby sie wlaczyc
				{
					LED_Ctrl(LED_GREEN, LED_OFF);
					LED_Ctrl(LED_BLUE, LED_OFF);
					LED_Ctrl(LED_RED, LED_ON); // RED LED - swieci sie
					alarmFlag = 0; //nieaktywny
					alarmSet = 1; //ubrojony
				}
				else if(strcmp (rx_buf,alarm_RESET)==0) //"RESET" - wylacza aktywowany alarm, ale nadal jest uzbrojony, bedzie czekac na warunek alarmu zeby sie wlaczyc
				{
					LED_Ctrl(LED_RED, LED_OFF);
					LED_Ctrl(LED_GREEN, LED_OFF);
					LED_Blink(LED_BLUE, 70);	// BLUE LED - blink
					LED_Ctrl(LED_RED, LED_ON);
					
					alarmFlag = 0; //nieaktywny
					alarmSet = 1;  //ubrojony
				}
				else if(strcmp (rx_buf,alarm_OFF)==0) //"SET OFF" - wylacza alarm jesli jest aktywowany i go rozbraja (nie bedzie reagowal na warunek alarmu)
				{
					LED_Ctrl(LED_BLUE, LED_OFF);
					LED_Ctrl(LED_RED, LED_OFF);
					LED_Ctrl(LED_GREEN, LED_ON);	// GREEN LED - turn ON
					
					alarmFlag = 0; //nieaktywny
					alarmSet = 0; //nieuzbrojony
				}	//////------------------------------------------UART Bluetooth------------------------------------------//////				
				else if(alarmSet ==1 && alarmFlag == 1 && strcmp (rx_buf,"cokolwiek")!=0) //"RESET" w wersji bluetooth bo inaczej nie rozumie
				{
					LED_Ctrl(LED_GREEN, LED_OFF);
					LED_Ctrl(LED_BLUE, LED_OFF);
					LED_Ctrl(LED_RED, LED_OFF);
					LED_Blink(LED_BLUE, 70);	//BLUE LED - Blink
					LED_Ctrl(LED_RED, LED_ON); //RED LED - swieci
					
					alarmFlag = 0; //nieaktywny
					alarmSet = 1;  //uzbrojony
					
				}
				else if(alarmFlag == 0 && alarmSet ==1 && strcmp (rx_buf,"cokolwiek")!=0) //"SET OFF/ON" w wersji bluetooth bo inaczej nie rozumie
				{
					
					LED_Ctrl(LED_BLUE, LED_OFF);
					LED_Ctrl(LED_RED, LED_OFF);
					LED_Ctrl(LED_GREEN, LED_ON);	// GREEN LED - swieci
					
					alarmFlag = 0; //nieaktywny
					alarmSet = 0; //nieuzbrojony
				}	
				else if(alarmFlag == 0 && alarmSet ==0 && strcmp (rx_buf,"cokolwiek")!=0) //"SET ON/OFF" w wersji bluetooth bo inaczej nie rozumie
				{
					LED_Ctrl(LED_GREEN, LED_OFF);
					LED_Ctrl(LED_BLUE, LED_OFF);
					LED_Ctrl(LED_RED, LED_ON); // RED LED - swieci
					
					alarmFlag = 0; //nieaktywny
					alarmSet = 1;  //ubrojony
				}						
				else //-----------Inny przypadek komendy-----------//
				{
					if(alarmFlag == 1) {
						//-----------Wylaczenie wszystkich aktywnych diód-----------//
						LED_Ctrl(LED_GREEN, LED_OFF);
						LED_Ctrl(LED_BLUE, LED_OFF);
						LED_Ctrl(LED_RED, LED_OFF);
						
						UART_Println("!!!------------Alarm------------!!!"); //komunikat o alarmie
					}
					else //-----------Zla komenda-----------//
						{
						for(i=0;Error[i]!=0;i++)	// Zla komenda
						{
							while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Czy nadajnik gotowy?
							UART0->D = Error[i];
						}
						while(!(UART0->S1 & UART0_S1_TDRE_MASK));	// Czy nadajnik gotowy?
						UART0->D = 0xa;		// Nastepna linia
					}
				}
			}
			rx_buf_pos=0;
			rx_FULL=0;	// Dana skonsumowana
		}
	}
}
/**
 * @brief System time update. 
 */
void SysTick_Handler(void) {
	msTicks++;
	newTick = 1;
}