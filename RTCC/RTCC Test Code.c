

//Light LED's as we upstroke, unlight as we down stroke
//short LED goes to power, , long side to ground

//This will be the Demo Version
/*SendMessage only when big red button is pushed
 Send: Total Upstroke, Volume, Leak Rate, Adjusted Volume(Volume - LeakRate)
 */

/* For India Mark II pumps
 * Time had to be hard-set due to a GSM problem in Ghana
 * Volume was never calibrated
 */

/*
 * File:   the real thing.c
 * Authors: Tony Beers, John Snyder, Jacqui Young, Avery deGruchy, Earl Swope
 *
 * Created on June 26, 2014
 */

// ****************************************************************************
// *** Include Statemets ******************************************************
// ****************************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <xc.h>
#include <string.h>
//#include <HTC.h> I think it's a complier, but we have xc.h right!?


// ****************************************************************************
// *** PIC24F32KA302 Configuration Bit Settings *******************************
// ****************************************************************************
// FBS
#pragma config BWRP = OFF               // Boot Segment Write Protect (Disabled)
#pragma config BSS = OFF                // Boot segment Protect (No boot program flash segment)

// FGS
#pragma config GWRP = OFF               // General Segment Write Protect (General segment may be written)
#pragma config GSS0 = OFF               // General Segment Code Protect (No Protection)

// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Select (Fast RC Oscillator (FRC))
#pragma config SOSCSRC = ANA            // SOSC Source Type (Analog Mode for use with crystal)
#pragma config LPRCSEL = HP             // LPRC Oscillator Power and Accuracy (High Power, High Accuracy Mode)
#pragma config IESO = OFF               // Internal External Switch Over bit (Internal External Switchover mode enabled (Two-speed Start-up enabled))

// FOSC
#pragma config POSCMOD = NONE           // Primary Oscillator Configuration bits (Primary oscillator disabled)
#pragma config OSCIOFNC = OFF           // CLKO Enable Configuration bit (CLKO output disabled)
#pragma config POSCFREQ = HS            // Primary Oscillator Frequency Range Configuration bits (Primary oscillator/external clock input frequency greater than 8MHz)
#pragma config SOSCSEL = SOSCHP         // SOSC Power Selection Configuration bits (Secondary Oscillator configured for high-power operation)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Both Clock Switching and Fail-safe Clock Monitor are disabled)

// FWDT
#pragma config WDTPS = PS32768          // Watchdog Timer Postscale Select bits (1:32768)
#pragma config FWPSA = PR128            // WDT Prescaler bit (WDT prescaler ratio of 1:128)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable bits (WDT disabled in hardware; SWDTEN bit disabled)
#pragma config WINDIS = ON              // Windowed Watchdog Timer Disable bit (Windowed WDT enabled)

// FPOR
#pragma config BOREN = BOR3             // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware, SBOREN bit disabled)
#pragma config LVRCFG = OFF             // (Low Voltage regulator is not available)
#pragma config PWRTEN = ON              // Power-up Timer Enable bit (PWRT enabled)
#pragma config I2C1SEL = PRI            // Alternate I2C1 Pin Mapping bit (Use Default SCL1/SDA1 Pins For I2C1)
//#pragma config BORV = V20               // Brown-out Reset Voltage bits (Brown-out Reset set to lowest voltage (2.0V))
#pragma config MCLRE = ON               // MCLR Pin Enable bit (RA5 input pin disabled,MCLR pin enabled)

// FICD
#pragma config ICS = PGx1               // ICD Pin Placement Select bits (EMUC/EMUD share PGC1/PGD1)

// FDS
#pragma config DSWDTPS = DSWDTPSF       // Deep Sleep Watchdog Timer Postscale Select bits (1:2,147,483,648 (25.7 Days))
#pragma config DSWDTOSC = LPRC          // DSWDT Reference Clock Select bit (DSWDT uses Low Power RC Oscillator (LPRC))
#pragma config DSBOREN = ON             // Deep Sleep Zero-Power BOR Enable bit (Deep Sleep BOR enabled in Deep Sleep)
#pragma config DSWDTEN = ON             // Deep Sleep Watchdog Timer Enable bit (DSWDT enabled)


//*****************************************************************************

//These variables were changed to be constants so that their values would
//not be changed accidentally - 6/6/14 Avery deGruchy

// ****************************************************************************
// *** Constants **************************************************************
// ****************************************************************************
const int xAxis = 11;                           // analog pin connected to x axis of accelerometer
const int yAxis = 12;                           // analog pin connected to y axis of accelerometer
const float MKII = 0.467;                       // 0.4074 L/Radian; transfer variable for mkII delta handle angle to outflow
const float leakSensorVolume = 0.01781283;      // This is in Liters; pipe dia. = 33mm; rod diam 12 mm; gage length 24mm
const int alarmHour = 0x0000;                   // The weekday and hour (24 hour format) (in BCD) that the alarm will go off
const int alarmStartingMinute = 1;              // The minimum minute that the alarm will go off
const int alarmMinuteMax = 5;                   // The max number of minutes to offset the alarm (the alarmStartingMinute + a random number between 0 and this number)
const int adjustmentFactor = 511;               // Used to ajust the values read from the accelerometer
const int decimalAccuracy = 3;                  // Number of decimal places to use when converting floats to strings
const int pulseWidthThreshold = 0x0800;         // The value to check the pulse width against (2048)
const int networkPulseWidthThreshold = 0x4E20;  // The value to check the pulse width against (about 20000)
const int upstrokeInterval = 10;                // The number of milliseconds to delay before reading the upstroke
int waterPrimeTimeOut = 7000;                   // Equivalent to 7 seconds (in 50 millisecond intervals); 50 = upstrokeInterval
long leakRateTimeOut = 18000;                   // Equivalent to 18 seconds (in 50 millisecond intervals); 50 = upstrokeInterval
long timeBetweenUpstrokes = 3000;               // 3 seconds (based on upstrokeInterval)
const int angleDeltaThreshold = 2;              // The angle delta to check against
const float upstrokeToMeters = 0.01287;
const int minimumAngleDelta = 10;
int k; //variable used in I2C for transfering memory

