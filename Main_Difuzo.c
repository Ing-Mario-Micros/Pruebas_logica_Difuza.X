/*
 * File:   DHT11_DSPIC_Main.c
 * Author: mario
 *
 * Created on 25 de agosto de 2022, 07:32 PM
 */

#include <xc.h>
//Fosc = 7.37MHz Por Defecto
//Fosc = 20 MHz Configurado mediante cristal
#define FCY 5000000
#include <libpic30.h> //Libreria necesaria para el uso de los retardos en el micro
#include "RS232.h"  //Libreria para el uso de comucicación RS232
#include "Timers.h" //Libreria encargada de administrar los Timers
#include "DHT11.h"  //Función encargada de la lectura del DHT11
#include "S_Difuso.h"



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

#define LED_CPU _RC13
#define LED_TEST _RC14


// Pines del DHT
//#define DATA_DIR _TRISB11
//#define DATA_IN _RB11
//#define DATA_OUT _LATB11

extern char Vector_Datos[];
/*------------------------- Función de Interrupción Timer 1 ----------------*/
void __attribute__((interrupt,auto_psv)) _T1Interrupt(void);

void __attribute__((interrupt,auto_psv)) _U2RXInterrupt(void);

/*------------------------- Variables PWM --------------------------------*/
void Por_PWM (float);
char Valvula;
/*--------------------------- Variables DHT11 ---------------------------*/
extern unsigned char Temp,Hum,Che,bandera;




void main(void) {
    /*------------------ Configuración de Pines Digital --------------------*/
    TRISC=0;    //Definir Puerto D como salida 
    LATC=0;     //Establecer estado inicial del puerto en estado logico de 0
    LED_TEST = 1; //Definir PIN D9 como indicador de reinicio en el Micro
    /*------------------ Configuración del Timer 1 -------------------------*/
    Activar_Timer1(); //Función Activación del Timer 1
    /*------------------ Configuración de RS232 ---------------------------*/
    Activar_RS232(); //Función de activación del Modulo RS232 Incluidas las interrupciones de Recepción
    /*------------------ Configuracion DHT11 ------------------------------*/
    Configurar_DHT11_1();
    
    /*------------------- Configuración de PWM -----------------------------*/
    PR2=1249;
    TMR2=0;
    _T2IF=0;
    T2CON= 0x0000;
    OC1R=0;
    OC1RS=200;
    OC1CON= 0x0000;
    _OCM=0b110;
    T2CON= T2CON|0x8000;
    /*-------------------- Sistema detecta Reinicios ------------------------*/
    __delay_ms(250);
    LED_TEST = 0; //Finalización de configuración LED de reinicio off
    /***------------------- Ciclo Infinito Principal ----------------------***/
    
    MensajeRS232("Iniciando Sistema \n");
    while(1){
        
        MensajeRS232(BufferR2);
        Promediar_Sensores();
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
        
        
        /*------------------------- Logica Difuza -------------------------*/
        //se fuzzifica el valor de la temperatura para hallar los grados de pertenencia
        Fuzzificacion(error);
        //se calcula el valor de activación de cada regla
        AnalisisReglas();
        //se calcula el valor de salida por el metodo de momento medio MOM
        sal=Defuzzificacion();
        
        MensajeRS232("Pertenencia negativo Mayor =");
        ImprimirDecimal(uenM);
        Transmitir('\n');
        
        MensajeRS232("Pertenencia negativo menor =");
        ImprimirDecimal(uenm);
        Transmitir('\n');
        
        MensajeRS232("Pertenencia cero =");
        ImprimirDecimal(uez);
        Transmitir('\n');
        
        MensajeRS232("Pertenencia positivo menor =");
        ImprimirDecimal(uepm);
        Transmitir('\n');
        
        MensajeRS232("Pertenencia positivo Mayor =");
        ImprimirDecimal(uepM);
        Transmitir('\n');
        
        MensajeRS232("Valor de salida =");
        ImprimirDecimal(sal);
        Transmitir('\n');
        Por_PWM (sal/100);
        __delay_ms(2000);
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
/*----------------------------- Funciones de PWM ---------------------------*/
void Por_PWM (float PPWM){
    //char aux=0
    //aux =(PPWM*1842);
    if(PPWM<0){
            PPWM=PPWM*(-1);
            Valvula=0;
    }
    else{
        Valvula=1;
    }
    ImprimirDecimal (PPWM);
    //OC1RS=8,0202*PPWM + 446,98;
    OC1RS = (int)( ((769+1)*PPWM)+480 );
    if(PPWM==0.0){
        OC1RS = 0;
    }
    //OC1RS = 480;
}

