#include <WiFi.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <string.h>
#include <mpu9255_esp32.h>

#define SCREEN_HEIGHT 160
#define SCREEN_WIDTH 128

#define HOME 0
#define PLAY 1
#define GAME 2
#define FINISH 3
#define LEADERBOARD 4
#define POST 5

const int serial_update_period = 20;
float sample_rate = 2000;
float sample_period = (int)(1e6 / sample_rate);
int serial_counter;
const uint8_t button_pin = 16;

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const int POST_PERIOD = 6000; //periodicity of getting a number fact.
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

MPU9255 imu;
TFT_eSPI tft = TFT_eSPI();

class Button {
  public:
    unsigned long t_of_state_2;
    unsigned long t_of_button_change;
    unsigned long debounce_time;
    unsigned long hold_time;
    int pin;
    int movement;
    int state;
    bool button_pressed;
    Button(uint8_t p);
    void read();
    int update();
};

Button::Button(uint8_t p) {
  t_of_state_2 = millis();
  t_of_button_change = millis();
  debounce_time = 10;
  hold_time = 1000;
  pin = p;
  movement = 0;
  state = 0;
  button_pressed = 0;
}

Button button(button_pin);

void Button::read() {
  int state = digitalRead(button_pin);
  button_pressed = !state;
}

int Button::update() {
      read();
      movement = 0;
            
      if (state == 0) {
        movement = 0;
        if (button_pressed) {
          state = 1;
          t_of_button_change = millis();
        }
      } else if (state == 1) {
        if (!button_pressed) {
          state = 0;
          t_of_button_change = millis();
        }
        else if (millis() - t_of_button_change >= debounce_time) {
          state = 2;
          t_of_state_2 = millis();
        }
      } else if (state == 2) {
        if (!button_pressed) {
          state = 4;
          movement = 0;
          t_of_button_change = millis();
        }
        else if (millis() - t_of_state_2 >= hold_time) {
          state = 3;
        }
      } else if (state == 3) {
        if (!button_pressed) {
          state = 4;
          movement  = 0;
          t_of_button_change = millis();
        }
      } else if (state == 4) {
        if (!button_pressed && millis() - t_of_button_change >= debounce_time) {
          state = 0;
          if (millis() - t_of_state_2 < hold_time) {
            movement = 1;
          }
          else {
            movement = 2;
          }
        }
        else if (button_pressed && millis() - t_of_state_2 < hold_time) {
          state = 2;
          t_of_button_change = millis();
        }
        else if (button_pressed && millis() - t_of_state_2 >= hold_time) {
          state = 3;
          t_of_button_change = millis();
        }
      }

      return movement;
    }

class Functions
{
  private:
  public:
    void lookup(String input, String* output);
    void post(int score, String* output);
    float get_angle();
    float get_gyro();
    void pretty_print(int startx, int starty, String input, int fwidth, int fheight, int spacing);
};

void Functions::lookup(String input, String* output) {
  WiFiClient client; //instantiate a client object
  if (client.connect("608dev.net", 80)) { //try to connect to host
    client.println("GET " + input + " HTTP/1.1");
    client.println("Host: 608dev.net");
    client.print("\r\n");
    
    unsigned long count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      String line = client.readStringUntil('\n');
      if (line == "\r") { //found a blank line!
        break;
      }
      if (millis() - count > 6000) break;
    }
    count = millis();
    String op; //create empty String object
    while (client.available()) { //read out remaining text (body of response)
      op += (char)client.read();
    }
    *output = op;
    client.stop();
  } else {
    Serial.println("connection failed");
    Serial.println("wait 0.5 sec...");
    *output = "ERROR";
    client.stop();
  }
}

uint8_t char_append(char* buff, char c, uint16_t buff_size) {
        int len = strlen(buff);
        if (len>buff_size) return false;
        buff[len] = c;
        buff[len+1] = '\0';
        return true;
}

void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial){
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n',response,response_size);
      if (strcmp(response,"\r")==0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis()-count>response_timeout) break;
    }
    memset(response, 0, response_size);  
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response,client.read(),OUT_BUFFER_SIZE);
    }
    client.stop();
    if (serial) Serial.println("-----------");  
  }else{
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}        

void Functions::post(int score, String* output){
  char body[100]; //for body
  sprintf(body,"kerberos=abhimo&score=%d",score);
  int body_len = strlen(body); //calculate body length (for header reporting)
  sprintf(request_buffer,"POST http://608dev.net/sandbox/sc/abhimo/bopit/bopit_table.py HTTP/1.1\r\n");
  strcat(request_buffer,"Host: 608dev.net\r\n");
  strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer,"\r\n"); //new line from header to body
  strcat(request_buffer,body); //body
  strcat(request_buffer,"\r\n"); //header
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
}

void Functions::pretty_print(int startx, int starty, String input, int fwidth, int fheight, int spacing) {
  int x = startx;
  int y = starty;
  String temp = "";
  for (int i = 0; i < input.length(); i++) {
    if (fwidth * temp.length() <= (SCREEN_WIDTH - fwidth - x)) {
      if (input.charAt(i) == '\n') {
        tft.setCursor(x, y);
        tft.print(temp);
        y += (fheight + spacing);
        temp = "";
        if (y > SCREEN_HEIGHT) break;
      } else {
        temp.concat(input.charAt(i));
      }
    } else {
      tft.setCursor(x, y);
      tft.print(temp);
      temp = "";
      y += (fheight + spacing);
      if (y > SCREEN_HEIGHT) break;
      if (input.charAt(i) != '\n') {
        temp.concat(input.charAt(i));
      } else {
        tft.setCursor(x, y);
        y += (fheight + spacing);
        if (y > SCREEN_HEIGHT) break;
      }
    }
    if (i == input.length() - 1) {
      tft.setCursor(x, y);
      tft.print(temp);
    }
  }
}