// ****************************************************************************
// *** Global Variables *******************************************************
// ****************************************************************************
//static char phoneNumber[] = "+233247398396";     // Number for the Black Phone
static char phoneNumber[] = "+17177784498"; //possibly not need the +   // Number for Upside Wireless
float longestPrime = 0;                            // total upstroke fo the longest priming event of the day
float leakRateLong = 0;                            // largest leak rate recorded for the day
float volumeDemo = 0;
float totalVolume = 0;
float totalVolumeDemo = 0;

// Total Volume extracted from 0:00-2:00
//float volume24 = 0;
//float volume46 = 0;
//float volume68 = 0;
//float volume810 = 0;
//float volume1012 = 0;
//float volume1214 = 0;
//float volume1416 = 0;
//float volume1618 = 0;
//float volume1820 = 0;
//float volume2022 = 0;
//float volume2224 = 0;

// Set the time manually - change this before you program!!
static char *timeString = "+++CCLK: \"14/07/25,12:59:00+00\"";

// Removed the timestring to use a manual time instead
//static char timeString[44];                     //stores an array of characters from the output of the AT+CCLK? query
static char secondString[3];
static char minuteString[3];
static char hourString[3];
static char dayString[3];
static char monthString[3];
static char yearString[3];

int queueCount = 0;
int queueLength = 7;                            //don't forget to change angleQueue to this number also
float angleQueue[7];

// ****************************************************************************
// *** Functions **************************************************************
// ****************************************************************************

//Delays for the given number of milliseconds
void delayMs(int ms) //tested 05-20-14
{
    int myIndex;

    while (ms > 0){
        myIndex = 0;
        while (myIndex < 667){
            myIndex++;
        }
        ms--;
    }
}

//Returns the number of digits in the given integer
int longLength(long num) { //Tested 06-03-2014
    int length = 0;

    do {
        length++;   //Increment length
        num /= 10;  //Get rid of the one's place
    }while(num != 0);

    return length;
}

//Sets the given char array to an array filled with the digits of the given long
void longToString(long num, char *numString) { //Tested 06-04-2014
    //Declares an array of digits to refer to
    char const digits[] = "0123456789";

    //Gets the number of digits in the given number
    int length = longLength(num);

    //Creates a char array with the appropriate size (plus 1 for the \0 terminating character)
    char *result = numString;

    // Add 1 to the length to make room for the '-' and inserts the '-' if the number is negative
    if(num < 0) {
        length++;
        result[0] = '-';
        num *= -1;  // Make the number positive
    }

    //Sets i to be the end of the string
    int i = length - 1;

    //Loops through the char array and inserts digits
    do {
        //Set the current index of the array to the corresponding digit
        result[i] = digits[num % 10];

        //Divide num by 10 to lose the one's place
        num /= 10;
        i--;
    }while(num != 0);

    //Insert a terminating \0 character
    result[length] = '\0';
}

//Returns the number of characters (not including \0) in the given string
int stringLength(char *string) { // Tested 06-09-2014
    int i = 0;

    //Checks for the terminating character
    while(string[i] != '\0') {
        i++;
    }

    return i;
}

//Concatenates two strings
void concat(char *dest, const char *src) { // Tested 06-09-2014

    //Increments the pointer to the end of the string
    while (*dest) {
        dest++;
    }

    //Assigns the rest of string two to the incrementing pointer
    while ((*dest++ = *src++) != '\0');
}

//Fills the char array with the digits in the given float
//Fills the char array with the digits in the given float
void floatToString(float myValue, char *myString) { //tested 06-20-2014

    int padLength = 0;               // Stores the number of 0's needed for padding (2 if the fractional part was .003)
    long digit;                      // Stores a digit from the mantissa
    char digitString[5];             // Stores the string version of digit
    char mString[20];                // Stores the mantissa as a string
    char eString[20];                // Stores the exponent as a string
    int decimalShift;                // Keeps track of how many decimal places we counted
    long exponent = (long)myValue;   // Stores the exponent value of myValue
    float mantissa = myValue - (float)exponent;   //Stores the fractional part
    int sLength = 0;   // The length of the final string

    // Make the mantissa and exponent positive if they were negative
    if(myValue < 0) {
        mantissa *= -1;
        exponent *= -1;
    }

    // Counts the padding needed
    while(mantissa < 1 && mantissa != 0) {
        mantissa = mantissa * 10.0;  // Stores the mantissa with the given decimal accuracy
        padLength++;    // Increment the number of 0's needed
    }
    padLength--;     // Subtract one because we don't want to count the last place shift

    mString[0] = '\0';
    eString[0] = '\0';
    digitString[0] = '\0';
    myString[0] = '\0';

    //Gets the string for the exponent
    longToString(exponent, eString);

    // Get the mantissa digits only if needed
    // (if padLength is -1, the number was a whole number. If it is above the decimal accuracy,
    // we had all 0's and do not need a mantissa
    if(padLength > -1 && padLength < decimalAccuracy) {
        // Updates the decimal place
        decimalShift = padLength;

        // Extracts the next one's place from the mantissa until we reached our decimal accuracy
        while(decimalShift < decimalAccuracy) {
            digit = (long)mantissa;     // Get the next digit

            longToString(digit, digitString);   // Convert the digit to a string

            concat(mString, digitString);       // Add the digit string to the mantissa string

            mantissa = mantissa - (float)digit;
            mantissa = mantissa * 10;   // Shift the decimal places to prepare for the next digit

            decimalShift++;     // Update the decimal shift count
        }

        if(myValue < 0) {
            concat(myString, "-");    // Adds the '-' character
            sLength++;  // Add one to the length for the '-' character
        }

        // Concatenates the exponent, decimal point, and mantissa together
        concat(myString, eString);
        concat(myString, ".");

        // Adds 0's to the mantissa string for each 0 needed for padding
        int i;
        for(i = 0; i < padLength; i++) {
            concat(myString, "0");
        }

        concat(myString, mString);

        //The length of the final string (lengths of the parts plus 1 for decimal point, 1 for \0 character, and the number of 0's)
        sLength += stringLength(eString) + stringLength(mString) + 2 + padLength;

        // Removes any trailing 0's
        while(myString[sLength - 2] == '0') {
            myString[sLength - 2] = '\0';
            sLength--;
        }
    } else {
        if(myValue < 0) {
            concat(myString, "-");    // Adds the '-' character
            sLength++;  // Add one to the length for the '-' character
        }

        // Concatenates the exponent, decimal point, and mantissa together
        concat(myString, eString);

        //The length of the final string (lengths of the parts plus 1 for \0 character)
        sLength += stringLength(eString) + 1;
    }

    myString[sLength - 1] = '\0';   // Add terminating character
}

