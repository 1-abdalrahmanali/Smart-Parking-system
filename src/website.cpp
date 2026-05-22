#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Abdalrahman Ali";
const char* password = "12345678";

extern int availableSlots;

WebServer server(80);

String getHTML() {

  String html = "<!DOCTYPE html><html>";

  html += "<head>";
  html += "<title>Smart Parking</title>";

  // CSS
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#1e1e2f;color:white;}";
  html += ".card{margin:50px auto;padding:20px;width:300px;background:#333;border-radius:15px;}";
  html += ".count{font-size:80px;color:orange;}";
  html += "</style>";

  html += "</head>";

  // Body
  html += "<body>";
  html += "<div class='card'>";
  html += "<h1>Smart Parking</h1>";
  html += "<p>Available Slots</p>";

  // available slots
  html += "<div class='count' id='count'>";
  html += String(availableSlots);
  html += "</div>";

  html += "<p id='time'>Updated now</p>";
  html += "</div>";

  // JavaScript for page update
  html += "<script>";
  html += "async function updateCount(){";
  html += "const response = await fetch('/getUpdatedCount');";
  html += "const data = await response.text();";
  html += "document.getElementById('count').innerHTML = data;";
  html += "document.getElementById('time').innerHTML = new Date().toLocaleTimeString();";
  html += "}";
  html += "setInterval(updateCount,1000);";
  html += "</script>";
  html += "</body></html>";

  return html;
}

void startWebsite() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected! Open: http://" + WiFi.localIP().toString());

  server.on("/", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/html", getHTML());
  });
  server.on("/getUpdatedCount", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", String(availableSlots));
  });
  
  server.begin();
}

 void handleWebClient() { server.handleClient(); }