const byte csPin = 10;          // LoRa radio chip select
const int resetPin = -1;       // LoRa radio reset
const byte irqPin = 2;         // change for your board; must be a hardware interrupt pin

byte acknowledge = 11;        // symbol or word to check from esp8266 reply(callback)dd

byte localAddress = 0xBB;     // address of this device
byte destination = 0xFF;      // destination to send to

unsigned long lastSendTime = 0;        // last send time
int interval = 5000;                   // send msg every 5 seconds
byte maxSentMsg = 5;                  // continue to send until max limit reached,then show "failed to send"
byte numSentMsg = 0;                   // track how many msgs already sent

void ISR_sendCount() {
  if (state == countingState || state == failedsendState) {
    state = sendingState;             // pause counting
    numSentMsg = 0;                   // reset tracking of msgs sent
  }
  else if(state == sendSuccessState){
     state = countingState;
  }
  
}

void LoRaSetup() {
  LoRa.setSpreadingFactor(12);
  LoRa.setSyncWord(0xaa);                         // used to only receive lora with the same syncword, receive lora within the network only
  LoRa.setPins(csPin, resetPin, irqPin);
  if (!LoRa.begin(433E6)) {                       // initialize ratio at 915 MHz
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                                 // if failed, do nothing
  }
  Serial.println("LoRa init succeeded.");

}

void sendFishCount() {
  if (state == sendingState) {                            //send continuously until esp8266 replied
    if (millis() - lastSendTime > interval) {    //send every interval seconds
      String message = String(fishcount);        // send a message
      sendMessage(message);
      Serial.println("Sending " + message);
      lastSendTime = millis();                    // timestamp the message

      numSentMsg += 1;                              //track how many messages sent
    }
  }

  if (numSentMsg == maxSentMsg) {
    state = failedsendState;                           //used to stop sending
  }
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                            // start packet
  LoRa.write(destination);                       // add destination address
  LoRa.write(localAddress);                      // add sender address
  LoRa.write(outgoing.length());                 // add payload length
  LoRa.print(outgoing);                          // add payload
  LoRa.endPacket();                              // finish packet and send it                          // increment message ID
}

void listenForCallback() {
  if (state == sendingState) {
    onReceive(LoRa.parsePacket());  //only read received data when sending to another lora started
  }
}

void onReceive(int packetSize) {
  if (LoRa.parsePacket() == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();                  // recipient address
  byte sender = LoRa.read();                    // sender address
  byte incomingLength = LoRa.read();            // incoming msg length

  String incoming = "";

  while (LoRa.available()) {
    incoming += (char)LoRa.read();
  }

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("error: message length does not match length");
    return;                                    // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                                    // skip rest of function
  }

  if (incoming == String(acknowledge)) {
    state = sendSuccessState;                      //stop sending after received callback
    Serial.print("sent success");
    Serial.println("press button to continue counting");
    numSentMsg = 0;
  }

  // if message is for this device, or broadcast, print details:
  Serial.println("Received from: 0x" + String(sender, HEX));
  Serial.println("Sent to: 0x" + String(recipient, HEX));
  Serial.println("Message length: " + String(incomingLength));
  Serial.println("Message: " + incoming);
  Serial.println("RSSI: " + String(LoRa.packetRssi()));
  Serial.println("Snr: " + String(LoRa.packetSnr()));
  Serial.println();
}