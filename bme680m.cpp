/***********************************************************************
 * 
 * Extended & Modified for Raspberry Pi October 2018 Paul van Haastrecht
 * 
 * version 2.0 September 2020 / paulvha
 * - fix compile issues on Pi-os (Buster)
 * 
 * 
 * Hardware connections: (raspberry B/B+/3/4)
 * 
 * components side pin 1 - left
 * 
 * I2C connection (as used in this program)
 * VIN       power in : 3V or 5V (pin 1)
 * 3v3       power out (max 100mA) NOT connected
 * GND       GND (pin 6)
 * SCK       I2C SCL (pin 5)        *1
 * sdo       NOT connected
 * sdi       I2C SDA (pin 3)        *1
 * cs        NOT connected
 * 
 *      *1 : this is default, defined in rasp_BME680.h 
 * 
 * By default address is set for 0x77. Add a jumper between SDO and GND
 * to change to 0x76 (use the -A option in the program to select)
 * 
 * Development environment specifics:
 * Raspberry Pi / linux Jessie release
 ******************************************************************
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
 * 
 * bme680_lib.cpp
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
#define  VERSION "2.0 september 2020"

#define  MAXBUF     200
#define  LOOPDELAY  5       // 5 seconds delay default

typedef struct bmeval
{
    uint8_t overSampleT;    // oversample temperature  
    uint8_t overSampleH;    // oversample humidity
    uint8_t overSampleP;    // oversample pressure
    uint8_t filter;         // filter coefficient
    uint16_t heaterT;       // heater temperature in celsius
    uint16_t heaterM;       // heater time in ms
    float tempC;            // temperature from the BME680
    float humid;            // humidity from the BME680
    float pressure;         // pressure from the BME680
    float sealevel;         // hold current pressure at sealevel
    float height;           // hold calculated height based on pressure
    float dewpoint;         // hold calculated dewpoint
    uint32_t gas_resistance; // resistance of MOX sensor
} bmeval;

typedef struct measure
{
    int       verbose;        // display debug information
    uint16_t  loop;           // # of measurement loops
    uint16_t  loop_delay;     // loop delay (sec)
    char      format[MAXBUF]; // output format
    char      v_save_file[MAXBUF];   // value savefile
    struct bmeval bme;       // BME680 info
} measure;

char progname[20];

/* global constructer */ 
rasp_BME680 MyBme;

/* used as part of p_printf() */
bool NoColor= false;

/*********************************************************************
*  @brief generate timestamp
*  @param buf : store timestamp
**********************************************************************/  
void time_stamp(char *buf)
{
    time_t ltime;
    struct tm *tm;
    
    ltime = time(NULL);
    tm = localtime(&ltime);
    
    static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    
    static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    sprintf(buf, "%.3s %.3s%3d %.2d:%.2d:%.2d %d",
    wday_name[tm->tm_wday],  mon_name[tm->tm_mon],
    tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
    1900 + tm->tm_year);
}

/*********************************************************************
*  @brief close hardware and program correctly
*  @param val : exit value
**********************************************************************/
void closeout(int val)
{
    /* stop BME680 sensor */
    MyBme.reset();
    
    /* close I2C channel */
    MyBme.hw_close();
    
    exit(val);
}

/*********************************************************************
 * @brief : catch signals to close out correctly 
 * @param sig_num : signal raised to program
 *********************************************************************/
void signal_handler(int sig_num)
{
    switch(sig_num)
    {
        case SIGINT:
        case SIGKILL:
        case SIGABRT:
        case SIGTERM:
        default:
            printf("\nStopping BME680 monitor\n");
            closeout(EXIT_SUCCESS);
            break;
    }
}

/*********************************************************************
 * @brief : setup signals 
 *********************************************************************/
void set_signals()
{
    struct sigaction act;
    
    memset(&act, 0x0,sizeof(act));
    act.sa_handler = &signal_handler;
    sigemptyset(&act.sa_mask);
    
    sigaction(SIGTERM,&act, NULL);
    sigaction(SIGINT,&act, NULL);
    sigaction(SIGABRT,&act, NULL);
    sigaction(SIGSEGV,&act, NULL);
    sigaction(SIGKILL,&act, NULL);
}

