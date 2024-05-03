#ifndef	_CHILD_NODE_H_
#define	_CHILD_NODE_H_

#include "polimos.h"
EnergyMonitor emon1;                   // Create an instance
RF24 radio(9, 10);                    // nRF24L01(+) radio attached using Getting Started board
RF24Network network(radio);          // Network uses that radio

MPU6050 mpu6050(Wire);

const uint16_t this_node = 01;       // Address of our node in Octal format
const uint16_t other_node = 00;      // Address of the other node in Octal format
double Irms = 0;
float angleY = 0;
float angleX = 0;
float power = 0;
struct payload_tx {                   // Structure of our payload; all data here are sent out.
    float gyroY = angleY;
    float gyroX = angleX;
    float current = power;
};


void child_setup(void) {
    Serial.begin(115200);

    ///MPU init
    Wire.begin();
    mpu6050.begin();
    mpu6050.calcGyroOffsets(true);

    ///RF24 init
    Serial.println(F("RF24Network TX"));

    // CT init
    emon1.current(A0, 90.9);             // Current: input pin, calibration. [child 1 = 60],[child 2 = 200 | -11.6]

    if (!radio.begin()) {
        Serial.println(F("Radio hardware not responding!"));
        while (1) {
            // hold in infinite loop
        }
    }
    radio.setChannel(90);
    network.begin(/*node address*/ this_node);

}

void child_update() {
    network.update(); // Check the network regularly


    for(int i =0; i<50; i++){
    mpu6050.update();
    angleY = mpu6050.getAngleY();
    angleX = mpu6050.getAngleX();

    }

    double rms = 0;
    int n = 10;
    for(int i =0; i<n; i++){
    rms += emon1.calcIrms(1480);  // Calculate Irms only
    }
    Irms = (rms/n)-5.9;
    if(Irms > 0.8){
    power = Irms*230;
    } else{
        power = 0;              
    }


  // Sending Message to other_node

    Serial.print(F("Sending... ["));
    payload_tx payload;      // send all data in payload
    RF24NetworkHeader header(/*to node*/ other_node);   // name of node to send to
    bool ok = network.write(header, &payload, sizeof(payload));   // payload foward command

  // Serial Monitor Output
    Serial.print("\tangleY : ");
    Serial.print(payload.gyroY);
    Serial.print("\tangleX : ");
    Serial.print(payload.gyroX);
    Serial.print("\tcurrent : ");
    Serial.print(payload.current);
    Serial.print("    ");
    Serial.println(ok ? F("\t]  ...ok.") : F("\t] -> failed."));

    delay(5000);

}

#endif //_CHILD_NODE_H_