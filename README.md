# Desarrollo de fluorómetro aplicado a la medición de ficocianina
Proyecto Integrador para obtener el título de Ingeniero en Computación.  
- Autor: Segura, Gastón Marcelo  
- Director: Ing. Rodriguez Gonzalez, Santiago

Este proyecto integrador, realizado en el Laboratorio de Hidráulica de la Universidad Nacional de Córdoba, se centra en el desarrollo de un prototipo de sensor de fluorescencia para detección de ficocianina. Se busca diseñar una solución para integrar la medición de ficocianina con la de clorofila en un único sistema.

## 📌 Características
- Utiliza el sensor **AS7262** y el LED **TLCR6800** para medición de ficocianina.
- Utiliza el sensor **AS7263** y el LED **XZCB25X109FS** para medición de clorofila.
- Soporta configuración de ganancia y tiempo de integración de los sensores.
- Utiliza la librería SparkFun AS726X de Andrew England para facilitar la comunicacion I2C con el sensor AS7262. Este proyecto funcionó con la versión 1.0.5, por lo tanto se recomienda instalar la misma.

## 💻 Protocolos
Los sensores AS7262 y AS7263 comparten la misma dirección I2C, la cual está definida por hardware y no puede ser modificada. Esto impide que ambos dispositivos operen
simultáneamente utilizando el mismo protocolo de comunicación. Para resolver esta limitación, y aprovechando que ambos sensores también pueden operar mediante UART, se decide implementar un esquema híbrido: el sensor AS7262 se configura para comunicarse mediante I2C, mientras que el AS7263 utiliza UART. Esta solución permite integrar ambos dispositivos en el mismo sistema sin conflictos de dirección, garantizando una comunicación efectiva y sin interferencias.

## 🔧 Conexión de la comunicación I2C y UART entre sensores y ESP32
- **ESP32 TX2 (GPIO 17)** → **RX del AS7263**
- **ESP32 RX2 (GPIO 16)** → **TX del AS7263**
- **ESP32 I2C_SDA/I2C_SCL** → **AS7262 SDA_S/SCL_S**
- **Alimentación:** 3.3V

## 🔦 Modulación
Con el objetivo de optimizar la relación señal/ruido en las mediciones, se implementa una técnica de modulación basada en una onda cuadrada que consiste en alternar la corriente de alimentación de los dos LEDs emisores entre dos niveles: un valor máximo de 100 mA y un valor mínimo de 0 mA. Durante los ciclos de encendido (a 100 mA), los LEDs emiten luz con su máxima intensidad, lo que maximiza la fluorescencia detectada por los sensores. El valor medido en este estado es proporcional a la señal de interés. Por otro lado, durante los ciclos de apagado (a 0 mA), los sensores solo registran el ruido de fondo, ya que no hay emisión de luz desde los LEDs. Este enfoque permite separar la señal de interés del ruido inherente al sistema, mejorando la precisión de las mediciones.

## 🔬 Funcionamiento
El prototipo está diseñado para operar en dos modos: detección de ficocianina o detección de clorofila. Adicionalmente, incluye la opción de calibración que permite ajustar el dispositivo antes de realizar mediciones. Ambos sensores realizan dos mediciones ya que utilizan la modulación descrita en el título anterior, activando el LED correspondiente a cada caso. Para calibrar el sensor de ficocianina, se enciende el LED de 450 nm y se apaga el de 620 nm. Esto se debe a que la calibración se realiza con rodamina, un pigmento que requiere un LED azul para su excitación. Aunque ambos sensores permanecen encendidos, solo uno de ellos realiza el sensado en cada momento, dependiendo del modo de operación seleccionado.

Tanto la ganancia como el tiempo de integración se configuran en sus valores máximos para garantizar la obtención de la máxima intensidad de fluorescencia posible. 

El código implementado está desarrollado en C++ y ejecutado en el entorno Arduino IDE para programar una ESP32.

