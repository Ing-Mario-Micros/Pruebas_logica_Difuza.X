/*
 * File:   DHT11_DSPIC_Main.c
 * Author: mario
 *
 * Created on 25 de agosto de 2022, 07:32 PM
 */

#include <xc.h>
//Fosc = 7.37MHz Por Defecto
#define FCY 5000000
#include <libpic30.h>
#include "RS232.h"
#include "Timers.h"
#include "DHT11.h"



// DSPIC30F4013 Configuration Bit Settings

// 'C' source line config statements

// FOSC
#pragma config FOSFPR = HS              // Oscillator (HS)
#pragma config FCKSMEN = CSW_FSCM_OFF   // Clock Switching and Monitor (Sw Disabled, Mon Disabled)

// FWDT
#pragma config FWPSB = WDTPSB_16        // WDT Prescaler B (1:16)
#pragma config FWPSA = WDTPSA_512       // WDT Prescaler A (1:512)
#pragma config WDT = WDT_OFF            // Watchdog Timer (Disabled)

// FBORPOR
#pragma config FPWRT = PWRT_64          // POR Timer Value (64ms)
#pragma config MCLRE = MCLR_EN          // Master Clear Enable (Enabled)

// FICD
#pragma config ICS = ICS_PGD            // Comm Channel Select (Use PGC/EMUC and PGD/EMUD)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define LED_CPU _RD3

// Pines del DHT
//#define DATA_DIR _TRISB11
//#define DATA_IN _RB11
//#define DATA_OUT _LATB11

extern char Vector_Datos[];
/*------------------------- Función de Interrupción Timer 1 ----------------*/
void __attribute__((interrupt,auto_psv)) _T1Interrupt(void);

void __attribute__((interrupt,auto_psv)) _U2RXInterrupt(void);
/*---------------------------- Logica Difuza ---------------------------*/
char error = 0, Derror = 0, error_Ant=0;
void Obt_Error (void);  //Función encargada de obtener el valor del error y la derivada del error





/*--------------------------- Variables DHT11 ---------------------------*/
extern unsigned char Temp,Hum,Che,bandera;




void main(void) {
    /*------------------ Configuración de Pines Digital --------------------*/
    TRISD=0;    //Definir Puerto D como salida 
    LATD=0;     //Establecer estado inicial del puerto en estado logico de 0
    _LATD9 = 1; //Definir PIN D9 como indicador de reinicio en el Micro
    /*------------------ Configuración del Timer 1 -------------------------*/
    Activar_Timer1(); //Función Activación del Timer 1
    /*------------------ Configuración de RS232 ---------------------------*/
    Activar_RS232(); //Función de activación del Modulo RS232 Incluidas las interrupciones de Recepción
    /*------------------ Configuracion DHT11 ------------------------------*/
    Configurar_DHT11_1();
    /*-------------------- Sistema detecta Reinicios ------------------------*/
    __delay_ms(250);
    _LATD9 = 0; //Finalización de configuración LED de reinicio off
    /***------------------- Ciclo Infinito Principal ----------------------***/
    
    MensajeRS232("Iniciando Sistema Difuso\n");
    while(1){
        
        MensajeRS232(BufferR2);
        
        LeerHT11();
        MensajeRS232("Referencia =");
        ImprimirEntero(Vector_Datos[18]);
        Transmitir('\n');
        Obt_Error ();
        MensajeRS232("Error =");
        ImprimirEntero(error);
        Transmitir('\n');
        MensajeRS232("Derivada del Error =");
        ImprimirEntero(Derror);
        Transmitir('\n');
        Transmitir('T');
        ImprimirEntero(Vector_Datos[28]); //Los valores de temperatura se almacenan en vector de datos posición 28
        Transmitir('H');
        Transmitir(Hum/10 + 48);
        Transmitir(Hum%10 + 48);
        Transmitir('\n');
        
        
        __delay_ms(1000);
    }
}
void __attribute__((interrupt,auto_psv)) _T1Interrupt(void){
    LED_CPU=LED_CPU^ 1; // Conmutar PinC13 LED CPU
    bandera=1;
    _T1IF=0;            // Reset de bandera de interrupción en Cero
}
void __attribute__((interrupt,auto_psv)) _U2RXInterrupt(void){
    Interrupcion_RS232();
}
/*---------------------------- Logica Difuza ---------------------------*/
void Obt_Error (void){
    error=Vector_Datos[18]-Vector_Datos[28];
    Derror=(error - error_Ant);
    error_Ant=error;
}