void turnOffSIM() {
    PORTBbits.RB8 = 0;
    // Turn off SIM900
    while (PORTAbits.RA7 == 1) {  // While STATUS light is on (SIM900 is on)
        PORTBbits.RB8 = 1;        // Hold in PWRKEY button
    }
    PORTBbits.RB8 = 0;            // Let go of PWRKEY
}

void turnOnSIM() {
    PORTBbits.RB8 = 0;
    // Turn on SIM900
    while (PORTAbits.RA7 == 0) {  // While STATUS light is not on (SIM900 is off)
        PORTBbits.RB8 = 1;        // Hold in PWRKEY button
    }
    PORTBbits.RB8 = 0;            // Let go of PWRKEY
}

// This function test for network status and attemps to connect to the
// network. If no netork is found in a minute, the SIM is reset in order
// to connect agian.
void tryToConnectToNetwork() {

    int networkTimeoutCount = 0;        // Stores the number of times we reset the SIM
    int networkTimeout = 0;             // Stores the number of times we did not have connection
    int networkConnectionCount = 0;     // Stores the number of times we have detected a connection
    int keepTrying = 1;                 // A flag used to keep trying to connect to the network

    while(keepTrying) { // Wait until connected to the network, or we tried for 20 seconds

        delayMs(1000);  // Delay for 1 second

        // Check for network take the appropriate action
        if(connectedToNetwork()) {
            networkConnectionCount++;

            // 4 consecutive connections means we can exit the loop
            if(networkConnectionCount == 4) {
                keepTrying = 0;
            }

        } else {
            // If we have no network, reset the counter
            networkConnectionCount = 0;

            // Increase the network timeout
            networkTimeout++;

            // We tried to connect for 1 minute, so restart the SIM900
            if(networkTimeout > 60) {

                turnOffSIM();
                delayMs(3000);
                turnOnSIM();
                delayMs(3000);

                // Reset the network timeout
                networkTimeout = 0;
                networkTimeoutCount++;

                // If we have tried to reset 5 times, we give up and exit
                if(networkTimeoutCount == 5) {
                    keepTrying = 0;
                }
            }
        }
    }
}

int connectedToNetwork(void) { //True when there is a network connection

    // Make sure you start at the beginning of the positive pulse
    if (PORTBbits.RB14 == 1) {
        while (PORTBbits.RB14) { };
    }

    // Wait for rising edge
    while (PORTBbits.RB14 == 0) { };

    // Reset the timer
    TMR1 = 0;

    // Get time at start of positive pulse
    int prevICTime = TMR1;

    // Wait for the pulse to go low
    while (PORTBbits.RB14) { };

    // Wait for the pulse to go high again
    while (PORTBbits.RB14 == 0) { };

    // Get time at end of second positive pulse
    int currentICTime = TMR1;

    long pulseDistance = 0;
    if (currentICTime >= prevICTime) {
        pulseDistance = (currentICTime - prevICTime);
    } else {
        pulseDistance = (currentICTime - prevICTime + 0x10000);
    }

    //Check if this value is right
    return (pulseDistance >= networkPulseWidthThreshold); // True, when there is a network connection.

}

int readWaterSensor(void) { // RB5 is one water sensor

    if (PORTBbits.RB5 == 1) {
        while (PORTBbits.RB5) { }; //make sure you start at the beginning of the positive pulse
    }

    while (PORTBbits.RB5 == 0) { }; //wait for rising edge

    int prevICTime = TMR1; //get time at start of positive pulse

    while (PORTBbits.RB5) { };

    int currentICTime = TMR1; //get time at end of positive pulse

    long pulseWidth = 0;
    if (currentICTime >= prevICTime) {
        pulseWidth = (currentICTime - prevICTime);
    } else {
        pulseWidth = (currentICTime - prevICTime + 0x100000000);
    }

    //Check if this value is right
    return (pulseWidth <= pulseWidthThreshold);
}

//Returns the decimal value for the lower 8 bits in a 16 bit BCD (Binary Coded Decimal)
int getLowerBCDAsDecimal(int bcd) { //Tested 06-04-2014

    //Get the tens digit (located in the second nibble from the right)
    //by shifting off the ones digit and anding
    int tens = (bcd >> 4) & 0b0000000000001111;

    //Get the ones digit (located in the first nibble from the right)
    //by anding (no bit shifting)
    int ones = bcd & 0b0000000000001111;

    //Returns the decimal value by multiplying the tens digit by ten
    //and adding the ones digit
    return (tens * 10) + ones;
}

//Returns the decimal value for the upper 8 bits in a 16 bit BCD (Binary Coded Decimal)
int getUpperBCDAsDecimal(int bcd) { //Tested 06-04-2014

    //Get the tens digit (located in the first nibble from the left)
    //by shifting off the rest and anding
    int tens = (bcd >> 12) & 0b0000000000001111;

    //Get the ones digit (located in the second nibble from the left)
    //by shifting off the rest and anding
    int ones = (bcd >> 8) & 0b0000000000001111;

    //Returns the decimal value by multiplying the tens digit by ten
    //and adding the ones digit
    return (tens * 10) + ones;
}

//Returns the hour of day from the internal clock
                        //Tested 06-04-2014
int getTimeHour(void) { //to determine what volume variable to use;

    //don't want to write, just want to read
    _RTCWREN = 0;
    //sets the pointer to 0b01 so that reading starts at Weekday/Hour
    _RTCPTR = 0b01;

    // Ask for the hour from the internal clock
    int myHour = RTCVAL;

    int hourDecimal = getLowerBCDAsDecimal(myHour);

    return hourDecimal;
}

