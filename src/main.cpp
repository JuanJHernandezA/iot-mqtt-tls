#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include "secrets.h"   

// ======================
// Configuración WiFi
// ======================
void conectarWiFi() {
  Serial.print("Conectando a WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// ======================
// Configuración MQTT
// ======================
WiFiClientSecure secureClient;
PubSubClient client(secureClient);

// Callback: recibe mensajes del tópico “in”
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido en ");
  Serial.print(topic);
  Serial.print(": ");
  String mensaje;
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }
  Serial.println(mensaje);


  if (mensaje.equalsIgnoreCase("ON")) {
    Serial.println("Activando sistema de riego...");
    // digitalWrite(pinRiego, HIGH);
  } else if (mensaje.equalsIgnoreCase("OFF")) {
    Serial.println("Apagando sistema de riego...");
    // digitalWrite(pinRiego, LOW);
  }
}

// Conexión al broker MQTT
void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT... ");
    if (client.connect("ESP32Client", MQTT_USER, MQTT_PASS)) {
      Serial.println("conectado!");
      client.subscribe(MQTT_TOPIC_IN); 
      client.setCallback(callback); 
      Serial.printf("suscrito a: %s\n", MQTT_TOPIC_IN);
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" — Reintentando en 5s...");
      delay(5000);
    }
  }
}

// Publicar dato en el tópico “out”
void enviarDatoAlDominio(int valor) {
  if (!client.connected()) {
    conectarMQTT();
  }

  char mensaje[10];
  sprintf(mensaje, "%d", valor);

  if (client.publish(MQTT_TOPIC_OUT, mensaje)) {
    Serial.printf("Dato publicado (%s): %d\n", MQTT_TOPIC_OUT, valor);
  } else {
    Serial.println("Error al publicar el mensaje MQTT.");
  }
}


WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "Sensor de Humedad activo");
}

void setup() {
  Serial.begin(115200);

  conectarWiFi();

  secureClient.setInsecure(); 
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  conectarMQTT();

  server.on("/", handleRoot);
  server.begin();
  Serial.println("Servidor Web iniciado en puerto 80");
}

// ======================
//Loop principal
// ======================
void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();
  server.handleClient();

  // Ejemplo: lectura simulada de humedad
  int humedad = random(30, 80);
  enviarDatoAlDominio(humedad);
  delay(5000);
}
