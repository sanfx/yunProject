
#include <EEPROM.h>
#include <YunClient.h>

namespace control
{

void setRelayStatus(int relay, int state) {
  digitalWrite (relay, state);
  if (relay == 8) {
    EEPROM.put(0, state);
  }
  if (relay == 9) {
    EEPROM.put(1, state);
  }
//  EEPROM.commit();
  delay(100);
}

int getRelayStatus(int relay) {
  // a relay is represented by its pin number
  if (relay == 8) {
    SerialUSB.println(EEPROM.read(0));
    return EEPROM.read(0);
  }
  if (relay == 9) {
    SerialUSB.println(EEPROM.read(1));
    return EEPROM.read(1);
  }
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


void toggleRelays(YunClient client , int relay) {
  // relay number is represented by pin it is connected to
  // pin 8 is relay in URL and EEPROM addr is 0
  // pin 9 is relay2 in URL and EEPROM addr is 1
  // pin 10 is relay3 in URL and EEPROM addr is 2
  // get the state to set. i.e. opposite of current state
  int state = !digitalRead(relay);
  if ((state == 1) or (state == 0)) { // TODO: I think this condition can be removed.
    if ((relay == 9) or (relay == 8) or (relay == 10) ) {
      setRelayStatus(relay, state);
      outputJsonHeader(client, relay, state);
    }
  }
}
}



