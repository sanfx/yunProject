#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <stdlib.h>
#include "DHT.h"

int calibrationTime = 10;
//the time when the sensor outputs a low impulse
long unsigned int lowIn;
long unsigned int relay2 = 9; // light
//before we assume all motion has stopped
long unsigned int pause = 5000;

boolean lockLow = true;
boolean takeLowTime;

int pirPin = 3;    //the digital pin connected to the PIR sensor's output
int ledPin = 13;

int DHpin = 2; // temp-hum sensor

DHT dhtBedRom(DHpin, DHT22);

YunServer server;

float indorTempinC;
float indorTempinF;
float indoorHumidity;
float dP; // dew point
float dPF; // dew point in fahrenheit
float tF; // temperature in fahrenheit
float hi; // heat index in fahrenheit of indoor
float hIinFah;
float hIinCel;  // heat index in celcius of indoor

byte dat [5];  // incoming data array.

int pin;

// reference: http://en.wikipedia.org/wiki/Dew_point

double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

void setup() {
  Serial.begin(9600);
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(pirPin, LOW);

  //  //give the sensor some time to calibrate
  Serial.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    Serial.print(".");
    delay(700);
  }
  Serial.println(" done");
  Serial.println("SENSOR ACTIVE");
  pinMode(8, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(10, OUTPUT);
  Bridge.begin();
  dhtBedRom.begin();
  pinMode (DHpin, OUTPUT);
  server.listenOnLocalhost();
  server.begin();
}

void toggleRelays(YunClient client , int pin, int state) {
  digitalWrite(pin, state);
  outputJsonHeader(client, pin, state);
}

void outputJsonHeader(YunClient client , int pin, int state) {
  client.println("Status: 200");
  client.println("Content-type: application/json");
  client.println();
  if (state == 1) {
    client.print("{\"relay\":\"Turned On\"}");
  }
  else {
    client.print("{\"relay\":\"Turned Off\"}");
  }
}

void loop() {

  indoorHumidity = dhtBedRom.readHumidity();
  // get clients coming from server
  // Read temperature as Celsius
  indorTempinC = dhtBedRom.readTemperature();
  hi = dhtBedRom.computeHeatIndex(indorTempinF, indoorHumidity);
  hIinCel = (hi + 40) / 1.8 - 40;

  //  use dtostrf(FLOAT,WIDTH,PRECSISION,BUFFER);
  dP = dewPointFast(indorTempinC, indoorHumidity);
  dPF = ((dP * 9) / 5) + 32;
  String dewPoint = String(dP, 2);

  if (digitalRead(pirPin) == HIGH) {
    digitalWrite(ledPin, HIGH);   //the led visualizes the sensors output pin state
    if (lockLow) {
      //makes sure we wait for a transition to LOW before any further output is made:
      lockLow = false;
      digitalWrite(relay2, HIGH);
      Serial.println("---");
      Serial.print("motion detected at ");
      Serial.print(millis() / 1000);
      Serial.println(" sec");
      //      delay(50);
    }
    takeLowTime = true;
  }

  if (digitalRead(pirPin) == LOW) {
    digitalWrite(ledPin, LOW);  //the led visualizes the sensors output pin state
    if (takeLowTime) {
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }
    // if the sensor is low for more than the given pause,
    // we assume that no more motion is going to happen
    Serial.print(millis() - lowIn);
    Serial.print("    ");
    Serial.print(lockLow);
    Serial.print("    ");
    Serial.println(pause);
    if (!lockLow && millis() - lowIn > pause) {
      //makes sure this block of code is only executed again after
      //a new motion sequence has been detected
      lockLow = true;
      digitalWrite(relay2, takeLowTime);
      Serial.print("motion ended at ");      //output
      Serial.print((millis() - pause) / 1000);
      Serial.println(" sec");
      delay(50);
    }
  }

  YunClient client = server.accept();

  // Is there a new client ?
  if (client) {
    String command = client.readStringUntil('/');
    command.trim();
    // is it relay command?
    if (command == "relay") {
      int stat = client.parseInt();
      if (stat == 1) {
        toggleRelays(client, 8, 1);
      } else {
        toggleRelays(client,8, 0);
      }
    } else if (command == "relay2") {
      int r_stat = client.parseInt();
      if (r_stat == 1) {
        toggleRelays(client, relay2, 1);
      }
      else {
        toggleRelays(client,relay2, 0);
      }
    } else if (command == "relay3") {
      int r_sta = client.parseInt();
      if (r_sta == 1) {
        toggleRelays(client, 10, 1);
      }
      else {
        toggleRelays(client,10, 0);
      }
    } else if (command == "temperature") {
      // set JSON header
      client.println("Status: 200");
      client.println("Content-type: application/json");
      client.println();
      client.print("{\"temperature\":\"");
      client.print(indorTempinC);
      client.print("\"}");
    } else if (command == "humidity") {
      // set JSON header
      client.println("Status: 200");
      client.println("Content-type: application/json");
      client.println();
      client.print("{\"humidity\":\"");
      client.print(indoorHumidity);
      client.print("\"}");
    }
    // close connection and free resources.
    client.stop();
  }
}