/**********************************************************
 * @brief : get the real parameter to set the BME680 oversampling
 *           for humidity, temperature or pressure as requested
 * @param : select oversampling indicator
 * @return : value to use for Bosch library or 0xff in case of error
 **********************************************************/
uint8_t getOversample(uint8_t opt)
{
    switch(opt)
    {
        case 0:     return(BME680_OS_NONE);
        case 1:     return(BME680_OS_1X);
        case 2:     return(BME680_OS_2X);
        case 4:     return(BME680_OS_4X);
        case 8:     return(BME680_OS_8X);
        case 16:    return(BME680_OS_16X);
        default:    return(0xff);
     }
}

/****************************************************************
 * @brief : get the real parameter to set the BME680 filter 
 * @param : select filter indicator
 * @return : value to use for Bosch library or 0xff in case of error
 ****************************************************************/
uint8_t getfilter(uint8_t opt)
{
    switch(opt)
    {
        case 0:     return(BME680_FILTER_SIZE_0);
        case 1:     return(BME680_FILTER_SIZE_1);
        case 3:     return(BME680_FILTER_SIZE_3);
        case 7:     return(BME680_FILTER_SIZE_7);
        case 15:    return(BME680_FILTER_SIZE_15);
        case 31:    return(BME680_FILTER_SIZE_31);
        case 63:    return(BME680_FILTER_SIZE_63);
        case 127:   return(BME680_FILTER_SIZE_127);
        default:    return(0xff);
    }
}

/*********************************************************************
*  @brief : display usage information  
*  @param mm ; measurement variables
**********************************************************************/
void usage(struct measure *mm)
{

    p_printf(YELLOW, (char *)
    "%s [options] \n\n"
    
    "\nBME680 settings: \n\n"

    "-F #       filter coefficient       (default %d)\n"
    "-H #       humidity oversampling    (default %d)\n"
    "-M #       calculate height compared to sealevel pressure\n"
    "-P #       pressure oversampling    (default %d)\n"
    "-T #       temperature oversampling (default %d)\n"
    "-C #       heater temperature       (default %d C)\n"
    "-K #       heater warm-up time      (default %d Ms)\n"

    "\nprogram settings: \n\n"
    "-B         no colored output\n"
    "-L #       loop count               (default 0: endless)\n"
    "-D #       delay between loops      (default %d seconds)\n"
    "-O string  output format string\n"
    "-V #       verbose level (1 = user program, 2 + driver messages.\n"
    "-W file    save formatted output to file\n"
    
    "\nI2C settings: \n\n"
    "-A #       i2C address              (default 0x%02x)\n"
    "-i         interface with HARD_I2C  (default software I2C)\n"
    "-I #       I2C speed 1 - 400        (default %d Khz)\n"
    "-s #       SOFT I2C GPIO # for SDA  (default GPIO %d)\n"
    "-d #       SOFT I2C GPIO # for SCL  (default GPIO %d)\n"

    "\n\nVersion %s\n"

    ,progname, mm->bme.filter, mm->bme.overSampleH,
    mm->bme.overSampleP, mm->bme.overSampleT ,mm->bme.heaterT, 
    mm->bme.heaterM, LOOPDELAY,I2Csettings.I2C_Address, 
    I2Csettings.baudrate, DEF_SDA, DEF_SCL, VERSION);
}

/*********************************************************************
 *  @brief : perform init (take in account commandline option) 
 *  @param mm ; measurement variables
 * 
 *  speed of i2C, i2C addresses etc. &  set oversampling /filter.
 *********************************************************************/
