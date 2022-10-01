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


#define SENSOR1 0x0200
#define SENSOR2 0x0400
#define SENSOR3 0x0800
#define SENSOR4 0x1000
/*-------------------- Variables de lectura en el sensor --------------------*/
unsigned char Temp=10,Hum=20,Che,bandera = 0;
/*--------------------------- Funciones DHT11 ---------------------------*/
void Configurar_DHT11_1(void);
void Promediar_Sensores(void);
void LeerHT11(unsigned int);
unsigned char LeerByte(unsigned int);
unsigned char LeerBit(unsigned int);
unsigned char Check();

/*------------------ Configuracion DHT11 ------------------------------*/
 void Configurar_DHT11_1(void){   
    ADPCFG=0xFFFF;    // Del Analogo 8 al 15 como Pin Digital
    /**Configuración del primer sensor**/
    LATB=LATB&(!SENSOR1); //DATA_OUT=0  PIN RB9// Inicializar pin de salida en cero
    TRISB=TRISB|(SENSOR1);//DATA_DIR=1  PIN RB9// Definir el puerto como Entrada
    /**Configuración del Segundo sensor**/
    LATB=LATB&(!SENSOR2); //DATA_OUT=0  PIN RB10// Inicializar pin de salida en cero
    TRISB=TRISB|(SENSOR2);//DATA_DIR=1  PIN RB10// Definir el puerto como Entrada
 }
/* -------------------------Funciones DHT11------------------------------ */
 void Promediar_Sensores(void){
    char T1,T2,T3,T4;
    char H1,H2,H3,H4;
    LeerHT11(SENSOR2);
    T1=Temp;
    H1=Hum;
    Vector_Datos[28]=T1;
 }
 
void LeerHT11(unsigned int Direc_Sensor){
  unsigned char i,contr=0;
  unsigned char repetir=0;
  unsigned char contador=0;
  bandera=0;
  TMR1=0;
  do{
      /*------------- Condición De Start Inicio -----------------*/
    //DATA_DIR=0;
    TRISB=TRISB&(!Direc_Sensor);//DATA_DIR=0;
    LATB=LATB&(!Direc_Sensor); //DATA_OUT=0;
    //DATA_OUT=0;
    __delay_ms(18);
    TRISB=TRISB|(Direc_Sensor);//DATA_DIR=1;
    while((PORTB&Direc_Sensor)==Direc_Sensor){//DATA_IN==1
      if(bandera==1) break;  
    }
    __delay_us(40);
    if((PORTB&Direc_Sensor)!=Direc_Sensor) contr++;//DATA_IN==0
    __delay_us(80);
    if((PORTB&Direc_Sensor)==Direc_Sensor) contr++;//DATA_IN==1
    //__delay_us(120);
    while((PORTB&Direc_Sensor)==Direc_Sensor){//DATA_IN==0
      if(bandera==1) break;  
    }
     /*---------------- Condición de Start Final ----------------*/
    Hum=LeerByte(Direc_Sensor);
    LeerByte(Direc_Sensor);
    Temp=LeerByte(Direc_Sensor);
    LeerByte(Direc_Sensor);
    Che=LeerByte(Direc_Sensor);
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

unsigned char LeerByte(unsigned int Direc_Sensor){
  unsigned char res=0,i;
  
  for(i=8;i>0;i--){
    res=(res<<1) | LeerBit(Direc_Sensor);  
  }
  return res;
}
unsigned char LeerBit(unsigned int Direc_Sensor){
    unsigned char res=0;
     while((PORTB&Direc_Sensor)!=Direc_Sensor){//DATA_IN==0
       if(bandera==1) break;  
     }
     __delay_us(13);
     if((PORTB&Direc_Sensor)==Direc_Sensor) res=0;//DATA_IN==1
     __delay_us(22);
     if((PORTB&Direc_Sensor)==Direc_Sensor){//DATA_IN==1
       res=1;
       while((PORTB&Direc_Sensor)==Direc_Sensor){//DATA_IN==1
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

