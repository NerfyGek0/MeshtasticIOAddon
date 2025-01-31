# MeshtasticSensors
Documentation of how to use Meshtastic serial and MQTT functionality to automation custom hardware

# Why 
This is not a perfict or the most efficent solution by any streatch. Depending on your usecase a LoraWan solution might be better fit or customising the meshtastic firmware (or contributing), or just usign the Meshtastic "Remote Hardware Module" to control any spare GPIO on the board you are running meshtastic on. What it does is allow someone to use Meshtastic for what its good at (a solid, user friendly, debugable, low power communication link), with arduio boards that have been sitting in a draw waiting for project. Combining these two elements you can create a custom hardware interface that can send and recieve information to an MQTT server that you control (aka self host) which can then be piped anywhere for example NodeRed, HomeAssistant, Ignition Maker edition (what I use) or any program that can access MQTT. 
This is ideal for controling hardware that requires many GPIO ports or sequenced control that would be hard if not impossible to acheve with the Remote Hardware Module. 

# Background
I had been attempting to create a custom Zigbee module and device in Zigbee2MQTT to to control my frount gate that is well outside of my WiFi network. While I did make some progress it was painfully slow and I still needed to extend my zigbee network to reach the gate. While having a break from that project I was playing with some Meshtastic units but after discovering there was no one to chat to in my area I started expermenting to see if there was anyway to get this to control my gate. 
+ The Remote Hardware Module is cool and a good idea however I needed more GPIO than what was avaliable, I wanted to add some logic (scripts) based what the IO was doing to meshtastic but there was not option and finally the read, write and watch commands are all initated from other nodes.
+ Modifying the Meshtastic firmware seemed like the next option however this would mean I would have to patch in my changes each time I ran an update or I could cause bugs. I could contribut however this project is already doing a lot and its a chat app first; this seems a bit out of scope.
I did also consider:
+ 433Mhz serial modules, however these are not encripted. Its also unclear how they would handel multiple devices or if they would re-try sending a message if it failed to reach its peer the first time. 
+ Encripted 433Mhz units however Its unclear if I bought some from ebay if I could run multiple of them togearther or if I could buy the same type again in the future. Meshtastic has a range of devices and as a project does not seem to be going anywhere.
+ LoraWan and this range seems to be a more commercial solution, looking at the documenation it seemed like similar to Zigbee where I would be trying to shoehorn in a custom one of a kind device. This could be a compleatly wrong take; let me know if I am way off base here.
At this point I already had the Meshtastic radios and a bunch of old arduinos

# How
