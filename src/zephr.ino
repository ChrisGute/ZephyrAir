#include <Wire.h>
#include "SparkFunBME280.h"
#include "SparkFunCCS811.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ESP8266WebServer.h>

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Arduino.h>
#include "sensirion_uart.h"
#include "sps30.h"

// TODO
// Figure out better error handling for setup. If sensors are down we need to stop.

#define CCS811_ADDR 0x5B
#define SETUP_PIN 14
int switchValue;
// TODO find bme address 

//Global Defs
BME280 sensor_bme280;
CCS811 sensor_ccs811(CCS811_ADDR);
ESP8266WebServer server(24800);
static const unsigned long REFRESH_INTERVAL = 30000;
static unsigned long lastRefreshTime = 0;
String sensordata = "{}";

// Screen settings
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  ////Serial.begin(115200);
  delay(1000);
  ////Serial.println("Starting up Zephyr air quality system\n");
  Wire.begin();

  pinMode(SETUP_PIN,INPUT_PULLUP);

  // Setup SSD1306 display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    ////Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.clearDisplay();

  // Setup and init SPS30
  ////debuglines("Starting up SPS30");
  ////Serial.print("Starting up SPS30... ");
  delay(2000);
  sensirion_uart_open();
  int probe_result = sps30_probe();
  int start_measurement_result = sps30_start_measurement();
  char buf[50];
  ////sprintf(buf, "probe:%d, start_measurement:%d", probe_result, start_measurement_result);
  ////Serial.println("OK");
  ////debuglines("Starting up SPS30 OK");

  // Setup and init BME280
  //sensor_bme280.setI2CAddress(0x76);
  ////Serial.print("Starting up BME280... ");
  if (sensor_bme280.beginI2C() == false) {
    ////Serial.println("Sensor connect failed");
    while(1); //Hang if there was a problem.
  }
  sensor_bme280.setReferencePressure(101600);
  ////Serial.println("OK");

  // Setup and init CCS811
  ////Serial.print("Starting up CCS811... ");
  CCS811Core::status returnCode = sensor_ccs811.begin();
  if (returnCode != CCS811Core::SENSOR_SUCCESS)
  {
    ////Serial.println("Sensor connect failed");
    while (1); //Hang if there was a problem.
  }
  ////Serial.println("OK");

  //Setup wifi
  WiFiManager wifiManager;
  switchValue = digitalRead(SETUP_PIN);
  if(switchValue == HIGH) {
    ////Serial.print("Resetting wifi settings... ");
    wifiManager.resetSettings();
    ////Serial.println("Done");
  }
  wifiManager.autoConnect("ZephyrAir");
  ////Serial.println("Connected to wifi.");
  updateScreen();
  
  // Setup web handlers 
  ////Serial.print("Starting web server... ");
  server.onNotFound([](){
    server.send(404, "text/plain", "404 -- Not Found");
  });

  server.on("/", [](){
    server.send(200, "text/plain", "Welcome to the Zephyr air quality system");
  });

  server.on("/", [](){
    server.send(200, "text/plain", "Welcome to the Zephyr air quality system");
  });

  server.on("/stats", [](){
    server.send(200, "text/plain", sensordata);
  });
  
  server.begin();
  ////Serial.println("OK");
  ////debuglines("Server started");
  sensordata = "{" + get_ccs_data() + "," + get_bme_data() + "," + get_sps_data() + "}";
}

void loop() {
  if(millis() - lastRefreshTime >= REFRESH_INTERVAL){
    lastRefreshTime += REFRESH_INTERVAL;
    ////debuglines("Fetching data");
    sensordata = "{" + get_ccs_data() + "," + get_bme_data() + "," + get_sps_data() + "}";
    ////debuglines("Got data");
  }
  //Serial.println(sensordata);
  delay(10);
  server.handleClient();
}

void updateScreen() {
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.display();
}

void debuglines(String data) {
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(data);
  display.display();
}