//Transmits the given characters along serial lines
void sendMessage (char message[160]) //Tested 06-02-2014
{
    int stringIndex = 0;
    int delayIndex;

    U1BRG=25;               //set baud to 9600, assumes FCY=4Mhz/19200
    U1STA=0;
    U1MODE=0x8000;          //enable UART for 8 bit data
    //no parity, 1 stop bit
    U1STAbits.UTXEN=1;      //enable transmit

    while (stringIndex < stringLength(message)){
        if(U1STAbits.UTXBF == 0){
            U1TXREG= message[stringIndex];
            stringIndex++;
            for (delayIndex = 0; delayIndex < 1000; delayIndex++){ }
        }
        else {
            for (delayIndex = 0; delayIndex < 30000; delayIndex++){ }
        }
    }
}
/* First, retrieve time string from the SIM 900
   Then, parse the string into separate strings for each time partition
   Next, translate each time partition, by digit, into a binary string
   Finally, piece together strings (16bytes) and write them to the RTCC */
                        // Tested 06-02-2014
void getTime(void) {    // This asks the SIM what time it is

    int i = 0;
    sendMessage("ATE0\r\n");    // Turn off echo; same as quectel SIM
    delayMs(250);
    U1STAbits.OERR = 0;
    sendMessage("AT+CCLK?\r\n"); //quectel same as sim900

    while ( i < 32 ) {
        if (U1STAbits.URXDA == 1) {     // Receive buffer has at least one number
            // that can be read
            if (U1STAbits.OERR == 1) {  // If receive buffer has overflown,
                U1STAbits.OERR = 0;     // Reset the receive buffer, emptying state.
            }
            timeString[i] = U1RXREG;    // Put character into array.
            i++;
        }
        else { /* do nothing */ }
    }
}

//Return current time in seconds (the seconds passed so far in the day)
long timeStamp(void) { // Tested 06-04-2014

    long timeStampValue = 0;

    //Set the pointer to 0b01 so that reading starts at Weekday/Hour
    _RTCPTR = 0b01; // decrements with read or write
    _RTCWREN = 0;   //don't want to write, just want to read

    long binaryWeekdayHour = RTCVAL;  // write wkdy & hour to variable, dec. RTCPTR
    long binaryMinuteSecond = RTCVAL; // write min & sec to variable, dec. RTCPTR

    //For some reason, putting the multiplication for hours on one line like this:
    //
    //  timeStampValue = getLowerBCDAsDecimal(binaryWeekdayHour) * 60 * 60;
    //
    //caused an error. We would get some unknown value for the timestamp, so
    //we had to break the code up across multiple lines. So don't try to
    //simplify this!
    timeStampValue = getLowerBCDAsDecimal(binaryWeekdayHour);
    timeStampValue = timeStampValue * 60 * 60;
    timeStampValue = timeStampValue + (getUpperBCDAsDecimal(binaryMinuteSecond) * 60);
    timeStampValue = timeStampValue + getLowerBCDAsDecimal(binaryMinuteSecond);

    return timeStampValue;
}

void sendTextMessage(char message[160]) { // Tested 06-02-2014


    sendMessage("AT+CMGF=1\r\n");//sets to text mode; This is same for quectel and sim900
    delayMs(250);
    sendMessage("AT+CMGS=\""); //beginning of allowing us to send SMS message
    sendMessage(phoneNumber);
    sendMessage("\"\r\n");  //middle of allowing us to send SMS message
    delayMs(250);
    sendMessage(message);

    delayMs(250);
    sendMessage("\x1A");    // method 2: sending hexidecimal representation; control z
    // of 26 to sendMessage function (line 62)
    // & the end of allowing us to send SMS message

    delayMs(5000);  // Give it some time to send the message
}

//Resets the clock and values
void pressReset() { //Tested 06-17-2014

    //Variable reset (all the variable of the message)
    longestPrime = 0;
    leakRateLong = 0;
    volumeDemo = 0;
//    volume24 = 0;
//    volume46 = 0;
//    volume68 = 0;
//    volume810 = 0;
//    volume1012 = 0;
//    volume1214 = 0;
//    volume1416 = 0;
//    volume1618 = 0;
//    volume1820 = 0;
//    volume2022 = 0;
//    volume2224 = 0;

    //getTime(); //Reset RTCC
}

/*void __attribute__((__interrupt__,__auto_psv__)) _DefaultInterrupt(){ //Tested 06-05-2014

    if (IFS3bits.RTCIF) { //If Alarm "is ringing", do this ....

        /////////////////////////////////////////////
        // Should we wake the SIM up here?
        /////////////////////////////////////////////
*/
        /* message type,
         * version # (no version number, we should just be able to check for new values in JSON),
         * date? (no date, the sms should have that within it),
         * sequence #; incase we didn't get a message (sent and recieved last 30, 60 total),
         * message data (all the variables):
         *      leakageCoefficient (leakRateLongString)
         *      longestPrime
         *      volume02String
         *      volume24String
         *      volume46String
         *      volume68String
         *      volume810String
         *      volume1012String
         *      volume1214String
         *      volume1416String
         *      volume1618String
         *      volume1820String
         *      volume2022String
         *      volume2224String
         *      version #?
         *      (won't include max/ min level in this version)
         *      error1? (forseeable errors in the future)
         *      error2? (forseeable errors in the future)
         *      (won't use battery percentage till january)
         *      check sum (sum of all the values)
         *
         *
         * {"t":"d","d":[{"l":123.123,"p":123.123,"v":[123.123,123.123,123.123,123.123,123.123,123.123,123.123,123.123,123.123,123.123,123.123,123.123]}]}
         *
         */

 /*       //Message assembly and sending; Use *floatToString() to send:
        char longestPrimeString[20];
        longestPrimeString[0] = 0;

        char leakRateLongString[20];
        leakRateLongString[0] = 0;

        char volumeDemoString[20];
        volumeDemoString[0] = 0;

        char totalVolumeDemoString[20];
        totalVolumeDemoString[0] = 0;


        floatToString(leakRateLong, leakRateLongString);
        floatToString(longestPrime, longestPrimeString);
        floatToString(volumeDemo, volumeDemoString);
        floatToString(totalVolumeDemo,totalVolumeDemoString);

        long checkSum = longestPrime + leakRateLong  + volumeDemo;

        char stringCheckSum[20];
        floatToString(checkSum, stringCheckSum);

        //will need more formating for JSON 5-30-2014
        char dataMessage[160];
        dataMessage[0] = 0;

        concat(dataMessage, "(\"t\":\"d\",\"d\":(\"l\":");
        concat(dataMessage, leakRateLongString);
        concat(dataMessage, ",\"p\":");
        concat(dataMessage, longestPrimeString);
        concat(dataMessage, ",\"v\":<");
        concat(dataMessage, volumeDemoString); //This takes leakRate into account
        concat(dataMessage, ",\"tv\":");
        concat(dataMessage, totalVolumeDemoString); //this does not take leak rate into account
        concat(dataMessage, ">))");

        // Try to establish network connection
        tryToConnectToNetwork();

        delayMs(2000);

        // Send off the data
        sendTextMessage(dataMessage);

        pressReset();

        IFS3bits.RTCIF = 0; //turns off the alarm interrupt flag

        ////////////////////////////////////////////////
        // Should we put the SIM back to sleep here?
        ////////////////////////////////////////////////

    } else {
        // Other interrupts sent here
    }
}*/