## 📜 Código Fuente
```cpp
#include "AS726X.h"

AS726X sensor;    // Sensor as7262
#define RX_PIN 16 // Pin de RX2 en ESP32 conectado a TX del sensor
#define TX_PIN 17 // Pin de TX2 en ESP32 conectado a RX del sensor

//------------------------------------------------

bool ficocianina = false; // false = clorofila
bool calibracion = false;

//------------------------------------------------

void sendATCommand(String command, float* channelValues = nullptr);
float clampToZero(float value);


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
  Wire.setClock(400000);          // Fast mode
  sensor.setGain(3);              // Valores de ganancia: 0(1x), 1(3.7x), 2(16x), 3(64x)
  sensor.setIntegrationTime(255); // Tiempo de integración: 255 * 2.8ms = 714 ms
  sensor.disableIndicator();
  if(!(ficocianina && !calibracion)){
    sensor.disableBulb();         // Para apagar el led rojo
  }

}

void setupAS7263()
{
  sendATCommand("AT"); 
  delay(1000); 

  sendATCommand("ATINTTIME=255"); // Valores de tiempo de integracion: desde 1 a 255

  sendATCommand("ATGAIN=3");      // Valores de ganancia: 0(1x), 1(3.7x), 2(16x), 3(64x)

  if(ficocianina && !calibracion)
  {
    sendATCommand("ATLED1=0");    // Apaga led azul
  }
  
}

void loop() {
  if(ficocianina)
  {

    float violet_100mA, blue_100mA, green_100mA, yellow_100mA, orange_100mA, red_100mA;
    float violet_0mA, blue_0mA, green_0mA, yellow_0mA, orange_0mA, red_0mA;
    
    if(!calibracion){ // led rojo
      sensor.enableBulb();
      sensor.setBulbCurrent(3);       // Los valores posibles de corriente son 0: 12.5mA, 1: 25mA, 2: 50mA, 3: 100mA
    }else{            // led azul
      sendATCommand("ATLED1=100");    // Prende led azul
      sendATCommand("ATLEDC=0x30");   // Corriente del led azul a 100 mA
    }

    delayMicroseconds(286000);        // Recordar que el tiempo de integracion es de 255(714 ms), por lo tanto 714 ms + 286 ms = 1 segundo

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
      sensor.disableBulb();        // Apaga led rojo
    }else{
      sendATCommand("ATLED1=0");   // Apaga led azul
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
    Serial.print(clampToZero(violet_100mA - violet_0mA), 2);
    Serial.print(" , 500nm: ");
    Serial.print(clampToZero(blue_100mA - blue_0mA), 2);
    Serial.print(" , 550nm: ");
    Serial.print(clampToZero(green_100mA - green_0mA), 2);
    Serial.print(" , 570nm: ");
    Serial.print(clampToZero(yellow_100mA - yellow_0mA), 2);
    Serial.print(" , 600nm: ");
    Serial.print(clampToZero(orange_100mA - orange_0mA), 2);
    Serial.print(" , 650nm: ");
    Serial.println(clampToZero(red_100mA - red_0mA), 2);

  }else { // CLOROFILA --------------------------------------------------------------------------------------
    float channels_100mA[6] = {0.0};          // Almacena los valores de los canales para 100 mA
    float channels_0mA[6] = {0.0};            // Almacena los valores de los canales para 0 mA
    float fluorescence[6] = {0.0};            // Almacena los valores de fluorescencia calculados
  
    sendATCommand("ATLED1=100");              // Prende led azul
    sendATCommand("ATLEDC=0x30");             // Configura corriente a 100 mA

    delayMicroseconds(286000);
    sendATCommand("ATCDATA", channels_100mA); // Lee los valores que devuelve el sensor con el led a 100 mA

    sendATCommand("ATLED1=0");                // Apaga led azul

    delayMicroseconds(286000);

    sendATCommand("ATCDATA", channels_0mA);   // Lee los valores que devuelve el sensor con el led a 0 mA

    // Imprime los valores de cada canal con la resta de los valores de 100 mA y 0 mA
    String channelNames[] = {"610", "680", "730", "760", "810", "860"};
    for (int i = 0; i < 6; i++) {
      fluorescence[i] = clampToZero(channels_100mA[i] - channels_0mA[i]);
      Serial.print(channelNames[i] + " nm: ");
      Serial.print(fluorescence[i], 2);       // Muestra 2 decimales
      Serial.print(" , ");
    }
    Serial.println("");
  }
}

/*
  Función que se encarga de reacondicionar el valor que retorna el sensor as7263 al enviar un comando AT
*/
void sendATCommand(String command, float* channelValues) {
    Serial2.print(command + "\r\n");  // Envía el comando al sensor
    delay(300);                       // Espera para la respuesta del sensor

    if (command == "ATCDATA" && channelValues != nullptr) {
        String response = "";         // Almacena el valor actual del canal
        int channelIndex = 0;

        while (Serial2.available()) {
            char c = Serial2.read();
            if (c == ',' || c == '\r' || c == '\n' || c == 'O') {     // Detecta separadores y elimina saltos de línea
                if (response.length() > 0 && channelIndex < 6) {
                    channelValues[channelIndex] = response.toFloat(); // Convierte a float
                    response = "";
                    channelIndex++;
                }
            } else if (Serial2.peek() == 'K') {
                Serial2.read(); // Lee y descarta el 'K' para completar "OK"
                break;          // Termina la lectura al encontrar "OK"
            } else {
                response += c;  // Agrega el carácter al valor actual
            }
        }
    } else {
        while (Serial2.available()) {
            Serial2.read();     // Descarta la salida para otros comandos
        }
    }
}

// Función para evitar valores negativos
float clampToZero(float value) {
  return value < 0 ? 0 : value;
}
```

