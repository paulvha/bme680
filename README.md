# version 1.0	initial version  Ocotber 2018

Copyright (c) 2018 Paul van Haastrecht <paulvha@hotmail.com>


## Background
As part of a larger project I am looking at analyzing and understanding the air quality. 
The aim of this project was to better understand the kind of gas-types that are in the air. 

I have ported the library to CPP on a Raspberry PI running Raspbian Jessie release. It has been 
adjusted and extended for stable working.

 
## Software installation

BCM2835 library
Install latest from BCM2835 from : http://www.airspayce.com/mikem/bcm2835/

1. wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.56.tar.gz
2. tar -zxf bcm2835-1.56.tar.gz		// 1.56 was version number at the time of writing
3. cd bcm2835-1.56
4. ./configure
5. sudo make check
6. sudo make install

In order for this software to run you should NOT enable i2C in raspi-config to load the kernel drivers. 
It works directly on the hardware, but you’ll have to run program as root if you select hard-I2C

twowire library
Obtain the latest version from : https://github.com/paulvha/twowire

1. download the zip-file (clone or download / download zip-file) in the wanted directory
2. unzip twowire-master.zip (*1)
3. cd twowire-master
4. make install


*1) if you do not have unzip : sudo apt-get install zip unzip

BME680 software
Obtain the latest version from : https://github.com/paulvha/bme680

1. Download the zip-file (clone or download / download zip-file) in the wanted directory
2. unzip bme680-master.zip (*1)
3. cd bme680-master
4. create the executable : make
5. To run you have to be as sudo ./bme680m -h ….

## Documentation
Detailed description of the many options in bme680.odt in the documents directory, along with 
other information

## BOSCH BME60 files
The files BME60C, BME680_defs.h and xxx are original files from BOSCH. The current version is
included (19 Jun 2018). They may have different license rights and conditions. For the latest 
info please check https://github.com/BoschSensortec/BME680_driver


