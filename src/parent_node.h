#ifndef	_PARENT_NODE_H_
#define	_PARENT_NODE_H_

#include "polimos.h"
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>


MPU6050 mpu6050(Wire);

RF24 radio(02, 15);             //CE, CS for esp8266
RF24Network network(radio);     // Network uses that radio
const uint16_t this_node = 00;  // Address of our node in Octal format (04, 031, etc)
const uint16_t child_one = 01; // Address of the other node in Octal format
const uint16_t child_two = 02;

EnergyMonitor emon1;                   // Create an instanc
double power;

float gx = 0;
float gy = 0;
double Irms = 0;

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"





// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
String nodeDataPaths;

// Database data paths nodes
String currentPath = "/current";
String yanglePath = "/yangle";
String xanglePath = "xangle";
String timePath = "/timestamp";
String loclong = "/longitude";
String loclat = "/latitude";

// Parent Node (to be updated in every loop)
String parentPath;
String node_info;
String sensorData;

FirebaseJson json;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Variable to save current epoch time
int timestamp;

// Structure of our payload; all data here are sent out.


struct payload_child {              
	float gyroY;
	float gyroX;
	float current;
};

// Timer variables (send new readings every three minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 18000;


// Initialize WiFi
void initWiFi() {
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	Serial.print(F("Connecting to WiFi .."));
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print('.');
		delay(1000);
	}
	Serial.println(WiFi.localIP());
	Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
	timeClient.update();
	unsigned long now = timeClient.getEpochTime();
	return now;
}

void parent_setup(){
	Serial.begin(115200);
	
	///MPU init
	Wire.begin();
	mpu6050.begin();
	mpu6050.calcGyroOffsets(true);
	
	//emon lib setup
	emon1.current(A0, 69);             // Current: input pin, calibration.
	
	// R24 Setup
	Serial.println(F("RF24Network RX"));

	if (!radio.begin()) {
		Serial.println(F("Radio hardware not responding!"));
		while (1) {
		// hold in infinite loop
		}
	}
	radio.setChannel(90);
	network.begin(/*node address*/ this_node);

	initWiFi();
	timeClient.begin();

	// Assign the api key (required)
	config.api_key = API_KEY;

	// Assign the user sign in credentials
	auth.user.email = USER_EMAIL;
	auth.user.password = USER_PASSWORD;

	// Assign the RTDB URL (required)
	config.database_url = DATABASE_URL;

	Firebase.reconnectWiFi(true);
	fbdo.setResponseSize(4096);

	// Assign the callback function for the long running token generation task */
	config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

	// Assign the maximum retry of token generation
	config.max_token_generation_retry = 5;

	// Initialize the library with the Firebase authen and config
	Firebase.begin(&config, &auth);

	// Getting the user UID might take a few seconds
	Serial.println("Getting User UID");
	while ((auth.token.uid) == "") {
		Serial.print('.');
		delay(1000);
	}
	// Print user UID
	uid = auth.token.uid.c_str();
	Serial.print("User UID: ");
	Serial.println(uid);
	delay(1000);
	
	// Update database path
	databasePath = "/UsersData/" + uid + "/readings";
}

void parent_update(){
    // Send new readings to database
    if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
        sendDataPrevMillis = millis();
        
        //Get current timestamp
        timestamp = getTime();
        Serial.print (F("time: "));
        Serial.println (timestamp);

        unsigned long starttime = 0;
        unsigned long endtime = 0;

        //child 1
        starttime = millis();
        endtime = starttime;
        network.update(); 
        while ((network.available()) && (endtime - starttime) <=5000){
			RF24NetworkHeader header;          // If so, grab it and print it
			payload_child payload;
			network.read(header, &payload, sizeof(payload));
			if (header.from_node == child_one){
				
				parentPath= databasePath + "/CN01" + "/" + String(timestamp);
				Serial.println(F("Node CN01: data ready to send"));
				
				json.set(currentPath.c_str(), String(payload.current));
				json.set(yanglePath.c_str(), String(payload.gyroY));
				json.set(xanglePath.c_str(), String(payload.gyroX));
				json.set(timePath, String(timestamp));

				// Serial Monitor Output
				Serial.print("[ angleY : ");
				Serial.print(payload.gyroY);
				Serial.print("\tangleX : ");
				Serial.print(payload.gyroX);
				Serial.print("\tcurrent : ");
				Serial.print(payload.current);
				Serial.println(" ]");
				Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
				json.clear();
			}

			//child 2
			if (header.from_node == child_two){
				
				parentPath= databasePath + "/CN02" + "/" + String(timestamp);
				Serial.println(F("Node CN02: data ready to send"));
				
				json.set(currentPath.c_str(), String(payload.current));
				json.set(yanglePath.c_str(), String(payload.gyroY));
				json.set(xanglePath.c_str(), String(payload.gyroX));
				json.set(timePath, String(timestamp));

				// Serial Monitor Output
				Serial.print("[ angleY : ");
				Serial.print(payload.gyroY);
				Serial.print("\tangleX : ");
				Serial.print(payload.gyroX);
				Serial.print("\tcurrent : ");
				Serial.print(payload.current);
				Serial.println(" ]");
				Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
				json.clear();
			}           
			endtime = millis();
        }
        delay(100);

        //Parent node
        starttime = 0;
        starttime = millis();
        endtime = starttime;

        while ((endtime - starttime) <=1000){          
            for(int i =0; i<50; i++){
            mpu6050.update();
            gx = mpu6050.getAngleX();
            gy = mpu6050.getAngleY();
            }
            Irms = emon1.calcIrms(1480);
            power = Irms*230;
            struct payload_mine {                  
                float pgyroY = gy;
                float pgyroX = gx;
                float pcurrent = power;
            };

            payload_mine payload;
            parentPath= databasePath + "/PN00" + "/" + String(timestamp);
            Serial.println(F("Node PN00: data ready to send"));
            json.set(currentPath.c_str(), String(payload.pcurrent));
            json.set(yanglePath.c_str(), String(payload.pgyroY));
            json.set(xanglePath.c_str(), String(payload.pgyroX));
            json.set(timePath, String(timestamp));
            Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());

            // Serial Monitor Output
            Serial.print("[ angleY : ");
            Serial.print(payload.pgyroY);
            Serial.print("\tangleX : ");
            Serial.print(payload.pgyroX);
            Serial.print("\tcurrent : ");
            Serial.print(payload.pcurrent);
            Serial.println(" ]");
            json.clear();

            endtime = millis();
        }
        
            endtime = millis();
    }
        delay(100);
}

#endif 	//_PARENT_NODE_H_