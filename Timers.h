/* 
 * File:   Timers.h
 * Author: mario
 *
 * Created on 29 de agosto de 2022, 09:47 PM
 */

#ifndef TIMERS_H
#define	TIMERS_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif
/*------------------------ Configuración del Timer 1 -------------------------*/
void Activar_Timer1(void);
/*---------------------- Función de Interrupción Timer 1  --------------------*/
void Interrupcion_Timer1 (void);

/*------------------------ Configuración del Timer 1 -------------------------*/
void Activar_Timer1(void){
    PR1=65535;  //Timer de Time Out y de LED CPU se define el valor para sobreflujo
    TMR1=0;     //inicialización del Timer 1 con un valor de cero para no generar Interrupciones
    _T1IF=0;    //Inicialización de la bandera del Timer 1 en cero para detectar condición de sobre flujo en el mismo
    T1CON = 0x8010; //Configuración de timer 1 con preescaler de 8 y Fuente de oscilador fuente de reloj principal
    /**** Interrupción Timer 1 ****/
    _T1IE = 1;  //Habilitación Interrupción Timer1
    _T1IP = 7;  //Definición de Prioridad del Timer1
    _T1IF = 0;  //Inicializar la bandera de interrupción en 0
}
/*---------------------- Función de Interrupción Timer 1  --------------------*/
void Interrupcion_Timer1 (void){
    
}

#endif	/* TIMERS_H */

