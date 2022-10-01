/* 
 * File:   DHT11.h
 * Author: mario
 *
 * Created on 30 de agosto de 2022, 08:42 PM
 */

#ifndef DHT11_H
#define	DHT11_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif
#include <xc.h>

// Pines del DHT
#define DATA_DIR _TRISB11
#define DATA_DIR1 0x0400
#define DATA_IN _RB11
#define DATA_OUT _LATB11

/*-------------------- Variables de lectura en el sensor --------------------*/
unsigned char Temp=10,Hum=20,Che,bandera = 0;
/*--------------------------- Funciones DHT11 ---------------------------*/
void Configurar_DHT11_1(void);
void LeerHT11(void);
void LeerHT11_1(void);
unsigned char LeerByte(void);
unsigned char LeerBit(void);
unsigned char Check();

/*------------------ Configuracion DHT11 ------------------------------*/
 void Configurar_DHT11_1(void){   
    ADPCFG=0xFFFF;    // Del Analogo 8 al 15 como Pin Digital
    DATA_OUT=0;       // Inicializar pin de salida en cero
    DATA_DIR=1;       // Definir el puerto como Entrada
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
    //DATA_DIR=0;
    TRISB=TRISB&(!0x0800);//DATA_DIR=0;
    LATB=LATB&(!0x0800); //DATA_OUT=0;
    //DATA_OUT=0;
    __delay_ms(18);
    TRISB=TRISB|(0x0800);//DATA_DIR=1;
    while((PORTB&0X800)==0x0800){//DATA_IN==1
      if(bandera==1) break;  
    }
    __delay_us(40);
    if((PORTB&0X800)!=0x0800) contr++;//DATA_IN==0
    __delay_us(80);
    if((PORTB&0X800)==0x0800) contr++;//DATA_IN==1
    //__delay_us(120);
    while((PORTB&0X800)==0x0800){//DATA_IN==0
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
  Vector_Datos[28]=Temp;
}
void LeerHT11_1(void){
  unsigned char i,contr=0;
  unsigned char repetir=0;
  unsigned char contador=0;
  bandera=0;
  TMR1=0;
  do{
      /*------------- Condición De Start Inicio -----------------*/
    TRISB=TRISB&(!DATA_DIR1);
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
  Vector_Datos[28]=Temp;
  
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
     while((PORTB&0X800)!=0x0800){//DATA_IN==0
       if(bandera==1) break;  
     }
     __delay_us(13);
     if((PORTB&0X800)==0x0800) res=0;//DATA_IN==1
     __delay_us(22);
     if((PORTB&0X800)==0x0800){//DATA_IN==1
       res=1;
       while((PORTB&0X800)==0x0800){//DATA_IN==1
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
#endif	/* DHT11_H */

