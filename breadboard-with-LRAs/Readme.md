## Breadboard with LRA's
LRAs are Loingitudinal Resonance Actuators, mostly used in mobiles. The model here is VLV101040A from vibronics.com.
This breadboard is used to test their usage for our purposes.
Electronics consist of WemosS2Mini controller, ULN2003 Darlington drver IC and a buck converter to provide the supply voltage for the LRAs independent from the controller.
The resonance frequecy of the LRA is 170Hz which means a period of about 5ms for one oscillation. We use only one half of this period to switch on voltage/current and let the device produce a mechanical pulse.
![breadboard with LRAs](IMG_0320.JPG)
## software
to be used with hand taster device as input. ESP32_BLIND is basis for this modification.
