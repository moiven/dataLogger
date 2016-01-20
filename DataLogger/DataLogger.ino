/**
  *****************************GPS data logger by Marvin Ibarra***************************
  *
  * simple project where I use the Venus GPS with SMA connector and Openlog from sparkfun.
  * The gps module outputs a stream of data or NEMA code and the arduino parses this data
  * into an easier to read string of data. This data is then sent to the Sparkfun Openlog 
  * where it gets stored into a .TXT file.
  * 
  * The data is saved to the logger by modifying LOG_INTERVAL (in seconds). Data is not 
  * just saved by a schedule but also events can be saved. A rotary encoder is used to 
  * cycle through the pre-defined events and a button saves the event choosen. The event  
  * will be saved along with the string of data.
  * 
  * A 64x48 pixel OLED from Adafruit is used to display the latitude, longitude, date, time,
  * the remaining time before next scheduled save (in seconds), and toggled event.
  * 
  * The arduino used is the 3.3V pro mini. This is all powered by Sparkfun Power Cell
  * and a 6000mAh polymer lithium ion battery. Estimated 51 hours to a single charge.
**/

#include <SPI.h>
#include <Wire.h>
#include <SFE_MicroOLED.h>

/////////Button defines/////////
#define BUTTON          5

//////////microOLED defines//////////
#define PIN_CS          10
#define PIN_RESET       9
#define PIN_DC          8
#define FIRST_LINE      0
#define SECOND_LINE     8
#define THIRD_LINE      16
#define FOURTH_LINE     24
#define FIFTH_LINE      32
#define SIXTH_LINE      40

//////////encoder defines//////////
#define ENCODER_PIN1    2
#define ENCODER_PIN2    3
#define STR_EVENT_SIZE  10

//////////OpenLog defines/////////
#define LOG_INTERVAL 300 //in seconds
#define TIME_CONVERT    7   //UTC to MST


//////////encoder variables//////////
volatile int8_t lastEncoded = 0, encoderFlag = 0;
String event[STR_EVENT_SIZE] = {"Pit Stop", "Interest", "Rest Area", "SOS", "Trail", "Pic", "Haunted", "Fun", "Secret", "Camp"};
int history = 0, pos = 0;

//////////GPS variables//////////
String hour, minute, latitudeDegree, latitudeMinute, latPole, longitudeDegree, longitudeMinute, longPole, day, month, year;

//////////OLED variables/////////
bool updateFlag = false;
const String TIME_ZONE = "MST";

//////////Data Logger variables/////////
int sCounter = LOG_INTERVAL;
bool printFlag = false, eventFlag = false;

MicroOLED oled(PIN_RESET, PIN_DC, PIN_CS);

void setup() 
{
  pinMode(ENCODER_PIN1, INPUT_PULLUP);
  pinMode(ENCODER_PIN2, INPUT_PULLUP);
  pinMode(BUTTON, INPUT_PULLUP);

  //ISR with interrupt 0 (pin 2) and 1 (pin 3)
  attachInterrupt(0, readEncoder, RISING);
  attachInterrupt(1, readEncoder, RISING);

  //oled init
  oled.begin();
  //oled font size
  oled.setFontType(0);
  
  //serial init
  Serial.begin(115200);
}

/**************************************Encoder ISR***************************************/
void readEncoder()
{
  int8_t MSB = digitalRead(ENCODER_PIN1);  //most significant bit
  int8_t LSB = digitalRead(ENCODER_PIN2); //least significant bit

  //shift left 1 bit
  MSB = MSB << 1;

  //bitwise OR
  int8_t encoded = MSB | LSB;

  //used flag instead of incrementing count
  //because (1)it would skip by 2 or 3, (2)busy waiting would not make it efficient
  //and (3)after 1-2ms micros will start behaving erratically
  if(encoded != B11)
  {
    //save previous encode
    lastEncoded = encoded;
    return;
  }
  
  //clockwise dir
  if(lastEncoded == B10)
    encoderFlag = 1;
    
  //counter-clockwise dir
  else if(lastEncoded == B01)
    encoderFlag = -1;

  return;
}

/*************************************Event Cycle*********************************/
void eventCycle()
{
  //increasing or decreasing by encoderFlag
  pos += encoderFlag;

  //making sure the index does not go out of bounds
  if(pos > (STR_EVENT_SIZE - 1))
    pos = 0;
  else if(pos < 0)
    pos = (STR_EVENT_SIZE - 1);
}

/***************************************Read GPS NEMA*************************************/
void readGPS(String& nemaCode)
{
  char gpsData;
  do
  {
    if(Serial.available())
    {
      gpsData = Serial.read();
      nemaCode += gpsData;
    }
  }
  while(gpsData != 0x0A);   //0x0A is hex for '\n' or the exit case
  
  return;
}

