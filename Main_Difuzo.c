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
void AnalisisReglas(void);
float Defuzzificacion(void);

float Minimo(float a, float b);  //Para conjunción en las reglas
float Maximo(float a, float b, float c); //Para agregación en la defuzzificacion

float SetErrorNegativoMayor(char);
float SetErrorNegativoMenor(char);
float SetErrorZero(char);
float SetErrorPositivoMenor(char);
float SetErrorPositivoMayor(char);

float SetDErrorNegativoMayor(char);
float SetDErrorNegativoMenor(char);
float SetDErrorZero(char);
float SetDErrorPositivoMenor(char);
float SetDErrorPositivoMayor(char);

float uenM,uenm,uez,uepm,uepM,udenM,udenm,udez,udepm,udepM, 
        r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12,r13,r14,r15,
        r16,r17,r18,r19,r20,r21,r22,r23,r24,r25,sal;
//uenm: grado de pertenecia del conjunto error negativo Mayor
//uenM: grado de pertenecia del conjunto error negativo menor
//uez: grado de pertenecia del conjunto error cero
//uepm: grado de pertenecia del conjunto error positivo menor
//uepM: grado de pertenecia del conjunto error positivo mayor

//udenm: grado de pertenecia del conjunto derivada error negativo Mayor
//udenM: grado de pertenecia del conjunto derivada error negativo menor
//udez: grado de pertenecia del conjunto derivada error cero
//udepm: grado de pertenecia del conjunto derivada error positivo menor
//udepM: grado de pertenecia del conjunto derivada error positivo mayor

//r1: activación de la regla1
//r2: activación de la regla2
//r3: activación de la regla3
//r4: activación de la regla4
//r5: activación de la regla5
//sal: salida de la defuzzificación
/*------------------------- Variables PWM --------------------------------*/
void Por_PWM (float);
char Valvula;
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
    _LATD9 = 0; //Finalización de configuración LED de reinicio off
    /***------------------- Ciclo Infinito Principal ----------------------***/
    
    MensajeRS232("Iniciando Sistema Difuso\n");
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

/*---------------------------- Logica Difuza ---------------------------*/
void Obt_Error (void){
    error=Vector_Datos[18]-Vector_Datos[28];
    Derror=(error - error_Ant)/2;
    error_Ant=error;
}

