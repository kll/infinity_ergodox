TMK/chibios for Input Club Infinity Ergodox
===========================================
This is an unofficial keyboard firmware for Infinity Ergodox. 

[http://input.club/devices/infinity-ergodox](http://input.club/devices/infinity-ergodox)
    
It is based on Jun Wako's TMK firmware

[https://github.com/tmk/tmk_core](https://github.com/tmk/tmk_core) and [https://github.com/tmk/tmk_keyboard](https://github.com/tmk/tmk_keyboard). The readme files for those projects has a lot of useful information, as do the [Wiki](https://github.com/tmk/tmk_keyboard/wiki).

News and important information
------------------------------
**Always read this section every time you pull a new version**

**15-May-2015**

* A [bug](https://github.com/fredizzimo/infinity_ergodox/issues/9) that corrupts the LCD and LCD backlight, after the keyboard has been on for a while has been fixed. It's strongly recommended to upgrade.
* It's now possible to specify that USB cable to the computer is connected to the right half. See the "Physical connection" section for more information.

**10-Apr-2016**

* The location of most submodules have moved to my own forks. Therefore you need to run `git submodule sync --recursive` before doing `git submodule update --init --recursive`

Physical connection
-------------------
Connect the **left** half to the computer, and connect a link cable between the two halves.

If you want to connect the **right** half instead of the left one to the computer, you need to add the following to the end of the make command. `MASTER=right`. You need to add this for all the make commands that type, so to program the keyboard, the following command should be used `make program MASTER=right`. For consistency, you can also specify left, but it's the default so therfore it's not needed. Furthermore, you can edit the makefile itself, so that you don't need to type it all the time, but remember to not send pull request with that option on!

The firmware does currently not work when chaining several keyboards together. So you are limited to a single keyboard consisting of a left hand plus a right hand. This limitation can quite easily be fixed in the future. However I can't test the chaining myself, so I need some help, open a ticket if you are interested.

Git notes
---------
This repository uses submodules, so after you:

* Clone the repository
* Pull a new version
* Switch a branch

You should run

1. `git submodule sync --recursive` (This is not strictly needed, but in case the location of the repositories changes, it's good to do)
1. `git submodule update --init --recursive`

Prerequisites
-------------
If you are using Windows, I recommend that you use Cygwin (that's what I'm using). I haven't tested any other operating systems, but if you can install the required tools it will probably work.

In order to compile the firmware you need:

1.  GNU make
1.  [GCC ARM Embedded](https://launchpad.net/gcc-arm-embedded):
	* Note that the 5.x and 6.x versions don't work at the moment, so I recommend that you download the 4.9 version.
	* You can also download the ChibiStudio from [www.chibios.org](http://www.chibios.org), which comes with eclipse and other tools.
	* Make sure to add the bin folder to the path. In Cygwin you can edit .bash_profile in your home directory and add it. Here's what I added to the end of the file `PATH="${PATH}:/cygdrive/C/ChibiStudio/tools/GNU Tools ARM Embedded/4.9 2015q3/bin"`
	* For OSX, you can install the px4/px4/gcc-arm-none-eabi-49 brew package:
	
			brew tap px4/px4
			brew update
			brew install gcc-arm-none-eabi-49
		
1. dfu-util:

 	* On windows you also need to install some drivers. You can find more detailed instructions on the [Kiibohd controller wiki page](https://github.com/kiibohd/controller/wiki/Loading-DFU-Firmware). You also need to ensure that dfu-util is in your path.
 	
	* On OSX, you can install `dfu-util` via brew: `brew install dfu-util`

Build
-----
Just run `$ make [makefile options]` to build the everything.

You can override some makefile variables by specifying options, for example if you can type `make MASTER=right` if you want to connect the right hand keyboard to the computer instead of the left one. Some options related to keyboard features and debug can also be specified, refer to the official [TMK build documentation](https://github.com/fredizzimo/tmk_core/blob/master/doc/build.md) for more information.

Upload
------
To upload(flash) a new firmware to the keyboard, first enter the bootloader mode, either by pressing the dedicated flash button on the bottom of the keyboard, or by pressing a mapped bootloader button.

The default layout has that button mapped on layer 2(keyboard functions). 

When the keyboard is in bootloader mode, and the LCD is red with no text, you can run this command.

$ make program [makefile options]

Make sure you specify the exact same options as for the build command.

For simple layout updates, you only need to flash the master(left) half. But each time there's a new firmware version or if you want to make some visualization changes, you need to flash them both.

Changing keyboard layout
------------------------
Changing the keyboard layout works the same way as for the other TMK based keyboards. So read [this](https://github.com/tmk/tmk_keyboard/wiki/FAQ-Keymap).

LCD Visualization
-----------------
In order to customize the LCD visualization, which includes both the backlight and the LCD screen display itself, you need to edit the visualizer\_user.c file. The file is quite well commented, so just read through the comments, and start experimenting. At the very least you probably want to edit the layer names and colors, in the update\_user\_visualizer\_state function.

Currently there's no support for LED visualization. That should be easy to add, but I haven't installed LED's myself, so I would be unable to test. Contributions are welcome, but I can also consider making this myself if someone is willing to test. So open a ticket if you are interested.
