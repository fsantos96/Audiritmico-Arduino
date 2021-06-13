#include "arduinoFFT.h" // Liberia para medir la serie de f
#include <WiFi.h> // libreria para wifi
#include <WiFiClientSecure.h> // libreri para wifi seguro
#include <HTTPClient.h> // liberia para usar http
#include <ArduinoJson.h> // libreria para procesar json
#include <UniversalTelegramBot.h> // libreria de telegram
#include "config.h" // archivo de configuracion

#define SAMPLES 128 // muestra necesaria para el calculo de la frecuencia
#define SAMPLING_FREQ 2048 // constante que permite relacionar los bits a herts (el valor maximo medido en bits)

arduinoFFT FFT = arduinoFFT(); //inicializacion de la libreria de FFT
WiFiClientSecure client; // inicializa el cliente de wifi
UniversalTelegramBot bot(BOTtoken, client); // inicializa al bot de telegram
unsigned long lastTimeBotRan; // variable de delay para pedir los mensajes de telegram 
unsigned int samplingPeriod; // variable para guardar el peridodo de la serie
unsigned long microSeg; // variable para guardar los microsegundo de ejecucion
String chat_id = "";
unsigned int senialMax = 0;  //Valor minimo de la señal
unsigned int senialMin = 2048;  //Valor Maximo de la señal
int amplitude = 0;
int decibels = 0; // variable para guardar la medida en decibeles
// variables para guardar los puntos los puntos de la serie
double vReal[SAMPLES];
double vIm[SAMPLES];

void setup() {
  Serial.begin(115200);
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQ)); // calcula el periodo para los calculos de Fourier
  pinMode(PIRPin, INPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);
  pinMode(RedPin, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  // levanta el cliente seguro.
  client.setInsecure();
  delay(100);
//conecta al wifi
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(10);
}

// apaga todos los leds
void turnOfAllLeds() {
  digitalWrite(GreenPin, LOW);
  digitalWrite(BluePin, LOW);
  digitalWrite(RedPin, LOW);
}

// prende la combinacion de los colores
void turnOnRbgLeds(int colorLed) {
  turnOfAllLeds();
  switch (colorLed) {
    case 0: //rojo
      digitalWrite(RedPin, HIGH);
      break;
    case 1: //azul
      digitalWrite(BluePin, HIGH);
      break;
    case 2: // verde
      digitalWrite(GreenPin, HIGH);
      break;
    case 3: // banco
      digitalWrite(RedPin, HIGH);
      digitalWrite(GreenPin, HIGH);
      digitalWrite(BluePin, HIGH);
      break;
    case 4: // cyan
      digitalWrite(GreenPin, HIGH);
      digitalWrite(BluePin, HIGH);
      break;
    case 5: //amarillo
      digitalWrite(RedPin, HIGH);
      digitalWrite(GreenPin, HIGH);
      break;
    case 6: // magenta
      digitalWrite(RedPin, HIGH);
      digitalWrite(BluePin, HIGH);
      break;
    default:
      digitalWrite(BluePin, HIGH);
  }
}

void turnOnLedsWithConfig(int peak) {
  HTTPClient http;
  http.begin(baseApi + "/getArduinoConfig");
  int httpCode = http.GET();
  if (httpCode > 0) { //Check for the returning code

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      bot.sendMessage(chat_id, "Ocurrio un error al procesar su configuracion", "Markdown");
    } else {
     int amountConfigurations = doc["amountConfigurations"].as<int>();
     int colorId = 0;
     for(int i = 0; i < amountConfigurations; i++) {
       if(doc["configurations"][i]["min"].as<int>() <= peak && doc["configurations"][i]["max"].as<int>() > peak) {
         colorId = doc["configurations"][i]["color"].as<int>()
         break;
       }
     }

     turnOnRbgLeds(colorId); 
    }
  } else {
    Serial.println("Error al obtener los datos de la api");
    bot.sendMessage(chat_id, "Ocurrio un error al obtener su configuracion", "Markdown");
  }
  Serial.println("libero");
  http.end();
}

// funcion que procesa los mensaje de telegram
void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    if (text == "/start") {
      String welcome = "Bienvenido, " + from_name + "\n";
      welcome += "Ingrese al sitio " + urlSite + " para configurar tus colores\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void sendMessageIfDbAreHigh(int decibels) {
 if (decibels >= 70) {
    bot.sendMessage(chat_id, "El volumen que esta usando es muy alto", "Markdown");
  }
}

void loop() {
   // escucha los mensajes de telegrm
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

  // se recorre de a un mensaje
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }

  // seteo de los puntos de la serie en la variable
  for (int i = 0; i < SAMPLES; i++) {
    microSeg = micros();
    vReal[i] = analogRead(MicPin);
    vIm[i] = 0;

    
    if (vReal[i] > senialMax)
    {
      senialMax = vReal[i];  // Guardo el valor maximo de la señal
    }
    else if (vReal[i] < senialMin)
    {
      senialMin = vReal[i];  //  Guardo el valor minimo de la señal
    }
    // pausa para la lectura
    while (micros() < (microSeg + samplingPeriod )) {

    }
  }

  amplitude = senialMax - senialMin;  // se calcula la amplitud
  decibels = map(amplitude, 0, 2048, 48, 120);  // relacionamos el minimo y maximo en bits contra el min en decibeles
  // funcion para enviar mensaje por telegram si el volumen es alto
  sendMessageIfDbAreHigh(decibels);
  // crea una ventana de muestra y se evalua la longitud de la onda
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  //
  FFT.Compute(vReal, vIm, SAMPLES, FFT_FORWARD);
  // se obtiene la medidas en hz de cada punto
  FFT.ComplexToMagnitude(vReal, vIm, SAMPLES);

  //se obtiene el pico maximo.
  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQ);
  turnOnLedsWithConfig(peak);
  Serial.println(peak);
  delay(2000); //ajustar este time dps de pruebas

}