void init_hardware(struct measure *mm)
{
    /* hard_I2C requires  root permission */    
    if (I2Csettings.I2C_interface == hard_I2C)
    {
        if (geteuid() != 0)
        {
            p_printf(RED,(char *)"You must be super user\n");
            exit(EXIT_FAILURE);
        }
    }

    /* set hardware and I2C measurement settings */
    if (mm->verbose) 
    {
        printf((char *)"initialize BCM2835 / BME680\n");
        printf((char *)"set slaveaddres 0x%x\n",I2Csettings.I2C_Address);
        printf((char *)"set baudrate %dKhz\n",I2Csettings.baudrate);
        
        // potentially enable debug messages
        if (mm->verbose == 2) MyBme.setDebug(1);
    }
    
    if (MyBme.begin() != true)
    {
        p_printf(RED,(char *)"error during starting BME680\n");
        exit(EXIT_FAILURE);
    }
 
    /* set BME680 measurement settings */
    if (MyBme.setHumidityOversampling(getOversample(mm->bme.overSampleH)) == false)
    {
        p_printf(RED,(char *) "incorrect BME680 humidity oversampling: %d\n",mm->bme.overSampleH);
        closeout(EXIT_FAILURE);
    }  
 
    if (MyBme.setTemperatureOversampling(getOversample(mm->bme.overSampleT)) == false)
    {
        p_printf(RED,(char *) "incorrect BME680 temperature oversampling: %d\n",mm->bme.overSampleT);
        closeout(EXIT_FAILURE);
    }   
 
    if (MyBme.setPressureOversampling(getOversample(mm->bme.overSampleP)) == false)
    {
        p_printf(RED,(char *) "incorrect BME680 pressure oversampling: %d\n",mm->bme.overSampleP);
        closeout(EXIT_FAILURE);
    } 
  
    if (MyBme.setIIRFilterSize(getfilter(mm->bme.filter)) == false)
    {
        p_printf(RED,(char *) "incorrect BME680 filter size: %d\n",mm->bme.filter);
        closeout(EXIT_FAILURE);
    } 
 
    if (MyBme.setGasHeater(mm->bme.heaterT, mm->bme.heaterM) == false)
    {
        p_printf(RED,(char *) "incorrect BME680 gas setting: temp %d, time %d\n",mm->bme.heaterT,mm->bme.heaterM);
        closeout(EXIT_FAILURE);
    }     
}

/*********************************************************************
 *  @brief : set default variable values
 *  @param mm ; measurement variables
 *********************************************************************/
void init_variables(struct measure *mm)
{
    /* set I2C */
    I2Csettings.I2C_interface = soft_I2C;// I2c communication
    I2Csettings.I2C_Address = BME680_DEFAULT_ADDRESS;
    I2Csettings.sda = DEF_SDA;           // default SDA line for soft_I2C
    I2Csettings.scl = DEF_SCL;           // SCL GPIO for soft_I2C
    I2Csettings.baudrate = BME680_SPEED; // set default baudrate   
    
    /* BME680 measurement settings */
    mm->bme.overSampleT = 16;           // oversampling
    mm->bme.overSampleH = 8; 
    mm->bme.overSampleP = 8;
    mm->bme.filter = 7;                 // filter
    mm->bme.heaterT = 300;              // heater temperature
    mm->bme.heaterM = 150;              // heater time
    
    /* set program instructions */
    mm->verbose = 0;
    mm->loop = 0;
    mm->loop_delay = LOOPDELAY;
    mm->format[0] = 0x0;
    mm->v_save_file[0] = 0x0;
    
    /* reset results */
    mm->bme.sealevel = 0;
    mm->bme.height = 0;
    mm->bme.tempC = 0;
    mm->bme.pressure =0;
    mm->bme.humid=0;
    mm->bme.dewpoint=0;
}

/*********************************************************************
 * @brief : Read BME680 for temperature, humidity, pressure 
 *           and calculation height and dew_point
 * @param mm ; measurement variables
 * 
 * @return : TRUE if OK, false if error
 *********************************************************************/
