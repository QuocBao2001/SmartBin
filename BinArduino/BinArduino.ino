#include <Servo.h>
#include <ArduinoJson.h>

//declare states of FSM
enum states {closing, send_infor, auto_open, control_open};

// declare servo object and its pin
Servo myServo;
int servoPin = 6;

// declare pin for 2 ultrasonic
int triggerPin2 = 7;
int echoPin2 = 8;

int triggerPin1 = 9;
int echoPin1 = 10;

// bin height
int binH = 30; //cm

// flag open by user
String openFlag = "";

// threshold distance
int thresDis = 50; //cm

states state;

long timeOpenning;
long curTime;

long lastTimeSend = millis();

long ctrlDelayTime = 5000;

float readUltrasonicDistance(int triggerPin, int echoPin)
{
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  // Reads the echo pin, and returns the sound wave travel time in microseconds
  long travelTime = pulseIn(echoPin, HIGH);
  // return distance in cm
  return 0.01723 * travelTime;
}

states closingState(){
  // get distance to user
  float userDis = readUltrasonicDistance(triggerPin1, echoPin1);

  if (userDis < thresDis) {
    // attach servo and set to 180 degree
    myServo.write(180);
    state = auto_open;

    // get Openning time
    timeOpenning = millis();
  }
  else if(Serial.available()){
    // read serial until end of line
    openFlag = Serial.readStringUntil('\n');
    // cast to int
    ctrlDelayTime = openFlag.toInt();
    // attach servo and set to 180 degree
    myServo.write(180);
    // get Openning time
    timeOpenning = millis();
    state = control_open;
  }
  else{
    // get current time
    curTime = millis();
    long delayTime = curTime - lastTimeSend;
    if ( delayTime > 2000){
      state = send_infor;    
    }
  }
  return state;
}

states sendInforState(){
  // get rubbish distance
  float rubDis = readUltrasonicDistance(triggerPin2, echoPin2);
  // count rubbish percent
  float rubPer = (1 - rubDis/binH)*100;
  float dis2user = readUltrasonicDistance(triggerPin1, echoPin1);
  StaticJsonDocument<200> doc;
  doc["rubPer"] = rubPer;
  doc["rubDis"] = dis2user;

  serializeJson(doc, Serial);
  lastTimeSend = millis();
  return closing;
}

states autoOpenState(){
  // get distance to user
  long userDis = readUltrasonicDistance(triggerPin1, echoPin1);
  if (userDis <= thresDis) {
    // if people still stand near me, reset openning time
    timeOpenning = millis();
  }  
  // get current time
  curTime = millis();
  long delayTime = curTime - timeOpenning;
  if ( delayTime > 2000){
    // attach servo and set to 0 degree
    myServo.write(0); 
    state = closing;    
  }
  return state;
}

states controlOpenState(){
  // get current time
  curTime = millis();
  if (curTime - timeOpenning >= ctrlDelayTime){
    // attach servo and set to 0 degree
    myServo.write(0);
    state = closing;
  }
  return state;
}

void setup(){
  state = closing;
  // attach servo and set to 0 degree
  myServo.attach(servoPin);
  myServo.write(0);  

  pinMode(triggerPin1, OUTPUT);
  pinMode(triggerPin2, OUTPUT);

  pinMode(echoPin1, INPUT);
  pinMode(echoPin2, INPUT);
  Serial.begin(115200);


  while (true){
    if (Serial.available()){
      String Msg = Serial.readStringUntil('\n');
      Serial.println(Msg);
      if (Msg.startsWith("Web server")){
        break;
      }
    }
  }



  Serial.println("finish setup");
}

void loop(){
  switch (state) {
    case closing:
      state = closingState();
      break;
    case send_infor:
      state = sendInforState();
      break;
    case auto_open:
      state = autoOpenState();
      break;
    case control_open:
      state = controlOpenState();
      break;
  }
}