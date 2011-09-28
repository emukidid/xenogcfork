
MakeVGC 1.0 - ZipZap 2004

-------------------------

MakeVGC is a Viper GC BIOS Encryptor. Compile your program to run at address 0x81300000, and call MakeVGC to convert it to encrypted .vgc file.

How does it work ? The "KEY" of the BIOS was dumped using the Viper GC special debug commands as explained in ViperDev package, then the original BIOS is "xored" with this "KEY". Don't use binloader but use MakeVGC instead, it's much faster! (boots programs in 0 second instead of 2 seconds)


Example Code
 
------------

Try to flash pong.vgc to your VIPER! You'll see Desktopman's PONG. Press "START" to run original BIOS ! (simply resets the console)

I have Included Desktop'S PONG in this package, I hope he won't mind!! It's to demonstrate how to make homebrew code with the Viper GC! Read GCQuickstart.txt to setup the kit (I searched files on google!). I have changed make.bat to include the call to makevgc and so it compiles at 0x81300000. Also it was not working at first, I had to correct the crt0.s so the stack = 0x81200000 (or use a crt0.s that has a hardcoded stack value (r1) instead of loading stack value from RAM!). I changed original main.c, because original version didn't initialize pad.

TODO: make an example with latest libogc version instead of old version! But I wasn't able to compile new version on my computer :-( (problem with "make", no target found...)


THANK YOU TO :
 - Destopman for the pong source
 - Libogc team for their lib
 - All YAGCD contributors
 - Viper Team for the cool chip
 - Whoever wrote "GCQuickstart.txt"
 - All coders on GC!