bool read_BME680(struct measure *mm)
{
    if (mm->verbose) printf("Try reading BME680 values\n"); 
    
    /* get temperature */
    mm->bme.tempC = MyBme.readTemperature();
    
    if (mm->bme.tempC == NAN)
    {
        p_printf(RED,(char *)"can not read temperature\n");
        return(false);
    }       

    /* get humidity */
    mm->bme.humid = MyBme.readHumidity();
    
    if (mm->bme.humid == NAN)
    {
        p_printf(RED,(char *)"can not read humidity\n");
        return(false);
    }  

    /* get pressure */
    mm->bme.pressure = MyBme.readPressure();
    
    if (mm->bme.pressure == NAN)
    {
        p_printf(RED,(char *)"can not read pressure\n");
        return(false);
    } 
    
    /* get gas */
    mm->bme.gas_resistance = MyBme.readGas();
    if (mm->bme.gas_resistance == 0)
    {
        p_printf(RED,(char *)"can not gas resistance\n");
        return(false);
    } 

    // Calculate hight in meters
    mm->bme.height = MyBme.readAltitude(mm->bme.sealevel);

    // calculate dew_point
    mm->bme.dewpoint = MyBme.calc_dewpoint(mm->bme.tempC, mm->bme.humid);

    return(true);
}

/***************************************************************
 * @brief add to output buffer before checking on length
 * @param buf : end result buffer
 * @param buf1 : data to add to buf
 ***************************************************************/
void add_to_buf(char * buf, char * buf1 )
{
    if (strlen(buf) + strlen(buf1) > MAXBUF)    return;
    
    strcat(buf, buf1);
}

/*****************************************************************
 * @brief : format output buffer
 * @param mm ; measurement variables
 * @param buf ; formatted data to output
 *  
 * output format can be defined
 * 
 * BME results :
 *  T = temperature from BME
 *  H = Humidity from BME
 *  P = Pressure from BME
 *  M = height compared to sealevel
 *  R = Resistance from BME
 *  D = dewpoint
 * 
 * Markup: 
 *  \l = local time
 *  \t = tab
 *  \s = space
 *  \, = comma
 *  \; = semicolon
 *  \\x = character x is included (x can be any)
 *  \n = new line
 *****************************************************************/
void format_output(struct measure *mm, char *buf)
{
    char    *p,tm[30];
  
    /* use default output if no specific format was requested */
    if (strlen(mm->format) == 0 )
    {
        sprintf(buf, "Temp: %2.2f\tHumidity: %2.2f\tpressure: %2.2f\t gas resistance %u Kohm\n",mm->bme.tempC, mm->bme.humid, mm->bme.pressure/100, mm->bme.gas_resistance/1000);
        return;
    }
    else
    {
        /* reset output buffer */
        buf[0] = 0x0;   
    }
    
    p = mm->format;
    
    while (*p != 0x0)
    {
        // BME results
        if (*p == 'T')       sprintf(tm, " Temp: %2.2f",mm->bme.tempC);
        else if (*p == 'H') sprintf(tm, " Humidity: %2.2f",mm->bme.humid);
        else if (*p == 'P') sprintf(tm, " Pressure: %2.2f",mm->bme.pressure/100);
        else if (*p == 'M') sprintf(tm, " Height: %2.2f",mm->bme.height);
        else if (*p == 'R') sprintf(tm, " Resistance: %d",mm->bme.gas_resistance/1000);
        else if (*p == 'D') sprintf(tm, " Dewpoint: %2.2f",mm->bme.dewpoint);
        
        // markup
        else if (*p == '\\')
        {
            p++;

            if (*p == 't') sprintf(tm, "\t");
            else if (*p == 's') sprintf(tm, " ");
            else if (*p == 'n') sprintf(tm, "\n");
            else if (*p == ',') sprintf(tm, ",");
            else if (*p == ';') sprintf(tm, ";");
            else if (*p == 'l')
            {
                // get timestamp
                time_stamp(tm);
            }
            else if (*p == '\\')
            {
                p++; 
                sprintf(tm, "%c",*p);
            }
        }
        
        // trouble ...
        else
        {
            printf("Illegal character %c in output format string: %s\n", *p, mm->format);

            sprintf(buf, "Temp: %2.2f\tHumidity: %2.2f\tpressure: %2.2f\t gas resistance %u Kohm\n",mm->bme.tempC, mm->bme.humid, mm->bme.pressure/100, mm->bme.gas_resistance/1000);

            return;
        }
   
        add_to_buf(buf,tm);
        
        p++;    
 
    }
    
    add_to_buf(buf, (char *) "\n");
}

