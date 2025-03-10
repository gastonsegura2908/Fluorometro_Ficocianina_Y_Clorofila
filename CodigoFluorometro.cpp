
#include "AS726X.h"

AS726X sensor; // sensor as7262
#define RX_PIN 16 // Pin de RX2 en ESP32 conectado a TX del sensor
#define TX_PIN 17 // Pin de TX2 en ESP32 conectado a RX del sensor

//------------------------------------------------

bool ficocianina = true; // false = clorofila
bool calibracion = false;

//------------------------------------------------

void sendATCommand(String command, float* channelValues = nullptr);



void setup() {
  Wire.begin();
  Serial.begin(115200);

  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Comunicación UART con el sensor

  setupAS7262();

  setupAS7263();

}

void setupAS7262()
{
  if (sensor.begin() == false)
  {
    Serial.println("Sensor does not appear to be connected. Please check wiring. Freezing...");
    while (1)
      ;
  }
  Wire.setClock(400000);
  sensor.setGain(3); // Establece la ganancia en 16x. // de 0 a 3 puede ser
  sensor.setIntegrationTime(255); // Tiempo de integración: 59 * 2.8ms = 165.2 ms
  sensor.disableIndicator();
  // Para encender el led rojo que esta en bulb3: ------------------------------------------------
  if(!(ficocianina && !calibracion)){
    // Para apagarlo: ------------------------------------------
    sensor.disableBulb();
    //-----------------------------------------------------------------------------------------------
  }

}

void setupAS7263()
{
  sendATCommand("AT"); 
  delay(1000); // Esperar un poco antes de iniciar

  sendATCommand("ATINTTIME=255"); //valores: desde 1 a 255

  sendATCommand("ATGAIN=3"); // valores:0(1x),1(3.7x),2(16x),3(64x)

  if(ficocianina && !calibracion)
  {
    sendATCommand("ATLED1=0");   // Apaga LED_DRV
  }
  
}

void loop() {

  if(ficocianina)
  {

    float violet_100mA, blue_100mA, green_100mA, yellow_100mA, orange_100mA, red_100mA;
    float violet_0mA, blue_0mA, green_0mA, yellow_0mA, orange_0mA, red_0mA;
    
    if(!calibracion){
      sensor.enableBulb();
      sensor.setBulbCurrent(3); // los valores posibles de corriente son 0: 12.5mA, 1: 25mA, 2: 50mA.(VAN EN EL PRIMER PARAMETRO)
    }else{
      sendATCommand("ATLED1=100"); // Prende LED_DRV
      sendATCommand("ATLEDC=0x30");  // corriente del led,posibles valores:0,1,2,3:100 mA
    }
    delayMicroseconds(286000);

    sensor.takeMeasurements();
    if (sensor.getVersion() == SENSORTYPE_AS7262)
    {
      violet_100mA = sensor.getCalibratedViolet();
      blue_100mA = sensor.getCalibratedBlue();
      green_100mA = sensor.getCalibratedGreen();
      yellow_100mA = sensor.getCalibratedYellow();
      orange_100mA = sensor.getCalibratedOrange();
      red_100mA = sensor.getCalibratedRed();      
    }else{
      Serial.print("Error al obtener version del sensor");
      Serial.println();
    }

    if(!calibracion){
      sensor.disableBulb();
    }else{
      sendATCommand("ATLED1=0");   // Apaga LED_DRV
    } 
    delayMicroseconds(286000);

    sensor.takeMeasurements();
    if (sensor.getVersion() == SENSORTYPE_AS7262)
    {
      violet_0mA = sensor.getCalibratedViolet();
      blue_0mA = sensor.getCalibratedBlue();
      green_0mA = sensor.getCalibratedGreen();
      yellow_0mA = sensor.getCalibratedYellow();
      orange_0mA = sensor.getCalibratedOrange();
      red_0mA = sensor.getCalibratedRed();
    }else{
      Serial.print("Error al obtener version del sensor");
      Serial.println();
    }

    Serial.print("450nm: ");
    Serial.print(violet_100mA - violet_0mA, 2);
    Serial.print(" , 500nm: ");
    Serial.print(blue_100mA - blue_0mA, 2);
    Serial.print(" , 550nm: ");
    Serial.print(green_100mA - green_0mA, 2);
    Serial.print(" , 570nm: ");
    Serial.print(yellow_100mA - yellow_0mA, 2);
    Serial.print(" , 600nm: ");
    Serial.print(orange_100mA - orange_0mA, 2);
    Serial.print(" , 650nm: ");
    Serial.println(red_100mA - red_0mA, 2);

  }else { // CLOROFILA
    float channels_100mA[6] = {0.0}; // Almacena los valores de los canales para 100 mA
    float channels_0mA[6] = {0.0};  // Almacena los valores de los canales para 50 mA
    float fluorescence[6] = {0.0};   // Almacena los valores de fluorescencia calculados

    // Prende LED con 100 mA y mide
    sendATCommand("ATLED1=100"); // Prende LED_DRV
    sendATCommand("ATLEDC=0x30");   // Configura corriente a 100 mA
    delayMicroseconds(286000);
    sendATCommand("ATCDATA", channels_100mA);

    // Configura LED a 0 mA y mide
    sendATCommand("ATLED1=0");
    delayMicroseconds(286000);
    sendATCommand("ATCDATA", channels_0mA);

    // Calcula fluorescencia total para cada canal
    String channelNames[] = {"610", "680", "730", "760", "810", "860"};
    for (int i = 0; i < 6; i++) {
      fluorescence[i] = channels_100mA[i] - channels_0mA[i];
      Serial.print(channelNames[i] + " nm: ");
      Serial.print(fluorescence[i], 2); // Muestra 2 decimales
      Serial.print(" , ");
    }
    Serial.println("");
  }
}

void sendATCommand(String command, float* channelValues) {
    Serial2.print(command + "\r\n");  // Envía el comando al sensor
    delay(300); // Espera para la respuesta del sensor

    if (command == "ATCDATA" && channelValues != nullptr) {
        String response = ""; // Almacena el valor actual del canal
        int channelIndex = 0;

        while (Serial2.available()) {
            char c = Serial2.read();
            if (c == ',' || c == '\r' || c == '\n' || c == 'O') { // Detecta separadores y elimina saltos de línea
                if (response.length() > 0 && channelIndex < 6) {
                    channelValues[channelIndex] = response.toFloat(); // Convierte a float
                    response = "";
                    channelIndex++;
                }
            } else if (Serial2.peek() == 'K') {
                Serial2.read(); // Lee y descarta el 'K' para completar "OK"
                break;          // Termina la lectura al encontrar "OK"
            } else {
                response += c; // Agrega el carácter al valor actual
            }
        }
    } else {
        while (Serial2.available()) {
            Serial2.read(); // Descarta la salida para otros comandos
        }
    }
}
