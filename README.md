# xenogcfork by Vingt-2
This is another fork of xenogcfork originally published by emukidid.

Here is the major difference in this fork:
* Replaced multi-game disc shell with a robust dol loader looking for a ``xeno.dol`` file in either memory card.
* You will still need a readable, bootable disc in your drive (can be anything).
* A .dol can easily be placed onto a memory card using swiss-gc (turning on file management in swiss's settings)

# Installation  
There are two ways to install this onto your XenoGC: The simplest and basically free method is to use the software updater. Unfortunately, you still need to take your cube apart to get to the xenogc modchip, and will need to solder the RST and GND pads of it to a switch, so the reset line of the atmega8 can be grounded on demand so it can be flashed.
The second method (typically if the first one failed ..., which it shouldn't) is to wire the 6 pins of an ISP flasher (a 6 pins usbtiny programmer will work great), you can then use AVRDude on your computer to flash your atmega8 in mere seconds :).


*DISCLAIMER*: Flashing a software *always* comes with the risk of failure, and potential breakage of your device. In 99% of the time, a bad flash can be fixed by method #2, but it will require a tiny bit more soldering, and buying a proper programmer. 

# Flashing using the software flasher
This consists of using the XenoFlash.dol utility provided at the root of this repository. Note that this tool is a little finicky, however, I have been able to properly flash my chip with these exact steps about a dozen times without fail, so I'd say it's pretty safe (I repeatedly flashed the chip using my programmer, checked the version, then used these steps to flash it with the dol, and verified that it was indeed updated) so I'm confident this will work (on an actual XenoGC, that is, with an Atmega8 chip).
First, you will unfortunately still need to take your Gamecube apart, and to get to the optical drive. You will then need to solder a wire from the RST pad (any of the xeno letter !) and the ground pad to a Single Pole, Single Throw (aka... a switch) (see picture [1]). Once that's done..
  * Place the XenoFlash.dol on your sd card / memcard, and boot load it with the loader of your choice.
  * Wait til the dol has fully booted and is displaying instructions.
  * Turn the switch ON (that is to drive RST to GND), the LED should turn OFF. If it doesn't, verify your wires.
  * Press Y, this will attempt to erase the flash.
  * Turn the switch back OFF (that is normal operation), the LED should be lit up again (red)
  * Turn the switch back ON (that is to drive RST to GND), the LED should turn off again.
  * Press Y, this will actually properly erase the flash.
  * Turn the switch OFF (that is, normal operation), the LED should NOT light up at this point.
  * Turn the switch back ON (that, is to drive RST to GND), the led should still NOT light up, obviously
  * Press A, this will flash the firmware onto the chip. This is a longer operation, wait until it says it's done.
  * Turn the switch back OFF, (normal operation) and the LED should shine again (red, not orange on an original XenoGC). If it does you can turn off the gamecube. If it doesn't, turn the switch back on again (that, is to drive RST to GND) and retry to flash until it's successful (as in the led turns on).
 
At this point, turning on your gamecube and pressing start should greet you with either your xeno.dol loading, or an error message if none was found in your memcards.

[Picture of the wiring](software_installer_switch.jpg)[1]

# Flashing using a USB programmer
You can also flash the chip directly using the ISP protocol to talk to the atmega8. The make file is already setup to use a usb programmer, I suggest you get yourself [one of these](https://www.amazon.com/USBtinyISP-Programmer-Bootloader-Download-Interface/dp/B01FDD4EP0/ref=pd_sbs_147_1/144-1489403-8576528?_encoding=UTF8&pd_rd_i=B01FDD4EP0&pd_rd_r=1a83009a-2ba1-4ee5-8a71-049222208b30&pd_rd_w=Rpcqt&pd_rd_wg=AUjDZ&pf_rd_p=b65ee94e-1282-43fc-a8b1-8bf931f6dfab&pf_rd_r=BW638ZGZ8ZXYEM6SSNVF&psc=1&refRID=BW638ZGZ8ZXYEM6SSNVF), or similar !
You will need to download [avrdude](https://www.nongnu.org/avrdude/) and place both files at the root of the repo.
Now is solder time, I like using DuPont head style cables so I can tightly connect each pin of the programmer. Once you have soldered all the wires to the pcb and connected each pin to the programmer (diagram coming).
Just enter make flash in a command line in the repo, (you mind need to install make on windows). This will flash xenoAT.hex that is located in XenoAT/. You can now put the optical drive back on its socket and give it a spin.

# To compile this yourself
You will need to install both [devkitppc](https://devkitpro.org/wiki/Getting_Started) and their gamecube libraries (for the .dol flasher) as well as [WinAVR](https://sourceforge.net/projects/winavr/files/WinAVR/20050214/) This version preferred.
