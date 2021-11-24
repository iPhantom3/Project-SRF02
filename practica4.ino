#include <EEPROM.h>
#include <Wire.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

//Variables para el reloj
volatile unsigned char segundos = 0;
volatile unsigned char minutos;
volatile unsigned char hora;
volatile unsigned char dia;
volatile unsigned char mes;
volatile unsigned int anyo;
bool flagReloj = false;

int flag = 0;

//Direccion LCD
#define LCD05_I2C_ADDRESS byte((0xC6)>>1)

// Registros lcd
#define COMMAND_REGISTER byte(0x00)
#define FIFO_AVAILABLE_LENGTH_REGISTER byte(0x00)
#define LCD_STYLE_16X2 byte(5)

// Codigos de los comandos del lcd
#define CURSOR_HOME             byte(1)
#define SET_CURSOR              byte(2)
#define SET_CURSOR_COORDS       byte(3)
#define SHOW_BLINKING_CURSOR    byte(6)
#define CLEAR_SCREEN            byte(12)
#define CARRIAGE_RETURN         byte(13)
#define BACKLIGHT_ON            byte(19)
#define BACKLIGHT_OFF           byte(20) 
#define SET_DISPLAY_TYPE        byte(24) 
#define CONTRAST_SET            byte(30)
#define BRIGHTNESS_SET          byte(31)

//Registros de Sensor
#define COMMAND_REGISTER byte(0x00)
#define SOFTWARE_REVISION byte(0x00)
#define RANGE_HIGH_BYTE byte(2)
#define RANGE_LOW_BYTE byte(3)
#define AUTOTUNE_MINIMUM_HIGH_BYTE byte(4)
#define AUTOTUNE_MINIMUM_LOW_BYTE byte(5)

//Variables para sensores
unsigned char cantSensores = 8;
unsigned char SRF02_I2C_ADDRESS[8] = {0x73, 0x74, 0x75, 0x77, 0x78, 0x79, 0x7B, 0x7E};
const unsigned char REAL_RANGING_MODE[3] = {80,81,82};
unsigned char modo = 1;

/*
 * Funciones para lcd
 */
void mostrar_medicion(int sensor, int medicion){
  int fifo_length;
  while( (fifo_length=read_fifo_length(LCD05_I2C_ADDRESS))<4 ) {}
  
  ascii_chars(LCD05_I2C_ADDRESS, sensor, medicion);
}

void mostrar_hora_lcd(String fecha, String fecha_hora){
  
  clear_screen(LCD05_I2C_ADDRESS);

  int fifo_length;
  while( (fifo_length=read_fifo_length(LCD05_I2C_ADDRESS))<4 ) {}

  Wire.beginTransmission(LCD05_I2C_ADDRESS);
  Wire.write(COMMAND_REGISTER);

  for(int i=0; i<fecha_hora.length(); i++){
    Wire.write(fecha_hora[i]);
  }
  Wire.endTransmission();

  set_cursor_coords(LCD05_I2C_ADDRESS, 2, 1);

  Wire.beginTransmission(LCD05_I2C_ADDRESS);
  Wire.write(COMMAND_REGISTER);

  for(int i=0; i<fecha.length(); i++){
    Wire.write(fecha[i]);
  }
  Wire.endTransmission();
}

void clear_screen(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);
  
  Wire.write(COMMAND_REGISTER); 
  Wire.write(CLEAR_SCREEN); 
  
  Wire.endTransmission();
}

void cursor_home(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);
  
  Wire.write(COMMAND_REGISTER); 
  Wire.write(CURSOR_HOME); 
  
  Wire.endTransmission();
}

void carriage_return(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);
  
  Wire.write(COMMAND_REGISTER); 
  Wire.write(CARRIAGE_RETURN); 
  
  Wire.endTransmission();
}

void set_cursor(byte dir_lcd, byte pos)
{
  Wire.beginTransmission(dir_lcd);
  
  Wire.write(COMMAND_REGISTER); 
  Wire.write(SET_CURSOR); 
  Wire.write(pos);
  
  Wire.endTransmission();
  
}

void set_cursor_coords(byte dir_lcd, byte linea, byte columna){
  
  Wire.beginTransmission(dir_lcd);
  
  Wire.write(COMMAND_REGISTER); 
  Wire.write(SET_CURSOR_COORDS); 
  Wire.write(linea);
  Wire.write(columna);
  
  Wire.endTransmission();
}

