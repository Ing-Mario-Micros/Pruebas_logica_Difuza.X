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
#define DATA_DIR _TRISB11
#define DATA_IN _RB11
#define DATA_OUT _LATB11

/*------------------------- Función de Interrupción Timer 1 ----------------*/
void __attribute__((interrupt,auto_psv)) _T1Interrupt(void);

void __attribute__((interrupt,auto_psv)) _U2RXInterrupt(void);
/*---------------------------- Logica Difuza ---------------------------*/
unsigned char Ref = 26;
char error = 0, Derror = 0, error_Ant=0;
void Obt_Error (void);  //Función encargada de obtener el valor del error y la derivada del error
/*--------------------------- Variables DHT11 ---------------------------*/
unsigned char Temp=10,Hum=20,Che,bandera = 0,Leer_DHT =0;
/*--------------------------- Funciones DHT11 ---------------------------*/
void LeerHT11(void);
unsigned char LeerByte(void);
unsigned char LeerBit(void);
unsigned char Check();

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
    ADPCFG=0xFFFF;    // Del Analogo 8 al 15 como Pin Digital
    DATA_OUT=0;       // Inicializar pin de salida en cero
    DATA_DIR=1;       // Definir el puerto como Entrada
    /*-------------------- Sistema detecta Reinicios ------------------------*/
    __delay_ms(250);
    _LATD9 = 0; //Finalización de configuración LED de reinicio off
    /***------------------- Ciclo Infinito Principal ----------------------***/
    MensajeRS232("Iniciando Sistema Difuso\n");
    while(1){
        
        MensajeRS232(BufferR2);
        
        LeerHT11();
        Obt_Error ();
        MensajeRS232("Error =");
        ImprimirEntero(error);
        Transmitir('\n');
        MensajeRS232("Derivada del Error =");
        ImprimirEntero(Derror);
        Transmitir('\n');
        Transmitir('T');
        Transmitir(Temp/10 + 48);
        Transmitir(Temp%10 + 48);
        Transmitir(' ');
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
    error=Ref-Temp;
    Derror=(error - error_Ant);
    error_Ant=error;
}

/* -------------------------Funciones DHT11------------------------------ */
void LeerHT11(void){
  unsigned char i,contr=0;
  unsigned char repetir=0;
  unsigned char contador=0;
  bandera=0;
  TMR1=0;
  do{
      /*------------- Condición De Start Inicio -----------------*/
    DATA_DIR=0;
    DATA_OUT=0;
    __delay_ms(18);
    DATA_DIR=1;
    while(DATA_IN==1){
      if(bandera==1) break;  
    }
    __delay_us(40);
    if(DATA_IN==0) contr++;
    __delay_us(80);
    if(DATA_IN==1) contr++;
    //__delay_us(120);
    while(DATA_IN==1){
      if(bandera==1) break;  
    }
     /*---------------- Condición de Start Final ----------------*/
    Hum=LeerByte();
    LeerByte();
    Temp=LeerByte();
    LeerByte();
    Che=LeerByte();
    if(bandera==1){
      repetir=1;
      bandera=0;
      contador++;
      TMR1=0;
    }
  }while(repetir==1 && contador<6);
  
  repetir=0;
  if(contador==6){
    Temp=0;
    Hum=0;
    contador=0;
  }
}
unsigned char LeerByte(void){
  unsigned char res=0,i;
  
  for(i=8;i>0;i--){
    res=(res<<1) | LeerBit();  
  }
  return res;
}
unsigned char LeerBit(void){
    unsigned char res=0;
     while(DATA_IN==0){
       if(bandera==1) break;  
     }
     __delay_us(13);
     if(DATA_IN==1) res=0;
     __delay_us(22);
     if(DATA_IN==1){
       res=1;
       while(DATA_IN==1){
         if(bandera==1) break;  
       }
     }  
     return res;  
}
unsigned char Check(void){
  unsigned char res=0,aux;
  aux=Temp+Hum;
  if(aux==Che) res=1;
  return res;  
}



