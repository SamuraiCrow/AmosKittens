This result of trying to run different .amos files, on Amos Kittens, there is stuff missing, the list is to keep track of problems with etch .amos program, so we can fix problems we have.

**Amal_Editor.AMOS**
(c) 1990-1992 Europress Software Ltd

(works maybe, takes lot time to start in debug mode, it does not work in AmosPRO some programing error some where.)

This code uses command “Exit If” command, it does not read the menu with “Choice”, other than that code is better structured.
It’s possible “Exit If” command is not 100% implemented.

**Amal_Editor_Fixed.AMOS**
(c) 1990 Mandarin / Jawx

It does load, but does not render correct, after running the program, AmigaOS starts crashing…

**Disc_Manager.AMOS**

Missing token 0x258C - command "@apml@"
+Synyax error on line 1677
if I'm not mistaken this token means part of program is compiled, so can't support that.

**Font8x8_Editor.AMOS**

Works it starts "AMOSPro_Producitivity2:Font8x8_editor.amos"

**IFF_Compactor.AMOS**

Works it starts "AMOSPro_Producitivity2:IFF_Compactor.AMOS"

**Menu_Editor.AMOS**

Works it starts "AMOSPro_Producitivity1:Menu_editor.amos"

**Object_Editor.AMOS**

Stops at "Screen Base" command, that is not implemented.

**Resource_Bank_Maker.AMOS**

Interface script error, need to debug the script.

**Sample_Bank_Maker.AMOS**

 Crashes as soon as you try to load a sample, maybe something wrong on the stack.
 The Amos program does use some 680x0 routines, I have no idea if they are compatible, or not.
