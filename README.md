# MeshtasticSensors

# Why 
This is not a perfict or the most efficent solution by any streatch. Depending on your usecase a LoraWan solution might be better fit or customising the meshtastic firmware (or contributing), or just usign the Meshtastic "Remote Hardware Module" to control any spare GPIO on the board you are running meshtastic on. 

What this does is allow someone to use Meshtastic for what its good at solid comms link (user friendly, debugable, low power communication link), with arduino boards that have been sitting in a draw waiting for project. Combining these two elements you can create a custom hardware interface that can run its own logic that can send and recieve information to an MQTT server that you control. From here that data can be piped to anything that has a MQTT client NodeRed, HomeAssistant, Ignition Maker edition (what I use) etc.

This is ideal for controling hardware that requires many GPIO ports or would benifit form having local logic (scripts) that can exicute at the source node. This can allow for more a higher level of control, minimise unnessaccary radio utilization and can provide a watchdog for meshtastic (its still not perfict). 

## Background
I had been attempting to create a custom Zigbee module and device in Zigbee2MQTT to to control my frount gate that is well outside of my WiFi network. While I was making painfully slow progress I would still needed to extend my zigbee network to reach the gate. While having a break from that project I was playing with some Meshtastic units but after a while I started to wonder if these could be used to control my gate to control my gate. 
The meshtastic firmware already has some modules that can acheve some of what I wanted but each had limitations:
+ The Remote Hardware Module built into meshtastic is cool and a good idea however I needed more GPIO than what was avaliable, I wanted to run some logic (scripts) at the node but this is not an option with this module. Finally the read, write and watch commands are all initated from other nodes; the source node cannot just broardcast when it sees an event.
+ The Canned Message Module was also intresting but was not enough for what I needed. 
+ Modifying the Meshtastic firmware seemed like the next option however this would require I patch in my changes each time there is an update; my patch could become incompatible over time. I could contribut however as this is for one off custom devices its best being handeled by a seperate microcontroller. 

I did also consider:
+ 433Mhz serial modules, however these are not encripted. 
+ Encripted 433Mhz serial modules however these do not interface to MQTT and they typically dont confirm message recipt.
+ LoraWan seems quite meature looking at the documenation however it appears I would be in a similar situation to Zigbee where I would be trying to shoehorn in a custom one of a kind device into a well defined ecosystem.

# My Personal Goal for this
For the gate controller I wanted to be able to command it to open, close, stop, explicity request its status (open, closed, closing, open, opening, unknown) and self report its status when it changes. Further I wanted the gate to retain all its current functinality specifically all remotes needed to continue to work and if my hardware failed it was not to compromise the gates functions. 


# How it works
The home automation monitors and sends commands to your devices via your self hosted MQTT server, meshtastic forms a conduit between the MQTT server and the arduino serial ports, the arduino module monitors its serial looking for messages its named in (discarding the rest); when one is found it will act on that command. Status messages can also be set as detirmed by the end devices up to the MQTT server. 
There are four elements to make this work: configuring the meshtastic link, defining the message format, setting up the arduino hardware, writing the arduino software. 

## Meshtastic config
This consists of a chanel called serial, this channel is setup on at least two nodes. 

The first node which must have a network connection (WiFi or Ethernet) is setup with MQTT to your home automation MQTT server (Do not user a public MQTT server). The serial chanel is then setup to push and pull any message on this chanel to the defined MQTT topic.

The second node and any other aditional nodes will have a real serial (as in two physical wires Tx and Rx and will run at a baud rate etc) ports. These nodes will join the meshtastic serial chanel and will have the meshtastic Serial Module enabled in simple mode. This will push any messages recieved on the channel to the real serial port and vice versa. You effectivly now have a bi-directional link between the arduinos serial ports and a MQTT topic. 

Aditional setup notes: 
+ Follow the meshtastic documentation for the inital setup; the below instructions assume the regon is setup, device compatability is good, network has been configured, devices have names etc.
+ My working setup was all done on firmware 2.5.11 (there are bugs in the serial moduel in version prior to this!!)
+ The chanel you use must be called "serial", this is a requirement of the serial module for meshtastic.
+ On the first node (the WiFi node) you will need to:
  + In the MQTT config enable JSON output
  + In the Lora config enable "OK to MQTT"
