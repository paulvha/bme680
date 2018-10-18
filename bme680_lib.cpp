/*!
 * *****************************************************************
 * Based on original file :
 * 
 * @file arduino_BME680.cpp
 *
 * @mainpage Adafruit BME680 temperature, humidity, barometric pressure and gas sensor driver
 *
 * @section intro_sec Introduction
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
 * @section author Author
 *
 * Written by Ladyada for Adafruit Industries.
 *
 * @section license License
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
 * bme60m.cpp
 * rasp_BME680.h
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


#include "rasp_BME680.h"

/* debug messages */
int _bme_debug=0;

/* global constructor for I2C (hardware of software) */ 
TwoWire TWI;

/* Our hardware interface functions */
static int8_t i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
static int8_t i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len);
static void delay_msec(uint32_t ms);
static unsigned long millis();

// needed for millis()
struct timeval tv, tv_s;

/* holds the I2C information */
struct bmeI2C_p I2Csettings;

/*********************************************************************
 PUBLIC FUNCTIONS
 *********************************************************************/

/*********************************************************************
 *    @brief  Instantiates sensor with Hardware I2C.
 *********************************************************************/
rasp_BME680::rasp_BME680() {
    I2Csettings.sda = DEF_SDA;
    I2Csettings.scl = DEF_SCL;
    I2Csettings.I2C_interface = soft_I2C;
    I2Csettings.I2C_Address = BME680_DEFAULT_ADDRESS;
    I2Csettings.baudrate = BME680_SPEED;
  
  _filterEnabled = _tempEnabled = _humEnabled = _presEnabled = _gasEnabled = false;
}

/********************************************************************
 * @brief  close Hardware correctly on the Raspberry Pi
 ********************************************************************/
void rasp_BME680::hw_close( void ) {
    TWI.close();
}

/*********************************************************************
 * @brief  Strictly software reset.  Run .begin() afterwards
 *******************************************************************/
void rasp_BME680::reset( void ) {
    bme680_soft_reset(&gas_sensor);
}

/*********************************************************************
 * @brief  Set debug messages
 *******************************************************************/
void rasp_BME680::setDebug( int level ) {
    if (level)  _bme_debug = 1;
    else   _bme_debug = 0; 
}

/********************************************************************/
/*!
    @brief Initializes the sensor

    Hardware is initialized, verifies the BME680 is in the I2C bus, 
    then reads calibration data in preparation for sensor values reads.

    @return True on sensor initialization success. False on failure.
*/
/*********************************************************************/
bool rasp_BME680::begin() {

    if (TWI.begin(I2Csettings.I2C_interface,I2Csettings.sda,I2Csettings.scl))
    {
        p_printf(RED,(char *) "Error during starting I2C\n");
        return(false);
    } 
    
    gas_sensor.read = &i2c_read;
    gas_sensor.write = &i2c_write;
    gas_sensor.delay_ms = &delay_msec;
    gas_sensor.intf= BME680_I2C_INTF;   // set I2C

    if (bme680_init(&gas_sensor) != BME680_OK) return false;
    
    if (_bme_debug)
    {
        printf("T1 = %d\n",gas_sensor.calib.par_t1);
        printf("T2 = %d\n",gas_sensor.calib.par_t2);
        printf("T3 = %d\n",gas_sensor.calib.par_t3);
        printf("P1 = %d\n",gas_sensor.calib.par_p1);
        printf("P2 = %d\n",gas_sensor.calib.par_p2);
        printf("P3 = %d\n",gas_sensor.calib.par_p3);
        printf("P4 = %d\n",gas_sensor.calib.par_p4);
        printf("P5 = %d\n",gas_sensor.calib.par_p5);
        printf("P6 = %d\n",gas_sensor.calib.par_p6);
        printf("P7 = %d\n",gas_sensor.calib.par_p7);
        printf("P8 = %d\n",gas_sensor.calib.par_p8);
        printf("P9 = %d\n",gas_sensor.calib.par_p9);
        printf("P10 = %d\n",gas_sensor.calib.par_p10);
        printf("H1 = %d\n",gas_sensor.calib.par_h1);
        printf("H2 = %d\n",gas_sensor.calib.par_h2);
        printf("H3 = %d\n",gas_sensor.calib.par_h3);
        printf("H4 = %d\n",gas_sensor.calib.par_h4);
        printf("H5 = %d\n",gas_sensor.calib.par_h5);
        printf("H6 = %d\n",gas_sensor.calib.par_h6);
        printf("H7 = %d\n",gas_sensor.calib.par_h7);
        printf("G1 = %d\n",gas_sensor.calib.par_gh1);
        printf("G2 = %d\n",gas_sensor.calib.par_gh2);
        printf("G3 = %d\n",gas_sensor.calib.par_gh3);
        printf("Heat Range = %d\n",gas_sensor.calib.res_heat_range);
        printf("Heat Val = %d\n",gas_sensor.calib.res_heat_val);
        printf("SW Error = %d\n",gas_sensor.calib.range_sw_err);
    }

    /* set default values and enable */
    setTemperatureOversampling(BME680_OS_8X);
    setHumidityOversampling(BME680_OS_2X);
    setPressureOversampling(BME680_OS_4X);
    setIIRFilterSize(BME680_FILTER_SIZE_3);
    setGasHeater(320, 150); // 320*C for 150 ms
    
    /* don't do anything till we request a reading */
    gas_sensor.power_mode = BME680_FORCED_MODE;

    /* set start time for millis() */
    gettimeofday(&tv_s, NULL);
    
    return true;
}

