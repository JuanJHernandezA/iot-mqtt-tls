#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WebServer.h>

// ======================
// ⚙️ CONFIGURACIÓN WIFI
// ======================
const char* ssid = "......";
const char* password = "......";

// ======================
// 🌐 CONFIGURACIÓN MQTT (TLS)
// ======================
const char* mqtt_server = "esp32univalle.duckdns.org";
const int mqtt_port = 8883;
const char* mqtt_user = "alvaro";
const char* mqtt_pass = "supersecreto";
const char* mqtt_topic = "test/topic";

// Usamos cliente seguro
WiFiClientSecure secureClient;
PubSubClient client(secureClient);

// ======================
// 🔁 FUNCIONES MQTT
// ======================
void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("🔌 Conectando al broker MQTT... ");
    if (client.connect("ESP32Client", mqtt_user, mqtt_pass)) {
      Serial.println("✅ Conectado!");
    } else {
      Serial.print("❌ Error, rc=");
      Serial.print(client.state());
      Serial.println(" — Reintentando en 5s...");
      delay(5000);
    }
  }
}

void enviarDatoAlDominio(int valor) {
  if (!client.connected()) {
    conectarMQTT();
  }

  char mensaje[10];
  sprintf(mensaje, "%d", valor);

  if (client.publish(mqtt_topic, mensaje)) {
    Serial.printf("✅ Dato publicado: %d\n", valor);
  } else {
    Serial.println("❌ Error al publicar el mensaje MQTT.");
  }
}

// ======================
// 🖥️ SERVIDOR WEB LOCAL
// ======================
WebServer server(80);

void handleRoot() {
  String html = "<html><head><title>Control de Valor</title></head><body>";
  html += "<h2>Ingrese un número entre 0 y 100</h2>";
  html += "<form action='/enviar' method='GET'>";
  html += "<input type='number' name='valor' min='0' max='100' required>";
  html += "<input type='submit' value='Enviar'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleEnviar() {
  if (server.hasArg("valor")) {
    int valor = server.arg("valor").toInt();
    if (valor >= 0 && valor <= 100) {
      enviarDatoAlDominio(valor);
      server.send(200, "text/html",
        "<h3>✅ Valor publicado correctamente: " + String(valor) + "</h3><a href='/'>Volver</a>");
    } else {
      server.send(400, "text/html", "<h3>❌ Valor fuera de rango (0–100)</h3><a href='/'>Volver</a>");
    }
  } else {
    server.send(400, "text/html", "<h3>⚠️ No se recibió ningún valor</h3>");
  }
}

// ======================
// 🚀 SETUP
// ======================
void setup() {
  Serial.begin(115200);
  Serial.println("\nIniciando...");

  // Conexión WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Conectado a WiFi");
  Serial.print("📶 IP local: ");
  Serial.println(WiFi.localIP());

  // MQTT seguro (ignorar verificación del certificado en desarrollo)
  secureClient.setInsecure();  // ⚠️ Permite conexión sin verificar CA (solo para pruebas)
  client.setServer(mqtt_server, mqtt_port);

  // Iniciar servidor web
  server.on("/", handleRoot);
  server.on("/enviar", handleEnviar);
  server.begin();
  Serial.println("🌐 Servidor web iniciado");
}

// ======================
// 🔁 LOOP
// ======================
void loop() {
  if (!client.connected()) {
    conectarMQTT();
  }
  client.loop();
  server.handleClient();
}