/***********************************Parse the NEMA CODE***********************************/
void parseNema(String& nemaCode)
{
  int commaLoc[13], j = 0;

  //find the location of every comma
  //to find the start and end of every field
  //decided not to use this method because (1)the GPS will output a default nema code with leading zeros even if it cannot 
  //comm with a satellite so there wont be any need to find the commas because the comma locations will never change
/*  for(int i = 0; i < nemaCode.length(); i++)
  {
    if(nemaCode[i] == ',')
      commaLoc[j++] = i;
  }

  //get the hour
  hour = nemaCode.substring(commaLoc[0]+1, commaLoc[0]+3);
  //get the minutes
  minute = nemaCode.substring(commaLoc[0]+3, commaLoc[0]+5);
  //get the latitude degree
  latitude = nemaCode.substring(commaLoc[2]+1, commaLoc[commaLoc[3]);
  //get the latitude pole (N or S)
  latPole = nemaCode.substring(commaLoc[3]+1, commaLoc[4]);
  //get the longitude
  longitude = nemaCode.substring(commaLoc[4]+1, commaLoc[5]);
  //get the longitude pole (E or W)
  longPole = nemaCode.substring(commaLoc[5]+1, commaLoc[6]);
  //get the day
  day = nemaCode.substring(commaLoc[8]+1, commaLoc[8]+3);
  //get the month
  month = nemaCode.substring(commaLoc[8]+3, commaLoc[8]+5);
  //get the year
  year = nemaCode.substring(commaLoc[8]+5, commaLoc[8]+7);
*/
  //get the hour
  hour = nemaCode.substring(7, 9);
  //get the minutes
  minute = nemaCode.substring(9, 11);
  //get the latitude degree
  latitudeDegree = nemaCode.substring(20, 22);
  //get the latitude minute
  latitudeMinute = nemaCode.substring(22, 29);
  //get the latitude pole (N or S)
  latPole = nemaCode.substring(30, 31);
  //get the longitude degree
  longitudeDegree = nemaCode.substring(32, 35);
  //get the longitide minute
  longitudeMinute = nemaCode.substring(35, 42);
  //get the longitude pole (E or W)
  longPole = nemaCode.substring(43, 44);
  //get the day
  day = nemaCode.substring(57, 59);
  //get the month
  month = nemaCode.substring(59, 61);
  //get the year
  year = nemaCode.substring(61, 63);
}

/********************************Convert Date and Time*************************************/
void convertData()
{  
  //convert hour to integer
  int hourInt = hour.toInt();
  //convert day to intger
  int dayInt = day.toInt();
  //convert universal time to other
  hourInt -= TIME_CONVERT;
  
  //check for negative number
  //move day back by 1
  if(hourInt < 0)
  {
    hourInt += 24;
    dayInt -= 1;
  }
  //convert hour integer to string and add a leading zero if needed
  if(hourInt < 10)
    hour = '0' + String(hourInt);
  else
    hour = String(hourInt);
  //convert day integer to string
  day = String(dayInt);

  //convert latitude degree to float
  float latDeg = latitudeDegree.toFloat();
  //convert longitude to float
  float longDeg = longitudeDegree.toFloat();
  //convert latitude minutes to float
  float latMin = latitudeMinute.toFloat();
  //convert longitude to float
  float longMin = longitudeMinute.toFloat();

  //dividing the minutes by 60 and adding to the degree
  latDeg += latMin/60;
  longDeg += longMin/60;

  //convert back to string and if poles are not N or E convert to negative degrees
  if(latPole == "S")
    latitudeDegree = '-' + String(latDeg);
  if(longPole == "W")
    longitudeDegree = '-' + String(longDeg);
  else
  {
    latitudeDegree = String(latDeg);
    longitudeDegree = String(longDeg);
  }
}
/************************************update to OLED***************************************/
void updateOLED()
{
  //refreshing the OLED
  oled.clear(PAGE);
  //latitude on the 1st line
  oled.setCursor(0,FIRST_LINE);
  oled.print(latitudeDegree);
  //longitude on the 2nd line
  oled.setCursor(0,SECOND_LINE);
  oled.print(longitudeDegree);
  //date on the 3rd line
  oled.setCursor(0,THIRD_LINE);
  oled.print(month + '/' + day + '/' + year);
  //time on the 4th line
  oled.setCursor(0,FOURTH_LINE);
  oled.print(hour + ':' + minute + " " + TIME_ZONE);
  //next save in seconds on the 5th line
  oled.setCursor(0,FIFTH_LINE);
  if(printFlag)
    oled.print("Saved!");
  else
    oled.print("NXTS:" + String(sCounter) + 's');
  //event display on the 6th line
  oled.setCursor(0,SIXTH_LINE);
  if(eventFlag)
    oled.print("Saved!");
  else
    oled.print(event[pos]);

  oled.display();
}

/************************************Print to logger*************************************/
void printLogger()
{  
  //string of all the data parsed
  String data = month +'/'+ day +'/'+ year +','+ hour +':'+ minute +','+ latitudeDegree +','+ longitudeDegree;
  
  //adding the event if triggered
  if(eventFlag)
    data += " Event: " + event[pos];
    
  //printing to the logger
  Serial.println(data);
}

/***************************************Main Loop****************************************/
void loop() 
{
  //check if there is new gps data
  //gets the data and parses it
  //it has a 1Hz update rate so the counter decrements every second
  //the time is UTC so time and date are converted
  //an update to the OLED flag is triggered
  if(Serial.available())
  {
    String nemaCode;
    readGPS(nemaCode);
    parseNema(nemaCode);
    convertData();
    sCounter--;
    updateFlag = true;
  }

  //when the rotary encoder is rotated it cycles through the array of string events
  //an update to the OLED flag is triggered
  //the encoder flag will restored
  if(encoderFlag != 0)
  {
    eventCycle();
    encoderFlag = 0;
    updateFlag = true;
  }

  //timed log instead of logging data every data collection (although possible)
  //a counter that decrements every second is the timer
  //once it reaches zero it logs the data and resets the timer
  //an update to the OLED flag is triggered
  if(sCounter <= 0)
  {
    printLogger();
    printFlag = true;
    sCounter = LOG_INTERVAL;
  }

  //button check, would much rather use interrupts but none available
  //logs data the same way as the timed log but adds an event to the string
  //can log data anytime the button is pressed
  //an update to the OLED flag is triggered
  if(pulseIn(BUTTON, LOW, 500))
  {
    eventFlag = true;
    printLogger();
  }

  //the OLED updater. refreshed the screen before populating the new data
  //turns off all the flags
  if(updateFlag)
  {
    updateOLED();
    updateFlag = false;
    printFlag = false;
    eventFlag = false;
  }
}