## 🚀 Instrucciones de Uso
1. Conectar el dispositivo a la PC mediante el cable USB.
2. Abrir **Arduino IDE** o **PlatformIO** con el monitor serie a **115200 baudios**.
3. Observar los datos en el monitor serie que nos devuelven los sensores.

## ⌨️ Configuración de Variables
- **`ficocianina = true`** → Medición de ficocianina.
- **`ficocianina = false`** → Medición de clorofila.
- **`calibracion = false/true`** → Modo de medición sin calibración o con calibración.
- **`sensor.setGain(3)`** → Configura la ganancia del sensor AS7262 con sus posibles valores: 0(1x), 1(3.7x), 2(16x), 3(64x)
- **`sensor.setIntegrationTime(255)`** → Configura el tiempo de integracion del sensor AS7262 con los valores que van desde 1 hasta 255, el valor de tiempo de integración sera el colocado multiplicado por 2.8 ms.
- **`sendATCommand("ATGAIN=3")`** → Configura la ganancia del sensor AS7263, similar a los que sucede con el AS7262
- **`sendATCommand("ATINTTIME=255")`** → Configura el tiempo de integracion del sensor AS7263, similar a los que sucede con el AS7262

## 📊 Ejemplo de Salida en Monitor Serie  
Se procede a observar los valores de cuentas que devuelve cada uno de los canales del sensor que se seleccionó(el AS7262 en este caso):
```
450nm: 12.34 , 500nm: 10.56 , 550nm: 8.23 , 570nm: 7.45 , 600nm: 6.78 , 650nm: 5.90
```

## 🔗 Recursos
En este proyecto se incluyen dos carpetas denominadas "sensores" y "emisores", en las cuales están las hojas de datos de cada uno de los elementos utilizados. Además, se adjunta el informe del proyecto integrador realizado, para más información.

---
📌 **Autor:** Gaston Marcelo Segura.  
📅 **Última actualización:** Marzo 2025.