/**********************************************************
 *  @brief output values to screen and file (if requested) 
 *  @return true if OK, false if error
 **********************************************************/
bool do_output_values(struct measure *mm)
{
    FILE    *fp = NULL ;
    char    buf[MAXBUF];

    if (mm->verbose) printf("output BME680 values\n");  
    
    /* create output string */
    format_output(mm, buf);
        
    /* display output */
    p_printf(YELLOW,(char *) "%s",buf);
     
    /* append output to a save_file (if requested) */
    if (strlen (mm->v_save_file) > 0)
    {
        if(mm->verbose >1 ) printf("Appending data to file %s\n",mm->v_save_file);
        
        // save baseline to file
        fp = fopen (mm->v_save_file, "a");
           
        // Checks if file is open
        if (fp == NULL )
        { 
            p_printf(RED,(char *) "Issue with opening output file: %s\n", mm->v_save_file);                      
            return(false);
        }
        
        // write ouput
        if (fwrite(buf, sizeof(char), strlen(buf),fp) != strlen(buf))
        { 
            p_printf(RED,(char *) "Issue during writing output file: %s\n", mm->v_save_file);                      
            fclose(fp);
            return(false);
        }
        
        fclose(fp);
    }
    
    return(true);
}
/*******************************************************************
 * @brief : main loop for measurement
 * @param mm ; measurement variables
 *******************************************************************/
void main_loop(struct measure *mm)
{
    uint16_t lloop;
        
    /* setup loop count */
    if (mm->loop > 0)   lloop = mm->loop;
    else lloop=1;
    
    printf((char *)"starting mainloop\n");
    
    while (lloop > 0)
    {
        /* read values */
        if (read_BME680(mm) == false) closeout(EXIT_FAILURE);
        
        /* do output */
        if (do_output_values(mm) == false)  closeout(EXIT_FAILURE);

        /* delay */
        if(mm->verbose) printf("wait %d seconds\n",mm->loop_delay);
        sleep(mm->loop_delay);
        
        /* loop count */
        if(mm->loop > 0)    lloop--;
    }
}

/*********************************************************************
 * @brief Parse parameter input (either commandline or file)
 * 
 * @param opt : option character detected
 * @param option : pointer to potential option value
 * @param mm ; measurement variables
 *  
 *********************************************************************/ 
