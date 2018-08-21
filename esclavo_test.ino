/*
Test protocolo ESP-NOW esclavo.
Recibimos del maestro los valores de la entrada ADC, el periodo 
desde que se inicializó y el estado de los pines GPIO 0-15 y
respondemos con los valores de la entrada ADC, el periodo desde que 
se inicializó y el estado de los pines GPIO 0-15 del esclavo
*/

#include <ESP8266WiFi.h> 
extern "C" {
  #include <espnow.h>
}

#define PinLed  2                                                     // led PCB GPIO 2

//Estructura de datos RX del maestro(la misma en el maestro)
    struct ESTRUCTURA_DATOS_RX {
    uint16_t ADC_RX =0;                                               // entrada ADC
    uint32_t PERIODO_RX =0;                                           // millis()
    uint16_t MASCARA_RX =0;                                           // GPIO 0-15
};
//Estructura de datos TX al maestro (la misma en el maestro)
    struct ESTRUCTURA_DATOS_TX{
    uint16_t ADC_TX;                                                   // entrada ADC
    uint32_t PERIODO_TX;                                               // millis()
    uint16_t MASCARA_TX;                                               // GPIO 0-16
};
void setup() {
  Serial.begin(115200);
  pinMode(PinLed, OUTPUT);
  delay(250);
  Serial.println();
    
  if (esp_now_init()!=0){                                               // inicializamos ESP_Now
    Serial.println("FALLO al inicializar ESP_Now");   
    for (uint8_t x=0; x !=5; x++){                                      // indicación error a través del led
      digitalWrite(PinLed, LOW);                                        // ON led
      delay(150);
      digitalWrite(PinLed, HIGH);                                       // OFF led
      delay(150);
    }
    ESP.restart();                                                      // reiniciamos..
    delay(10);
  }
  Serial.print("AP MAC esclavo: ");                                     // obtenemos la MAC del Punto de Acceso
  Serial.println(WiFi.softAPmacAddress());
  Serial.print("STA MAC esclavo: ");                                    // obtenemos la MAC de la estación base
  Serial.println(WiFi.macAddress());
  esp_now_set_self_role(2);                                             // tipo 0 =sin función, 1 =maestro, 2 =esclavo, 3=maestro y esclavo 
  pinMode(PinLed, OUTPUT);
  digitalWrite(PinLed, HIGH);                                           // apagamos led
  delay(3000);
}
uint8_t led_on =0;
uint32_t espera =0, espera_mem =0;
uint8_t MAC_maestro[6] = {0x68, 0xC6, 0x3A, 0xD6, 0xEF, 0x37};          // MAC AP  MAESTRO
void loop() {
    esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len){ //RX
    Serial.print("<-RX del maestro con MAC: ");
    Serial.print(mac[0], HEX);Serial.print(mac[1], HEX);Serial.print(mac[2], HEX);
    Serial.print(mac[3], HEX);Serial.print(mac[4], HEX);Serial.print(mac[5], HEX);
    if (len){
      Serial.println(" OK");
      
      led_on =1;
    }else{
      Serial.println(" ERROR");
      led_on =0;
    }
    ESTRUCTURA_DATOS_RX DT;
    memcpy(&DT, data, sizeof(DT));

    Serial.print("ADC: "); Serial.println(DT.ADC_RX);
    Serial.print("Periodo RUN: "); Serial.println(DT.PERIODO_RX);
    Serial.print("Mascara GPIO: "); Serial.println(DT.MASCARA_RX, BIN);
  });
  if (led_on ==1){
    digitalWrite(PinLed, LOW);                                          // encendido
    ESTRUCTURA_DATOS_TX DR;  
    DR.PERIODO_TX = millis();                   
    DR.ADC_TX = analogRead(A0);
    DR.MASCARA_TX =GPIO_mascara();
    uint8_t data_[sizeof(DR)];
    memcpy(data_, &DR, sizeof(DR));
    uint8_t len_ = sizeof(data_);
    esp_now_send(MAC_maestro, data_, len_);                             // TX datos al maestro 
    Serial.println("->TX datos al maestro: ");
    Serial.print("ADC: "); Serial.println(DR.ADC_TX);
    Serial.print("Periodo RUN: "); Serial.println(DR.PERIODO_TX);
    Serial.print("Mascara GPIO: "); Serial.println(DR.MASCARA_TX, BIN);
    espera =millis();
    led_on =2;
  }
  if (led_on ==2 && millis() -espera >400){
    digitalWrite(PinLed, HIGH);                                           // apagado 
    led_on =0;
  }
}
// Obtenemos la mascara para el estado de GPIO 0-15 (falta el estado de GPIO16)
uint16_t GPIO_mascara(){
uint16_t mascara_GPIO =0;
  for(uint8_t x =0; x !=16; x++){
    bitWrite(mascara_GPIO, x, digitalRead(x));
  }
  return (mascara_GPIO);
}
