#include "HX711.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <DHT.h>
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

//wifi
const char* ssid = "DESKTOP";
const char* password = "12345678";
const char* mqtt_server = "192.168.137.177";
const char* mqtt_topic = "Pesa";

//variables apra wifi
int ledPin = 2; //Connection state
int ledPin2 = 4; //Show received messages
WiFiClient wifiClient;
PubSubClient client(wifiClient);

IPAddress server(192,168,137,177);
const int mqtt_port = 1883;

//INCKINACION
int pinSensor = 14;
int pinLed = 4;

//Buzzer
const int Buzzer= 13;
const int Tiempo_Alto=500;
const int Tiempo_Bajo=1000;

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

//Humedad
int SENSOR = 15;
int temp;
int humedad;

DHT dht (SENSOR, DHT11);

//Peso
const int zero = 2;
const int LOADCELL_DOUT_PIN = 19;
const int LOADCELL_SCK_PIN = 18;

HX711 balanza;

int peso_calibracion = 185; // Es el peso referencial a poner, en mi caso mi celular pesa 185g (SAMSUMG A20)
long escala;

int state_zero = 0;
int last_state_zero = 0;

//Función calibración
void calibration() { // despues de hacer la calibracion puedes borrar toda la funcion "void calibration()"
  boolean conf = true;
  long adc_lecture;

  // restamos el peso de la base de la balaza
  lcd.setCursor(0, 0);
  lcd.print("Calibrando base");
  lcd.setCursor(4, 1);
  lcd.print("Balanza");
  delay(3000);
  balanza.read();
  balanza.set_scale(); //La escala por defecto es 1
  balanza.tare(20);  //El peso actual es considerado zero.
  lcd.clear();

  //Iniciando calibración
  while (conf == true) {

    lcd.setCursor(1, 0);
    lcd.print("Peso referencial:");
    lcd.setCursor(1, 1);
    lcd.print(peso_calibracion);
    lcd.print(" g        ");
    delay(3000);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("Ponga el Peso");
    lcd.setCursor(1, 1);
    lcd.print("Referencial");
    delay(3000);

    //Lee el valor del HX711
    adc_lecture = balanza.get_value(100);

    //Calcula la escala con el valor leido dividiendo el peso conocido
    escala = adc_lecture / peso_calibracion;

    //Guarda la escala en la EEPROM
    EEPROM.put( 0, escala );
    delay(100);
    lcd.setCursor(1, 0);
    lcd.print("Retire el Peso");
    lcd.setCursor(1, 1);
    lcd.print("referencial");
    delay(3000);
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.print("READY!!....");
    delay(3000);
    lcd.clear();
    conf = false; //para salir de while
    lcd.clear();

  }
}

void setup() 
{ 
  // Conectarse a la red WiFi
  WiFi.begin(ssid,password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a la red WiFi...");
  }
  Serial.println("Conexión WiFi establecida.");

 //Broker connect__________________
  client.setServer(mqtt_server, 1883);
  delay(1500);

  //INCLINACION
   pinMode(pinSensor,INPUT);
    pinMode(pinLed,OUTPUT);
  //BUZZER
  pinMode(Buzzer, OUTPUT);
  //dht
  dht.begin();
  //LCD
  lcd.begin(16, 2); 


  balanza.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);//asigana los pines para el recibir el trama del pulsos que viene del modulo
  pinMode(zero, INPUT);//declaramos el pin2 como entrada del pulsador
  pinMode(13,OUTPUT);
  lcd.init(); // Inicializamos el lcd
  lcd.backlight(); // encendemos la luz de fondo del lcd

  EEPROM.get( 0, escala );//Lee el valor de la escala en la EEPROM

  if (digitalRead(zero) == 1) { //esta accion solo sirve la primera vez para calibrar la balanza, es decir se presionar ni bien se enciende el sistema
    calibration();
  }
  balanza.set_scale(escala); // Establecemos la escala
  balanza.tare(20);  //El peso actual de la base es considerado zero.
  //serial
  Serial.begin(115200);
  balanza.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  delay(250);
  
  balanza.set_scale(397);
  balanza.tare(10);  // Hacer 10 lecturas, el promedio es la tara
}

