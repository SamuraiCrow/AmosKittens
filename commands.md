

A small document describing how some commands works different under Amos Kittens.
If you like to write programs for Amos Kittens, get the Amos Pro manual, 
most of the commands are the same and should work the same, 
only the commands that are different are listed here.
not implmented commands not listed, dummy commands not listed, 
see other docs like issue for more info.


**Wait**

	Wait 1/50 of a second (20ms), just like normal AMOS wait, 
	but Wait command will also handle "On Menu Proc" events, 
	two reasons for this, 
	first its bad idea to process events in interpreter as it slows down all AMOS programs, 
	second it forces developers to not busy loop there AMOS programs.

**Wait VBL**
	
	Will wait for vertical blanking, 
	but same as Wait command will also process Menu selections, make sure you have Wait or Wait VBL in
	your programs if your using "On Menu Gosub","On Menu Proc" or "On Menu Goto" commands

**Screen Colour**

	Will always return 256 colors, even if you have opened screens with just 8 colors. this not bug, 
	it's becouse Amos Kittens can only use chunky mode, and it is 8bit.
	
**Load Iff**

	Will load .png/.jpg any file supported by datatype system on AmigaOS, 
	true color images are converted into 8bit grayscale images, or 8bit floyd dittered images.
	(in the future this command can load in images as truecolor), loading option will be in global variabel.
	IFF images with HAM6 format also supported.
	
**Save Iff**

	Will save any to any image format supported by datatype system, should default to IFF, 
	if no file extension is set.

**spack**

	This command will compress a screen or part of screen into a bank, 
	unlike Amos Pro that is not limited 6bit graphics,
	this command also support 8bit graphics on Amos Kittens.
	this command will count the number of colors on the screen before compressing the image,
	if its 64 colors or less, it will use AMOS packed format, if more then 64 colors 
	it will exstend the color platte table. 

**Unpack**

	This command support 1bit to 8bit compressed banks, including HAM6.
	
**Joy**

	Almost the same as Amos Pro, but supports more buttons.
	
	bit 0 - value 1 - joystick up
	bit 1 - value 2 - joystick down
	bit 2 - value 4 - joystick left
	bit 4 - value 8 - joystick right
	bit 5 - value 16 - joystick button 1
	bit 6 - value 32 - joystick button 2
	bit 7 - value 64 - joystick button 3
	bit 8 - value 128 - joystick button 4
	bit 9 - value 256 - joystick button 5
	...
	
	
