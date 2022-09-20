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

void Fuzzificacion(char Error);

float SetErrorNegativoMayor(char);
float SetErrorNegativoMenor(char);
float SetErrorZero(char);
float SetErrorPositivoMenor(char);
float SetErrorPositivoMayor(char);

float uenM,uenm,uez,uepm,uepM,r1,r2,r3,sal;
//uenm: grado de pertenecia del conjunto error negativo Mayor
//uenM: grado de pertenecia del conjunto error negativo menor
//uez: grado de pertenecia del conjunto error cero
//uepm: grado de pertenecia del conjunto error positivo menor
//uepM: grado de pertenecia del conjunto error positivo mayor

//r1: activación de la regla1
//r2: activación de la regla2
//r3: activación de la regla3
//sal: salida de la defuzzificación


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
        /*------------------------- Logica Difuza -------------------------*/
        //se fuzzifica el valor de la temperatura para hallar los grados de pertenencia
        Fuzzificacion(error);
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

void Fuzzificacion(char Error){
  //Función que concentra el calculo de los grados de pertenencia
  uenM=SetErrorNegativoMayor(Error);
  uenm=SetErrorNegativoMenor(Error);
  uez=SetErrorZero(Error);
  uepm=SetErrorPositivoMenor(Error);
  uepM=SetErrorPositivoMayor(Error);
  
}
float SetErrorNegativoMayor(char Error){
    //Función que determina el grado de pertenencia al conjunto de error negativo Mayor
    float grado;
    if(Error<-3){
      grado=1;    
    }else if(Error<-1){
      grado=(-0.5*Error)-0.5;  
    }else{
      grado=0;  
    }
    return grado; 
}
float SetErrorNegativoMenor(char Error){
    //Función que determina el grado de pertenencia al conjunto de error negativo menor
    float grado;
    if(Error<-3){
        grado=0;    
    }else if(Error<-1){
        grado=(0.5*Error)+1.5;  
    }else if(Error<0){
        grado=(-1*Error)+0;  
    }else{
        grado=0;  
    }
    return grado; 
}
float SetErrorZero(char Error){
    //Función que determina el grado de pertenencia al conjunto de error cero
    float grado;
    if(Error<-1){
        grado=0;    
    }else if(Error<0){
        grado=(1*Error)+1;  
    }else if(Error<1){
        grado=(-1*Error)+1;  
    }else{
        grado=0;  
    }
    return grado; 
}
float SetErrorPositivoMenor(char Error){
    //Función que determina el grado de pertenencia al conjunto de error positivo menor
    float grado;
    if(Error<0){
        grado=0;    
    }else if(Error<1){
        grado=(1*Error)+0;  
    }else if(Error<3){
        grado=(-0.5*Error)+1.5;  
    }else{
        grado=0;  
    }
    return grado;  
}
float SetErrorPositivoMayor(char Error){
    //Función que determina el grado de pertenencia al conjunto de error positivo Mayor
    float grado;
    if(Error<1){
      grado=0;    
    }else if(Error<3){
      grado=(0.5*Error)-0.5;  
    }else{
      grado=1;  
    }
    return grado; 
}






