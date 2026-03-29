#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// ==============================
// CONFIGURATION (MUST CHANGE!)
// ==============================
const char* ssid = "AIR";         // Change to your Wi-Fi SSID
const char* password = "asdqwe1234"; // Change to your Wi-Fi Password
const char* pi_ip = "10.147.40.182";          // Change to your Raspberry Pi IP
const int pi_port = 8888;
//여기까진 Setting up WiFi credentials and UDP target에 대한 내용

WiFiUDP udp;

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[SUCCESS] WiFi Connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());
}
//여기까진 Initializing serial communication and connecting to the physical Wi-Fi network에 대한 내용
void loop() {
  JsonDocument doc;
  doc["device_id"] = "ARM_NODE_01";
  doc["timestamp"] = millis();
  
  static int counter = 0;
  counter++;

  String current_state = "SAFE";
  int err_code = 0;
  bool is_e_stop = false;
  float shoulder_effort = 45.2;
  float shoulder_temp = 41.5;

  int cycle = counter % 20;

  if (cycle == 5) {
    // Stage 2: WARNING (High load, no immediate action needed)
    current_state = "WARNING";
    err_code = 1001; 
    shoulder_effort = 85.0; // Torque rising
    shoulder_temp = 62.0;   // Temp rising
  } else if (cycle == 10) {
    // Stage 3: DANGER (Critical limit, requires speed reduction)
    current_state = "DANGER";
    err_code = 2001; 
    shoulder_effort = 140.5; // High torque warning
    shoulder_temp = 78.0;    // Overheating warning
  } else if (cycle == 15) {
    // Stage 4: ERROR (Collision, immediate halt)
    current_state = "ERROR";
    err_code = 4021; 
    is_e_stop = true;
    shoulder_effort = 210.5; // Torque spike
    shoulder_temp = 88.3;    // Critical overheat
  }
  //여기까진 Simulating 4 stages of physical degradation (Safe, Warning, Danger, Error) based on effort and temperature에 대한 내용

  JsonObject status = doc["status"].to<JsonObject>();
  status["state"] = current_state;
  status["error_code"] = err_code;
  status["emergency_stop"] = is_e_stop;

  JsonObject joints = doc["joints"].to<JsonObject>();
  JsonArray names = joints["names"].to<JsonArray>();
  names.add("base"); names.add("shoulder"); names.add("elbow"); names.add("wrist"); names.add("gripper");
  
  JsonArray positions = joints["positions"].to<JsonArray>();
  positions.add(1.57); positions.add(-0.52); positions.add(1.20); positions.add(0.0); positions.add(0.5);
  
  JsonArray efforts = joints["efforts"].to<JsonArray>();
  efforts.add(12.5); efforts.add(shoulder_effort); efforts.add(30.1); efforts.add(5.0); efforts.add(1.2);
  
  JsonArray temps = joints["temperatures"].to<JsonArray>();
  temps.add(35.2); temps.add(shoulder_temp); temps.add(38.0); temps.add(32.1); temps.add(30.0);
  
  JsonObject pose = doc["end_effector"]["pose"].to<JsonObject>();
  pose["x"] = 250.5; pose["y"] = 100.2; pose["z"] = 300.0;
  pose["roll"] = 0.0; pose["pitch"] = 1.57; pose["yaw"] = 0.0;
  doc["end_effector"]["gripper_active"] = true;

  String payload;
  serializeJson(doc, payload);

  udp.beginPacket(pi_ip, pi_port);
  udp.print(payload);
  udp.endPacket();

  Serial.println("State [" + current_state + "] sent: " + payload);
  delay(500); 
}
