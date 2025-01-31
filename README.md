# MeshtasticSensors
Documentation of how to use Meshtastic serial and MQTT functionality to automation custom hardware

# Why 
This is not a perfict or the most efficent solution by any streatch. Depending on your usecase a LoraWan solution might be better fit or customising the meshtastic firmware (or contributing), or just usign the Meshtastic "Remote Hardware Module" to control any spare GPIO on the board you are running meshtastic on. 

What this does is allow someone to use Meshtastic for what its good at solid comms link (user friendly, debugable, low power communication link), with arduio boards that have been sitting in a draw waiting for project. Combining these two elements you can create a custom hardware interface that can run its own logic that can send and recieve information to an MQTT server that you control. From here that data can be piped to anything that has a MQTT client NodeRed, HomeAssistant, Ignition Maker edition (what I use) etc.

This is ideal for controling hardware that requires many GPIO ports or would benifit form having local logic (scripts) that can exicute at the source node. This can allow for more suficiated control, minimise unnessaccary radio utilization and can provide a watchdog for meshtastic (its still not perfict). 

## Background
I had been attempting to create a custom Zigbee module and device in Zigbee2MQTT to to control my frount gate that is well outside of my WiFi network. While I did make some progress it was painfully slow and I still needed to extend my zigbee network to reach the gate. While having a break from that project I was playing with some Meshtastic units but after a while I started to wonder if these could be used to control my gate to control my gate. 
+ The Remote Hardware Module built into meshtastic is cool and a good idea however I needed more GPIO than what was avaliable, I wanted to run some logic (scripts) at the node but this is not an option with this module. Finally the read, write and watch commands are all initated from other nodes; the source node cannot just broardcast when it sees an event. I also looked at the Canned Message Module but I still wanted to have some logic at the source node. 
+ Modifying the Meshtastic firmware seemed like the next option however this would require I patch in my changes each time there is an update; my patch could become incompatible over time. I could contribut however this project is already doing a lot and its a chat app first; this seems a bit too custome and out of scope.

I did also consider:
+ 433Mhz serial modules, however these are not encripted. 
+ Encripted 433Mhz serial modules however Its unclear how multiple would work togeather, if diffrent brands would work togeather and if they have any handshak to confirm recipt of messages. Meshtastic has a range of devices and as a project does not seem to be going anywhere. 
+ LoraWan seems to be a more commercial solution and looking at the documenation it seemed like similar to Zigbee where I would be trying to shoehorn in a custom one of a kind device. This could be a compleatly wrong take; let me know if I am way off base here.


# How it works
There are four elements to make this work: configuring the meshtastic link, defining the message format, setting up the arduino hardware, writing the arduino software. 

## Meshtastic config
This consists of a chanel called serial, this channel is setup on at least two nodes. 

The first node which must have a network connection (WiFi or Ethernet) is setup for MQTT to your home automation MQTT server (Do not user a public MQTT server). The serial chanel is then setup to push and pull any message on this chanel to the defined MQTT topic.

The second node and any other aditional nodes will have a real serial (as in Tx Rx baud etc) ports. These nodes will join the meshtastic serial chanel and will have the meshtastic Serial Module enabled in simple mode. This will push any messages recieved on the channel to the real serial port and vice versa. You effectivly now have a bi-directional serial link to a MQTT topic. 

## message format
While I currnelly only have a single device I defined a simple format for the message that could enable multiple devices to talk on the link at once. Just prefix the message you would like to send with the target devices name, then write hte message. for example "Gate1:Open_cmd". The gate could then respond with "Gate1:Opening", "Gate1:Opened". You could also request updates with "Gate1:Ack_cmd" and the gate woudl respond with "Gate1:Opened". However this requies some logic in the source node to process and send these messages. 

## Arduino Hardware

