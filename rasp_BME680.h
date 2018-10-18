/*!
 * *****************************************************************
 * Based on original file :
 * @file arduino_BME680.h
 *
 * Adafruit BME680 temperature, humidity, barometric pressure and gas sensor driver
 *
 * This is the documentation for Adafruit's BME680 driver for the
 * Arduino platform.  It is designed specifically to work with the
 * Adafruit BME680 breakout: https://www.adafruit.com/products/3660
 *
 * These sensors use I2C to communicate, 2 pins (SCL+SDA) are required
 * to interface with the breakout.
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Ladyada for Adafruit Industries.
 *
 * BSD license, all text here must be included in any redistribution.
 * 
 * *******************************************************************
 * Modified and extended for Raspberry Pi
 * Initial version : October 2018 Paul van Haastrecht
 * *******************************************************************
 * Resources / dependencies:
 * 
 * BCM2835 library (http://www.airspayce.com/mikem/bcm2835/)
 * twowire library (https://github.com/paulvha/twowire)
 * 
 * library from Bosch 19 jun 2018 / version 3.5.9: 
 * Source : https://github.com/BoschSensortec/BME80_driver
 *  bme680
 *  bme680.h
 *  bme60_defs.h
 * 
 ****** supporting files : ****
 * bme680m.cpp
 * bme680_lib.cpp
 * 
 * *****************************************************************
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation, either version 3 of the 
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/ 

#ifndef __R_BME680_H__
#define __R_BME680_H__

# include <bcm2835.h>
# include <getopt.h>
# include <signal.h>
# include <stdint.h>
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <time.h>
# include <twowire.h>
# include <math.h>
# include <sys/time.h>
# include "bme680.h"


/*=======================================================================
    I2C information
  -----------------------------------------------------------------------*/
# define BME680_DEFAULT_ADDRESS       0x77     ///< The default I2C address

/* default speed 100 Khz*/
# define BME680_SPEED 100

/* default GPIO for SOFT_I2C */
# define DEF_SDA 2
# define DEF_SCL 3

/*! driver information */
struct bmeI2C_p
{
    /*! structure with values */
    bool         hw_initialized;     // initialized or not
    bool         I2C_interface;      // hard_I2C or soft_I2C
    uint8_t     I2C_Address;        // slave address
    uint16_t    baudrate;           // speed
    uint8_t     sda;                // SDA GPIO (soft_I2C only)
    uint8_t     scl;                // SCL GPIO (soft_I2C only)
};

extern struct bmeI2C_p I2Csettings;

/*=======================================================================
   rasp_BME680 Class I2C usage.
   Wraps the Bosch library for usage
  -----------------------------------------------------------------------*/

class rasp_BME680
{
    public:
   
    /*! constructor */
    rasp_BME680();
    
    /*! enable or disable debug messages from the driver */
    void setDebug( int level ) ;
    
    /*! reset BCM2835 and release memory - if applicable */
    void hw_close(void);
    
    /*! perform soft reset on BME680 */
    void reset(void);
    
    /*! set hardware and sensor */
    bool  begin(void);
    
    /*! obtain  / calculate results */
    float readTemperature(void);
    float readPressure(void);
    float readHumidity(void);
    float calc_dewpoint(float temp, float hum);
    uint32_t readGas(void);
    float readAltitude(float seaLevel);

    /*! set BME680 parameters */
    bool setTemperatureOversampling(uint8_t os);
    bool setPressureOversampling(uint8_t os);
    bool setHumidityOversampling(uint8_t os);
    bool setIIRFilterSize(uint8_t fs);
    bool setGasHeater(uint16_t heaterTemp, uint16_t heaterTime);

private:
    /*! Perform a reading */
    bool performReading(void);

    /*! @brief Begin a reading.
     *  
     *  @return When the reading would be ready as absolute time in millis().
     */
    unsigned long beginReading(void);

    /*! values assigned after calling performReading() */
    float temperature;
    /// Pressure (Pascals) assigned after calling performReading() 
    float pressure;
    /// Humidity (RH %) assigned after calling performReading() 
    float humidity;
    /// Gas resistor (ohms) assigned after calling performReading()
    float gas_resistance;

    /*! indicate sampling value has been set and obtain result */
    bool _filterEnabled, _tempEnabled, _humEnabled, _presEnabled, _gasEnabled;
    
    /*! holds the expected time for the results to be ready */
    unsigned long _meas_end;

    /*! needed for communication with driver from Bosch */
    struct bme680_dev gas_sensor;
};


/*=======================================================================
    to display in color 
  -----------------------------------------------------------------------*/
void p_printf (int level, char *format, ...);

/*! color display enable */
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define WHITE   5

#define REDSTR "\e[1;31m%s\e[00m"
#define GRNSTR "\e[1;92m%s\e[00m"
#define YLWSTR "\e[1;93m%s\e[00m"
#define BLUSTR "\e[1;34m%s\e[00m"

/*! set to disable color output */
extern bool NoColor;

#endif