/*********************************************************************/
/*!
    @brief Performs a reading and returns the ambient temperature.
    @return Temperature in degrees Centigrade
*/
/*********************************************************************/
uint32_t rasp_BME680::readGas(void) {
  performReading();
  return(gas_resistance);
}

/*********************************************************************/
/*!
    @brief Performs a reading and returns the ambient temperature.
    @return Temperature in degrees Celcius
*/
/*********************************************************************/
float rasp_BME680::readTemperature(void) {
  performReading();
  return(temperature);
}

/*********************************************************************/
/*!
    @brief Performs a reading and returns the barometric pressure.
    @return Barometic pressure in Pascals
*/
/*********************************************************************/
float rasp_BME680::readPressure(void) {
  performReading();
  return(pressure);
}

/*********************************************************************/
/*!
    @brief Performs a reading and returns the relative humidity.
    @return Relative humidity as floating point
*/
/*********************************************************************/
float rasp_BME680::readHumidity(void) {
  performReading();
  return(humidity);
}

/*********************************************************************/
/*!
    @brief Calculates the altitude (in meters).

    Reads the current atmostpheric pressure (in hPa) from the sensor
    and calculates via the provided sea-level pressure (in hPa).
    
    sealevel is like 101325 NOT 1013.25 !!

    @param  seaLevel      Sea-level pressure in hPa
    @return Altitude in meters
*/
/*********************************************************************/
float rasp_BME680::readAltitude(float seaLevel) {
    // Equation taken from BMP180 datasheet (page 16):
    //  http://www.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

    // Note that using the equation from wikipedia can give bad results
    // at high altitude. See this thread for more information:
    //  http://forums.adafruit.com/viewtopic.php?f=22&t=58064

    float atmospheric = readPressure(); 
    return 44330.0 * (1.0 - pow(atmospheric / seaLevel, 0.190284));
}

/*********************************************************************
    @brief Set sensor parameters and begin reading.
    
    @return expected time for reading to be complete
**********************************************************************/
unsigned long rasp_BME680::beginReading(void) {
  
  uint8_t set_required_settings = 0;
  uint16_t meas_period;
  
  if (_meas_end != 0) {
    /* A measurement is already in progress */
    return _meas_end;
  }

  /* Select the power mode */
  /* Must be set before writing the sensor configuration */
  gas_sensor.power_mode = BME680_FORCED_MODE;

  /* Set the requested sensor settings */
  if (_tempEnabled)
    set_required_settings |= BME680_OST_SEL;
  if (_humEnabled)
    set_required_settings |= BME680_OSH_SEL;
  if (_presEnabled)
    set_required_settings |= BME680_OSP_SEL;
  if (_filterEnabled)
    set_required_settings |= BME680_FILTER_SEL;
  if (_gasEnabled)
    set_required_settings |= BME680_GAS_SENSOR_SEL;

  if (_bme_debug) printf("Setting sensor settings\n");
  
  if (bme680_set_sensor_settings(set_required_settings, &gas_sensor) != BME680_OK)
  {
    if (_bme_debug) p_printf(RED, (char *) "ERROR during setting sensor settings\n");
    return (false);
  }
  
  /* Set the power mode to trigger start of measurement cycle */
  if (_bme_debug) printf("Setting power mode\n");

  if (bme680_set_sensor_mode(&gas_sensor) != BME680_OK)
  {
    if (_bme_debug) p_printf(RED, (char *) "ERROR during setting power mode\n");
    return (false);
  }

  /* Get the total measurement duration so as to sleep or wait till the
   * measurement is complete */

  bme680_get_profile_dur(&meas_period, &gas_sensor);
  _meas_end = millis() + meas_period;

  return _meas_end;
}