String get_bme_data(){
  //Serial.println("\nBME280\n-----------------------------");
  String jsondata = "";

  jsondata = jsondata + "\"BME280\": {\"humidity_percent\":";
  jsondata = jsondata + sensor_bme280.readFloatHumidity();
  jsondata = jsondata + ", \"pressure_hpa\":";
  jsondata = jsondata + sensor_bme280.readFloatHumidity();
  jsondata = jsondata + ", \"temp_f\":";
  jsondata = jsondata + sensor_bme280.readTempF();
  jsondata = jsondata + ", \"dewpoint_f\":";
  jsondata = jsondata + sensor_bme280.dewPointF();
  jsondata = jsondata + ", \"altitude_f\":";
  jsondata = jsondata + sensor_bme280.readFloatAltitudeFeet();
  jsondata = jsondata + "}";
  
  //Serial.print("Humidity(%): ");
  //Serial.print(sensor_bme280.readFloatHumidity(), 0);

  //Serial.print(" Pressure(hPa): ");
  //Serial.print(sensor_bme280.readFloatPressure() / 100.0F, 0);

  //Serial.print(" Temp(f): ");
  //Serial.print(sensor_bme280.readTempC(), 2);
  //Serial.print(sensor_bme280.readTempF(), 2);

  //Serial.print(" Dewpoint(f): ");
  //Serial.print(sensor_bme280.dewPointC(), 2);
  //Serial.print(sensor_bme280.dewPointF(), 2);

  //Serial.print(" Alt(ft): ");
  //Serial.print(sensor_bme280.readFloatAltitudeMeters(), 1);
  //Serial.print(sensor_bme280.readFloatAltitudeFeet(), 1);
  
  //Serial.println();
  return jsondata;
}

String get_ccs_data(){
  //Serial.println("\nCCS881\n-----------------------------");
  String jsondata = "";
  if (sensor_ccs811.dataAvailable()){
    sensor_ccs811.readAlgorithmResults();
    jsondata = jsondata + "\"CCS881\": {\"co2e_ppm\":";
    jsondata = jsondata + sensor_ccs811.getCO2();
    jsondata = jsondata + ", \"tvoc_ppb\":";
    jsondata = jsondata + sensor_ccs811.getTVOC();
    jsondata = jsondata + "}";

    // Pass temp data back into sensor for next run 
    float BMEtempC = sensor_bme280.readTempC();
    float BMEhumid = sensor_bme280.readFloatHumidity();
    sensor_ccs811.setEnvironmentalData(BMEhumid, BMEtempC);
  } else {
    jsondata = jsondata + "\"CCS881\": {\"humidity_percent\":-1, \"pressure_hpa\":-1}";
  }

  return jsondata;
}

String get_sps_data() {
  String jsondata = "";

  int probe_result = sps30_probe();
  int start_measurement_result = sps30_start_measurement();
  //char buf[50];
  //sprintf(buf, "probe:%d, start_measurement:%d", probe_result, start_measurement_result);

  sps30_measurement measurement;

  s16 result = sps30_read_measurement(&measurement);
  while(result != 0){
    delay(50);
    result = sps30_read_measurement(&measurement);
  }

  bool err_state = SPS_IS_ERR_STATE(result);
  String mc_1p0 = String(measurement.mc_1p0);
  String mc_2p5 = String(measurement.mc_2p5);
  String mc_4p0 = String(measurement.mc_4p0);
  String mc_10p0 = String(measurement.mc_10p0);
  String nc_0p5 = String(measurement.nc_0p5);
  String nc_1p0 = String(measurement.nc_1p0);
  String nc_2p5 = String(measurement.nc_2p5);
  String nc_4p0 = String(measurement.nc_4p0);
  String nc_10p0 = String(measurement.nc_10p0);
  String typical_particle_size = String(measurement.typical_particle_size);

  jsondata = jsondata + "\"SPS30\": {\"err_state\":" + err_state + ", \"typical_particle_size\":" + typical_particle_size.c_str() +
                        ", \"mc_1p0\":" + mc_1p0.c_str() + ", \"mc_2p5\":" + mc_2p5.c_str() + ", \"mc_4p0\":" + mc_4p0.c_str() + ", \"mc_10p0\":" + mc_10p0.c_str() +
                        ", \"nc_0p5\":" + nc_0p5.c_str() + ", \"nc_1p0\":" + nc_1p0.c_str() + ", \"nc_2p5\":" + nc_2p5.c_str() + ", \"nc_4p0\":" + nc_4p0.c_str() + ", \"nc_10p0\":" + nc_10p0.c_str() +
                        "}";
  
  return jsondata;
}