void loop() 
{ 
// ================================================================================================  Creación de los objetos para JSON
    
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  
  //INCLINACION 
    if(digitalRead(pinSensor))
      {
        digitalWrite(pinLed,HIGH);
        delay(1000);
        digitalWrite(pinLed,LOW);
        delay(1000);
      }
      else
        digitalWrite(pinLed,LOW);
        //DHT11
  humedad = dht.readHumidity();
    temp = dht.readTemperature();

    lcd.clear();
    lcd.setCursor(1,16);
    lcd.print("Humedad: ");
    lcd.print(humedad);
    lcd.print("%");


  int state_zero = digitalRead(zero);
  float peso;
  peso = balanza.get_units(10);  //Mide el peso de la balanza
  
  //Muestra el peso
  lcd.setCursor(1, 0);
  lcd.print("Peso: ");
  lcd.print(peso, 0);
  lcd.println(" g        ");
  delay(5);

  //Botón de zero, esto sirve para restar el peso de un recipiente 
  if ( state_zero != last_state_zero) {

    if (state_zero == LOW) {
      balanza.tare(10);  //El peso actual es considerado zero.
    }
  }
  last_state_zero  = state_zero;

  if (peso>=500)digitalWrite(13,1);
  else if(peso<=500)digitalWrite(13,0);
  
   if (balanza.is_ready()) 
     { float reading = balanza.get_units(10);      
       Serial.println(reading  );
     } 
   else 
      Serial.println("HX711 not found.");
  
  delay(500);  

  //BUZZEER
 if (peso >= 100) {
  digitalWrite(Buzzer, HIGH); 
  } else {
  digitalWrite(Buzzer,LOW);

}
  float temperature1 = dht.readTemperature();
  float humidity1=dht.readHumidity();
  float peso1= peso;
  String json = "{\"sensor_id\":\"1\", \"value\":\""+String(temperature1)+"\"}";
  String json2= "{\"sensor_id\":\"2\", \"value\":\""+String(humidity1)+"\"}";
  String json3= "{\"sensor_id\":\"3\", \"value\":\""+String(peso1)+"\"}";
// ===========================================================================================  Converción y Envio de los Datos a JSON
//String json in serial
    //Sensor 1
    Serial.println(json);
    int str_len = json.length() + 1;//Calculte length of string
    char char_array[str_len];//Creating array with that length
    json.toCharArray(char_array, str_len);//Convert string to char array    
    client.publish("Temp", char_array); // Function send by MQTT with topic and value
   //Sensor 2
    Serial.println(json2);
    int str_len2 = json2.length() + 1;//Calculte length of string
    char char_array2[str_len2];//Creating array with that length
    json2.toCharArray(char_array2, str_len2);//Convert string to char array    
    client.publish("Hum", char_array2); // Function send by MQTT with topic and value
   //Sensor 3
    Serial.println(json3);
    int str_len3 = json3.length() + 1;//Calculte length of string
    char char_array3[str_len3];//Creating array with that length
    json3.toCharArray(char_array3, str_len3);//Convert string to char array    
    client.publish("Peso", char_array3); // Function send by MQTT with topic and value
  

}

void reconnect() {
  // Connection loop
  while (!client.connected()) {
    Serial.print("Tying connect...");
    // Intentar reconexión
    if (client.connect("client")) { 
      Serial.println("Connected");
      client.subscribe("Pesa"); 
    }else {
      Serial.print("Failed connect, Error rc=");
      Serial.print(client.state());
      Serial.println(" Trying in 5 seconds");
      delay(5000);
      Serial.println (client.connected()); 
    }
  }
}

  