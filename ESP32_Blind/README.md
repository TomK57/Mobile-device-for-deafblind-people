Accesspoint / Websocket example code with html webserver

copyright DHS, Desert Hard- & Software, Harald Wüst

Functionality:

The Sketch opens an acces point called "ap" with IP-adress 192.168.2.2.
A Web-Browser can access the startpage on this 192.168.2.2 or with ap.local

With the button "Socket communication" a socket connection to the device can be opened.

In the text field a single characters from the DeafBlind character set can be entered which is then send to the device and outputet to the DeafBlind tick-interface.

Also character entries made on the DeafBlind tick-interface are send to the web-socket console so that a communication with the DeafBlind can be established.

There are two special characters to extend the functionality:

- '#' character which switches the character set to another character set (for example numbers, special characters). With each entry of a '#' character 
  the character set changes to the next one and back to the standard character set when the last set is reached (round robin).
  The following two character sets are currently available:
  'a','e','n','i','d','t','ä','o','k','m','f','l','g','ö','r','u','y','b','p','z','w','q','j','s','#','x','v','ü','c','h','*'  (standard set) and
  '1','2','3','4','5','6','7','8','9','0','!','"',' ','%','&','/','(',')','<','>',',','.','?',';','#','-','+',':','@','=','*'  (extended set)


- '*' character switches to a command mode. In command mode the next entered character is interpreted as a special command:

  > '*' a second command character directly after the command character locks the comand mode permanently intil another '*' character is recieved. This 
        can be used to enter a sequence of comands faster
  > 'w' switch to local wifi network, the access point is closed and a connection to the local wifi network is established
  > 'c' switch to client mode, the access point is colsed and a connection to another ap-server access point is opened to build up a network of devices.
  > 's' switch to ap-server mode, the access point is opend to allow other devices to connect to the network of devices, only one server should be active.

All the single character entries can also be made using the DeafBlind tick-device or the serial interface.

If a longer string is entered on the websocket or the serial interface it is interpreted as a configuration command. The first character of the command defines the functionality:

- 'o'   Send a string of characters to the network. Example o hello -> sends the character sequence h e l l o to the network which is outputed to all 
        connected DeafBlind tick devices.
- 's'   Set the stabilization time for character tick detection. Example s 20 -> sets the stabTime to 20 ms
- 'c'   Set the pulse count for character output on tick-device. Example c 2  -> sets the pulseCount to 2
- 'd'   Set the character speed for the 'o' command. Example d 500 -> sets the outSpeed to 500 ms
- 'RRR' reeboot the device

To start a DeafBlind network the devices should be startet one after the other and switched to client mode. Only the last one should stay in server mode where all other devices will than connect to.