// The following integers are used for turning the corresponding time-value strings
// into binary numbers that are used to program the RTCC registers
int translate(char digit) { // Tested 06-02-2014
    int binaryNumber;

    if (digit == '0') {
        binaryNumber = 0b0000;
    }
    else if (digit == '1') {
        binaryNumber = 0b0001;
    }
    else if (digit == '2') {
        binaryNumber = 0b0010;
    }
    else if (digit == '3') {
        binaryNumber = 0b0011;
    }
    else if (digit == '4') {
        binaryNumber = 0b0100;
    }
    else if (digit == '5') {
        binaryNumber = 0b0101;
    }
    else if (digit == '6') {
        binaryNumber = 0b0110;
    }
    else if (digit == '7') {
        binaryNumber = 0b0111;
    }
    else if (digit == '8') {
        binaryNumber = 0b1000;
    }
    else if (digit == '9') {
        binaryNumber = 0b1001;
    }

    return binaryNumber;
}

void getYear(void) { // Tested 06-02-2014
    yearString[0] = timeString[10];
    yearString[1] = timeString[11];
    yearString[2] = 0; // null (not \0) signals message to stop
}

void getMonth(void) { // Tested 06-02-2014
    monthString[0] = timeString[13];
    monthString[1] = timeString[14];
    monthString[2] = 0;
}

void getDay(void) { // Tested 06-02-2014
    dayString[0] = timeString[16];
    dayString[1] = timeString[17];
    dayString[2] = 0;
}

void getHour(void) { // Tested 06-02-2014
    hourString[0] = timeString[19];
    hourString[1] = timeString[20];
    hourString[2] = 0;
}

void getMinute(void) { // Tested 06-02-2014
    minuteString[0] = timeString[22];
    minuteString[1] = timeString[23];
    minuteString[2] = 0;
}

void getSecond(void) { // Tested 06-02-2014
    secondString[0] = timeString[25];
    secondString[1] = timeString[26];
    secondString[2] = 0;
}

void RTCCSet(void) { // Tested 06-02-2014
    // Get time string from SIM900
    //getTime();

    // Parse time string into smaller, partitioned strings
    getYear();
    getMonth();
    getDay();
    getHour();
    getMinute();
    getSecond();

    // Translate each time partition by digit, into binary numbers
    int binaryYearTens = translate(yearString[0]);
    int binaryYearOnes = translate(yearString[1]);

    int binaryMonthTens = translate(monthString[0]);
    int binaryMonthOnes = translate(monthString[1]);

    int binaryDayTens = translate(dayString[0]);
    int binaryDayOnes = translate(dayString[1]);

    // weekday bits will not be used
    int binaryHourTens = translate(hourString[0]);
    int binaryHourOnes = translate(hourString[1]);

    int binaryMinuteTens = translate(minuteString[0]);
    int binaryMinuteOnes = translate(minuteString[1]);

    int binarySecondTens = translate(secondString[0]);
    int binarySecondOnes = translate(secondString[1]);

    // Write the time to the RTCC
    // The enclosed code was graciously donated by the KWHr project

    __builtin_write_RTCWEN();//does unlock sequence to enable write to RTCC, sets RTCWEN

    RCFGCAL = 0b0010001100000000;
    RTCPWC  = 0b0000010100000000;
    _RTCPTR = 0b11;// decrements with read or write
    // Thanks KWHr!!!

    // Assemble the binary date integers
    int binaryYear = (binaryYearTens << 4) + binaryYearOnes;
    int binaryMonth = (binaryMonthTens << 4) + binaryMonthOnes;
    int binaryDay = (binaryDayTens << 4) + binaryDayOnes;

    int binaryHour = (binaryHourTens << 4) + binaryHourOnes;
    int binaryMinute = (binaryMinuteTens << 4) + binaryMinuteOnes;
    int binarySecond = (binarySecondTens << 4) + binarySecondOnes;

    int binaryBlank = 0x00;     // unimplemented bits in the year
    int binaryWeekday = 0x00;   // weekday is not implemented in this version

    // Combining the dates into the register values see line 94
    long binaryBlankYear = (binaryBlank << 8) + binaryYear;
    long binaryMonthDay = (binaryMonth << 8) + binaryDay;
    long binaryWeekdayHour = (binaryWeekday << 8) + binaryHour;
    long binaryMinuteSecond = (binaryMinute << 8) + binarySecond;

    // Program actual register bits, in theory. Not sure if 'YRTEN' (for example) is proper syntax
    // for programming the register
    RTCVAL = binaryBlankYear;
    RTCVAL = binaryMonthDay;
    RTCVAL = binaryWeekdayHour;
    RTCVAL = binaryMinuteSecond;

    _RTCEN = 1; //RTCC module is enabled
    _RTCWREN = 0; // disable writing
}

