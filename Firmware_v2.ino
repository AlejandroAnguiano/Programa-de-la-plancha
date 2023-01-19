/*//             SMT WOLAS 
    ALEJANDRO VILLALOBOS ANGUIANO
    ANGEL DANIEL RUELAS ZAVALA
*/

#include <thermistor.h>             
thermistor therm1(A0,1);            //Thermistor conectado a A0 
//LCD config
#include <Wire.h>                   
#include <LiquidCrystal_I2C.h>      
LiquidCrystal_I2C lcd(0x27,16,2);   //Definir la LCD A 0X27

//Entradas y Salidas
int but_1 = 12;
int but_2 = 11;
int but_3 = 10; 
int but_4 = 9;
int SSR = 3;
int buzzer = 6;
int Thermistor_PIN = A0;
int LDR = 4;
int laser = 5;

//Variables
unsigned int millis_before, millis_before_2;    //Usamos estos para crear la frecuencia de actualización del bucle
unsigned int millis_now = 0;
float refresh_rate = 500;                       // Frecuencia de actualización de LCD.
float pid_refresh_rate  = 50;                   // Tasa de actualización de PID
float seconds = 0;                              //Variable utilizada para almacenar el tiempo transcurrido                  
int running_mode = 0;                           //Almacenamos el modo seleccionado en ejecución aquí
int selected_mode = 0;                          //Modo seleccionado para el menú
int max_modes = 3;                              //Por ahora solo trabajamos con 1 modo...
bool but_3_state = true;                        //Almacenar el estado del botón (ALTO O BAJO)
bool but_4_state  =true;                        //Almacenar el estado del botón (ALTO O BAJO)
float temperature = 0;                          //Almacenar el valor de la temperatura aquí
float preheat_setoint = 140;                    //El valor de rampa de precalentamiento del modo 1 es 140-150ºC
float soak_setoint = 150;                       //Modo 1 pre-reflujo es 150ºC por unos segundos
float reflow_setpoint = 200;                   //El pico de reflujo del Modo 1 es de 200ºC
float temp_setpoint = 0;                        //Usado para control PID
float pwm_value = 255;                          /// El SSR está APAGADO con ALTO, por lo que 255 PWM APAGARÍA el SSR
float MIN_PID_VALUE = 0;
float MAX_PID_VALUE = 180;                      //Valor máximo de PID. Puedes cambiar esto.
float cooldown_temp = 40;                       //Cuando esta bien tocar el plato

/////////////////////PID VARIABLES///////////////////////
/////////////////////////////////////////////////////////
float Kp = 2;               
float Ki = 0.0025;          
float Kd = 9;               
float PID_Output = 0;
float PID_P, PID_I, PID_D;
float PID_ERROR, PREV_ERROR;
/////////////////////////////////////////////////////////

void setup() {
//Defina los pines como salidas o entradas
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, HIGH);        //Asegúra que comenzamos con el SSR APAGADO (apagado con ALTO)
  pinMode(buzzer, OUTPUT); 
  digitalWrite(buzzer, LOW);  
  pinMode(but_1, INPUT_PULLUP);
  pinMode(but_2, INPUT_PULLUP);
  pinMode(but_3, INPUT_PULLUP);
  pinMode(but_4, INPUT_PULLUP);
  pinMode(Thermistor_PIN, INPUT);

  lcd.init();                     //Iniciar la pantalla LCD
  lcd.backlight();              //Activar retroiluminación 
  Serial.begin(9600);
  tone(buzzer, 1800, 200);     
  millis_before = millis();
  millis_now = millis();
}