/*********************************************************************/
/*!
    @brief calculate dew point
    @param temp : current temperature in celsius
    @param hum : current humidity
    
    using the Augst-Roche-Magnus Approximation.
    
    @return dewpoint
 *********************************************************************/   
float rasp_BME680::calc_dewpoint(float temp, float hum) {
    
    float td, H;
 
    H = log(hum/100) + ((17.625 * temp) / (243.12 + temp));
    td = 243.04 * H / (17.625 - H);
    
    return(td);
}

/*********************************************************************/
/*!
    @brief Performs a full reading of all 4 sensors in the BME680.

    Assigns the internal #temperature, #pressure, #humidity
    and #gas_resistance member variables

    @return True on success, False on failure
*/
/*********************************************************************/

bool rasp_BME680::performReading(void) {

    struct bme680_field_data data;
    
    /* trigger start reading */   
    unsigned long meas_end = beginReading();
    
    if (meas_end == 0) {
        if (_bme_debug) p_printf(RED, (char *) "ERROR during begin reading\n");
        return(false);
    }
    
    unsigned long now = millis();
    
    if (meas_end > now) {
        unsigned long meas_period = meas_end - now;
    
        if (_bme_debug)  printf("Waiting (ms) %ld\n",meas_period);
    
        /* Delay 2 x waiting timing till the measurement is expected to beready */
        gas_sensor.delay_ms(meas_period * 2); 
    }
    
    _meas_end = 0; /* Allow new measurement to begin */
    
    if(bme680_get_sensor_data(&data, &gas_sensor) != BME680_OK)
        return false;
    
    /* if NO new fields */
    if ( ! data.status & BME680_NEW_DATA_MSK)
    {
        if (_bme_debug)  printf("No new fields\n");
        temperature = pressure = humidity = NAN;
        gas_resistance = 0;
        return (true);
    }
    
    if (_tempEnabled)  temperature = data.temperature / 100.0;
    else temperature = NAN;
    
    if (_humEnabled)   humidity = data.humidity / 1000.0;
    else humidity = NAN;
        
    if (_presEnabled)  pressure = data.pressure;
    else pressure = NAN;
    
    /* Avoid using measurements from an unstable heating setup */
    if (_gasEnabled) {
    
        // if heater was stable
        if (data.status & BME680_HEAT_STAB_MSK) {
          gas_resistance = data.gas_resistance;
        } else {
            gas_resistance = 0;
            //printf("Gas reading unstable!\n");
        }
    }
    
    return true;
}

/*********************************************************************/
/*!
    @brief  Enable and configure gas reading + heater
    @param  heaterTemp desired temperature in degrees Centigrade
    @param  heaterTime Time to keep heater on in milliseconds
    
    @return True on success, False on failure
*/
/*********************************************************************/
bool rasp_BME680::setGasHeater(uint16_t heaterTemp, uint16_t heaterTime) {
    
  gas_sensor.gas_sett.heatr_temp = heaterTemp;
  gas_sensor.gas_sett.heatr_dur = heaterTime;

  if ( (heaterTemp == 0) || (heaterTime == 0) ) {
    // disabled!
    gas_sensor.gas_sett.run_gas = BME680_DISABLE_GAS_MEAS;
    _gasEnabled = false;
  } else {
    gas_sensor.gas_sett.run_gas = BME680_ENABLE_GAS_MEAS;
    _gasEnabled = true;
  }
  return true;
}

/*********************************************************************/
/*!
    @brief  Setter for Temperature oversampling
    @param  oversample Oversampling setting, 
        can be BME680_OS_NONE (turn off Temperature reading),
        BME680_OS_1X, BME680_OS_2X, BME680_OS_4X, BME680_OS_8X or BME680_OS_16X
    
    @return True on success, False on failure
*/
/*********************************************************************/

bool rasp_BME680::setTemperatureOversampling(uint8_t oversample) {
  
  if (oversample > BME680_OS_16X) return false;

  gas_sensor.tph_sett.os_temp = oversample;

  if (oversample == BME680_OS_NONE)
    _tempEnabled = false;
  else
    _tempEnabled = true;

  return true;
}


