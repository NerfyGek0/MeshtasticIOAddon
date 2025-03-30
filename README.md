# MeshtasticSensors

# Disclaimer 
This is not a "it just works" solution; it is however flexible, cheep and could be adapted to fit any weird use case that consumer products don’t have a solution for. Further this is more of a concept with a working example, unless you want to control the exact same hardware in the same situation I did you will need to heavily adapt my solution. Please follow your local laws, regulations and some common sense; I take no responsibility for any damage to property, equipment or persons from someone following the information contained in this repository. 

# What is this?
Instructions to create a MQTT controlled IO (input/output) board that can run its own logic at LoRa distances.  
This solution lets Meshtastic do what its good at a solid comms link (user friendly, debugable, low power communication link) and combines it with the flexibility of an Arduino board (easy to use, lots of IO and all of them have at least one serial port). Combining these two elements you can create a custom hardware interface that can run its own local logic that can send and receive information to an MQTT server that you control. From there it can be piped to anything that has a MQTT client; NodeRed, HomeAssistant, Ignition Maker edition etc.

This is ideal for controlling hardware that requires many GPIO ports or would benefit form having local logic that can execute at the source node. This can allow for more a higher level of control, allow source node to make pre-defined decisions when radio is unavailable, minimise unnecessary radio utilisation and can provide a watchdog for the Meshtastic radio. 

Depending on your use case a better path might be to use:
+ A LoraWan product
+ Customising the Meshtastic firmware (or contributing)
+ Just using the Meshtastic "Remote Hardware Module" to control any spare GPIO
+ Or using a Shelly device with their "LORA add-on" clip on module (this looks cool). 

# How it works
The home automation monitors and sends commands to your devices via your self hosted MQTT server, Meshtastic forms a conduit between the MQTT server and the Arduino serial ports, the Arduino module monitors its serial port looking for messages prefixed with its name (discarding the rest); when a message that is prefixed with its name is detected by the Arduino it will act on the content of the message. Status messages can also be set by the Arduino as required back to the MQTT server self initiate or on request, these are also prefixed with the devices name for clarity.  
There are five elements to make this work: configuring the Meshtastic link, defining the message format, setting up the Arduino hardware, writing the Arduino software and setting up a MQTT client to handle the MQTT traffic (out of scope for this document). 

# My Personal Goal for this
I wanted to control my front gate that is well outside of my WiFi and zigbee network. Specifically to be able to command it to open, close, stop, explicitly request its status (open, closed, closing, open, opening, unknown) and self report its status when it changes. Further I wanted the gate to retain all its current functionality specifically all of the gates native remotes (433MHz radio controls) needed to continue to work even if my hardware failed.
I have tried to design this in such a way that multiple remote devices could be used however I am only using this for a single device (the gate) at the moment. 

## Meshtastic config
This consists of a Chanel called serial, this channel is setup on at least two nodes. 

The first node which must have a network connection (WiFi or Ethernet) is setup with MQTT to your home automation MQTT server (Do not user a public MQTT server). The serial Chanel is then setup to push and pull any message on this Chanel to the defined MQTT topic.

The second node and any other additional nodes will have a real serial ports (as in two physical wires Tx and Rx and will run at a baud rate etc). These nodes will join the Meshtastic serial Chanel and will have the Meshtastic Serial Module enabled in simple mode. This will push any messages received on the channel to the real serial port and vice versa. You effectively now have a bi-directional link between the Arduino’s serial ports and a MQTT topic. 

Additional setup notes: 
+ Follow the Meshtastic documentation for the initial setup; the below instructions assume the region is setup, device comparability is good, network (WiFi or LAN) has been configured, devices have names etc.
+ My working setup was all done on firmware 2.5.11 (there are bugs in the serial module in version prior to this!!)
+ The Chanel you use must be called "serial", this is a requirement of the serial module for Meshtastic.
+ On the first node (the WiFi node) you will need to:
  + In the MQTT config enable JSON output
  + In the Lora config enable "OK to MQTT"
+ On the second node (with serial port) you will need to:
  + In the device config set the role to "CLIENT MUTE" (if you know what your doing and your device is in a good location with a good antenna, then select a different mode other than CLIENT MUTE)
  + In the Serial Config enable "serial enabled"
  + In the Serial config set the correct RX (Pico GPIO pin 9) and TX (Pico GPIO pin 8) pin numbers
  + In the Serial config set a sensible baud rate like 19200; you can probably go faster but this will not be the bottle neck so why do it
  + In the Serial config set the "serial mode" to SIMPLE
  + In the Lora config enable "OK to MQTT"

