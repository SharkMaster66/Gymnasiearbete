#include <WiFiS3.h>

#define LIGHT_SENSOR_PIN A3
#define MOISTURE_SENSOR_PIN A4
#define TEMP_SENSOR_PIN A5
#define PUMP_PIN 7
#define GROWLIGHT_PIN 8
WiFiServer server(80);

const char ssid[] = "AutoBloom";
const char pass[] = "test1234";

// ===== HTML, CSS, JS go here =====
const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>TEST APP</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="stylesheet" href="/style.css">
</head>
<body>
<div class="card">
<h2>Light Sensor</h2>
<div class="value" id="lightValue">---</div>
<div class="status" id="lightStatus">Connecting...</div>
</div>
<div class="card">
<h2>Moisture Sensor</h2>
<div class="value" id="moistureValue">---</div>
<div class="status" id="moistureStatus">Connecting...</div>
</div>
<div class="card">
<h2>Temp Sensor</h2>
<div class="value" id="tempValue">---</div>
<div class="status" id="tempStatus">Connecting...</div>
</div>
<div class="card">
  <h2>Pump Control</h2>
  <button onclick="setPump(true)">ON</button>
  <button onclick="setPump(false)">OFF</button>
</div>
<div class="card">
  <h2>Grow Light</h2>
  <button onclick="setLight(true)">ON</button>
  <button onclick="setLight(false)">OFF</button>
</div>
<div class="card">
  <h2>Mode</h2>
  <button onclick="setMode(true)">AUTO</button>
  <button onclick="setMode(false)">MANUAL</button>
</div>
<script src="/script.js"></script>
</body>
</html>
)rawliteral";

const char STYLE_CSS[] PROGMEM = R"rawliteral(
body {
    font-family: Arial, sans-serif;
    background: #f2f2f2;
    height: 100vh;
    margin: 0;
}

.card {
    background: white;
    padding: 20px 30px;
    border-radius: 12px;
    box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
    text-align: center;
    min-width: 250px;
}

h2 {
    margin-bottom: 10px;
}

.value {
    font-size: 2.5rem;
    font-weight: bold;
}

.status {
    margin-top: 10px;
    font-size: 0.9rem;
    color: gray;
}

)rawliteral";

const char SCRIPT_JS[] PROGMEM = R"rawliteral(
function updateData() {
  fetch('/data')
    .then(res => res.json())
    .then(data => {
      document.getElementById("lightValue").textContent = data.light;
      document.getElementById("moistureValue").textContent = data.moisture;
      document.getElementById("tempValue").textContent = data.temp;

      document.getElementById("lightStatus").textContent = "Connected";
      document.getElementById("moistureStatus").textContent = "Connected";
      document.getElementById("tempStatus").textContent = "Connected";
    })
    .catch(() => {
      document.getElementById("lightStatus").textContent = "Disconnected";
    });
}

setInterval(updateData, 500);
window.addEventListener("load", updateData);

function setPump(on) {
  fetch(`/set?pump=${on ? "on" : "off"}`);
}

function setLight(on) {
  fetch(`/set?light=${on ? "on" : "off"}`);
}

function setMode(auto) {
  fetch(`/set?mode=${auto ? "auto" : "manual"}`);
}


)rawliteral";

// ===== End of embedded files =====


// SENSORER

int moisture = 0;
int light = 0;
int temp = 0;

// AKTUATORER

bool pumpOn = false;
bool lightOn = false;
int moistureThreshold = 500;
int lightThreshold = 500;
bool autoMode = true;

void setup() {
  Serial.begin(115200);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(MOISTURE_SENSOR_PIN, INPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(GROWLIGHT_PIN, OUTPUT);
  
  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(GROWLIGHT_PIN, LOW);

  WiFi.beginAP(ssid, pass);
  delay(2000);


  Serial.print("AP SSID: ");
  Serial.println(WiFi.SSID());

  Serial.print("AP IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  unsigned long start = millis();
  while (!client.available()) {
    if (millis() - start > 2000) { client.stop(); return; }
  }

  char req[64];
  int i = 0;
  while (client.available() && i < 63) {
    char c = client.read();
    if (c == '\n') break;
    req[i++] = c;
  }
  req[i] = 0;

  // Sensors
  light = analogRead(LIGHT_SENSOR_PIN);
  moisture = analogRead(MOISTURE_SENSOR_PIN);
  temp = analogRead(TEMP_SENSOR_PIN);

 
  // AKTUATORER
  
  // Automatic watering example
  if (autoMode) {
    if (!pumpOn && moisture < moistureThreshold) {
      pumpOn = true;
      Serial.println("Pump activated automatically");
    } else if (pumpOn) {
      pumpOn = false;
      Serial.println("Pump deactivated automatically");
    }

    if (light < lightThreshold) {
      lightOn = true;
    } else {
      lightOn = false;
    }
  }
  
  digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);
  digitalWrite(GROWLIGHT_PIN, lightOn ? HIGH : LOW);

  

  if (strstr(req, "GET /style.css")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/css");
    client.println();
    client.print(STYLE_CSS);
  } 
  else if (strstr(req, "GET /script.js")) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/javascript");
    client.println();
    client.print(SCRIPT_JS);
  } 
  else if (strstr(req, "GET /data")) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println();
  
  client.print("{\"light\":");
  client.print(light);
  
  client.print(",\"moisture\":");
  client.print(moisture);
  
  client.print(",\"temp\":");
  client.print(temp);
  
  client.print(",\"pump\":");
  client.print(pumpOn ? "true" : "false");
  
  client.print(",\"growLight\":");
  client.print(lightOn ? "true" : "false");
  
  client.print("}");
  }
  else if (strstr(req, "GET /set")) {
    if (strstr(req, "pump=on")) {
      pumpOn = true;
      Serial.println("Pump Activated");
    }
    if (strstr(req, "pump=off")) {
      pumpOn = false;
      Serial.println("Pump Deactivated");
    }
    if (strstr(req, "light=on")) {
      lightOn = true;
      Serial.println("Light Activated");
    }
    if (strstr(req, "light=off")) {
      lightOn = false;
      Serial.println("Light Deactivated");
    }
    if (strstr(req, "mode=auto")) {
      autoMode = true;
      Serial.println("Automatic mode Activated");
    }
    if (strstr(req, "mode=manual")) {
      autoMode = false;
      Serial.println("Automatic mode Deactivated");
    }
    
    // Example: threshold
    char *t = strstr(req, "threshold=");
    if (t) {
      moistureThreshold = atoi(t + 10);
    }
    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println();
    client.print("OK");
  }
  else {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.print(INDEX_HTML);
  }

  delay(5);
  client.stop();
}