/********************************************************************/
/*!
    @brief  Setter for Humidity oversampling
    @param  oversample Oversampling setting, 
        can be BME680_OS_NONE (turn off Humidity reading),
        BME680_OS_1X, BME680_OS_2X, BME680_OS_4X, BME680_OS_8X or BME680_OS_16X
    
    @return True on success, False on failure
*/
/*********************************************************************/

bool rasp_BME680::setHumidityOversampling(uint8_t oversample) {
  
  if (oversample > BME680_OS_16X) return false;

  gas_sensor.tph_sett.os_hum = oversample;

  if (oversample == BME680_OS_NONE)
    _humEnabled = false;
  else
    _humEnabled = true;

  return true;
}

/*********************************************************************/
/*!
    @brief  Setter for Pressure oversampling
    @param  oversample Oversampling setting, 
        can be BME680_OS_NONE (turn off Pressure reading),
        BME680_OS_1X, BME680_OS_2X, BME680_OS_4X, BME680_OS_8X or BME680_OS_16X
    
    @return True on success, False on failure
*/
/*********************************************************************/
bool rasp_BME680::setPressureOversampling(uint8_t oversample) {
  
  if (oversample > BME680_OS_16X) return false;

  gas_sensor.tph_sett.os_pres = oversample;

  if (oversample == BME680_OS_NONE)
    _presEnabled = false;
  else
    _presEnabled = true;

  return true;
}

/********************************************************************/
/*!
    @brief  Setter for IIR filter.
    @param filtersize Size of the filter (in samples). 
        Can be BME680_FILTER_SIZE_0 (no filtering), 
        BME680_FILTER_SIZE_1, BME680_FILTER_SIZE_3, 
        BME680_FILTER_SIZE_7, BME680_FILTER_SIZE_15, 
        BME680_FILTER_SIZE_31, BME680_FILTER_SIZE_63, 
        BME680_FILTER_SIZE_127
    @return True on success, False on failure

*/
/*********************************************************************/
bool rasp_BME680::setIIRFilterSize(uint8_t filtersize) {
  
  if (filtersize > BME680_FILTER_SIZE_127) return false;

  gas_sensor.tph_sett.filter = filtersize;

  if (filtersize == BME680_FILTER_SIZE_0)
    _filterEnabled = false;
  else
    _filterEnabled = true;

  return true;
}

