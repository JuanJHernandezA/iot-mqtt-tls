#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>

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

// Construir tópico MQTT dinámicamente
String construirTopico(const char* direccion) {
  String topico = String(PAIS) + "/" + String(PROVINCIA) + "/" + 
                  String(CIUDAD) + "/" + String(ID_DISPOSITIVO) + "/" + 
                  String(MQTT_USER) + "/" + String(direccion);
  return topico;
}

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
      String topicoIn = construirTopico("in");
      client.subscribe(topicoIn.c_str()); 
      client.setCallback(callback); 
      Serial.printf("suscrito a: %s\n", topicoIn.c_str());
    } else {
      Serial.print("Error, rc=");
      Serial.print(client.state());
      Serial.println(" — Reintentando en 5s...");
      delay(5000);
    }
  }
}

// Publicar dato en el tópico "out" en formato JSON
void enviarDatoAlDominio(int valor) {
  if (!client.connected()) {
    conectarMQTT();
  }

  // Construir JSON con el valor de humedad
  String jsonPayload = "{\"humedad\":" + String(valor) + "}";
  
  String topicoOut = construirTopico("out");
  if (client.publish(topicoOut.c_str(), jsonPayload.c_str())) {
    Serial.printf("Dato publicado (%s): %s\n", topicoOut.c_str(), jsonPayload.c_str());
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
