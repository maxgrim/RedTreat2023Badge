# How to fix the RedTreat 2023 badge

Unfortunately, there are hardware mistakes in this badge. These include:

- Wrong pin mapping of the ESP32-S2-MINI module (3.3v power, boot signal,
  reset/enable signal, USB signal, audio enable signal).
- Mistake in the 3.3->5 volt logic converter that sends signals to the RGB leds.

Below are the steps that we performed to fix a few of the badges produced. In
our experience it took on average 2-2.5 hours to fix one badge.

Recommended is to cover your repairs with solder mask / cure it with a UV light
as some of these repairs are sensitive to breaking again.

# Remove the ESP32

Using a hot-air gun, remove the ESP32 module from the PCB. Make sure not to rip
off pads or undo the shield of the ESP32 module itself. Moreover, the speaker
casing is quite heat sensitive. You can protect it by applying some Kapton tape
onto it.

![Speaker kapton tape](Pictures/kapton.jpg)

![ESP32 removed](Pictures/remove_esp32.jpg)

# Remap ESP32 pins

The picture below shows the remapping that needs to be done. Starting from the
left, going counter clockwise:

1. The power connection (3.3v) originating from the capacitor needs to be
   connected to the pad below it. At the current position it would cause a short
   on the ESP32 module.
2. The boot connection needs to be connected to the pad below it.
3. Both USB connections need to be moved 1 pad to the right.
4. The reset/enable connection needs to be moved a few pins up. Moreover, the
   ground plane that is connected currently to the pad needs to be disconnected.
   If the reset/enable pin is touching ground / is driven low, the ESP32 module
   will not boot.

After this, you can resolder the ESP32 module back onto the board with hot air.

![Remap ESP32 pins](Pictures/esp_pins_wrong.jpg)

## Step 1: power connection

Multiple ways to approach this. We decided to apply solder mask to the GND pin
on the ESP32 so that it will not be connected to the board anymore. Thereafter
we connected the original 3.3V pad with the pad below it with a piece of copper
tape to route the connection correctly / be able to handle some current as
opposed to thin (30+ AWG) wire.

Alternatively, you can re-route the 3.3V signal without the need of solder mask
on the ESP32 pad. But then you need to make sure that your connection can
sustain 60mA with peaks up to 200mA.

![Mask pad on ESP32](Pictures/mask_pad.jpg)

![Move the power signal](Pictures/power_move.jpg)

## Step 2: boot connection

The BOOT button allows to put the ESP32 into bootloader mode. This connection
needs to move 1 pad lower as welll, so disconnect it and connect it to the pad
below.

![Move the boot signal](Pictures/boot_button.jpg)

## Step 3: USB connections

Both USB connections need to move 1 pad to the right. So, cut the traces, and
reconnect them whilst making sure they are not shorted to each other.

![Cut the USB signal](Pictures/cut_usb.jpg)

![Move the USB signal](Pictures/move_usb.jpg)

## Step 4: Reset/Enable connections

Remove the ground plane connections that surround the pin and double check with
a multimeter in continuity mode. Then apply a piece of copper tape / thin wire
ad connect it to the correct capacitor or trace. Make sure to double check
thereafter if nothing is shorted to ground, as it will not boot in this case.

![Cut the groudn plane](Pictures/reset_ground_plane.jpg)

![Move the reset/enable signal](Pictures/reset_connection.jpg)

# Enable the audio

The audio part of the badge has its own 5.5->3 volt converter, which can be
enabled by a connection from the ESP32. However, due to the mismapping of the
ESP32 this pin does not work.

Therefore, you need to remove the current connection to the ESP32 module and
manually add a new connection on this IC to enable it by default, as shown on
the picture below.

![Enable audio](Pictures/enable_audio.jpg)

# Fix the logic converter

There is also a mistake in the logic converter that converts 3.3->5 volt for the
RGB signals coming from the ESP32. Our suspicion is that the 100kOhm resistors
are too large and should be 10kOhm. However, generally the RGB LEDs will also
accept a 3.3 volt level signal. Thus, the easies is to remove this part of the
circuitry (2 resistors and a mosfet) and directly connect the signal to the
LEDs, as shown below.

![Fix logic](Pictures/fix_logic.jpg)

# How to check if your repairs work

You can first apply power to the circuitry directly from your power supply. For
this, we used the GND/output pin of the 5->3.3 voltage regulator.

1. If the current draw is very high (we set a limit of 400mA to ours) then you
   have a short in your circuit.

2. If the current draw is only 3mA, then this is the power indicator LED burning
   on the board. This likely means that the ESP32 is not receiving power
   properly, or the RESET/ENABLE pin is not driving high, which prevents the
   ESP32 from booting.

Once you see a power draw of roughly 60mA, this is a strong indicator that the
ESP32 is powerered properly and booting.

Now you need to check the USB connection. Connect the board to your computer,
and bring it into the bootloader mode by holding the BOOT button whilst powering
it. The sequence can be repeated by: holding the RESET button, then also holding
the BOOT button, then release the RESET button, and thereafter release the BOOT
button. Now, the USB device should show up in your macOS/Linux system as a
ttyUSB (Linux) or cu.usbmodem (macOS) device. If the device does not appear,
multiple things can be wrong:

1. Your USB connection is not properly connected.
2. Your ESP32 does not go into bootloader mode because of the BOOT pin
   connection.

Once the device appears, you can flash the new firmware and test the sound / RGB
LEDs.
