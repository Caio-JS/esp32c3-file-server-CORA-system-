ESP32-C3-file-server (CORA sytem)

Features
-Upload and Download files
-Delete files on system by web
-Create folders

Hardware
-Esp32-C3
-Tft screen 1.8
-SD card slot
-SD card 8GB
-Breadboard
-Jumpers

Pinout
-Tft screen
{ Vcc = 3.3v
  Gnd = Gnd
  Reset = Gpio 9
  CS = Gpio 10
  SDA(MOSI) = Gpio 7
  A0 = Gpio 8
  SCK = Gpio 4
  Led = 3.3v
}
-SD card slot
{ CS = Gpio 1
  SDA(MOSI) = Gpio 7
  SCK = Gpio 4
  MISO = Gpio 5
}

How to use
-Power the Esp32-C3
-Connect to same Wi-Fi network
-Access the IP adress shown on tft screen
-Manage files on web site

Project Status
-Under development

Future Improvements
-Improve web interface
-Fix bugs and optimize code

Author
-Caio-JS
