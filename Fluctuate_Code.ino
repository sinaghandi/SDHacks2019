#include<ESP8266WiFi.h>
#include<math.h>

//Temperature Sensor Definitions
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k

// WiFi Definitions
const char* ssid = "Esp8266TestNet";
const char* password = "Esp8266Test"; // has to be longer than 7 chars
const char* value = "";
String request;

//Global variables for mode, ideal temp, and auto temps
int heatMode = 0; //0 = off, 1 = on, 2 = auto
int userTemp = 25; //room temperature, shouldn't matter as it changes as soon as mode is on, and fluctuate will always check mode first
int autoTemp = 45;
int autoThreshold = 30;

int enableCircuit = 2; //output pin for current restrictor
//int ambientTempPin = A1; //Future Expansion
int gloveTempPin = A0; //input pin for glove temp

//pinMode(ambientTempPin, INPUT);


WiFiServer server(80);
//void ICACHE_RAM_ATTR onTimerISR(){
////Transmits inner glove temperature, and regulates temperature
//
//  fluctuate(request);
//  client.print(getGloveTemp());
//
//}

void setup() {
  
  pinMode(enableCircuit, OUTPUT);
  pinMode(gloveTempPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);


   Serial.begin(115200);
   delay(10);


   WiFi.mode(WIFI_AP);
   WiFi.softAP(ssid, password, 1, 0);
  
   server.begin();   
//   cli(); //Stops interrupts, to setup for interrupts
//   timer1_attachInterrupt(onTimerISR);
//   timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
//   timer1_write(600000); //120000 us
//   sei(); //Allow interrupts again
}




void setMode(String request) { //takes client request as input, returns 1 if turn on heating, 0 if not, -1 if anything else

  Serial.println(request);
  if(request.indexOf("/heat/on") != -1) {heatMode =  1;  digitalWrite(LED_BUILTIN, LOW);Serial.println("poop");}  //on
  else if (request.indexOf("/heat/off") != -1) {heatMode =  0;  digitalWrite(LED_BUILTIN, HIGH);Serial.println("head");} //off
  else if(request.indexOf("/heat/auto") != -1) heatMode = 2; //auto
  
}

void setTemp (String request) { //read temperature input from user from app
  
  setMode(request);

  if(heatMode==1) {
    
  
    if(request.indexOf("?") != -1) {

      String tempstr = request.substring(request.indexOf("?")+1);
      userTemp = tempstr.toInt();

      }
      else {

        userTemp = 100;
      }
    }
  else if(heatMode == 2) { //in mode 2, input string will be of format "./heat/auto/threshold=autoThreshold&temp=autoTemp
  if(request.indexOf("?") != -1) {
    String x = request.substring(request.indexOf("threshold=") + strlen("threshold="), request.indexOf("&"));//returns characters between end of "threshold=" and "&"
    autoThreshold =  x.toInt(); //converts to int

    String y = request.substring(request.indexOf("&")+strlen("temp="));//returns characters after "&"
    autoTemp = y.toInt(); 
  }
  }
  
}

int celsiusToFarenheit(int celsius) {

  int farenheit =  (int) (celsius*1.8) + 32;
  return farenheit;
}

int getGloveTemp() { //Read ambient temperature from sensor on inside of glove

  int a = analogRead(gloveTempPin);

  float R = 1023.0/a-1.0;
  R = R0*R;

  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet

  return ((int) lround(temperature));
  
}


void fluctuate (String request) { //check temperature reading vs set reading. If lower, heat up. If higher, turn off
 
  setTemp(request);
  int gloveTemp = getGloveTemp();
  if(heatMode == 1) { //if mode = on

      if(gloveTemp < userTemp) digitalWrite(enableCircuit, HIGH);
      else digitalWrite(enableCircuit, LOW);
    
  }
  else if(heatMode == 2) { //if mode = auto

    if(gloveTemp < autoThreshold) digitalWrite(enableCircuit, HIGH); //if less than threshold, turn on. If greater than threshold, dont change state (could be on and heating up or off and doing nothing)
    else if((gloveTemp) >= autoTemp) digitalWrite(enableCircuit, LOW); //if temp is higher than the auto temp, turn off
    
  }
  else if(heatMode == 0) digitalWrite(enableCircuit, LOW);
  
}

void loop() {
  // Check of client has connected
  WiFiClient client = server.available();
  if(!client) {
    return;
  }

  // Read the request line
  request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  fluctuate(request);
  Serial.print("Temperature Reading (C): ");
  Serial.println(getGloveTemp());

  Serial.println("\nUser Temp: ");
  Serial.println(userTemp);
//  client.print(getGloveTemp());
  
//  // Send the response to the client //not necessary if sending data in interrupt
//  client.print(s);
//  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected when the function returns and the client object is destroyed
}