void show_blinking_cursor(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);

  Wire.write(COMMAND_REGISTER); 
  Wire.write(SHOW_BLINKING_CURSOR); 
  
  Wire.endTransmission();
}

void backlight_on(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);

  Wire.write(COMMAND_REGISTER); 
  Wire.write(BACKLIGHT_ON);
   
  Wire.endTransmission();
}

void backlight_off(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);

  Wire.write(COMMAND_REGISTER); 
  Wire.write(BACKLIGHT_OFF); 
  
  Wire.endTransmission();
}

void ascii_chars(byte dir_lcd, int sensor, int medicion){
  
  Wire.beginTransmission(dir_lcd); 
  Wire.write(COMMAND_REGISTER);

  Wire.write(sensor+49);
  for(int i=0; i<medicion; i++){
    Wire.write(124);
  }
  for(int i=0; i<6-medicion; i++){
    Wire.write(32);
  }
  Wire.write(32);
  
  Wire.endTransmission();
}

byte read_fifo_length(byte dir_lcd){
  
  Wire.beginTransmission(dir_lcd);
  Wire.write(FIFO_AVAILABLE_LENGTH_REGISTER); 
  Wire.endTransmission();
  
  Wire.requestFrom(dir_lcd,byte(1)); 
  while(!Wire.available()) { }
  return Wire.read();
}

void set_display_type(byte dir_lcd, byte tipo){
  
  Wire.beginTransmission(dir_lcd);
  
  Wire.write(COMMAND_REGISTER); 
  Wire.write(SET_DISPLAY_TYPE); 
  Wire.write(tipo);
  
  Wire.endTransmission();
}

/*
 * Funciones para Sensores
 */
void comando_escribir(byte dir_sensor, byte comando){

  Wire.beginTransmission(dir_sensor);
  Wire.write(COMMAND_REGISTER);
  Wire.write(comando);
  Wire.endTransmission();
}

byte leer_registro(byte dir_sensor, byte registro){
  
  Wire.beginTransmission(dir_sensor);
  Wire.write(registro);
  Wire.endTransmission();

  Wire.requestFrom(dir_sensor, byte(1));
  while(!Wire.available());
  return Wire.read();
  
}

void setup() {

  Serial.begin(9600);

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  attachInterrupt(digitalPinToInterrupt(3), despertar, RISING);
  
  noInterrupts();
  
  TCCR1A = 0;
  TCCR1B = 0;
  
  TCCR1A |= (1<<(COM1A1));
  TCCR1B |= (1<<(WGM12)) | (1<<(CS12));
  TIMSK1 |= (1<<(OCIE1A));

  OCR1A = 62499;
  
  interrupts();

  Wire.begin();
  delay(100);

  set_display_type(LCD05_I2C_ADDRESS,LCD_STYLE_16X2);
  clear_screen(LCD05_I2C_ADDRESS);
  cursor_home(LCD05_I2C_ADDRESS);
  show_blinking_cursor(LCD05_I2C_ADDRESS);
  backlight_off(LCD05_I2C_ADDRESS);

  for(int i = 0; i<=cantSensores;i++){
    if(i == 0){
      EEPROM.put(i,cantSensores);
    } else {
      EEPROM.put(i,SRF02_I2C_ADDRESS[i-1]);
    }
  }
}