+ On the second node (with serial port) you will need to:
  + In the device config set the Roal to "CLIENT MUTE" (Only change this if you know what your doing and your device is in a good location with a good antenna to enable re-broardcast of messages)
  + In the Serial Config enable "serial enabled"
  + In the Serial config set the correct RX (pico GPIO pin 9) and TX (pico GPIO pin 8) pin numbers
  + In the Serial config set a sensable baud rate like 19200; you can probbly go faster but this will not be the bottle neck so why do it
  + In the Serial config set the "serial mode" to SIMPLE
  + In the Lora config enable "OK to MQTT"

## message format
While I currnelly only have a single device I defined a simple format for the message that could enable multiple devices to talk on the link at once. Just prefix the message you would like to send with the target devices name, then write hte message. for example "Gate1:Open_cmd". The gate could then respond with "Gate1:Opening", "Gate1:Opened". You could also request updates with "Gate1:Ack_cmd" and the gate woudl respond with "Gate1:Opened". However this requies some logic in the source node to process and send these messages. 
This format could be improved, something like <name>:<type>:<message> where type specifys shorthands for status or command message would be clear than appeding _cmd to the message for commands.  

## Arduino Hardware
My gate controller is an "Automatic NES-24V3" it has multipel terminal connections to enable open, close, stop and peddestrial (only opens enough to let people walk in). While the controller does have feedback so it knows if its opened or closed it does not provides any terminals (in the documentaion) to share this with external hardware. However after looking at the actual PCB (second image) their is a white three wire plug on the right side of the board that connects to an "encoder" (this is what was written on it and this woring thrugh me a bit) thich turned out was just a 3 position switch. It took some probing with a multimeter and testing befor I figured out exacly how this switch interfaced to the gates micro controler. Turned out it was just a 5VDC signal on the first pin for gate open, second pin was gate closed with the third pin being ground. So I got the arduino to ease drop on these 5V signals by pluging the switch plug into a bread board and bread board passed these signals to the arduino and another plug that conects back to the gate controllers PCB.  

Initally I used relays to interface to the terminals but found out that a transistor would would work just as well. 
A 3.3V arduino board was being used at the start but I switched it to 5V so the posisition switch signals could be run direclly to the GPIO without any aditional level converting circuits; I was worried this might has some unfosceed impact on the gates micro controller seeing I did not have access to its schmatic. This meant that for the Tx pin on the 5V arduino to the 3.3V Rx pin of the pico required a pull down resistor (level converting), going the other way does not matter as its within the TTL voltage spec. 
Initally I was using a standard 7805 5V voltage regulator however the 24VDC turned out to be more like 29VDC and even with a heat sync the regulator was getting to hot and shutting down; this killed my frist pico (rest in pico). I then got a fancy DC/DC 5V buck converter (TSR12450) this worked a treat. 
In the inital first multi day tests I found that the meshtastic firmware could crash (Not sure what happened but it dropped off the mesh network) to combat this if the arduino does not recieve a message from MQTT (via the serial port) within 4 hours it will toggle the 3V3_EN (3.3V enable) pin on the pico running meshtastic effectilly power cyceling it; have not had this issue since. 

The third and fourth image show the proto type bread board in fritzing and the final board. There was a plan to actually order a PCB but the breadboard version has been working for nearly 4 months now. In anycase fritzing files are included. 

![image](https://github.com/user-attachments/assets/f207e753-0c47-4cc2-9a4a-5414607ea0ea)

<img src="https://github.com/user-attachments/assets/a0f389aa-78a5-4e52-931d-9678f0ed1b9e" width="700">

![IMG_20241106_154830_127](https://github.com/user-attachments/assets/63a201b0-3b7d-4774-8bf2-de759c8a33ea)

![IMG_20241107_122512_066(1)](https://github.com/user-attachments/assets/f72f3547-1ec2-4c53-96ef-894dc2d87198)

## Arduino Software
The arduino file contains code I was not plaining on shwoing anyone when I wrote it; please be nice. 

This is only an example, any micro controller with a serial port could be used. 

It does the following: 
+ check for and actions the commands open, close, stop, ped and ack
+ monitor the position pins and update the status of the gate as needed based on the combination of pins
+ Debounce the switch pins
+ Push the status to the second serial port for helpfull debugging
+ Meshhtastic watchdog




