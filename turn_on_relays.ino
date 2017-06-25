//#include <HashMap.h>
#include <Process.h>
#include "control.h"
#include <EEPROM.h>
#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <stdlib.h>
#include "DHT.h"

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
int addr = 0;

Process date;                 // process used to get the date
int hours, minutes, seconds;  // for the results
int lastSecond = -1;

////define the max size of the hashtable
//const byte HASH_SIZE = 5;
//
////storage
//HashType<char*, int> hashRawArray[HASH_SIZE];
////handles the storage [search,retrieve,insert]
//HashMap<char*, int> hashMap = HashMap<char*, int>( \hashRawArray , HASH_SIZE );

//void services() {
//
//  hashMap[0]("relay1", digitalRead(8));
//  hashMap[1]("relay2", digitalRead(relay2));
//  hashMap[2]("relay3", digitalRead(10));
//  hashMap[3]("temperature", indorTempinC);
//  hashMap[4]("humidity", indoorHumidity);
//  hashMap[5]("pirSense", digitalRead(pirPin)
//
//  SerialUSBprintln(hashMap);
//}
////////// manual override ////////////
unsigned int manualWatering = 0;
unsigned int manualLightOn = 0;
///////////////////////////////////////


int calibrationTime = 10;
//the time when the sensor outputs a low impulse
long unsigned int lowIn;
long unsigned int relay2 = 9; // light
//before we assume all motion has stopped
long unsigned int pause = 5000;

boolean lockLow = true;
boolean takeLowTime;

int pirPin = 3; //the digital pin connected to the PIR sensor's output
int ledPin = 13;
long unsigned int relay3 = 10; // solenoid
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

// reference: http://en.wikipedia.org/wiki/Dew_point

double dewPointFast(double celsius, double humidity)
{
  double a = 17.271;
  double b = 237.7;
  double temp = (a * celsius) / (b + celsius) + log(humidity * 0.01);
  double Td = (b * temp) / (a - temp);
  return Td;
}

void waterThePlant()
{
  if (lastSecond != seconds) { // if a second has passed

    if (hours <= 9) {
      SerialUSB.print("0");  // adjust for 0-9
    }
    SerialUSB.print(hours);
    SerialUSB.print(":");
    if (minutes <= 9) {
      SerialUSB.print("0");  // adjust for 0-9
    }
    SerialUSB.print(minutes);
    SerialUSB.print(":");
    if (seconds <= 9) {
      SerialUSB.print("0");  // adjust for 0-9
    }
    SerialUSB.println(seconds);


    if ((hours  == 16 || hours == 8 || hours == 12) and (minutes == 10) and (seconds <= 59) ) {
      digitalWrite(relay3, HIGH);
      SerialUSB.println(F("Watering the plant"));
    }
    else
    {
      if (manualWatering == 0) {
        digitalWrite(relay3, LOW);
      }
    }

    // restart the date process:
    if (!date.running()) {
      date.begin("date");
      date.addParameter("+%T");
      date.run();
    }
  }
  //if there's a result from the date process, parse it:
  while (date.available() > 0) {
    // get the result of the date process (should be hh:mm:ss):
    String timeString = date.readString();

    // find the colons:
    int firstColon = timeString.indexOf(":");
    int secondColon = timeString.lastIndexOf(":");

    // get the substrings for hour, minute second:
    String hourString = timeString.substring(0, firstColon);
    String minString = timeString.substring(firstColon + 1, secondColon);
    String secString = timeString.substring(secondColon + 1);

    // convert to ints,saving the previous second:
    hours = hourString.toInt();
    minutes = minString.toInt();
    lastSecond = seconds;          // save to do a time comparison
    seconds = secString.toInt();
  }
}

void setup() {
  Bridge.begin();        // initialize Bridge
  SerialUSB.begin(9600);
  pinMode(pirPin, INPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(pirPin, LOW);

  pinMode(8, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(10, OUTPUT);
  digitalWrite(8, control::getRelayStatus(8));
  digitalWrite(relay2, control::getRelayStatus(relay2));
  Bridge.begin();
  dhtBedRom.begin();
  pinMode (DHpin, OUTPUT);
  //  //give the sensor some time to calibrate
  SerialUSB.print("calibrating sensor ");
  for (int i = 0; i < calibrationTime; i++) {
    SerialUSB.print(".");
    delay(700);
  }
  SerialUSB.println(" done");
  SerialUSB.println("SENSOR ACTIVE");

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
      SerialUSB.println("---");
      SerialUSB.print("motion detected at ");
      SerialUSB.print(millis() / 1000);
      SerialUSB.println(" sec");
    }
    takeLowTime = true;
  }

  if ((digitalRead(pirPin) == LOW) and (manualLightOn == 0)) {
    digitalWrite(ledPin, LOW);  //the led visualizes the sensors output pin state
    if (takeLowTime) {
      lowIn = millis();          //save the time of the transition from high to LOW
      takeLowTime = false;       //make sure this is only done at the start of a LOW phase
    }
    // if the sensor is low for more than the given pause,
    // we assume that no more motion is going to happen

    if (!lockLow && millis() - lowIn > pause) {
      //makes sure this block of code is only executed again after
      //a new motion sequence has been detected
      lockLow = true;

      digitalWrite(relay2, takeLowTime);

      SerialUSB.print("motion ended at ");      //output
      SerialUSB.print((millis() - pause) / 1000);
      SerialUSB.println(" sec");
      delay(50);
    }
  }
  waterThePlant();
  YunClient client = server.accept();

  // Is there a new client ?
  if (client) {
    String command = client.readStringUntil('/');
    command.trim();
    // is it relay command?
    if (command == "relay") {
      control::toggleRelays(client, 8);
    } else if (command == "disablePIR") {
      manualLightOn = 1;
      client.println("Status: 200");
      client.println("Content-type: application/json");
      client.println();
      client.print("{\"PIR Sensor\":\"Disabled\"}");
    } else if (command == "enablePIR") {
      manualLightOn = 0;
      client.println("Status: 200");
      client.println("Content-type: application/json");
      client.println();
      client.print("{\"PIR Sensor\":\"Enabled\"}");
    }
    else if (command == "relay2") {
      control::toggleRelays(client, relay2);
    } else if (command == "relay3") {
      control::toggleRelays(client, relay3);
      manualWatering = int(digitalRead(relay3));
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
    } else if (command == "services") {
      client.println("Status: 200");
      client.println("Content-type: application/json");
      client.println();
      client.print("{\"relays\" : {\"pin\":");
      client.print(" {\"8\":\"");
      client.print(digitalRead(8));
      client.print("\", \"");
      client.print(relay2);
      client.print("\":\"");
      client.print(digitalRead(relay2));
      client.print("\", \"10\":\"");
      client.print(digitalRead(10));
      client.print("\"}}, \"EEPROM\":{\"0\":\"");
      client.print(EEPROM.read(0));
      client.print("\", \"1\":\"");
      client.print(EEPROM.read(1));
      client.print("\"},\"sensors\":{\"dht22\":{\"");
      client.print(DHpin);
      client.print("\": {\"temperature\":\"");
      client.print(indorTempinC);
      client.print("\", \"humidity\":\"");
      client.print(indoorHumidity);
      client.print("\"}}, \"pir\": {\"");
      client.print(pirPin);
      client.print("\" : \"");
      client.print(digitalRead(pirPin));
      client.println("\"}}}");
    }
    // close connection and free resources.
    client.stop();
  }
}