Functions func;

class BopIt
{
  private:
    String message; //contains previous query response
    String request_string;
    int state;
    int score = 0;
    unsigned long scrolling_timer;
    unsigned long timer;
    float angle_threshold = 0.3;
    float turn_time = 2500.;
    unsigned long turn_timer;
    int turn_count = 0;
    int action;
    uint16_t raw_reading;
    float scaled_reading_for_serial;
    int threshold = 0;
    const int serial_update_period = 20;
    float sample_rate = 2000; //Hz
    float sample_period = (int)(1e6 / sample_rate);
    int serial_counter = 0;
    int pin;

    String action_time(int action);
    
  public:
    BopIt();
    String update(int button);
};

BopIt bop;

BopIt::BopIt() {
  state = HOME;
  message = "Press to start! \n\nHold for leaderboard! \n\n";
  timer = micros();
}

String BopIt::update(int button) {
  if (state == HOME) {
    if (button == 1) { 
      state = PLAY;
    }
    if (button == 2) {
      state = LEADERBOARD;
    }
    return message;
  }
  else if (state == PLAY) { 
    if (turn_count == 4) {
      turn_time = turn_time * 0.8;
      turn_count = 0;
    }
    
    state = GAME;
    turn_timer = millis();
    action = random(0, 4);
    message = "";
    return message;
  }
  else if (state == GAME) {
    message = action_time(action);
    return message;
  }
  else if (state == FINISH) {
    state = POST;
    turn_count = 0;
    turn_time = 2000;
    message = "Game Over!\n";
    return message;
  }
    else if (state == POST) {
    state = HOME;
    String post_message = "";
    func.post(score, &post_message);
    message = "Game Over!\nPress to play again!\nHold for leaderboard!\n";
    return message;
  }
  else if (state == LEADERBOARD) {
    request_string = "/sandbox/sc/abhimo/bopit/bopit_table.py?kerberos=abhimo";
    String leaderboard = "";
    func.lookup(request_string, &leaderboard);
    state = HOME;
    message = "High scores:        \n                              " + leaderboard +  "\n" + "Press to play again!\n";
    return message;
  }
}

String BopIt::action_time(int action) {
  Serial.println(score);    
  if (millis() - turn_timer > turn_time) {
    message = "Game Over\nPress to play again!\nHold for leaderboard!\n";
    state = FINISH;
    return message;
  }
  else if (action == 0 and millis() - turn_timer < turn_time) {
    message = "Shout It!                                                                                                                                                                                                                                                                                                            \n";
    raw_reading = analogRead(A0);
//    Serial.println(raw_reading);
    scaled_reading_for_serial = (raw_reading / 3.3 / 4096.0);
    serial_counter++;
      if (scaled_reading_for_serial > 0.15) {
        threshold++; 
      }
      else if (threshold >= 3) {
        threshold = 0;
        turn_count++;
        score++;
        state = PLAY;
      }
    while (micros() > timer && micros() - timer < sample_period); //prevents rollover glitch
    timer = micros();
    return message;
  }
  else if (action == 1 and millis() - turn_timer < turn_time) { 
    message = "Bop It!                                                                                                                                                                                                                                                                                                             \n";
    int pin = digitalRead(button_pin);
    if (pin == 0) {
      turn_count++;
      score++;
      state = PLAY;
    }
    return message;
  }
  else if (action == 2 and millis() - turn_timer < turn_time) {
    message = "Tilt It!                                                                                                                                                                                                                                                                                                             \n";
    imu.readAccelData(imu.accelCount); 
    float x, y;
    x = imu.accelCount[0]*imu.aRes;
    y = imu.accelCount[1]*imu.aRes;
    
    if ((y > angle_threshold or y < -angle_threshold) and (x > angle_threshold or x < -angle_threshold)) {
      turn_count++;
      score++;
      state = PLAY;
    }
    return message;
  }
  else if (action == 3 and millis() - turn_timer < turn_time) {
    message = "Spin It!                                                                                                                                                                                                                                                                                                              \n";
    
    float x, y, z;
    imu.readGyroData(imu.gyroCount);
    x = imu.gyroCount[0] * imu.gRes;
    y = imu.gyroCount[1] * imu.gRes;
    z = imu.gyroCount[2] * imu.gRes;
    
    if (x > 25 or y > 25 or z > 25) {
      threshold++;
    }
    else if (threshold >= 25) { //Not just random noise
      threshold = 0;
      turn_count++;
      score++;
      state = PLAY;
    }
    return message;
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(1); //default font size
  tft.fillScreen(TFT_BLACK); //fill background
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  delay(100); //wait a bit (100 ms)
  Wire.begin();
  
  if (imu.setupIMU(1)){
    Serial.println("IMU Connected!");
  }else{
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  
  WiFi.begin("MIT"); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
//  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count<12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
//    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  
  pinMode(button_pin, INPUT_PULLUP);
  analogSetAttenuation(ADC_6db);
}

void loop() {  
  int change = button.update();
  String output = bop.update(change); //input: angle and button, output String to display on this timestep
  func.pretty_print(0, 8, output, 5, 7, 0);
}