void initAdc(void) //test with accelermometer (assigned 5-29-14)
{
    // 10bit conversion
    AD1CON1 = 0; // Default to all 0s
    AD1CON1bits.ADON = 0; // Ensure the ADC is turned off before configuration
    AD1CON1bits.FORM = 0; // absolute decimal result, unsigned, right-justified
    AD1CON1bits.SSRC = 0; // The SAMP bit must be cleared by software
    AD1CON1bits.SSRC = 0x7; // The SAMP bit is cleared after SAMC number (see
    // AD3CON) of TAD clocks after SAMP bit being set
    AD1CON1bits.ASAM = 0; // Sampling begins when the SAMP bit is manually set
    AD1CON1bits.SAMP = 0; // Don't Sample yet

    // Leave AD1CON2 at defaults
    //    Vref High = Vcc   Vref Low = Vss
    //    Use AD1CHS (see below) to select which channel to convert, don't
    //        scan based upon AD1CSSL
    AD1CON2 = 0;

    // AD3CON
    // This device needs a minimum of Tad = 600ns.
    // If Tcy is actually 1/8Mhz = 125ns, so we are using 3Tcy
    //AD1CON3 = 0x1F02;        // Sample time = 31 Tad, Tad = 3Tcy
    AD1CON3bits.SAMC = 0x1F; // Sample time = 31 Tad (11.6us charge time)
    AD1CON3bits.ADCS = 0x2; // Tad = 3Tcy

    // Conversions are routed through MuxA by default in AD1CON2
    AD1CHSbits.CH0NA = 0; // Use Vss as the conversion reference
    AD1CSSL = 0; // No inputs specified since we are not in SCAN mode
    // AD1CON2

}

//Returns the minutes and seconds (in BCD) to set the alarm to.
//Generates a random number of seconds between 1 and the alarmMinuteMax
//global variable to use for the minutes and seconds.
int getMinuteOffset() { // Tested 06-13-2014

    //Get the number of seconds possible in alarmMinuteMax minuites plus 10 seconds
    //Plus 10 seconds is so that we aren't calling the alarm right at midnight
    int minutesInSeconds = (alarmMinuteMax * 60) + 10;

    //Sets the seed randomly based on the time of day
    long time = timeStamp();     // Gets the time of day in seconds (long)
    int timeConverted = (int) (time / 3);    // Convert the time into an int to be supported by srand()
    srand(timeConverted);

    //Get a random time (in seconds)
    int randomTime = rand() % minutesInSeconds;

    //Get the minute part (plus the starting offset minute)
    int minutes = (randomTime / 60) + alarmStartingMinute;

    //Get the remaining seconds after minutes are taken out
    int seconds = randomTime % 60;

    //Get the tens and ones place for the minute
    int minuteTens = minutes / 10;
    int minuteOnes = minutes % 10;

    //Get the tens and ones place for the second
    int secondsTens = seconds / 10;
    int secondsOnes = seconds % 10;

    // Five minutes and one second (for an example reference)
    // 0x0501
    // 0b0000 0101 0000 0001

    //Get the time in BCD by shifting the minutes tens place
    int timeInBCD = minuteTens << 12;
    //Add the shifted minutes ones place
    timeInBCD = timeInBCD + (minuteOnes << 8);
    //Add the shifted seconds tens place
    timeInBCD = timeInBCD + (secondsTens << 4);
    //Add the seconds ones place
    timeInBCD = timeInBCD + secondsOnes;

    return timeInBCD;
}

// RA7 is connected to STATUS on SIM900 (input)
// RB2 is used as U1RX (input)
// RB5 is one water presence sensor (input)
// RB6 is another water presence sensor (input)
// RB7 is used as U1TX (input)
// RB8 connects to the PWRKEY of the SIM900 (output)
// RB14 connects to NETLIGHT of the SIM900 (input)
// RB12 used as AN12 (input)
// RB13 used as AN11 (input)
// Tested 06-03-2014

void initPicI2CPort (void)
{
    I2C1CONbits.I2CEN = 1; //Enable I2C port
    I2C1CONbits.A10M = 0; //Use 7-bit slave addresses
}
void setRTCCTime (unsigned char hours, unsigned char minutes, unsigned char seconds) //can use port B or A
{
    unsigned char hrten = (hours / 10);
    unsigned char hrone = (hours % 10);
    unsigned char minten = (minutes / 10);
    unsigned char minone = (minutes % 10);
    unsigned char secten = (seconds / 10);
    unsigned char secone = (seconds % 10);

    unsigned char stopORtcSec = secone + (secten << 4); // The seconds register
                                                        // with the oscillator
                                                        // disabled
    unsigned char startORtcSec = stopORtcSec + 0x80;    // The seconds register
                                                        // with the oscillator
                                                        // enabled
    unsigned char rtcMin = minone + (minten << 4);      // The minutes register
    unsigned char rtcHour = hrone + (hrten << 4) + 0x40;// The hours register with
                                                        // the time set to 24hr time

    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011110; //send "write" address for RTCC; This is the receive buffer
    while(I2C1STATbits.TRSTAT){} //wait till address is sent
    I2C1RCV = 0x00; //send sub-address
    while(I2C1STATbits.TRSTAT){} //wait till sub-address is sent
    I2C1RCV = stopORtcSec; //Stop Oscillator and set seconds
    while(I2C1STATbits.TRSTAT){} //wait till data is sent
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished

    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011110; //send "write" address for RTCC; This is the receive buffer
    while(I2C1STATbits.TRSTAT){} //wait till address is sent
    I2C1RCV = 0x01; //send sub-address for RTCMIN
    while(I2C1STATbits.TRSTAT){} //wait till sub-address is sent
    I2C1RCV = rtcMin; //set minutes
    while(I2C1STATbits.TRSTAT){} //wait till data is sent
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished

    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011110; //send "write" address for RTCC; This is the receive buffer
    while(I2C1STATbits.TRSTAT){} //wait till address is sent
    I2C1RCV = 0x02; //send sub-address for RTCHOUR
    while(I2C1STATbits.TRSTAT){} //wait till sub-address is sent
    I2C1RCV = rtcHour; //Set hours and 24 hour time
    while(I2C1STATbits.TRSTAT){} //wait till data is sent
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished

    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011110; //send "write" address for RTCC; This is the receive buffer
    while(I2C1STATbits.TRSTAT){} //wait till address is sent
    I2C1RCV = 0x03; //send sub-address for RTCWKDAY
    while(I2C1STATbits.TRSTAT){} //wait till sub-address is sent
    I2C1RCV = 0b00001111; // Set VBATEN and set the day of the week to 7 and
                          // clear the PWRFAIL bit
    while(I2C1STATbits.TRSTAT){} //wait till data is sent
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished

    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011110; //send "write" address for RTCC; This is the receive buffer
    while(I2C1STATbits.TRSTAT){} //wait till address is sent
    I2C1RCV = 0x00; //send sub-address for RTCSEC
    while(I2C1STATbits.TRSTAT){} //wait till sub-address is sent
    I2C1RCV = startORtcSec; //Start Oscillator and set seconds
    while(I2C1STATbits.TRSTAT){} //wait till data is sent
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished
}
/*When we're writing stuff:
 To write(0) to PORT A use GPIOA register (0x09)
 To read(1) from PORT B use GPIOB register (0x19)*/

