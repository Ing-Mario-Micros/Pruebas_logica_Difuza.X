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
void Fuzzificacion(char error);
void AnalisisReglas(void);
float Defuzzificacion(void);
float SetFrio(unsigned char);
float SetTibio(unsigned char);
float SetCaliente(unsigned char);

float uf,ut,uc,r1,r2,r3,sal;
//uf:grado de pertenencia a frio
//ut: grado de pertenencia a tibio
//uc: grado de pertenencia a caliente
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
        
        //se fuzzifica el valor de la temperatura para hallar los grados de pertenencia
        Fuzzificacion(error);
        //se calcula el valor de activación de cada regla
        AnalisisReglas();
        //se calcula el valor de salida por el metodo de momento medio MOM
        sal=Defuzzificacion();
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
float SetFrio(unsigned char Temp){
  //Función que determina el grado de pertenencia al conjunto frio
  float grado;
  if(Temp<12){
    grado=1;    
  }else if(Temp<20){
    grado=2.5-(Temp*1.0)/8;  
  }else{
    grado=0;  
  }
  return grado;    
}
float SetTibio(unsigned char Temp){
  //Función que determina el grado de pertenencia al conjunto tibio
  float grado;
  if(Temp<12){
    grado=0;    
  }else if(Temp<20){
    grado=((Temp*1.0)/8)-1.5;  
  }else if(Temp<28){
    grado=3.5-(Temp*1.0)/8;  
  }else{
    grado=0;  
  }
  return grado;    
}
float SetCaliente(unsigned char Temp){
  //Función que determina el grado de pertenencia al conjunto caliente
  float grado;
  if(Temp<20){
    grado=0;    
  }else if(Temp<28){
    grado=((Temp*1.0)/8)-2.5;  
  }else{
    grado=1;  
  }
  return grado;    
}
void Fuzzificacion(char error){
  //Función que concentra el calculo de los grados de pertenencia
  uf=SetFrio(error);
  ut=SetTibio(error);
  uc=SetCaliente(error);
}
void AnalisisReglas(void){
  //Función que calcula el valor de activación de las reglas
  r1=0;
  r2=0;
  r3=0;  
  if(uf>0)
    r1=uf;
  if(ut>0)
    r2=ut;
  if(uc>0)
    r3=uc;  
}
float Defuzzificacion(void){
  //Función que calcula el valor de salida apartir de los centroides de los conjuntos de salida
  float sal=0;
  if(r1>0)
    sal=sal+0.175*r1;
  if(r2>0)
    sal=sal+0.5*r2;
  if(r3>0)
    sal=sal+0.825*r3;
  return sal;  
}





