#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

YunServer server;


void setup() {
  Serial.begin(9600);
  pinMode(8, OUTPUT);
  Bridge.begin();

  server.listenOnLocalhost();
  server.begin();
}

void loop() {
  // get clients coming from server
  YunClient client = server.accept();

  // Is there a new client ?
  if(client) {
    String command = client.readStringUntil('/');
    // is it relay command?
    if (command == "relay"){
      int stat = client.parseInt();
      if (stat == 1){
        digitalWrite(8, HIGH);
      }
      else {
        digitalWrite(8, LOW);
      }
    }
    // close connection and free resources.
    client.stop();
  }
  delay(50); // Poll every 50ms
}