unsigned char readRTCCTime (unsigned char address)
{
    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011110; //send "write" address for RTCC
    while(I2C1STATbits.TBF){} //wait till address is sent
    I2C1RCV = address;
    while(I2C1STATbits.TBF){}
    I2C1CONbits.RSEN;//need to restart
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b11011111; //send "read" address for RTCC
    while(I2C1STATbits.TBF){}
    while(I2C1STATbits.ACKSTAT){}
    I2C1CONbits.RCEN; //initiates recieve 1 byte, if 1 enables the master to recieve
    while(I2C1STATbits.TBF == 0){}
    unsigned char k = I2C1RCV;
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished

    return k;
}



void writePortAI2C (int c)
{
    I2C1CONbits.SEN = 1; //Send Start Bit
    while(I2C1CONbits.SEN){} //wait till Start Bit is finished
    I2C1RCV = 0b01000000; //send "write" address for port expander
    while(I2C1STATbits.TBF){} //wait till address is sent
    I2C1RCV = 0x09; //send sub-address for PortA
    while(I2C1STATbits.TBF){} //wait till sub-address is sent
    I2C1RCV = c; //write all c to PortA
    while(I2C1STATbits.TBF){} //wait till data is sent
    I2C1CONbits.PEN = 1; //send Stop Bit
    while(I2C1CONbits.PEN){} //wait till Stop Bit is finished
}

void initialization(void) { //configures chip to work in our system (when power is turned on, these are set once)
    //finished
    ANSA = 0; // Make PORTA digital I/O
    TRISA = 0xFFFF; // Make PORTA all inputs

    ANSB = 0; // All port B pins are digital.  Individual ADC are set in the readADC function
    TRISB = 0xFFFF; // Sets all of port B to input
    TRISBbits.TRISB8 = 0;   // RB8 is an output
    T1CONbits.TCS = 0;      // Source is Internal Clock (8MHz)
    T1CONbits.TCKPS = 0b11; // Prescalar to 1:256
    T1CONbits.TON = 1;      // Enable the timer (timer 1 is used for the water sensor)
    U1BRG = 51;             // Set baud to 9600, FCY = 8MHz (#pragma config FNOSC = FRC)
    U1STA = 0;
    U1MODE = 0x8000;          //enable UART for 8 bit data
    //no parity, 1 stop bit
    U1STAbits.UTXEN=1;      //enable transmit

    initAdc(); //Call the initialize ADC function
    PORTBbits.RB8 = 0;        // Let go of PWRKEY
    delayMs(3000);

    // Turn on SIM900
    while (PORTAbits.RA7 == 0) {  // While STATUS light is not on (SIM900 is off)
        PORTBbits.RB8 = 1;        // Hold in PWRKEY button
    }

    PORTBbits.RB8 = 0;            // Let go of PWRKEY

    delayMs(3000);

    RTCCSet();        // Sets time; Pic asks Sim which asks cell tower to get current time

    sendMessage("AT+QBAND=\"GSM850_EGSM_DCS_MODE\"\r\n"); //for Quectel

    //sendMessage("AT+CBAND=\"PGSM_MODE\"\r\n"); //for sim900

    tryToConnectToNetwork();

    delayMs(2000);

    // Moved the RRTCCSet function up since we do not rely on network anymore
    //RTCCSet();        // Sets time; Pic asks Sim which asks cell tower to get current time

    _RTCWREN = 1; //allowing us to write to registers; Set Alarm for sending message
    ALCFGRPTbits.CHIME = 1; //don't need to reset alarm?
    ALCFGRPTbits.AMASK = 0b0110; //once a day
    ALCFGRPTbits.ALRMPTR = 0b0010; //sets pointer

    //The following two lines may not work
    ALRMVAL = 0x0000;              //set day and month to 0 and decrements pointer
    ALRMVAL = alarmHour;           //sets hour to 0 (12am), sets weekday to 0, and decrements pointer
    ALRMVAL = getMinuteOffset();   //set 5 min after midnight and set 1 second after midnight-

    //*********************************************
    // Make random number between 12:01-12:06
    // Assigned 05-30-2014; completed 06-09-2014
    //*********************************************

    ALCFGRPTbits.ALRMEN = 1; //enables the alarm
    _RTCWREN = 0; //no longer able to write to registers
    IEC3bits.RTCIE = 1;  //RTCC Interupt is enabled

    sendTextMessage("(\"t\":\"initialize\")");


    // Seconds current seconds
//    long timeInSeconds = timeStamp();
//    char r[20];
//    r[0] = 0;
//    longToString(timeInSeconds, r);
//    sendTextMessage(r);

    initPicI2CPort();
}