void Fuzzificacion(char Error){
  //Función que concentra el calculo de los grados de pertenencia del error
  uenM=SetErrorNegativoMayor(Error);
  uenm=SetErrorNegativoMenor(Error);
  uez=SetErrorZero(Error);
  uepm=SetErrorPositivoMenor(Error);
  uepM=SetErrorPositivoMayor(Error);
  
  //Función que concentra el calculo de los grados de pertenencia derivada del error
  udenM=SetDErrorNegativoMayor(Derror);
  udenm=SetDErrorNegativoMenor(Derror);
  udez=SetDErrorZero(Derror);
  udepm=SetDErrorPositivoMenor(Derror);
  udepM=SetDErrorPositivoMayor(Derror);
}
/***---------------------Conjuntos difuzos del Error-----------------------***/
float SetErrorNegativoMayor(char Error){
    //Función que determina el grado de pertenencia al conjunto de error negativo Mayor
    float grado;
    if(Error<-3){
      grado=1;    
    }else if(Error<-1){
      grado=(-0.5*Error)-1;  
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
        grado=(0.5*Error)+2;  
    }else if(Error<0){
        grado=(-1*Error)-1;  
    }else{
        grado=0;  
    }
    return grado; 
}
float SetErrorZero(char Error){
    //Función que determina el grado de pertenencia al conjunto de error cero
    float grado;
    if(Error<-2){
        grado=0;    
    }else if(Error<-1){
        grado=(1*Error)+2;  
    }else if(Error<1){
        grado=1;  
    }else if(Error<1){
        grado=(-1*Error)+2;  
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
        grado=(1*Error)-1;  
    }else if(Error<3){
        grado=(-0.5*Error)+2;  
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
      grado=(0.5*Error)-1;  
    }else{
      grado=1;  
    }
    return grado; 
}
/***-------------Conjuntos difuzos de la derivada del Error-----------------***/
float SetDErrorNegativoMayor(char Error){
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
float SetDErrorNegativoMenor(char Error){
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
float SetDErrorZero(char Error){
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
float SetDErrorPositivoMenor(char Error){
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
float SetDErrorPositivoMayor(char Error){
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
void AnalisisReglas(void){
  //Función que calcula el valor de activación de las reglas
  r1=0.0,r2=0.0,r3=0.0,r4=0.0,r5=0.0,r6=0.0,r7=0.0,r8=0.0,r9=0.0,r10=0.0,
         r11=0.0,r12=0.0,r13=0.0,r14=0.0,r15=0.0,r16=0.0,r17=0.0,r18=0.0,
         r19=0.0,r20=0.0,r21=0.0,r22=0.0,r23=0.0,r24=0.0,r25=0.0;
  if(udenM>0 && uenM>0) //regla F1C1
    r1=Minimo(uenM,udenM);
  if(udenm>0 && uenM>0) //regla F2C1
    r2=Minimo(udenm,uenM);
  if(udez>0 && uenM>0)    //regla F3C1 se super pondria la siguiente como lo soluciono?
    r3=Minimo(udez,uenM);
  if(udepm>0 && uenM>0) //regla F4C1
      r4=Minimo(udepm,uenM);
  if(udepM>0 && uenM>0) //regla F5C1
      r5=Minimo(udepM,uenM);
  if(udenM>0 && uenm>0) //regla F1C2
      r6=Minimo(udenM,uenm);
  if(udenm>0 && uenm>0) //regla F2C2
      r7=Minimo(udenm,uenm);
  if(udez>0 && uenm>0) //regla F3C2
      r8=Minimo(udez,uenm);
  if(udepm>0 && uenm>0) //regla F4C2
      r9=Minimo(udepm,uenm);
  if(udepM>0 && uenm>0) //regla F5C2
      r10=Minimo(udepM,uenm);
  if(udenM>0 && uez>0) //regla F1C3
      r11=Minimo(udenM,uez);
  if(udenm>0 && uez>0) //regla F2C3
      r12=Minimo(udenm,uez);
  if(udez>0 && uez>0) //regla F3C3
      r13=Minimo(udez,uez);
  if(udepm>0 && uez>0) //regla F4C3
      r14=Minimo(udepm,uez);
  if(udepM>0 && uez>0) //regla F5C3
      r15=Minimo(udepM,uez);
  if(udenM>0 && uepm>0) //regla F1C4
      r16=Minimo(udenM,uepm);
  if(udenm>0 && uepm>0) //regla F2C4
      r17=Minimo(udenm,uepm);
  if(udez>0 && uepm>0) //regla F3C4
      r18=Minimo(udez,uepm);
  if(udepm>0 && uepm>0) //regla F4C4
      r19=Minimo(udepm,uepm);
  if(udepM>0 && uepm>0) //regla F5C4
      r20=Minimo(udepM,uepm);
  if(udenM>0 && uepM>0) //regla F1C5
      r21=Minimo(udenM,uepM);
  if(udenm>0 && uepM>0) //regla F2C5
      r22=Minimo(udenm,uepM);
  if(udez>0 && uepM>0) //regla F3C5
      r23=Minimo(udez,uepM);
  if(udepm>0 && uepM>0) //regla F4C5
      r24=Minimo(udepm,uepM);
  if(udepM>0 && uepM>0) //regla F5C5
      r25=Minimo(udepM,uepM);
}
float Defuzzificacion(void){
  //Función que calcula el valor de salida apartir de los centroides de los conjuntos de salida
  float sal=0,aux;
  //Conjunto de salida pwm negativo Mayor
  if(r1>0 || r2>0|| r3>0|| r6>0){
    aux=Maximo(r1,r2,r3);  
    sal=sal+(-99.5*Maximo(aux,r6,0));
  }
  //Conjunto de salida pwm negativo Menor
  if(r7>0 || r8>0 ||r11>0){
    sal=sal+(-50*Maximo(r7,r8,r11));
  }
  //Conjunto de salida pwm zero
  if(r4>0 || r5>0 || r9>0|| r10>0 ||r12>0 ||r13>0 || r14>0|| r16>0|| r17>0
          || r21>0 || r22>0){
    sal=sal+0;
  }
  //Conjunto de salida pwm positivo Menor
  if(r15>0 || r18>0 || r19>0 ){
    sal=sal+50*Maximo(r15,r18,r19);
  }
  //Conjunto de salida pwm positivo Mayor
  if(r20>0 ||r23>0 || r24>0|| r25>0){
    aux=Maximo(r20,r23,r24);
    sal=sal+99.5*Maximo(aux,r25,0);
  }
  
  return sal;  
}
float Minimo(float a, float b){
  if(a<b)
    return a;
  else
    return b;  
}
float Maximo(float a, float b,float c){
  float Max=a;
  if(b>Max){
    Max=b;
    if(c>Max)
      return c;
    else
      return b;
  }else{
    if(c>Max)
      return c;
    else
      return a;  
  } 
}