void loop() {
  millis_now = millis();
  if(millis_now - millis_before_2 > pid_refresh_rate){    // Frecuencia de actualización del PID
    millis_before_2 = millis(); 
    
    temperature = therm1.analog2temp() ;
    
    if(running_mode == 1){   
      if(temperature < preheat_setoint){
        temp_setpoint = seconds*1.666;                    //Alcanza 150ºC hasta 90s (150/90=1.666)
      }  
        
      if(temperature > preheat_setoint && seconds < 90){
        temp_setpoint = soak_setoint;               
      }   
        
      else if(seconds > 90 && seconds < 110){
        temp_setpoint = reflow_setpoint;                 
      } 
       
      //Calculo De PID
      PID_ERROR = temp_setpoint - temperature;
      PID_P = Kp*PID_ERROR;
      PID_I = PID_I+(Ki*PID_ERROR);      
      PID_D = Kd * (PID_ERROR-PREV_ERROR);
      PID_Output = PID_P + PID_I + PID_D;
      //Define Valores de PID MAX
      if(PID_Output > MAX_PID_VALUE){
        PID_Output = MAX_PID_VALUE;
      }
      else if (PID_Output < MIN_PID_VALUE){
        PID_Output = MIN_PID_VALUE;
      }
      //Como el SSR está ENCENDIDO con BAJO, invertimos la señal pwm
      pwm_value = 255 - PID_Output;
      
      analogWrite(SSR,pwm_value);          //Cambiamos el Duty Cycle aplicado al SSR
      
      PREV_ERROR = PID_ERROR;
      
      if(seconds > 130){
        digitalWrite(SSR, HIGH);            //Con ALTO el SSR está APAGADO
        temp_setpoint = 0;
        running_mode = 10;                  //Modo de enfriamiento        
      }     
    }//End of running_mode = 1


    //El modo 10 está entre reflujo y enfriamiento
    if(running_mode == 10){
      lcd.clear();
      lcd.setCursor(0,1);     
      lcd.print("    COMPLETO    ");
      tone(buzzer, 1800, 1000);    
      seconds = 0;              //Reinicia el timer
      running_mode = 11;
      delay(3000);
    }    
  }//Fin de > millis_before_2 (Frecuencia de actualización del código PID)
  

  
  millis_now = millis();
  if(millis_now - millis_before > refresh_rate){          //Frecuencia de actualización de la impresión en la pantalla LCD
    millis_before = millis();   
    seconds = seconds + (refresh_rate/1000);              //Contamos el tiempo en segundos
    

    // El modo 0 es con SSR APAGADO (podemos seleccionar el modo con los botones)
    if(running_mode == 0){ 
      digitalWrite(SSR, HIGH);        //Con ALTO el SSR está APAGADO
      lcd.clear();
      lcd.setCursor(0,0);     
      lcd.print("T: ");
      lcd.print(temperature,1);   
      lcd.setCursor(9,0);      
      lcd.print("Rele OFF"); 
       
      lcd.setCursor(0,1);  
      if(selected_mode == 0){
        lcd.print("Select Mode");     
      }
      else if(selected_mode == 1){
        lcd.print("MODE 1");     
      }
      else if(selected_mode == 2){
        lcd.print("MODE 2");     
      }
      else if(selected_mode == 3){
        lcd.print("MODE 3");     
      }
      
      
    }//Fin del modo_ejecución = 0

     // El modo 11 es enfriamiento. SSR está APAGADO
     else if(running_mode == 11){ 
      if(temperature < cooldown_temp){
        running_mode = 0; 
        tone(buzzer, 1000, 100); 
      }
      digitalWrite(SSR, HIGH);       //Con ALTO el SSR está APAGADO
      lcd.clear();
      lcd.setCursor(0,0);     
      lcd.print("T: ");
      lcd.print(temperature,1);   
      lcd.setCursor(9,0);      
      lcd.print("SSR OFF"); 
       
      lcd.setCursor(0,1);       
      lcd.print("    COOLDOWN    ");  
    }//fin del modo_ejecución == 11

  // El modo 1 es el PID que se ejecuta con el modo 1 seleccionado
    else if(running_mode == 1){            
      lcd.clear();
      lcd.setCursor(0,0);     
      lcd.print("T: ");
      lcd.print(temperature,1);  
      lcd.setCursor(9,0);       
      lcd.print("Rele ON"); 
       
      lcd.setCursor(0,1); 
      lcd.print("S");  lcd.print(temp_setpoint,0); 
      lcd.setCursor(5,1);     
      lcd.print("PWM");  lcd.print(pwm_value,0); 
      lcd.setCursor(12,1); 
      lcd.print(seconds,0);  
      lcd.print("s");         
    }//Fin del modo_ejecución == 1
    
    
  }


  
  ///////////////////////Deteccion de botones////////////////////////////
  ///////////////////////////////////////////////////////////////////
  if(!digitalRead(but_3) && but_3_state){
    but_3_state = false;
    selected_mode ++;   
    tone(buzzer, 2300, 40);  
    if(selected_mode > max_modes){
      selected_mode = 0;
    }
    delay(150);
  }
  else if(digitalRead(but_3) && !but_3_state){
    but_3_state = true;
  }

  
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  if(!digitalRead(but_4) && but_4_state){
    if(running_mode == 1){
      digitalWrite(SSR, HIGH);        // Con ALTO el SSR está desactivado
      running_mode = 0;
      selected_mode = 0; 
      tone(buzzer, 2500, 150);
      delay(130);
      tone(buzzer, 2200, 150);
      delay(130);
      tone(buzzer, 2000, 150);
      delay(130);
    }
    
    but_4_state = false;
    if(selected_mode == 0){
      running_mode = 0;
    }
    else if(selected_mode == 1){
      running_mode = 1;
      tone(buzzer, 2000, 150);
      delay(130);
      tone(buzzer, 2200, 150);
      delay(130);
      tone(buzzer, 2400, 150);
      delay(130);
      seconds = 0;                    //Reinicio del timer
    }
  }
  else if(digitalRead(but_4) && !but_4_state){
    but_4_state = true;
  }
}
