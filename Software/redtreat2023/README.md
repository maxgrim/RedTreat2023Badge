# RedTreat 2023

Built with IDF version 5.1.1.

## Configuring WiFi

When the badge boots for the first time, it will go into AP mode and broadcast
the "RedTreat2023" SSID. You can connect to this SSID with the "RedTreat2023"
PSK. After this, a Captive Portal will show up that allows you to configure
the WiFi. If the portal does not show up, go to the IP address, which by
default is 10.10.0.1.

## Changing WiFi / reset settings

Do you want to change the WiFi SSID and/or PSK? There is a webpage that allow
you to do so. However, for some reason it is not running stable. Alternatively,
you can delete the current WiFi settings (and UUID) and let it return to AP
mode by:

1. Turn on the badge
2. Then, hold the BOOT button for ~6 seconds
3. The ESP will reboot, connect to the "RedTreat2023" WiFi and configure the
   new SSID and PSK.

## UUID

Each badge has a unique UUID assigned. You can view this UUID in 2 ways.

### UUID via web page

Once the badge is connected to the WiFi, you can access the webserver of the
badge. This is accessible via http://x.x.x.x/uuid.

### UUID via USB

With the esp-idf installed and the badge hooked up to your computer via USB,
you can read the serial console output with the following command.

```
idf_monitor.py -p /dev/cu.usbmodem14101 -b 115200
```

One of the lines shown on the screen is the UUID for your badge, like below:

```
#I (324) redtreat_main: UUID: 7c7b8d0a-e341-4c5e-9a99-78253676a263
```

## Connecting to MQTT

The MQTT server is: badge2023.treat.red.

The username is the UUID that is assigned to your badge.

The password is given during the conference / in the Slack channel.
