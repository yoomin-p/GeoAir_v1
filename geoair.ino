#include <M5Stack.h>
#include "Free_Fonts.h" 
#include <Wire.h>
#include "DFRobot_SHT20.h"
#include <TinyGPS++.h>
#include "FS.h"
#include "SD.h"

DFRobot_SHT20    sht20;

#define TFT_GREY 0x7BEF
#define DATA_LEN 32
#define X_LOCAL 10
#define Y_LOCAL 30

#define X_OFFSET 160
#define Y_OFFSET 25
#define FRONT 4



uint16_t CheckSum;
uint16_t CheckCode;
TinyGPSPlus gps;
HardwareSerial GPS(1);

const int GPS_RX = 5;
const int GPS_TX = 13;
uint16_t linenum = 0;

// Print the header for a display screen
void header(const char *string, uint16_t color)
{
  M5.Lcd.fillScreen(color);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_YELLOW , TFT_PURPLE);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_PURPLE);
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString(string, 160, 3, 4); 
}

String formatTime(String hour, String minute, String second){
  String returnValue = "";
  if (hour.length()<2){
    returnValue+="0"+hour;
  }else{
    returnValue+=hour;
  }
  returnValue+=":";
  if (minute.length()<2){
    returnValue+="0"+minute;
  }else{
    returnValue+=minute;
  }
  returnValue+=":";
  if (second.length()<2){
    returnValue+="0"+second;
  }else{
    returnValue+=second;
  }
  return returnValue;
}

String bigEndianDate(String year, String month, String day){
  String returnValue = year;
  if (month.length()<2){
    returnValue+="0"+month;
  }else{
    returnValue+=month;
  }
  if (day.length()<2){
    returnValue+="0"+day;
  }else{
    returnValue+=day;
  }
  return returnValue;
}
void writeFile(fs::FS &fs, String path, String message){
    String toPrint = "";
    boolean isFirst = !fs.exists(path);
    File file = fs.open(path, FILE_APPEND);
    M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*7 , FRONT);
    M5.Lcd.print("                                    ");
    if (isFirst){
      toPrint="Line #,Temperature (F),Relative Humidity,# Satellites Lock,Ground Speed (mph),Altitude (m),Lat,Lon,Month,Day,Year,Hour (GMT),Minute,Second,Battery Level,PM 1.0 STP,PM 2.5 STP,PM 10 STP,PM 1.0 ATM,PM 2.5 ATM,PM 10 ATM,0.3 um,0.5 um,1.0 um,2.5 um,5.0 um,10 um\n";
    }
    toPrint+=message;
    if(!file){
        M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*7 , FRONT);
        M5.Lcd.print("Failed to open file ");
    }else{
      if(file.print(toPrint)){
        M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*7 , FRONT);
        M5.Lcd.print("Log Size: " + String(file.size()/1024) +" kb");
      } else {
        M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*7 , FRONT);
        M5.Lcd.print("File write failed");
      }
    }
    file.close();
}

void setup() {
  
  M5.begin();
  Wire.begin();
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  GPS.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
  
  pinMode(10, OUTPUT);
  digitalWrite(10, 1);

  M5.Lcd.fillScreen(TFT_BLACK);               
  header("GeoAir", TFT_BLACK);

  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20(); 
}

uint8_t Air_val[32]={0};
int16_t p_val[16]={0};
uint8_t i=0;

void LCD_Display_Val(void){              

     for(int i=0,j=0;i<32;i++){
        if(i%2==0){
          p_val[j] = Air_val[i];
          p_val[j] = p_val[j] <<8;
        }else{
          p_val[j] |= Air_val[i];
          j++;
        }
      
     }
       M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
    

     M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*1 , FRONT);
     M5.Lcd.setTextSize(2);
     M5.Lcd.print("                  ");
     M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*1 , FRONT);
     M5.Lcd.setTextSize(1);
     M5.Lcd.print("PM2.5: "); 
     M5.Lcd.setTextSize(2);
     M5.Lcd.print(p_val[3]); 
     M5.Lcd.setTextSize(1);

}


int8_t getBatteryLevel()
{
  Wire.beginTransmission(0x75);
  Wire.write(0x78);
  if (Wire.endTransmission(false) == 0
   && Wire.requestFrom(0x75, 1)) {
    switch (Wire.read() & 0xF0) {
    case 0xE0: return 25;
    case 0xC0: return 50;
    case 0x80: return 75;
    case 0x00: return 100;
    default: return 0;
    }
  }
  return -1;
}

void loop() {
  
   if(Serial2.available()) {
      Air_val[i] = Serial2.read();
      Serial.write( Air_val[i]);
      i++;
   }else{
      i=0;
   }

  if(i==DATA_LEN){
      LCD_Display_Val();
      float humd = sht20.readHumidity();                  // Read Humidity
      float temp = (sht20.readTemperature() * (9.0/5.0)) + 32 ;               // Read Temperature
      String linebuf;

      M5.Lcd.setTextColor(TFT_WHITE,TFT_BLACK);
      linenum++;
      linebuf += String(linenum);
      linebuf += ",";
      linebuf += String(temp,6);
      linebuf += ",";
      linebuf += String(humd,6);
      linebuf += ",";
      linebuf += String(gps.satellites.value());
      linebuf += ",";
      linebuf += String(gps.speed.mph(),2);
      linebuf += ",";
      linebuf += String(gps.altitude.meters(),2);
      linebuf += ",";
      linebuf += String(gps.location.lat(),16);
      linebuf += ",";
      linebuf += String(gps.location.lng(),16);
      linebuf += ",";
      TinyGPSDate d; 
      d = gps.date;
      TinyGPSTime t;
      t= gps.time;
      linebuf += String(d.month());
      linebuf += ",";
      linebuf += String(d.day());
      linebuf += ",";
      linebuf += String(d.year());
      linebuf += ",";
      linebuf += String(t.hour());
      linebuf += ",";
      linebuf += String(t.minute());
      linebuf += ",";
      linebuf += String(t.second());
      linebuf += ",";
      linebuf += String(getBatteryLevel());
      linebuf += ",";
  
      for(int j=2;j<14;j++){
        linebuf += String(p_val[j]);
        linebuf += ",";
     }
      linebuf += "\r\n";

      String formattedDate = bigEndianDate(String(d.year()),String(d.month()),String(d.day()));
      String formattedTime = formatTime(String(t.hour()), String(t.minute()), String(t.second()));
      writeFile(SD,String("/") + formattedDate + String(".csv") ,  linebuf);

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*4 , FRONT);
      M5.Lcd.print("                               ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*4 , FRONT);
      M5.Lcd.print("Battery: ");
      M5.Lcd.print(getBatteryLevel());
      M5.Lcd.print("%");

      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*5 , FRONT);
      M5.Lcd.print("                         ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*5 , FRONT);
      M5.Lcd.print("GPS: " + String(gps.location.lat())+", "+String(gps.location.lng()));
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*6 , FRONT);
      M5.Lcd.print("                         ");
      M5.Lcd.setCursor(X_LOCAL, Y_LOCAL + Y_OFFSET*6 , FRONT);
      M5.lcd.print(formattedDate + " "+ formattedTime +" GMT");

  }

  smartDelay(100);
}


// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
    while (GPS.available()){
      gps.encode(GPS.read());
      break;
    }
}
