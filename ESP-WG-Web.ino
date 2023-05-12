// Include libraries
#include <WireGuard-ESP32.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi settings
const char ssid[] = "Your-WiFi-Network-Name";
const char password[] = "Your-WiFi-Network-Password";

// WireGuard settings
IPAddress local_ip(123, 123, 123, 123);                                      // IP address of the local interface
const char private_key[] = "GHlse+ybm/udXEvYs0EDpeUtNsOoKyJuEsQueIVUIEI=";   // Private key of the local interface
const char endpoint_address[] = "123.123.123.123";                           // Address of the endpoint peer
const char public_key[] = "75HF4FoC+tkihfaoksdh43tjkhlqwmlqS5Hk7JYp0qN=";    // Public key of the endpoint peer
const char presharedKey[] = "qo+ewLFqpmwslfavuEHqcPwaPAyHl+Gn+dgKhSzMQP9=";  // Pre-Shared Key
uint16_t endpoint_port = 12345;                                              // Port of the endpoint peer

// WireGuard class instance
static WireGuard wg;

// Set the server port
WebServer server(80);

void setup() {
  // Setting up a serial port for debugging
  Serial.begin(115200);
  
  // Connecting to a WiFi AP
  WiFi.begin(ssid, password);
  
  // Waiting until we connect to WiFi
  while (!WiFi.isConnected()) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  // Print the IP address in the local network
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  
  // Print the IP address in the WireGuard network
  Serial.print("WireGuard IP: ");
  Serial.println(local_ip.toString());

  // Handling requests to the root path
  server.on("/", handleRoot);
  
  // Handling requests to the readHall path
  server.on("/readHall", handleHall);
  
  // Starting a web server
  server.begin();

  // Synchronize system time via NTP
  configTime(9 * 60 * 60, 0, "time.google.com", "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");

  // Starting the WireGuard interface
  Serial.println("Initializing WG interface...");
  if (wg.begin(local_ip, private_key, endpoint_address, public_key, endpoint_port, preshared_key)) {
    Serial.println("OK");
  } else {
    Serial.println("FAIL");
  }
}

void loop() {  
  // Client service
  server.handleClient();
}

void handleHall() {
  // Read the value of the built-in Hall sensor
  int hall = hallRead();
  
  // Get current time in seconds
  unsigned long time = millis() / 1000;
  
  // Format JSON string with data
  String json = "{\"hall\": " + String(hall) + ", \"time\": " + String(time) + "}";
  
  // Send JSON response
  server.send(200, "application/json", json);
}

void handleRoot() {
  // Send HTML page with an AJAX script
  server.send(200, "text/html",
  "<!DOCTYPE html>\
  <html>\
    <head>\
      <meta charset=\"UTF-8\">\
      <style>\
        { box-sizing: border-box; margin: 0; padding: 0; }\
        body {\
          background-color: #333;\
          color: white; font-family: \"Lucida Console\", \"Courier New\", monospace;\
          display: flex; flex-direction: column;\
          align-items: center; justify-content: center;\
          height: 100vh;\
        }\
        h1 {\
          font-size: 36px;\
          margin-bottom: 20px;\
        }\
        h2 { margin-bottom: 20px; }\
        canvas { border: 2px solid white; border-radius: 15px; }\
      </style>\
    </head>\
    <body>\
      <h1>ESP32 Web Server with Wireguard</h1>\
      <h2>Hall Sensor: <span id=\"hallText\"></span></h2>\
      <canvas id=\"canvas\" width=\"400\" height=\"300\"></canvas>\
      <script>\
        var ctx = document.getElementById(\"canvas\").getContext(\"2d\");\
        var data = [];\
        \
        var maxData = 50;\
        var margin = 20;\
        var width = ctx.canvas.width - margin * 2;\
        var height = ctx.canvas.height - margin * 2;\
        var xStep = width / maxData;\
        var yMin = -100;\
        var yMax = 100;\
        var yScale = height / (yMax - yMin);\
        \
        function drawGrid() {\
          ctx.strokeStyle = \"#3A3A3A\";\
          ctx.lineWidth = 1;\
          ctx.beginPath();\
          \
          for (var x = margin; x <= ctx.canvas.width - margin; x += xStep) {\
            ctx.moveTo(x, margin);\
            ctx.lineTo(x, ctx.canvas.height - margin);\
          }\
          \
          for (var y = margin; y <= ctx.canvas.height - margin; y += yScale) {\
            ctx.moveTo(margin, y);\
            ctx.lineTo(ctx.canvas.width - margin, y);\
          }\
          \
          ctx.stroke();\
          ctx.strokeStyle = \"white\";\
          ctx.lineWidth = 2;\
          ctx.beginPath();\
          var y = margin + height - (0 - yMin) * yScale;ctx.moveTo(margin, y);\
          ctx.lineTo(ctx.canvas.width - margin, y);\
          ctx.stroke();\
        }\
        \
        function drawLine() {\
          ctx.strokeStyle = \"#2196F3\";\
          ctx.lineWidth = 2;\
          ctx.beginPath();\
          \
          for (var i = 0; i < data.length; i++) {\
            var x = margin + i * xStep;\
            var y = margin + height - (data[i] - yMin) * yScale;\
            if (i == 0) {\
              ctx.moveTo(x, y);\
            } else {\
              ctx.lineTo(x, y);\
            }\
          }\
          \
          ctx.stroke();\
        }\
        \
        function getData() {\
          var xhttp = new XMLHttpRequest();\
          xhttp.onreadystatechange = function() {\
            if (this.readyState == 4 && this.status == 200) {\
              var response = JSON.parse(this.responseText);\
              var hall = response.hall;\
              document.getElementById(\"hallText\").innerHTML = hall;\
              data.push(hall);\
              if (data.length > maxData) {\
                data.shift();\
              }\
              ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);\
              drawGrid();\
              drawLine();\
            }\
          };\
          xhttp.open(\"GET\", \"readHall\", true);\
          xhttp.send();\
        }\
        \
        setInterval(getData, 500);\
      </script>\
    </body>\
  </html>");
}