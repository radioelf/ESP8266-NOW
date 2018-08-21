/*
Test protocolo ESP-NOW maestro.
Enviamos al esclavo los valores del maestro de la entrada ADC, 
el periodo desde que se inicializó y el estado de los pines GPIO 0-15
y recibimos la respuesta del esclavo con los valor de la entrada ADC, 
el periodo desde que se inicializó y el estado de los pines GPIO 0-15

info : http://www.esploradores.com/practica-6-conexion-esp-now/

Creative Commons License Disclaimer

UNLESS OTHERWISE MUTUALLY AGREED TO BY THE PARTIES IN WRITING, LICENSOR OFFERS THE WORK AS-IS
AND MAKES NO REPRESENTATIONS OR WARRANTIES OF ANY KIND CONCERNING THE WORK, EXPRESS, IMPLIED,
STATUTORY OR OTHERWISE, INCLUDING, WITHOUT LIMITATION, WARRANTIES OF TITLE, MERCHANTIBILITY,
FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT, OR THE ABSENCE OF LATENT OR OTHER DEFECTS,
ACCURACY, OR THE PRESENCE OF ABSENCE OF ERRORS, WHETHER OR NOT DISCOVERABLE. SOME JURISDICTIONS
DO NOT ALLOW THE EXCLUSION OF IMPLIED WARRANTIES, SO SUCH EXCLUSION MAY NOT APPLY TO YOU.
EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGES.

http://creativecommons.org/licenses/by-sa/3.0/

  Author: Radioelf  http://radioelf.blogspot.com.es/
*/

#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}
#define PinLed  2                                                     // led PCB GPIO 2

//Estructura de datos TX al esclavo (la misma en el esclavo)
    struct ESTRUCTURA_DATOS_TX{
    uint16_t ADC_TX;                                                  // entrada ADC
    uint32_t PERIODO_TX;                                              // millis()
    uint16_t MASCARA_TX;                                              // GPIO 0-16
};
//Estructura de datos RX del esclavo (la misma en el esclavo)
    struct ESTRUCTURA_DATOS_RX{
    uint16_t ADC_RX;                                                  // entrada ADC
    uint32_t PERIODO_RX;                                              // millis()
    uint16_t MASCARA_RX;                                              // GPIO 0-16
};
void setup() {
  Serial.begin(115200);
  pinMode(PinLed, OUTPUT);
  delay(250);
  Serial.println();
   
  if (esp_now_init()!=0){                                             // inicializamos ESP_Now
    Serial.println("FALLO al inicializar ESP_Now");   
    for (uint8_t x=0; x !=5; x++){                                    // indicación error a través del led
      digitalWrite(PinLed, LOW);                                      // ON led
      delay(150);
      digitalWrite(PinLed, HIGH);                                     // OFF led
      delay(150);
    }
    ESP.restart();                                                    // reiniciamos..
    delay(10);
  }
  Serial.print("AP MAC maestro: ");                                   // obtenemos la MAC del Punto de Acceso
  Serial.println(WiFi.softAPmacAddress());
  Serial.print("STA MAC maestro: ");                                  // obtenemos la MAC de la estación base
  Serial.println(WiFi.macAddress());
  esp_now_set_self_role(1);                                           // tipo 0 =sin función, 1 =maestro, 2 =esclavo, 3=maestreo y esclavo 

  // Unión con el esclavo, AP MAC de modulo esclavo 1-> 86:F3:EB:9F:71:E5
  uint8_t esclavo_MAC[6] = {0x86, 0xF3, 0xeb, 0x9f, 0x71, 0xe5};
  uint8_t modo=2;                                                     // modo esclavo
  uint8_t canal=7;                                                    // canal 1-13
  uint8_t clave[0]={};                                                // sin clave 16 bytes 
  uint8_t clave_long=sizeof(clave);
  if (esp_now_add_peer(esclavo_MAC, modo, canal, clave, clave_long)){ // Anyade un nuevo par a la tabla de comunicación ESP-NOW.
	Serial.println("MAC esclavo: 0x86:0xF3:0xeb:0x9f:0x71:0xe5");
	Serial.print("Canal radio:  "); Serial.println(canal);
	if (clave_long !=0){
		Serial.print("Clave:  "); Serial.println(*clave);
		Serial.print("Longitud de la clave:  "); Serial.println(clave_long);
	}else{
	Serial.println("SIN clave");
	}
  }else{
	  Serial.println("esclavo: 0x86:0xF3:0xeb:0x9f:0x71:0xe5 NO incluido en la TABLA DE COMUNICACI0N!!!");
	  while(1){}
  }
  digitalWrite(PinLed, HIGH);                                         // apagado
  delay(3000);
}

void loop() {
  ESTRUCTURA_DATOS_TX DT;  
  DT.PERIODO_TX = millis();                   
  DT.ADC_TX = analogRead(A0);
  DT.MASCARA_TX =GPIO_mascara();
  Serial.print("Valor ADC: "); 
  Serial.println(DT.ADC_TX);
  Serial.print("Valor para el perido: ");
  Serial.println(DT.PERIODO_TX);
  Serial.print("Mascara GPIO: ");
  Serial.println(DT.MASCARA_TX, BIN);
  delay(20);
  uint8_t MAC_esclavo[6] = {0x86, 0xF3, 0xeb, 0x9f, 0x71, 0xe5};      // MAC AP  esclavo
  uint8_t data[sizeof(DT)];
  memcpy(data, &DT, sizeof(DT));
  uint8_t len = sizeof(data);
  esp_now_send(MAC_esclavo, data, len);                               // TX datos al esclavo

  delay(1);                                                           // espera rx
  esp_now_register_send_cb([](uint8_t* mac, uint8_t status){          // estado de la transmisión
    Serial.print("Enviado al esclavo con MAC: ");
    Serial.print(mac[0], HEX);Serial.print(mac[1], HEX);Serial.print(mac[2], HEX);
    Serial.print(mac[3], HEX);Serial.print(mac[4], HEX);Serial.print(mac[5], HEX);
    Serial.println((status)?" Error ":" OK");
    digitalWrite(PinLed, status);                                     // 1->OFF led=error, 0->ON led= OK
  });
  if (!digitalRead (PinLed)){                                         // OK
    delay (5000);
    digitalWrite(PinLed, HIGH);                                       // apagado
    delay (200);
  }
  // recepción de los datos del esclavo 
  esp_now_register_recv_cb([](uint8_t *mac, uint8_t *data, uint8_t len){ //RX
    Serial.print("RX del esclavo con MAC: ");
    Serial.print(mac[0], HEX);Serial.print(mac[1], HEX);Serial.print(mac[2], HEX);
    Serial.print(mac[3], HEX);Serial.print(mac[4], HEX);Serial.print(mac[5], HEX);
    if (len){
      Serial.println(" OK");
    }else{
      Serial.println(" ERROR");
    }
    ESTRUCTURA_DATOS_RX DR;
    memcpy(&DR, data, sizeof(DR));
    Serial.println("Datos del esclavo:");
    Serial.print("Valor ADC: "); Serial.println(DR.ADC_RX);
    Serial.print("Valor periodo RUN: "); Serial.println(DR.PERIODO_RX);
    Serial.print("Mascara GPIO: "); Serial.println(DR.MASCARA_RX, BIN);
  });
}
// Obtenemos la mascara para el estado de GPIO 0-15 (falta el estado de GPIO16)
uint16_t GPIO_mascara(){
uint16_t mascara_GPIO =0;
  for(uint8_t x =0; x !=16; x++){
    bitWrite(mascara_GPIO, x, digitalRead(x));
  }
  return (mascara_GPIO);
}

