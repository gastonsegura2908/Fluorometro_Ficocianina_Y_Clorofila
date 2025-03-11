# Desarrollo de fluorómetro aplicado a la medición de ficocianina
Proyecto Integrador para obtener el título de Ingeniero en Computación.  
- Autor: Segura, Gastón Marcelo  
- Director: Ing. Rodriguez Gonzalez, Santiago

Este proyecto integrador, realizado en el Laboratorio de Hidráulica de la Universidad Nacional de Córdoba, se centra en el desarrollo de un prototipo de sensor de fluorescencia para detección de ficocianina. Se busca diseñar una solución para integrar la medición de ficocianina con la de clorofila en un único sistema.

## 📌 Características
- Utiliza el sensor **AS7262** y el LED **TLCR6800** para medición de ficocianina.
- Utiliza el sensor **AS7263** y el LED **XZCB25X109FS** para medición de clorofila.
- Soporta configuración de ganancia y tiempo integración de los sensores.

## 🔧 Conexión de Hardware
- **ESP32 TX2 (GPIO 17)** → **RX del AS7263**
- **ESP32 RX2 (GPIO 16)** → **TX del AS7263**
- **ESP32 SDA/SCL** → **AS7262 (I2C)**
- **Alimentación:** 3.3V / 5V dependiendo del sensor

## 📜 Código Fuente
```cpp
#include "AS726X.h"

AS726X sensor;
#define RX_PIN 16 // RX2 en ESP32 conectado a TX del sensor
#define TX_PIN 17 // TX2 en ESP32 conectado a RX del sensor

bool ficocianina = true; // false = clorofila
bool calibracion = false;

void sendATCommand(String command, float* channelValues = nullptr);

void setup() {
  Wire.begin();
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
  setupAS7262();
  setupAS7263();
}

void setupAS7262() {
  if (!sensor.begin()) {
    Serial.println("Error: Sensor AS7262 no conectado");
    while (1);
  }
  Wire.setClock(400000);
  sensor.setGain(3);
  sensor.setIntegrationTime(255);
  sensor.disableIndicator();
  if (!(ficocianina && !calibracion)) {
    sensor.disableBulb();
  }
}

void setupAS7263() {
  sendATCommand("AT");
  delay(1000);
  sendATCommand("ATINTTIME=255");
  sendATCommand("ATGAIN=3");
  if (ficocianina && !calibracion) {
    sendATCommand("ATLED1=0");
  }
}

void loop() {
  if (ficocianina) {
    float violet_100mA, blue_100mA, green_100mA, yellow_100mA, orange_100mA, red_100mA;
    float violet_0mA, blue_0mA, green_0mA, yellow_0mA, orange_0mA, red_0mA;

    if (!calibracion) {
      sensor.enableBulb();
      sensor.setBulbCurrent(3);
    } else {
      sendATCommand("ATLED1=100");
      sendATCommand("ATLEDC=0x30");
    }
    delayMicroseconds(286000);

    sensor.takeMeasurements();
    if (sensor.getVersion() == SENSORTYPE_AS7262) {
      violet_100mA = sensor.getCalibratedViolet();
      blue_100mA = sensor.getCalibratedBlue();
      green_100mA = sensor.getCalibratedGreen();
      yellow_100mA = sensor.getCalibratedYellow();
      orange_100mA = sensor.getCalibratedOrange();
      red_100mA = sensor.getCalibratedRed();
    }
    
    Serial.print("450nm: "); Serial.print(violet_100mA - violet_0mA, 2);
    Serial.print(" , 500nm: "); Serial.print(blue_100mA - blue_0mA, 2);
    Serial.print(" , 550nm: "); Serial.print(green_100mA - green_0mA, 2);
    Serial.print(" , 570nm: "); Serial.print(yellow_100mA - yellow_0mA, 2);
    Serial.print(" , 600nm: "); Serial.print(orange_100mA - orange_0mA, 2);
    Serial.print(" , 650nm: "); Serial.println(red_100mA - red_0mA, 2);
  }
}

void sendATCommand(String command, float* channelValues) {
  Serial2.print(command + "\r\n");
  delay(300);
  if (command == "ATCDATA" && channelValues != nullptr) {
    String response = "";
    int channelIndex = 0;
    while (Serial2.available()) {
      char c = Serial2.read();
      if (c == ',' || c == '\r' || c == '\n' || c == 'O') {
        if (response.length() > 0 && channelIndex < 6) {
          channelValues[channelIndex] = response.toFloat();
          response = "";
          channelIndex++;
        }
      } else {
        response += c;
      }
    }
  }
}
```

## 🚀 Instrucciones de Uso
1. Conectar los sensores a la ESP32 siguiendo el esquema de conexiones.
2. Cargar el código en la ESP32 utilizando el **Arduino IDE** o **PlatformIO**.
3. Abrir el monitor serie a **115200 baudios**.
4. Observar los datos de espectrometría en el monitor serie.

## 📌 Configuración de Variables
- **`ficocianina = true`** → Medición de ficocianina.
- **`calibracion = false`** → Modo de medición estándar (sin calibración).
- **`sensor.setGain(3)`** → Configura la ganancia del sensor AS7262.
- **`sendATCommand("ATGAIN=3")`** → Configura la ganancia del sensor AS7263.

## 📊 Ejemplo de Salida en Monitor Serie
```
450nm: 12.34 , 500nm: 10.56 , 550nm: 8.23 , 570nm: 7.45 , 600nm: 6.78 , 650nm: 5.90
```

## 🔗 Recursos
- [AS726X Datasheet](https://ams.com/as726x)
- [ESP32 Documentation](https://docs.espressif.com/)

---
📌 **Autor:** Gaston Segura Marcelo  
📅 **Última actualización:** Marzo 2025

