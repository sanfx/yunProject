#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include<stdlib.h>
#include "DHT.h"

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
  pinMode(8, OUTPUT);
  Bridge.begin();
  dhtBedRom.begin();
  pinMode (DHpin, OUTPUT);
  server.listenOnLocalhost();
  server.begin();
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
  //  Bridge.put(DEWPNTkey, dewPoint);
  //  Bridge.put(HIkey, String(hIinCel, 4));

  YunClient client = server.accept();

  // Is there a new client ?
  if (client) {
    String command = client.readStringUntil('/');
    command.trim();
    // is it relay command?
    if (command == "relay") {
      int stat = client.parseInt();
      if (stat == 1) {
        digitalWrite(8, HIGH);
        client.println("Status: 200");
        client.println("Content-type: application/json");
        client.println();
        client.print("{\"relay\":\"Turned On\"}");
      }
      else {
        digitalWrite(8, LOW);
        client.println("Status: 200");
        client.println("Content-type: application/json");
        client.println();
        client.print("{\"relay\":\"Turned Off\"}");
      }

    } else if (command == "relay2") {
      int r_stat = client.parseInt();
      if (r_stat == 1) {
        digitalWrite(9, HIGH);
        client.println("Status: 200");
        client.println("Content-type: application/json");
        client.println();
        client.print("{\"relay\":\"Turned On\"}");
      }
      else {
        digitalWrite(9, LOW);
        client.println("Status: 200");
        client.println("Content-type: application/json");
        client.println();
        client.print("{\"relay\":\"Turned Off\"}");
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
  delay(50); // Poll every 50ms
}