## message format
While I currently only have a single device I defined a simple format for the message that could enable multiple devices to talk on the link at once. Just prefix the message you would like to send with the target devices name, then write the message. for example "Gate1:Open_cmd". The gate could then respond with "Gate1:Opening", "Gate1:Opened". You could also request updates with "Gate1:Ack_cmd" and the gate would respond with "Gate1:Opened". However this requires some logic in the source node to process and send these messages. 
This format could be improved, something like <name>:<type>:<message> where type specifys shorthand for status or command message would be clear than appending _cmd to the message for commands.  

## Arduino Hardware
My gate controller is an "Automatic NES-24V3" it has multiple terminal connections to enable open, close, stop and pedestrian (only opens enough to let people walk in). The controller does have feedback so it knows if its opened or closed however it does not provides any terminals (in the documentation) to share this with external hardware. However after looking at the actual PCB (second image) their is a white three wire plug on the right side of the board that connects to an "encoder switch" (printed on the side of it) which is just a 3 position switch. It took some probing with a multi meter and testing before I figured out exactly how this switch interfaced to the gates micro controller. Turned out it was just a 5VDC signal on the first pin for gate open, second pin was gate closed with the third pin being ground. So I got the Arduino to ease drop on these 5V signals by plugging the switch plug into a bread board so the Arduino board can ease drop on the signals and another plug that connects back to the gate controllers PCB. 

Initially I used relays to interface to the terminals but found out that a transistor would would work just as well and is more compact than having a relay interface board hanging off the side of by breadboard. 
A 3.3V Arduino board was being used at the start for compatibility with Meshtastic and the Rpi Pico board but I switched it to 5V so the position switch signals could be run directly to the GPIO without any additional level converting circuits; I was worried this might has some unforeseen impact on the gates micro controller as don't have access to its schematic. This meant that for the Tx pin on the 5V Arduino to the 3.3V Rx pin of the Pico required a pull down resistor (level converting), going the other way (3.3 to 5V) does not matter as its within the TTL voltage spec. 
Initially I was using a standard 7805 5V voltage regulator however the 24VDC Terminal on the Gate PCB turned out to run at 29-30VDC and even with a heat sync the regulator was getting to hot and shutting down; this killed my first Pico (rest in piece Pico). I then got a fancy DC/DC 5V buck converter (TSR12450) this worked a treat. 
In the initial first multi day tests I found that the Meshtastic firmware could crash (Not sure what happened but it dropped off the mesh network) to combat this if the Arduino does not receive a message from MQTT (via the serial port) within 4 hours it will toggle the 3V3_EN (3.3V enable) pin on the Pico running Meshtastic effectively power cycling it; have not had this issue since. This effectively gives Meshtastic a hardware based watchdog. 

The third and fourth image show the prototype bread board in fritzing and the final board. There was a plan to actually order a PCB but the breadboard version has been working without issue since Oct 2024. In any case fritzing files are included. 

![image](https://github.com/user-attachments/assets/f207e753-0c47-4cc2-9a4a-5414607ea0ea)

<img src="https://github.com/user-attachments/assets/a0f389aa-78a5-4e52-931d-9678f0ed1b9e" width="700">

![IMG_20241106_154830_127](https://github.com/user-attachments/assets/63a201b0-3b7d-4774-8bf2-de759c8a33ea)

![IMG_20241107_122512_066(1)](https://github.com/user-attachments/assets/f72f3547-1ec2-4c53-96ef-894dc2d87198)

## Arduino Software
The Arduino file contain the code I used for my gate. I was not planing on showing anyone when I wrote it; please be nice. Ill clean up the comments and make is cleaner if I get time. I will also implement the clearer message format (<name>:<type>:<message>) also if time permits. That said its taken me way to long to get this repo going, so updates will not happen any time soon. 

Keep in mind is only an example, any micro controller with a serial port could be used.

In any case unless you have a "Automatic NES-24V3" front gate you will likely need to heavily modify the code to fit your use case. Even if you have a "Automatic NES-24V3" it might have a light, gate buzzer or safety beams that you also want to control or get status updates on.

The code does the following: 
+ check for and actions any commands that are prefix with its name: open, close, stop, ped and ack
+ monitor the position pins and update the status of the gate as needed based on the combination of pins
+ De-bounce the switch pins
+ Push the status to the second serial port for helpful debugging in real time
+ Meshtastic watchdog
