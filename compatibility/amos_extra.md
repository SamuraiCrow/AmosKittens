**Compatibility list**

This list is to keep track of bugs / incompatibility issues, 
the list is used to track down bugs in Amos Kittens, 
to find out way Amos Kittens reports errors, when Amos Pro does not.

**AMAL_Editor.AMOS**

Syntax error at line 474

**AMAL_Joystick.AMOS**

works almost, nothing displayed

**Dataflex.AMOS**

Not working: stuck on some block splach screen.

**Font_Convert.AMOS**

works, (don't like bullet fonts)

**Get_Time.AMOS**

Pass1 error: No THEN in a structured test

**GMC_to_AMOS.AMOS**

works maybe

**Icon_To_Bob.AMOS**

works maybe

**IFF_Brush_To_Sprite.AMOS**

works maybe

**nstr_Conv.AMOS**

works

**Keyboard_Definer.AMOS**

error at line number: 23
(A4 should have keyboard layout, when Amos Kittens starts, 400 bytes)

**Map_Editor.AMOS**

Greited by not so interestng black screen, 
there is no menu, maybe there should be.

**Map_View.AMOS**

works

**Menu_Editor.AMOS**

missing token: 0894 - command: Menu Movable

**RAMOS1_3_Install.AMOS**

"Y Graphic" command not suppoted error
got out of index error on jmp$() array, with -1 index.
on line 176

**Scrolling_text_demo.AMOS**

cooper effect looks like is at the wrong location, but it looks like it works.

**Sonix.AMOS**

work, but there is DSI error, due to mystery mem corruption.

**Sound_tracker2_1.AMOS**

works,

**Sprite_Plotter.AMOS**

works (just make sure you pick vaild sprite, no error handling in this example.)

**Squash_A_Bob.AMOS**

works.

**Squash_Procs.AMOS**

exit quick, maybe work.

**Xerxes'Revenge.AMOS**

DSI error in bob clear.
bug in FOR NEXT loop.
("For" is not in program stack, can't jump back to start of the loop.)
so need to look if it got flushed, or if it got executed where it should not.
