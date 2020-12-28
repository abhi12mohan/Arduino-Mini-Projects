#include <SPI.h>
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

const int UPDATE_PERIOD = 20;
const uint8_t PIN_1 = 16; //button 1
const uint8_t PIN_2 = 5; //button 2
uint32_t primary_timer;
float sample_rate = 2000; //Hz
float sample_period = (int)(1e6 / sample_rate);
uint32_t timer;


int render_counter;
uint16_t raw_reading;
uint16_t scaled_reading_for_lcd;
uint16_t old_scaled_reading_for_lcd;
float scaled_reading_for_serial;
int state = 0;
float max_value = 0;
float min_value = 0;
float difference;
float old_value = 0;
float new_value = 0;
float count = 0;
float reading = 0;


void setup() {
  Serial.begin(115200);               // Set up serial port
  pinMode(PIN_1, INPUT_PULLUP);
  primary_timer = micros();
  render_counter = 0;
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set col
  analogSetAttenuation(ADC_6db); //set to 6dB attenuation for 3.3V full scale reading.
}

void loop() {
    raw_reading = analogRead(A0);
    reading = abs((raw_reading * 3.3 / 4096) - 2);
    
    switch(state){
      case 0:
        if (reading > 1.5){
          state = 1;
          timer = millis();
        }
        break;
      case 1:
        if (reading  > 1.5 && millis() - timer < 20){
          state = 0;
        }
        else if (reading > 1.5 && millis() - timer < 750 && millis() - timer > 50){
          tft.fillScreen(TFT_GREEN);
          state = 2;
        }
        else if (millis() - timer > 1000){
          state = 0; 
        }
        break;
      case 2:
        if (reading > 1.5){
          state = 3;
          timer = millis();
        }
        break;
      case 3:
        if (reading > 1.5 && millis() - timer < 20){
          state = 2;
        }
        else if (reading > 1.5 && millis() - timer < 750 && millis() - timer > 50){
          tft.fillScreen(TFT_BLACK);
          state = 0;
        }
        else if (millis() - timer > 1000){
          state = 2; 
        }
        break;      
    }

//    Serial.println(reading);
//    Serial.println(state);
    
    while (micros() > primary_timer && micros() - primary_timer < sample_period); //prevents rollover glitch every 71 minutes...not super needed
    primary_timer = micros();
}
