DeafBlind Accesspoint / Websocket code with html webserver                                    copyright DHS, Desert Hard- & Software, Harald Wüst

Functionality:

The Sketch opens an acces point called "ap" with IP-adress 192.168.2.2.
A Web-Browser can access the startpage on this 192.168.2.2 or with ap.local

With the button "Socket communication" a socket connection to the device can be opened.

In the text field a single characters from the DeafBlind character set can be entered which is then send to the device and outputet to the DeafBlind tick-interface.

Also character entries made on the DeafBlind tick-interface are send to the web-socket console so that a communication with the DeafBlind can be established.

There are two special characters to extend the functionality:

- '#' This character switches to another character set (for example numbers, special characters). With each entry of a '#' character 
  the character set changes to the next one and back to the standard character set when the last set is reached (round robin).
  The following two character sets are currently available:
  'a','e','n','i','d','t','ä','o','k','m','f','l','g','ö','r','u','y','b','p','z','w','q','j','s','#','x','v','ü','c','h','*'  (standard set) and
  '1','2','3','4','5','6','7','8','9','0','!','"',' ','%','&','/','(',')','<','>',',','.','?',';','#','-','+',':','@','=','*'  (extended set)
  To destinguish the character sets on the DeafBlind tick-device for the first, normal set one pulse and for the second, third, ... sets two or more
  pulses are generated.
  After entering the next character from an extra character set the character set is automatically switched back to the standard character set.

- '*' This character switches to a special command mode. 
  In command mode the next entered character is interpreted as a special command:
  > '*' A second command character directly after the command character locks the comand mode permanently until another '*' character is recieved. This 
        can be used to enter a sequence of comands faster
  > 'w' Switch to local wifi network, the access point is closed and a connection to the local wifi network is established
  > 'c' Switch to client mode, the access point is colsed and a connection to another ap-server access point is opened to build up a network of devices.
  > 's' Switch to ap-server mode, the access point is opend to allow other devices to connect to the network of devices, only one server should be active.
  > 'n' Send new line/line feed character to begin a new input text line
  > 'm' Switch output mode, there are two output modes: 
        send all   -> sends every entered charcater including the special charcaters to all connected devices (standard mode after startup)
        send chars -> sends only the normal characters from the character set, special characters or commands are not send out 
        so that the output text is not disturbed

All the single character input can come from the DeafBlind tick-device, the socket communication (other connected devices or web browsers) or the serial interface. So DeafBlind people can "talk" to each other or with non DeafBlinds using thier tick-devices.

If a longer string is entered via websocket or serial interface it is interpreted as a configuration command.
The first character of the command defines the functionality:

- 'o'   Send a string of characters to the network. Example "o hello" -> sends the character sequence  'h' 'e' 'l' 'l' 'o'  to the network 
        which is outputed to all connected DeafBlind tick devices.
- 's'   Set the stabilization time for character tick detection. Example s 20 -> sets the stabTime to 20 ms
- 'd'   Set the pulse duration for character tick output. Example d 20 -> sets the pulseDuration to 20 ms
- 'x'   Set the character speed for the 'o' command. Example x 200 -> sets the outSpeed to 200 ms
- 'n'   Change device name (wifi accesspoint and MDNS web browser name). Maximal length is 31 characters. Example n MaxMusterman -> sets name to MaxMusterman
- 'g'   Get (load) current configuration from file system. File ap.cfg is the startup configuration. Example g ap.cfg
- 'p'   Put (save) current configuration to file system. File ap.cfg is the startup configuration. File upload via web browser is possible. Example p ap.cfg
- 'i'   Change IP-Address, config save and reebot neccessary! (3.digit format with leading zerros) Example: i 192.168.002.002
- 'rrr' reeboot the tick-device


To start a DeafBlind network the devices should be startet one after the other giving them a uniqe name ('n MaxMusterman') and switch to client mode ('*' 'c'). Only the last device should stay in server mode where all other devices will than start to connect to.