void parse_cmdline(int opt, char *option, struct measure *mm)
{

    switch (opt) {
             
    case 'A':   // BME680 i2C address
        I2Csettings.I2C_Address = (int)strtod(option, NULL);
      
        if (I2Csettings.I2C_Address != 0x77 && I2Csettings.I2C_Address != 0x76)
        {
            p_printf(RED,(char *) "incorrect BME680 i2C address 0x%x\n",I2Csettings.I2C_Address);
            exit(EXIT_FAILURE);
        }   
        break;
    
    case 'B':   // set NO color output
        NoColor = true;
        break;
      
    case 'H':   // BME680 Humidity oversampling
        mm->bme.overSampleH = (uint8_t)strtod(option, NULL); 
        break;
      
     case 'P':   // BME680 pressure oversampling
        mm->bme.overSampleP = (uint8_t)strtod(option, NULL); 
        break;

    case 'F':   // BME680 filter 
        mm->bme.filter = (uint8_t)strtod(option, NULL); 
        break;
          
    case 'T':   // BME680 Temperature oversampling
        mm->bme.overSampleT = (uint8_t)strtod(option, NULL); 
        break;

    case 'C':   // BME680 heater temperature
        mm->bme.heaterT = (uint16_t)strtod(option, NULL); 
        if (mm->bme.heaterT > 400)
        {
          p_printf(RED,(char *) "Invalid amount %d. max 400\n",mm->bme.heaterT);
          exit(EXIT_FAILURE);
        }
        break;

    case 'K':   // BME680 heater warm-up time
        mm->bme.heaterM = (uint16_t)strtod(option, NULL); 
        if (mm->bme.heaterM > 4032)
        {
          p_printf(RED,(char *) "Invalid amount %d. max 4032mS\n",mm->bme.heaterM);
          exit(EXIT_FAILURE);
        }
        break;
           
    case 'I':   // I2C Speed
        I2Csettings.baudrate = (uint32_t) strtod(option, NULL);
     
        if (I2Csettings.baudrate < 1 || I2Csettings.baudrate > 400)
        {
          p_printf(RED,(char *) "Invalid i2C speed option %d\n",I2Csettings.baudrate);
          exit(EXIT_FAILURE);
        }
        break;
    
    case 'L':   // loop count
        mm->loop = (int) strtod(option, NULL);
        break;

    case 'M':   // set pressure sealevel
        if(strlen(option) != 6)
        {
          p_printf(RED,(char *)"invalid pressure must be 6 digits : %s\n",option);
          exit(EXIT_FAILURE);
        }
        
        mm->bme.sealevel = (float) strtod(option, NULL);
      break;
      
    case 'O':   // output string
        strncpy(mm->format,option,MAXBUF);
        break;
  
    case 'D':   // loop delay
        mm->loop_delay = (int) strtod(option, NULL);
        break;
      
    case 'V':   // verbose /debug message
        mm->verbose = (int) strtod(option, NULL);;
        if (mm->verbose > 2)
        {
          p_printf(RED,(char *)"Only level 1 or 2 supported %d\n",mm->verbose);
          exit(EXIT_FAILURE);
        }
        break;
     
    case 'W':   // save file
        strncpy(mm->v_save_file, option, MAXBUF);
        break;
    
    case 'i':   // use hardware I2C
        I2Csettings.I2C_interface = hard_I2C;
        break;
      
    case 'd':   // change default SCL line for soft_I2C
        I2Csettings.scl = (int)strtod(option, NULL);
      
        if (I2Csettings.scl < 2 || I2Csettings.scl == 4 || 
        I2Csettings.scl > 27 || I2Csettings.sda == I2Csettings.scl)
        {
          p_printf(RED,(char *) "invalid GPIO for SCL :  %d\n",I2Csettings.scl);
          exit(EXIT_FAILURE);
        }   
        break; 

    case 's':   // change default SDA line for soft_I2C
        I2Csettings.sda = (int)strtod(option, NULL);
      
        if (I2Csettings.sda < 2 || I2Csettings.sda == 4 || 
        I2Csettings.sda > 27 || I2Csettings.sda == I2Csettings.scl)
        {
          p_printf(RED,(char *) "invalid GPIO for SDA :  %d\n",I2Csettings.sda);
          exit(EXIT_FAILURE);
        }   
        break;
    
    default:    /* '?' */
        usage(mm);
        exit(EXIT_FAILURE);
    }
}

/*********************************************************************
 * 
 * @brief program starts here
 * @param argc : count of command line options provided
 * @param argv : command line options provided
 * 
 *********************************************************************/
int main(int argc, char *argv[])
{
    int opt;
    struct measure mm;
    
    /* initialize default setting */
    init_variables(&mm);
    
    /* set signals */
    set_signals();
    
    /* save name for (potential) usage display */
    strncpy(progname,argv[0],20);
    
    /* parse commandline */
    while ((opt = getopt(argc, argv, "A:C:F:H:K:M:P:T:I:L:O:D:s:d:BiV:")) != -1)
    {
        parse_cmdline(opt, optarg, &mm);
    }
    
    /* initialize the hardware */
    init_hardware(&mm);
    
    /* main loop (include command line options)  */
    main_loop(&mm);
    
    closeout(EXIT_SUCCESS);
    
    // STOP -WALL COMPLAINING
    exit(EXIT_SUCCESS); 
}
