#include <stdbool.h>

#include <stdint.h>

#include <stdio.h>

#include <inttypes.h>

#include <string.h>

#include "inc/hw_ints.h"

#include "inc/hw_types.h"

#include "inc/hw_memmap.h"

#include "inc/tm4c123gh6pm.h"

#include "driverlib/interrupt.h"

#include "driverlib/sysctl.h"

#include "driverlib/sysctl.c"

#include "driverlib/gpio.h"

#include "driverlib/pin_map.h"

#include "driverlib/uart.h"

#include "utils/uartstdio.h"

#include "utils/uartstdio.c"

#include "inc/hw_uart.h"

#include "driverlib/pwm.h"

#include "driverlib/rom.h"

#include "inc/tm4c123gh6pm.h"

#include "driverlib/debug.h"

#include <stdlib.h>

#include "driverlib/adc.h"

#include "driverlib/adc.c"

#define FRECUENCIA 250

double CARGA;

volatile double POTENCIA = 1;
volatile double POTENCIA_ANTERIOR = 0;
volatile double ANCHO_PULSO;

uint32_t PWMCLOCK = 0;
uint32_t ASSENTS = 0;
uint32_t PENT_POTENCIA = 0;
uint32_t MUESTRA = 0;
uint32_t VALOR_ADC;

volatile uint32_t VALOR_ADC_PROMEDIO;
volatile uint32_t PENT_ANCHO_PULSO;
volatile uint32_t ui32Status;

char TX[2];
char RX[2];
char ARRAY_PENT_POTENCIA[3];

void PWMConfig(void);

// FUNCIÓN PARA ESCRIBIR TEXTOS SOBRE LA TERMINAL
void BLUETHOOTHMessege(char * array) {
  while ( * array) {
    UARTCharPut(UART3_BASE, * array);
    array++;
  }
}
void UARTIntHandler(void) {
  ui32Status = UARTIntStatus(UART3_BASE, true);
  UARTIntClear(UART3_BASE, ui32Status);

  while (UARTCharsAvail(UART3_BASE)) {
    RX[0] = UARTCharGet(UART3_BASE);
    RX[1] = UARTCharGetNonBlocking(UART3_BASE);

    UARTCharPut(UART3_BASE, RX[0]);
    UARTCharPut(UART3_BASE, '\r');
    UARTCharPut(UART3_BASE, '\n');
    if (!GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_0)) {
      if (RX[0] == '1') {
        POTENCIA++;

        if (POTENCIA > 100) {
          POTENCIA = 100;
        }
      }
      if (RX[0] == '2') {
        POTENCIA--;
        if (POTENCIA < 1) {
          POTENCIA = 1;
        }
      }
      if (RX[0] == 'A') {
        POTENCIA = 1;
      }
      if (RX[0] == 'B') {
        POTENCIA = 15;
      }
      if (RX[0] == 'C') {
        POTENCIA = 25;
      }
      if (RX[0] == 'D') {
        POTENCIA = 35;
      }
      if (RX[0] == 'E') {
        POTENCIA = 50;
      }
      if (RX[0] == 'F') {
        POTENCIA = 65;
      }
      if (RX[0] == 'G') {
        POTENCIA = 75;
      }
      if (RX[0] == 'H') {
        POTENCIA = 100;
      }

      // CONVERSIÓN A CADENA DE CARACTERES
      ltoa(POTENCIA, ARRAY_PENT_POTENCIA, 10);

      // PSEUDO CONCATENACIÓN PARA MOSTRAR EN LA TERMINAL
      UARTCharPut(UART3_BASE, '\r');
      UARTCharPut(UART3_BASE, '\n');
      BLUETHOOTHMessege("CICLO DE TRABAJO:= ");
      UARTCharPut(UART3_BASE, ARRAY_PENT_POTENCIA[0]);
      UARTCharPut(UART3_BASE, ARRAY_PENT_POTENCIA[1]);
      UARTCharPut(UART3_BASE, ARRAY_PENT_POTENCIA[2]);
      BLUETHOOTHMessege("%");
      UARTCharPut(UART3_BASE, '\r');
      UARTCharPut(UART3_BASE, '\n');

      // PARTE ENTERA
      ANCHO_PULSO = (CARGA / 100) * POTENCIA;
      PENT_ANCHO_PULSO = (int) ANCHO_PULSO;
      PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, PENT_ANCHO_PULSO);
    }
  }
}

