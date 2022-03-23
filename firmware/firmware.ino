#include "painlessMesh.h"
//#include <WiFi.h>

#define   MESH_PREFIX     "whateverYouLike" // mesh information
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

/* Test sketch for Adafruit PM2.5 sensor with UART or I2C */
#include "Adafruit_PM25AQI.h"
Adafruit_PM25AQI aqi = Adafruit_PM25AQI();
int sda_pin = 8; // GPIO16 as I2C SDA
int scl_pin = 9; // GPIO17 as I2C SCL

//#include <SensirionI2CScd4x.h> //SCD
//SensirionI2CScd4x scd4x;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  PM25_AQI_Data data;
  if (! aqi.read(&data)) {
    Serial.println("Could not read from AQI");
    delay(500);  // try again in a bit!
    return;
  }
  Serial.println("AQI reading success");
  
  DynamicJsonDocument doc(1024); // Create JSON object
  doc["DeviceID"] = mesh.getNodeId();
  //Add PM readings from PM sensor to JSON object
  //Concentration Units (standard)
  doc["PM 1.0"] = data.pm10_standard;
  doc["PM 2.5"] = data.pm25_standard;
  doc["PM 10"] = data.pm100_standard;
  //Concentration Units (environmental)
  doc["PM 1.0 (env)"] = data.pm10_env;
  doc["PM 2.0 (env)"] = data.pm25_env;
  doc["PM 10 (env)"] = data.pm100_env;
  //Particles
  doc["Particles > 0.3um / 0.1L air:"] = data.particles_03um;
  doc["Particles > 0.5um / 0.1L air:"] = data.particles_05um;
  doc["Particles > 1.0um / 0.1L air:"] = data.particles_10um;
  doc["Particles > 2.5um / 0.1L air:"] = data.particles_25um;
  doc["Particles > 5.0um / 0.1L air:"] = data.particles_50um;
  doc["Particles > 10um / 0.1L air:"] = data.particles_100um;

// TO DO: Add all reading from SCD sensor and format JSON

  String payload;
  Serial.printf("Sending data over mesh \n");
  serializeJson(doc, Serial);
  serializeJson(doc, payload);
  mesh.sendBroadcast(payload);
  Serial.printf("Done sending data over mesh \n");
  
  taskSendMessage.setInterval( random( TASK_SECOND * 10, TASK_SECOND * 5 )); // Time to send another reading
}


// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}


void createMesh(){
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup message
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT ); //initialize mesh network
  Serial.println("Mesh Initialized?");
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  // can use scheduler to check if mainnode is still on
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
  Serial.print("Done setting up mesh \n");
}


void setupPMSensor(){
  Wire.setPins(sda_pin, scl_pin);
  Wire.begin();
  Serial.println("Adafruit PMSA003I Air Quality Sensor");
    // There are 3 options for connectivity!
  if (! aqi.begin_I2C()) {      // connect to the sensor over I2C
    Serial.println("Could not find PM 2.5 sensor!");
    while (10) delay(1);
  }
  Serial.println("PM25 found!"); 
}

  
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  setupPMSensor();
  createMesh();
}


void loop() {
  // it will run the user scheduler as well
  mesh.update();
  
}