void loop() {

  if(flag == 0){
    Serial.println(F("--- Menú de opciones ---"));
    Serial.println(F("-- Opcion <reloj>: Para establecer o restablecer el reloj."));
    Serial.println(F("-- Opcion <dormir>: Para poner en modo sleep el microprocesador."));
    Serial.println(F("-- Opcion <orden>: Para cambiar el numero de sensores a usar y el orden deseado."));
    Serial.println(F("-- Opcion <fecha>: Para mostrar la fecha y hora actual."));
    Serial.println(F("-- Opcion <mostrar>: Para mostrar el numero de sensores trabajando y su orden de disparo."));
    Serial.println(F("-- Opcion <cambiar>: Para cambiar la unidad de medida de los sensores"));
    Serial.println(F("~~> Por favor, introduzca la opcion deseada [Introducir solamente la palabra clave]: \n"));  

    flag++;
  }
  
  if(Serial.available()){
    String comando = Serial.readStringUntil('\n');
    comando.trim(); comando.toLowerCase();

    if (comando == "reloj"){
      activarReloj();
      Serial.println();
    } else if(comando == "fecha"){
      mostrarHora();
    }  else if(comando == "dormir"){
      cli();
      sleep_enable();

      sei();
      sleep_cpu();
      sleep_disable();
    } else if(comando == "orden"){
      cambiarOrden();
      Serial.println();
      
    } else if(comando == "mostrar"){
      for(int i = 0; i<=cantSensores; i++){
        if(i == 0){
          Serial.println("Cantidad = " + String(EEPROM.read(i)));
        } else {
          Serial.print("Sensor " + String(i) + " = 0x");
          Serial.println(EEPROM.read(i),HEX);
        } 
      }
      Serial.println();
    } else if(comando == "cambiar"){
      cambiarMedida();
      Serial.println();
    }
  }  
  
  for(int i = 0; i<cantSensores; i++){
    comando_escribir(SRF02_I2C_ADDRESS[i],REAL_RANGING_MODE[modo]);
    delay(100);
  }
   
  for(int i = 0; i<cantSensores; i++){
    byte high_byte_range = leer_registro(SRF02_I2C_ADDRESS[i],RANGE_HIGH_BYTE);
    byte low_byte_range = leer_registro(SRF02_I2C_ADDRESS[i],RANGE_LOW_BYTE);

    float medicion = int((high_byte_range<<8) | low_byte_range);
     
    if(modo == 0){
      if(medicion>98.42) medicion = 98.42;
      //Serial.print("Sensor 0x"); Serial.print(SRF02_I2C_ADDRESS[i],HEX); Serial.print(" = ");
      //Serial.print(medicion); Serial.println(" in.");
      medicion = medicion / 98.42;
    } else if(modo == 1){
      if(medicion>250.0) medicion = 250;
      //Serial.print("Sensor 0x"); Serial.print(SRF02_I2C_ADDRESS[i],HEX); Serial.print(" = ");
      //Serial.print(medicion); Serial.println(" cms.");
      medicion = medicion / 250;
    } else if(modo == 2){
      if(medicion>14575) medicion = 14575; 
      //Serial.print("Sensor 0x"); Serial.print(SRF02_I2C_ADDRESS[i],HEX); Serial.print(" = ");
      //Serial.print(medicion); Serial.println(" uS.");
      medicion = ((medicion /29.15)/2)/250;
    }
    
    mostrar_medicion(i, (medicion*5)+1);
    
    if(i == 3 && cantSensores>4){
      delay(2000);
      clear_screen(LCD05_I2C_ADDRESS);
    } else if(i == cantSensores-1 && cantSensores<8){
      cursor_home(LCD05_I2C_ADDRESS);
    }
  }
  delay(1000);
}

void despertar(){
}

void cambiarOrden(){
  
  Serial.println(F("Por favor introduzca la cantidad de sensores deseados."));
  
  while(true){

    if(Serial.available()){
      char comando = Serial.parseInt();

      if(comando < 9 && comando != NULL){
        cantSensores = comando;
        break;
      } else{
        Serial.println("\nHa introducido una cantidad erronea, vuelva a intentarlo.");
      }
    }
  }
  
  Serial.println(F("\nPor favor introduzca el orden deseado separado por comas y sin espacios[en decimal]."));
  Serial.println(F("Direcciones posibles[HEX-DEC]:"));
  Serial.println(F("0x73-115 0x74-116 0x75-117 0x77-119"));
  Serial.println(F("0x78-120 0x79-121 0x7B-123 0x7E-126"));
   
  while(!Serial.available()){}

  String comando = Serial.readStringUntil('\n');
  comando.trim();

  for (int i = 0; i < cantSensores ; i++){
    char index = comando.indexOf(',');
    SRF02_I2C_ADDRESS[i] = comando.substring(0, index).toInt();
    comando = comando.substring(index + 1);
  }
  
  for(int i = 0; i<=cantSensores; i++){
    if(i == 0){
      EEPROM.update(i,cantSensores);
    } else {
      EEPROM.update(i,SRF02_I2C_ADDRESS[i-1]);
    }
  }
  
  clear_screen(LCD05_I2C_ADDRESS);
}


void cambiarMedida(){
 
  Serial.println(F("->Si desea cambiar la unidad de medida introduzca el comando: medida <opcion>."));
  Serial.println(F("->Opciones:"));
  Serial.println(F("   -Opcion <0>: Resultado en pulgadas."));
  Serial.println(F("   -Opcion <1>: Resultado en centimetros."));
  Serial.println(F("   -Opcion <2>: Resultado en microsegundos."));
  

  while(true){
    if(Serial.available() > 0){
      String comando = Serial.readStringUntil('\n');
      comando.trim(); comando.toLowerCase();
      if(comando.substring(0,6) == "medida"){
        unsigned char aux = comando.substring(7).toInt();
        if(aux < 3 && aux >= 0){
          modo = aux;
          Serial.println();
          break;
        } else{
          Serial.println(F("Ha introducido el comando erroneamente, vuelva a introducirlo."));
        }
      } else{
        Serial.println(F("Ha introducido el comando erroneamente, vuelva a introducirlo."));
      }
    }
  }
}


