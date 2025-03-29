unsigned long lastSentTime0 = 0;
unsigned long lastSentTime1 = 0;
unsigned long radioWatchDog = 0;
String message = "startup";
String lastMessage = message;
String prefix = "gate1:";

int lastopened = HIGH; // Initial state (pull-up resistor means unpressed is HIGH)
int lastClosed = HIGH; // Initial state (pull-up resistor means unpressed is HIGH)
int lastMsgNum = 99;
unsigned long lastChangeTime = 0; // Timestamp for the last change
unsigned long movingTime = 0; // Timestamp for the last change
bool movingArmed = false; // Timestamp for the last change
bool isStable = false; // Track if the states are stable
bool commanded = false; // Track if the states are stable

void setup() {
  // Start serial communication at 9600 baud
  Serial.begin(9600);
  Serial1.begin(9600);
  serialPrintln("initialising");

  // Set A0, A1, A2, and A3 as outputs
  pinMode(A0, OUTPUT); // Command opened
  pinMode(A1, OUTPUT); // Command Close
  pinMode(A2, OUTPUT); // Command stop
  pinMode(A3, OUTPUT); // Command Ped
  pinMode(2, OUTPUT); // Enable Radio

  // Turn off all outputs not that LOW activates the relay
  digitalWrite(A0, LOW);
  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);
  digitalWrite(2, HIGH); 

  // Set pins 10 and 16 as inputs with internal pull-up resistors
  pinMode(16, INPUT); //INPUT_PULLUP); // closed=5v - pasivly reading the Gate micro controller using its pullups. 
  pinMode(10, INPUT); //INPUT_PULLUP); // opened=5V - pasivly reading the Gate micro controller using its pullups. 
  //pinMode(16, INPUT_PULLUP); // closed=5v - pasivly reading the Gate micro controller using its pullups. 
  //pinMode(10, INPUT_PULLUP); // opened=5V - pasivly reading the Gate micro controller using its pullups. 
}

// A3 the movment relay should be wired NC (closed). As this program is relaying the limit switches to the gate controller
// if the arduino fails or loses power the movement relay needs to close when its not powered to prevent the gate from
// limitSwitch at all. 
// The A3 movement relay under normal operation should be powered at all times (openeding the contacts) and pulsed for 0.5 seconds
// whenever the limit switches are reached. The gate controller 

void loop() {
  // Check if data is available to read
  if (Serial1.available() > 0) {
    String command = Serial1.readStringUntil('\n'); // Read the command until a newline

    // reset the watchdog count
    radioWatchDog = millis();

    // Check for commands and pulse the corresponding pin
    if (containsSubstring(command, "gate1:cmd open")) {
      pulsePin(A0, true);
      Serial.println("gate1:cmd open");
      lastMessage = "opening";
      serialPrintln(lastMessage);
      commanded = true;
    } else if (containsSubstring(command, "gate1:cmd close")) {
      pulsePin(A1, true);
      Serial.println("gate1:cmd close");
      lastMessage = "closing";
      serialPrintln(lastMessage);
      commanded = true;
    } else if (containsSubstring(command, "gate1:cmd stop")) {
      pulsePin(A2, true);
      Serial.println("gate1:cmd stop");
      lastMessage = "stopped";
      serialPrintln(lastMessage);
      commanded = true;
    } else if (containsSubstring(command, "gate1:cmd ped")) {
      pulsePin(A3, true);
      Serial.println("gate1:cmd ped");
      lastMessage = "pedestrian";
      serialPrintln(lastMessage);
      commanded = true;
    } else if (containsSubstring(command, "gate1:ack")) {
      // re-send last message
      Serial.println("gate1:ack");
      serialPrintln(lastMessage);
      commanded = true;
    } else {
      if (containsSubstring(command, "gate1:")){
        Serial.print("cmd unknown:");
        Serial.println(command);
      }
    }
  }

  // Monitor the pins
  int opened = digitalRead(16); // Read the open state
  int closed = digitalRead(10); // Read the closed state

  // Check if the states have changed
  if (opened != lastopened || closed != lastClosed) {
    lastChangeTime = millis(); // Reset the change timestamp
    isStable = false; // Set unstable state
  } else if (millis() - lastChangeTime >= 50) { // Check if stable for 1 second
    isStable = true; // States are stable
  }

  // Act based on last stable state if stable
  if (isStable) {
    //if (limitSwitch == LOW && confirmedopened == LOW) {
    if (opened == LOW && closed == HIGH) {
      if (lastMsgNum != 1){
        // updaet the mesh network
        lastMessage = "opened";
        serialPrintln(lastMessage);
        lastMsgNum = 1;
        commanded = false;
      } 
    //} else if (limitSwitch == LOW  && confirmedopened == HIGH) {
    } else if (opened == HIGH  && closed == LOW) {
      if (lastMsgNum != 2){
        // updaet the mesh network
        lastMessage = "closed";
        serialPrintln(lastMessage);
        lastMsgNum = 2;
        commanded = false;
      } 
    } else {
      if (lastMsgNum != 3){

        // only send update message to mesh if movment was not commanded by mesh
        // this way if remote is used we see that its moving. When commands are issued from mesh direction is known
        if (commanded == false){
          if (lastMessage == "opened"){
            lastMessage = "closing";
          } else if (lastMessage == "closed"){
            lastMessage = "opening";
          } else {
            lastMessage = "moving";
          }
          serialPrintln(lastMessage);
        }
        lastMsgNum = 3;
      }
    }
  }

  // Update the last states
  lastopened = opened;
  lastClosed = closed;

  // Repeat the last message to Debug every 10 seconds
  if (millis() - lastSentTime0 >= 120000) { // 120000 ms = 2 minutes
    if (lastMessage != "") {
      message = prefix + lastMessage;
      Serial.println(message);
    }
    lastSentTime0 = millis();
  }
  
  //*** Deprecated ack form mesh will be preiodically sent and can be sent on command to get current status. radio should only send status on demand now
  // 
  // Repeat the last message to UART every 5 minutes
  //if (millis() - lastSentTime1 >= 300000) { // 300000 ms = 5 minutes
  //  if (lastMessage != "") {
  //    message = prefix + lastMessage;
  //    Serial1.println(message);
  //  }
  //  lastSentTime1 = millis();
  //}

  // Restart the Radio if no messages or acks recieved from the mesh in over 4 hours
  // Meshtastic does sometimes die in the ass, so this is needed to make it relaiable
  if (millis() - radioWatchDog >= 14400000) { // 3600000 ms = 1 hour, 7200000 ms = 2 hours, 14400000 = 4 hours
    pulsePin(2, false);
    Serial.println("RESETING MESHTASTIC RADIO AS NO COMMS FROM MESH RECIEVED IN ALLOCATED TIME!!");
    // reset the count
    radioWatchDog = millis();
  }
}

// Function to pulse the specified pin for 1 second
void pulsePin(int pin, bool onPulse) {
  // Note that with the relay board a LOW on the output will activate the relay. hence why this onPulse seems backwards
  digitalWrite(pin, onPulse); // Turn the pin on
  delay(1000);             // Wait for 0.5 second
  digitalWrite(pin, !onPulse);  // Turn the pin off
}

//Function to print out to serial
void serialPrintln(String message){
  message = prefix + message;
  Serial.println(message);
  Serial1.println(message);
}

bool containsSubstring(String str, String substring) {
  return str.indexOf(substring) != -1; // Returns true if substring is found
}
