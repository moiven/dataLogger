/**
  *****************************GPS data logger by Marvin Ibarra***************************
  *
  * simple project where i use the Venus GPS with SMA connector and Openlog from sparkfun.
  * The gps module outputs a stream of data or NEMA code and the arduino parses this data
  * into an easier to read string of data. This data is then sent to the Sparkfun Openlog 
  * where it gets stored into a .TXT file.
**/

#include <SoftwareSerial.h>
SoftwareSerial gpsSerial(10, 11); // RX, TX

#define BUTTON 2
#define EVENT_LED 3

int count = 0;

/**************************************Setup******************************************/
void setup()
{
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(EVENT_LED, OUTPUT);
  Serial.begin(9600);
  gpsSerial.begin(9600);
}

/************************************Read NEMA****************************************/
void readGps(String& data)
{
  char gpsData;
  do
  {
    if(gpsSerial.available())
    {
      gpsData = gpsSerial.read();
      data += gpsData;
    }
  }
  while(gpsData != 0x0A);  //0x0A is hex for '\n'
  return;
}

/************************************Manipulate NEMA******************************************/
void manipulateData(String& data)
{
  int intHour;
  String hour, minute, latitude, latPole, longitude, longPole, day, month, year;
  int location[13], count = 0;
  
  //find the location of every comma
  //used to find the start and end of every field
  for(int i = 0; i < data.length(); i++)
  {
    if(data[i] == ',')
      location[count++] = i;
  }
  
  hour = data.substring(location[0]+1, location[0]+3);  //grabbing the hour
  //intHour = hour.toInt() - 6;  //converting to int to correct time zone
  //hour = String(intHour);  //converting it back to string
  minute = data.substring(location[0]+3, location[0]+5);  //grabbing the minutes
  latitude = data.substring(location[2]+1, location[3]);  //grabbing the latitude
  latPole = data.substring(location[3]+1, location[4]);  //grabbing the North or South pole
  longitude = data.substring(location[4]+1, location[5]);  //grabbing the longitude
  longPole = data.substring(location[5]+1, location[6]);  //grabbing the East or West pole
  day = data.substring(location[8]+1, location[8]+3);  //grabbing the day
  month = data.substring(location[8]+3, location[8]+5);  //grabbing the month
  year = data.substring(location[8]+5, location[8]+7);  //grabbing the year
  
  //string of easy to read data
  data = month + "/" + day + "/" + year + " " + hour + ":" + minute + "(UTC) " + latitude + latPole + " " + longitude + longPole;  //putting it all together
}
/*****************************************Printing Data******************************************/
void printData(String data)
{
  gpsSerial.println(data);
  //Serial.println(data);
}

/******************************************Printing Event******************************************/
void printEvent(String data)
{
  gpsSerial.println(data + " EVENT: Pit Stop");
  //Serial.println(data + " EVENT: Pit Stop");
}

/**********************************************main loop******************************************/
void loop()
{
  String data;
  
  //collect NEMA code
  readGps(data);
  //one count is 1 second
  count++;
  
  //button event
  //turn on LED for the whole event
  if(!digitalRead(BUTTON))
  {
    analogWrite(EVENT_LED, 172);
    manipulateData(data);
    printEvent(data);
    analogWrite(EVENT_LED, 0);
  }
  if(count < 300) return;  //the location is saved every 5 minutes
  
  manipulateData(data);  //move the data around
  printData(data);
  count = 0x00;
}