// 4-2-2014: Change the designators AN11 = X; AN12 = Y
// 4-4-2014: The task from 4-2-2014 (above) has been completed
int readAdc(int channel){ //check with accelerometer
    switch (channel)
    {
        case 12:
            ANSBbits.ANSB12 = 1; // AN12 is analog
            TRISBbits.TRISB12 = 1; // AN12 is an input
            AD1CHSbits.CH0SA = 12; // Connect AN12 as the S/H input
            break;

        case 11:
            ANSBbits.ANSB13 = 1; // AN11 is analog
            TRISBbits.TRISB13 = 1; // AN11 is an input
            AD1CHSbits.CH0SA = 11; //Connect AN11 as the S/H input
            break;

        case 4:
            ANSBbits.ANSB2 = 1; // AN4 is analog
            TRISBbits.TRISB2 = 1; // AN4 is an input
            AD1CHSbits.CH0SA = 4; // Connect AN4 as the S/H input
            break;

        case 2:
            ANSBbits.ANSB0 = 1; // AN2 is analog
            TRISBbits.TRISB0 = 1; // AN2 is an input
            AD1CHSbits.CH0SA = 2; // Connect AN2 as the S/H input
            break;
    }

    AD1CON1bits.ADON = 1; // Turn on ADC

    AD1CON1bits.SAMP = 1;
    while (!AD1CON1bits.DONE) {

    }

    unsigned int adcValue = ADC1BUF0;
    return adcValue;
}

void parseJSON(char *jsonString) {  //for recieving JSON messages; NOT complete as of 09-14-2014
    // { "t":"data","v":"1.0" }
    int endOfString = stringLength(jsonString); //NOT USED AS OF 06-17-2014
//
//    for(int i = 0; i < endOfString; i++) {
//        if(jsonString[i] == "") {
//
//        }
//    }
}

// Returns the current angle of the pump
//
// The accelerometer should be oriented on the pump handle so that when the pump
// handle (the side the user is using) is down (water present), the angle is positive. When the pump
// handle (the side the user is using) is up (no water present), the angle is negative.
float getHandleAngle() {
    // Gets a snapshot of the current sensor values
    signed int xValue = readAdc(xAxis) - adjustmentFactor; //added abs() 06-20-2014
    signed int yValue = readAdc(yAxis) - adjustmentFactor; //added abs() 06-20-2014
    float angle = atan2(yValue, xValue) * (180 / 3.141592); //returns angle in degrees 06-20-2014
    // Calculate and return the angle of the pump handle
    return angle;
}

float degToRad(float degrees) {
    return degrees * (3.141592 / 180);
}

void initializeQueue(float value) {
    int i = 0;

    // Set all values in the queue to the intial value
    for(i = 0; i < queueLength; i++) {
        angleQueue[i] = value;
    }
}

void pushToQueue(float value) {

    // Shift values down one
    int i = 0;
    for(i = 0; i < queueLength - 1; i++) {
        angleQueue[i] = angleQueue[i + i];
    }

    // Insert the value at the end of the queue
    angleQueue[queueLength - 1] = value;
}

float queueAverage() {
    float sum = 0;
    int i = 0;

    // Sum up all the values in the queue
    for(i = 0; i < queueLength; i++) {
        sum += angleQueue[i];
    }

    // Returns the average after converting queueLength to a float
    return sum / (queueLength * 1.0);
}

// Returns the difference between the last and first numbers in the queue
float queueDifference() {
    return angleQueue[queueLength - 1] - angleQueue[0];
}

void demoDataTxt(){ //sends the data via txt message
    char longestPrimeString[20];
        longestPrimeString[0] = 0;

        char leakRateLongString[20];
        leakRateLongString[0] = 0;

        char volumeDemoString[20];
        volumeDemoString[0] = 0;

        char totalVolumeDemoString[20];
        totalVolumeDemoString[0] = 0;


        floatToString(leakRateLong, leakRateLongString);
        floatToString(longestPrime, longestPrimeString);
        floatToString(volumeDemo, volumeDemoString);
        floatToString(totalVolumeDemo,totalVolumeDemoString);

        long checkSum = longestPrime + leakRateLong  + volumeDemo;

        char stringCheckSum[20];
        floatToString(checkSum, stringCheckSum);

        char dataMessage[160];
        dataMessage[0] = 0;

        //Do we want a different message for the Demo?
        concat(dataMessage, "(\"t\":\"d\",\"d\":(\"l\":");
        concat(dataMessage, leakRateLongString);
        concat(dataMessage, ",\"p\":");
        concat(dataMessage, longestPrimeString);
        concat(dataMessage, ",\"v\":<");
        concat(dataMessage, volumeDemoString); //This takes leakRate into account
        concat(dataMessage, ",\"tv\":");
        concat(dataMessage, totalVolumeDemoString); //this does not take leak rate into account
        concat(dataMessage, ">))");

        // Send off the data
        sendTextMessage(dataMessage);

        pressReset();
}

//int demoButton(){  //should send message if button is pushed. Also says if button is pushed or not
//
//    int status = 0;
//
//    //should we have a delay waiting for the button to be pushed
//    if (PORTBbits.RB3 == 1) {
//
//        status = 1;
//        demoDataTxt(); //Double check: don't want it to send 20messages, make sure it just sends one
//    }
//
//    return status;
//}

void sendIf30Min(void){
    if ((timeStamp() %1800 == 0)){
        sendTextMessage("Insert Battery Life here");
        }
}

// ****************************************************************************
// *** Main Function **********************************************************
// ****************************************************************************
void main (void) {

    initialization();

    unsigned char secAddress = 0x00;
    unsigned char minAddress = 0x01;
    unsigned char hourAddress = 0x02;

    unsigned char currentSec = 0;
    unsigned char currentMin = 30;
    unsigned char currentHour = 15;

    setRTCCTime(currentHour, currentMin, currentSec);

    unsigned char secReg;
    unsigned char minReg;
    unsigned char hourReg;

    while(1) {
        secReg = readRTCCTime(secAddress);
        minReg = readRTCCTime(minAddress);
        hourReg = readRTCCTime(hourAddress);

        unsigned char secLow = secReg & 0b00001111;
        unsigned char secHigh = (secReg & 0b01110000) >> 4;

        unsigned char minLow = minReg & 0b00001111;
        unsigned char minHigh = (minReg & 0b01110000) >> 4;

        unsigned char hourLow = hourReg & 0b00001111;
        unsigned char hourHigh = (hourReg & 0b00110000) >> 4;

        char message[11];
        message[0] = 254;
        message[1] = 128;
        message[3] = hourHigh + 48;
        message[4] = hourLow + 48;
        message[5] = ':';
        message[6] = minHigh + 48;
        message[7] = minLow + 48;
        message[8] = ':';
        message[9] = secHigh + 48;
        message[10] = secLow + 48;

        sendMessage(message);

    }
}