int main(void) {
  SysCtlClockSet(SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_SYSDIV_2_5);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
  GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, GPIO_PIN_0);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_6 | GPIO_PIN_7);
  GPIOPinConfigure(GPIO_PC6_U3RX);
  GPIOPinConfigure(GPIO_PC7_U3TX);
  UARTConfigSetExpClk(UART3_BASE, SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE | UART_CONFIG_STOP_ONE));
  IntMasterEnable();
  IntEnable(INT_UART3);
  UARTIntEnable(UART3_BASE, UART_INT_RX | UART_INT_RT);
  UARTEnable(UART3_BASE);
  PWMConfig();
  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
  GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
  ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_IE | ADC_CTL_END | ADC_CTL_CH10);
  ADCSequenceEnable(ADC0_BASE, 0);
  BLUETHOOTHMessege("CONFIGURADO CON EXITO\r\n");
  while (1) {
    if (GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_0)) {
      IntDisable(INT_UART3);
      ADCProcessorTrigger(ADC0_BASE, 0);
      while (!ADCIntStatus(ADC0_BASE, 0, false)) {}
      ADCSequenceDataGet(ADC0_BASE, 0, & VALOR_ADC);

      // PROMEDIO DE 50000 MEDICIONES
      if (MUESTRA <= 49999) {
        VALOR_ADC_PROMEDIO = VALOR_ADC_PROMEDIO + VALOR_ADC;
        MUESTRA++;
      }

      if (MUESTRA > 49999) {
        MUESTRA = 0;
        VALOR_ADC_PROMEDIO = VALOR_ADC_PROMEDIO / 50000;
        POTENCIA = VALOR_ADC_PROMEDIO;
        POTENCIA = POTENCIA / 2;
        POTENCIA = POTENCIA / 2;
        POTENCIA = POTENCIA / 4;
        POTENCIA = POTENCIA / 4;
        POTENCIA = POTENCIA / 8;
        POTENCIA = POTENCIA / 8;
        POTENCIA = POTENCIA * 100;
        // CONVERSIÓN A CADENA DE CARACTERES
        ltoa(POTENCIA, ARRAY_PENT_POTENCIA, 10);
        // PSEUDO CONCATENACIÓN PARA MOSTRAR EN LA TERMINAL
        UARTCharPut(UART3_BASE, '\r');
        UARTCharPut(UART3_BASE, '\n');
        BLUETHOOTHMessege("CICLO DE TRABAJO:= ");
        UARTCharPut(UART3_BASE, ARRAY_PENT_POTENCIA[0]);
        UARTCharPut(UART3_BASE, ARRAY_PENT_POTENCIA[1]);
        UARTCharPut(UART3_BASE, ARRAY_PENT_POTENCIA[2]);
        BLUETHOOTHMessege("%");
        UARTCharPut(UART3_BASE, '\r');
        UARTCharPut(UART3_BASE, '\n');
        VALOR_ADC_PROMEDIO = 0;

        if (POTENCIA == 0) {
          POTENCIA = 1;
        }
        // PARTE ENTERA
        ANCHO_PULSO = (CARGA / 100) * POTENCIA;
        PENT_ANCHO_PULSO = (int) ANCHO_PULSO;

        PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, PENT_ANCHO_PULSO);

      }

    }

    if (!GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_0)) {
      IntEnable(INT_UART3);
    }

  }

}
void PWMConfig(void) {
  SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
  PWMCLOCK = SysCtlClockGet() / 64;
  CARGA = (PWMCLOCK / FRECUENCIA) - 1;
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM0);
  GPIOPinTypePWM(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);
  GPIOPinConfigure(GPIO_PC4_M0PWM6);
  GPIOPinConfigure(GPIO_PC5_M0PWM7);

  PWMGenConfigure(PWM0_BASE, PWM_GEN_3, PWM_GEN_MODE_DOWN);
  PWMGenPeriodSet(PWM0_BASE, PWM_GEN_3, CARGA);
  PWMGenEnable(PWM0_BASE, PWM_GEN_3);
  PWMOutputState(PWM0_BASE, PWM_OUT_6_BIT | PWM_OUT_7_BIT, true);
  PWMPulseWidthSet(PWM0_BASE, PWM_OUT_6, 1 * CARGA / 100);
}
