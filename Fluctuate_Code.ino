#include<ESP8266WiFi.h>
#include<math.h>

//Temperature Sensor Definitions
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;            // R0 = 100k
const int pinTempSensor = A0;     // Grove - Temperature Sensor connect to A0

// WiFi Definitions
const char* ssid = "Esp8266TestNet";
const char* password = "Esp8266Test"; // has to be longer than 7 chars
const char* value = "";
String request;

//Global variables for mode, ideal temp, and auto temps
int mode = 0; //0 = off, 1 = on, 2 = auto
int userTemp = 72; //room temperature, shouldn't matter as it changes as soon as mode is on, and fluctuate will always check mode first
int autoTemp = 85;
int autoThreshold = 75;

int enableCircuit = D1; //output pin for current restrictor
//int ambientTempPin = A1; //Future Expansion
int gloveTempPin = A0; //input pin for glove temp

//pinMode(ambientTempPin, INPUT);
pinMode(enableCircuit, OUTPUT);
pinMode(gloveTempPin, INPUT);

WiFiServer server(80);

void setup() {

  cli(); //Stops interrupts, to setup for interrupts

  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 15624;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei(); //Allow interrupts again

   Serial.begin(115200);
   delay(10);
   pinMode(heatPin, OUTPUT);
   digitalWrite(heatPin, HIGH); // turn on

   WiFi.mode(WIFI_AP);
   WiFi.softAP(ssid, password, 1, 1);
  
   server.begin();
}


//Timer 1 Interrupt
ISR(TIMER1_COMPA_vect) {//Timer 1 interrupt at 1Hz because of math in setup
//Transmits inner glove temperature, and regulates temperature

  fluctuate(request);
  client.print(getGloveTemp());

}

void setMode(String request) { //takes client request as input, returns 1 if turn on heating, 0 if not, -1 if anything else

  if(request.indexOf("/heat/on") != -1) mode =  1; //on
  else if (request.indexOf("/heat/off") != -1) mode = 0; //off
  else if(request.indexOf("/heat/auto") != -1) mode = 2; //auto
  
}

void setTemp (String request) { //read temperature input from user from app
  
  mode = setMode(request);

  if(mode == 1) { //in mode 1, input string will be of format "./heat/on?temp"

    userTemp = (int) request.substring(request.indexOf("?")+1);

  }
  else if(mode == 2) { //in mode 2, input string will be of format "./heat/auto/threshold=autoThreshold&temp=autoTemp

    String x = request.substring(request.indexOf("threshold=" + strlen("threshold="), request.indexOf("&"));//returns characters between end of "threshold=" and "&"
    autoThreshold =  x.toInt(); //converts to int

    String y = request.substring(request.indexOf("&")+strlen("temp="));//returns characters after "&"
    autoTemp = y.toInt(); 
  }
  
}

int getGloveTemp() { //Read ambient temperature from sensor on inside of glove

  int a = analogRead(gloveTempPin);

  float R = 1023.0/a-1.0;
  R = R0*R;

  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15; // convert to temperature via datasheet

  return (int) lround(temperature);
  
}


void fluctuate (String request) { //check temperature reading vs set reading. If lower, heat up. If higher, turn off

  mode = setMode(request);
  setTemp(request);
  int gloveTemp = getGloveTemp();

  if(mode == 1) { //if mode = on

      if(gloveTemp < userTemp) digitalWrite(enableCircuit, HIGH);
      else digitalWrite(enableCircuit, LOW);
    
  }
  else if(mode == 2) { //if mode = auto

    if(gloveTemp < autoThreshold) digitalWrite(enableCircuit, HIGH); //if less than threshold, turn on. If greater than threshold, dont change state (could be on and heating up or off and doing nothing)
    else if(gloveTemp) >= autoTemp) digitalWrite(enableCircuit, LOW); //if temp is higher than the auto temp, turn off
    
  }
  
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
  
//  // Send the response to the client //not necessary if sending data in interrupt
//  client.print(s);
//  delay(1);
//  Serial.println("Client disconnected");

  // The client will actually be disconnected when the function returns and the client object is destroyed
}