/*********************************************************************/
/*!
    @brief Reads 8 bit values over I2C
    @param  dev_id : not used, but maintained for compatibility
    @param reg_addr : start register to read from
    @param reg_data : store the data read 
    @param len : total amount of bytes to be read.
    
    @return 0 = good, 1 = error
*/
/*********************************************************************/
int8_t i2c_read(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {

    Wstatus result;
    int retry = 3, i;
    char addr = (char) reg_addr;
    
    if (_bme_debug) printf("Reading from register 0x%x, %d bytes\n",reg_addr, len);
    
    /* set slave address */
    TWI.setSlave(I2Csettings.I2C_Address);

    while(1)
    {
        /* first write the register we want to read */
        if (TWI.i2c_write(&addr, 1) != I2C_OK) 
        {
            if (_bme_debug) p_printf(RED,(char *) "Error during reading register %d\n",addr);
            return(1);
        }
   
        /* read results from I2C */
        result = TWI.i2c_read((char *) reg_data, len);
        
        /* if failure, then retry as long as retrycount has not been reached */
        if (result != I2C_OK)
        {
            if (_bme_debug)  
                p_printf(YELLOW, (char *) " read retrying %d\n%d\n", result);
            if (retry-- > 0) continue;
        }
 
        /* process result */
        switch(result)
        {
            case I2C_OK:
                if (_bme_debug) {
                    printf("data read :");
                    for (i=0 ;i < len ; i++) printf(" 0x%x ", reg_data[i]);
                    printf("\n");
                }
                return(0);
                
            case I2C_SDA_NACK :
                if (_bme_debug) p_printf(RED, (char *) "Read NACK error\n");
                return(1);
    
            case I2C_SCL_CLKSTR :
                if (_bme_debug) p_printf(RED, (char *) "Read Clock stretch error\n");
                return(1);
                
            case I2C_SDA_DATA :
                if (_bme_debug) p_printf(RED, (char *) "not all data has been read\n");
                return(1);
                
            default:
                if (_bme_debug) p_printf(RED, (char *) "unkown return code\n");
                return(1);
        }
    }
}

/*********************************************************************/
/*!
    @brief Writes 8 bit values over I2C
    @param dev_id : not used, but maintained for compatibility
    @param reg_addr : first register to write to
    @param data : data to write. data[0] is data for reg_addr. This can 
                   be followed with a sequence of the next reg_addr and data    
    @param len : total amount of bytes in data.
    
    @return 0 = good, 1 = error
*/
/**********************************************************************/
int8_t i2c_write(uint8_t dev_id, uint8_t reg_addr, uint8_t *reg_data, uint16_t len) {
    
    int retry = 3, i;
    Wstatus result;
    char tmp[BME680_TMP_BUFFER_LENGTH +1];
    
    /* exceeding buffer during copy */
    if (len > BME680_TMP_BUFFER_LENGTH) return(1);

    if (_bme_debug){
        
        printf("\twriting to regaddrs 0x%x, data : 0x%x\n",reg_addr, reg_data[0]);

        // the BME680 allows for multiple register write in one go
        for (i = 1; i < len; i++)
        { 
            printf("\twriting to regaddrs 0x%x, data :",reg_data[i++]);
            printf(" 0x%x\n",reg_data[i]);
        }
    }

    /* set slave address */
    TWI.setSlave(I2Csettings.I2C_Address);
    
    /* copy register address and data in single buffer*/
    tmp[0] = reg_addr;
    for (i=0 ; i < len ;i++) tmp[1+i] = reg_data[i];
    
    while (1)
    {
        // perform a write of data
        result = TWI.i2c_write(tmp, (uint8_t) len +1);
        
        // if error, perform retry (if not exceeded)
        if (result != I2C_OK)
        {
            if (_bme_debug) printf(" send retrying %d\n", (int) result);
            if (retry-- > 0) continue;
        }
    
        switch(result)
        {
            case I2C_OK:
                return(0);
            
            case I2C_SDA_NACK :
                if (_bme_debug) p_printf(RED, (char *) "write NACK error\n");
                return(1);
        
            case I2C_SCL_CLKSTR :
                if (_bme_debug) p_printf(RED, (char *) "write Clock stretch error\n");
                return(1);
        
            case I2C_SDA_DATA :
                if (_bme_debug) p_printf(RED, (char *) "write not all data has been sent\n");
                return(1);
        
            default :
                if (_bme_debug) p_printf(RED, (char *) "Unkown error during writing\n");
                return(1);
        }
    }
}

/********************************************************************* 
 * @brief get milli-seconds since start of program *
 * @return Milli-seconds
 *********************************************************************/
static unsigned long millis() {
    
    gettimeofday(&tv, NULL);
    
    /* seconds to milliseconds, microseconds to milli seconds */
    unsigned long calc = ((tv.tv_sec - tv_s.tv_sec) * 1000) + ((tv.tv_usec - tv_s.tv_usec) / 1000);
    return(calc);
}

/*********************************************************************
 * @brief delay for requested milli seconds                         * 
 * @param milliseconds to wait                                      *
 *********************************************************************/
static void delay_msec(uint32_t ms){
    struct timespec sleeper;
    
    sleeper.tv_sec  = (time_t)(ms / 1000);
    sleeper.tv_nsec = (long) (ms % 1000) * 1000000; 
    nanosleep(&sleeper, NULL);
}

/*********************************************************************
 * @brief Display in color
 * @param format : Message to display and optional arguments
 *                 same as printf
 * @param level :  1 = RED, 2 = GREEN, 3 = YELLOW 4 = BLUE 5 = WHITE
 * 
 * if NoColor was set, output is always WHITE.
 *********************************************************************/
void p_printf(int level, char *format, ...) {
    
    char    *col;
    int     coll=level;
    va_list arg;
    
    //allocate memory
    col = (char *) malloc(strlen(format) + 20);
    
    if (NoColor) coll = WHITE;
                
    switch(coll)
    {
    case RED:
        sprintf(col,REDSTR, format);
        break;
    case GREEN:
        sprintf(col,GRNSTR, format);
        break;      
    case YELLOW:
        sprintf(col,YLWSTR, format);
        break;      
    case BLUE:
        sprintf(col,BLUSTR, format);
        break;
    default:
        sprintf(col,"%s",format);
    }

    va_start (arg, format);
    vfprintf (stdout, col, arg);
    va_end (arg);

    fflush(stdout);

    // release memory
    free(col);
}