ISR(TIMER1_COMPA_vect){
  
  if(flagReloj){
    segundos++;
    if(segundos == 60){
      minutos++;
      segundos = 0;
    } else if(minutos == 60){
      hora++;
      minutos = 0;
    } else if(hora == 24){
      dia++;
      hora = 0;

      if(mes == 1 || mes == 3 || mes == 5 || mes == 7 || mes == 8 || mes == 10 || mes == 12){
        if (dia == 32 && mes == 12){
          mes = 1;
          dia = 1;
          anyo++;
        } else if( dia == 32){
          mes++;
          dia = 1;
        } 
      } else if(mes == 4 || mes == 6 || mes == 9 || mes == 11){
        if (dia == 31){
          mes++;
          dia = 1;
        }
      }  else if(mes == 2){
        if (dia == 29){
            mes++;
            dia = 1;
        }
      }
    }
  }
}

void introducirDato(char * variable, String dato){

  Serial.print(F("Por favor, introduzca "));
  Serial.print(dato); Serial.print(": ");
  while(true){
    if(Serial.available()){
      char aux = Serial.parseInt();
      Serial.println();
      if(dato == "minutos"){
        if(aux < 60 && aux >= 0 && aux != NULL){
          *variable = aux;
          break;
        }
      } else if(dato == "hora"){
        if(aux < 24 && aux >= 0 && aux != NULL){
          *variable = aux;
          break;
        }
      } else if(dato == "dia"){
        if(aux <= 31 && aux > 0 && aux != NULL){
          *variable = aux;
          break;
        }
      } else if(dato == "mes"){
        if(aux <= 12 && aux > 0 && aux != NULL){
          *variable = aux;
          break;
        }
      }
      Serial.print(F("Ha introducido erroneamente el "));
      Serial.print(dato); Serial.println(F(". Vuelva a introducirlo."));
      Serial.print(F("Por favor, introduzca "));
      Serial.print(dato); Serial.print(": ");
    }
  }
}

void introducirAnyo(int * variable, String dato){

  Serial.print(F("Por favor, introduzca "));
  Serial.print(dato); Serial.print(": ");
  while(true){
    if(Serial.available()){
      int aux = Serial.parseInt();
      Serial.println();
      if(aux >= 2021 && aux != NULL){
        *variable = aux;
        break;
      }
      Serial.print(F("Ha introducido erroneamente el "));
      Serial.print(dato); Serial.println(F(". Vuelva a introducirlo."));
      Serial.print(F("Por favor, introduzca "));
      Serial.print(dato); Serial.println(": ");
    }
  }
}

void mostrarHora(){
  String fecha= "";
  String fecha_hora = "";
  if(dia<10){
    fecha = fecha + "0";
  }
  fecha = fecha + String(dia) + "/";
  if(mes<10){
    fecha = fecha + "0";
  }
  fecha = fecha + String(mes) + "/" + String (anyo);
  if(hora<10){
    fecha_hora = fecha_hora + "0";
  }
  fecha_hora = fecha_hora + String(hora) + ":";
  if(minutos<10){
    fecha_hora = fecha_hora + "0";
  }
  fecha_hora = fecha_hora + String(minutos) + ":";
  if(segundos<10){
    fecha_hora = fecha_hora + "0";
  }
  fecha_hora = fecha_hora + String(segundos);

  mostrar_hora_lcd(fecha, fecha_hora);

  delay(5000);
  cursor_home(LCD05_I2C_ADDRESS);
  clear_screen(LCD05_I2C_ADDRESS);
}

void activarReloj(){

  Serial.println(F("Debe introducir la fecha y la hora.\n"));
  introducirDato(&minutos, "minutos");
  introducirDato(&hora, "hora");
  introducirDato(&dia, "dia");
  introducirDato(&mes, "mes");
  introducirAnyo(&anyo, "año");

  flagReloj = true;

  noInterrupts();
  
  TCNT1H = 0;
  TCNT1L = 0;
  TIFR1 = 0;

  interrupts();

  segundos = 0;

}
