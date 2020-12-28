#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu9255_esp32.h>
#include<math.h>
#include<string.h>

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON_PIN = 16;
const int LOOP_PERIOD = 40;

char network[] = "MIT";
char password[] = "";

const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
const uint16_t IN_BUFFER_SIZE = 1000;
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char request_buffer[IN_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response_buffer[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP response

const int a = 5;
uint16_t m;
uint16_t p;
uint16_t t_server;
uint16_t t_mcu;
unsigned long primary_timer;

void caesar_cipher(char* message_in, char* message_out, int shift, bool encrypt, int out_buffer_size){
  if (not encrypt){
    shift = -1 * shift;
  }
  
  int pos = 0;
  while (pos < strlen(message_in) && pos < out_buffer_size - 1){
    int result = message_in[pos] + shift;
    
    if (result < 32){
      result = result + 95;
    }
    else if (result > 126){
      result = (result % 127) + 32; 
    }
    
    char output = result;
    
    message_out[pos] = output;
    pos++;
  }
}

void vigenere_cipher(char* message_in, char* message_out, char* keyword, bool encrypt, int out_buffer_size){
  int pos;
  int shift;
  char key;
  
  for (int j = 0; j < strlen(message_in) && j < out_buffer_size - 1; j++){
    pos = j % strlen(keyword);
    key = int(keyword[pos]);
    shift = key - 32;
    
    caesar_cipher(&message_in[j], &message_out[j], shift, encrypt, 2);
  }
}

void generate_keyword(int key, char* keyword){
  char alphabet[100] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  memset(keyword, 0, sizeof(keyword));
  for (int j = 0; j < 6; j++){
    int index = (key + (j * 6)) % 37;
    keyword[j] = alphabet[index];
  }
}

void dhke(int t, int p, int m, int b, char* message_in, char* message_out, bool encrypt, int out_buffer_size){
    t %= p;
    
    int key = 1;
    while (b > 0){
      if (b & 1) key = (key * t) % p;
      t = (t * t) % p;
      b >>= 1;
    }
    
    char keyword[out_buffer_size] = {0};
    generate_keyword(key, keyword);
    vigenere_cipher(message_in, message_out, keyword, encrypt, out_buffer_size);
}

void lookup(char* query, char* response, int response_size) {
  char body[200]; //for body;
  sprintf(body,"query=%s&p=%i&m=%i&t_mcu=%i", query, p, m, t_mcu);
  int body_len = strlen(body); //calculate body length (for header reporting)
  sprintf(request_buffer,"POST http://608dev.net/sandbox/sc/abhimo/key_exchange/key_exchange.py HTTP/1.1\r\n");
  strcat(request_buffer,"Host: 608dev.net\r\n");
  strcat(request_buffer,"Content-Type: application/x-www-form-urlencoded\r\n");
  sprintf(request_buffer+strlen(request_buffer),"Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
  strcat(request_buffer,"\r\n"); //new line from header to body
  strcat(request_buffer,body); //body
  strcat(request_buffer,"\r\n"); //header
  do_http_request("608dev.net", request_buffer, response_buffer, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT,true);
  
  char request_buffer[200];
  sprintf(request_buffer, "GET http://608dev.net/sandbox/sc/abhimo/key_exchange/key_exchange.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "\r\n"); //new line from header to body
  do_http_request("608dev.net", request_buffer, response, response_size, RESPONSE_TIMEOUT, true);
}

class Number_info {
    char message[400] = {0};
    char query_string[50] = {0};
    char message_in[50] = {0};
    int state;
  public:
    void update(char* output) {
      memset(query_string, 0, sizeof(query_string));
      memset(message_in, 0, sizeof(message_in));

      //change number below to test system for number info!
      strcat(message_in, "20");
      dhke(t_server, p, m, a, message_in, query_string, true, sizeof(query_string)); //encrypting initial query
      
      strcat(query_string, "&len=200");
      lookup(query_string, message, 200);
      memset(output, 0, OUT_BUFFER_SIZE);
      dhke(t_server, p, m, a, message, output, false, OUT_BUFFER_SIZE); //decrypting encrypted message
    }
};

void extract_values(char* response) {
  char* pt= strtok(response, "=");
  pt = strtok(NULL, ",");
  p = int(atof(pt));
  
  pt = strtok(NULL, "=");
  pt = strtok(NULL, ",");
  m = int(atof(pt));
  
  pt = strtok(NULL, "=");
  pt = strtok(NULL, ",");
  t_server = int(atof(pt));
}

Number_info num; //wikipedia object

void setup() {
  Serial.begin(115200); //for debugging if needed.
  WiFi.begin(network, password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.println(WiFi.localIP().toString() + " (" + WiFi.macAddress() + ") (" + WiFi.SSID() + ")");
    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  primary_timer = millis();
  
  char request_buffer[200];
  sprintf(request_buffer, "GET http://608dev.net/sandbox/sc/abhimo/key_exchange/key_output.py HTTP/1.1\r\n");
  strcat(request_buffer, "Host: 608dev.net\r\n");
  strcat(request_buffer, "\r\n"); //new line from header to body
  do_http_request("608dev.net", request_buffer, response, sizeof(response), RESPONSE_TIMEOUT, true);
  
  extract_values(response);
  
  int M = m % p;
  
  int result = 1;
  int A = a;
  
  while (A > 0){
    if (A & 1) result = (result * m) % p;
    M = (M * M) % p;
    A >>= 1;
  }
  
  t_mcu = result;
}

void loop() {
  if (digitalRead(BUTTON_PIN) == 0){
    num.update(response);
    
    if (strcmp(response, old_response) != 0) {//only draw if changed!
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.println(response);
    }
    memset(old_response, 0, sizeof(old_response));
    strcat(old_response, response);
  }
  
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}
