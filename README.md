# choose
asks input and returns the chosen input as returnvalue (to be used in scripts)

(c)2024, LhK-Soft by Laurens Koehoorn

This program is a rewritten version of CHOICE.EXE which is part of Microsoft's Windows 10/11 and also was included in MS-DOS 6.0.
For full history of Choice, please see https://en.wikipedia.org/wiki/Choice_(command)

I allways wrote scripts for DOS/Windows in Batch/CMD, but nowadays I like linux more. Bash is much more advanced, so after I installed MSYS2 I went on (re-)writing scripts
for BASH (inside windows). In here I still can use all tools and apps I've installed in Windows, but these scripts could not be ported to linux, like for instance,
the tool CHOICE is missing in linux. 
Yes, I could use the command 'select', an internal command of BaSH, but that's not the same. So I tried to port/fork CHOICE into linux.

I found the source of CHOICE in ReactOS. You can find its code here : https://github.com/reactos/reactos/blob/master/base/shell/cmd/choice.c
Tx for that, I still needed to recode some things to be able to compile the code in linux too. 
I succeeded with that, only the beep is not available. I've heard somehow you could make a notification beep in linux, if possible, but then you would need the
root-access, and normally any scripts (in which 'choose' will be used) should be possible to be used by any average user. So the 'beep' only works when
'choose' is compiled in win32. In linux, the code still is there, but nothing will happen.

Anyhow, most of the parameters are the same as with the original CHOICE, but instead of using a slash, use the minus-sign.
Compile and link the program (using 'make') and type `choose -h`, then you get a small help with the options you can use.

Like with CHOICE, 'choose' returns the chosen character as a return-value (errorlevel in cmd/windows).
For instance, with options like this : "ABC" and the user types "C", the program returns a value of '3', which your script can evaluate using '$?'. A return-value of 255
means something went wrong.

If you don't setup 'choose' with a timeout value, it will wait infinitely for the user to enter an option (unless the user presses Ctrl-C). If you do setup a timeout value
for 'choose', you also must setup a default-value which 'choose' will choose when the timeout has reached. So it never will return '0', but instead it returns the
value for that default value. All printable ASCII chars are available, accept for ASC(32) (space) and/or ASC(127). You could setup all chars in the range of ASC(33)..
until ASC(126).

I hope this tool will be handy for you, as it is for me.

The code (version 1.0) is as is. I publish it using the 'GNU General Public License v.3.0' license.

