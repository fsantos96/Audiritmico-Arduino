#include "arduinoFFT.h" // Liberia para medir la serie de f
#define SAMPLES 128 // muestra necesaria para el calculo de la frecuencia
#define SAMPLING_FREQ 2048 // constante que permite relacionar los bits a herts (el valor maximo medido en bits)

// #include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include "config.h";

//conexion segura para telegram
WiFiClientSecure client;
//iniacializa la conexion con el bot
UniversalTelegramBot bot(BOTtoken, client);

arduinoFFT FFT = arduinoFFT(); // inicializacion de la libreria FFT
unsigned int samplingPeriod; // variable para guardar el peridodo de la serie
unsigned long microSeg; // variable para guardar los microsegundo de ejecucion

unsigned int senialMax = 0;  //Valor minimo de la señal
unsigned int senialMin = 1024;  //Valor Maximo de la señal
int amplitude = 0;
int decibels = 0; // variable para guardar la medida en decibeles
// variables para guardar los puntos los puntos de la serie
double vReal[SAMPLES];
double vIm[SAMPLES];

void setup() {
  Serial.begin(115200);
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQ));
  pinMode(PIRPin, INPUT);
  pinMode(GreenPin, OUTPUT);
  pinMode(BluePin, OUTPUT);
  pinMode(RedPin, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);

  // inicializa el cliente para http
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

const turnOnLedsWithConfig() {
   HTTPClient http;
  // http.begin(baseApi + "/configuration");
  int httpCode = http.GET();
  if (httpCode > 0) { //Check for the returning code

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, http.getStream());
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      bot.sendMessage(chat_id, "Ocurrio un error al procesar su configuracion", "Markdown");
      return false;
    }

    int color = doc["colorConfig"].as<int>();
    turnOnRbgLeds(color);
  } else {
    Serial.println("Error al obtener los datos de la api");
    bot.sendMessage(chat_id, "Ocurrio un error al obtener su configuracion", "Markdown");
  }
  Serial.println("libero");
  http.end();
}
// prende la combinacion de los colores
void turnOnRbgLeds(colorLed) {
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

// funcion que revisa si hay nuevos mensajes en telegram
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    if (text == "/start") {
      String welcome = "Bienvenido, " + from_name + ". Le notificaremos los registros de movimiento\n";
      welcome += "Ingrese al sitio " + baseApi + "para configurar su sensor\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void sendMessageIfDbAreHigh(int decibels) {
 if (decibels >= 70) {
     // enviar mensaje a telegram
  }
}

void loop() {
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
      senialMin = vReal[i];  //  // Guardo el valor minimo de la señal
    }
    // pausa para la lectura
    while (micros() < (microSeg + samplingPeriod )) {

    }
  }

  amplitude = senialMax - senialMin;  // se calcula la amplitud
  decibels = map(amplitude, 0, 2048, 49, 120);  // relacionamos el minimo y maximo en bits contra el min en decibeles
  // funcion para enviar mensaje por telegram si el volumen es alto
  sendMessageIfDbAreHigh(decibels);
  
  // Se evalua la longitud de la onda
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  //
  FFT.Compute(vReal, vIm, SAMPLES, FFT_FORWARD);
  // se obtiene la medidas en hz de cada punto
  FFT.ComplexToMagnitude(vReal, vIm, SAMPLES);

  //se obtiene el pico maximo.
  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQ);
  Serial.println(peak);
  delay(2000);

}
