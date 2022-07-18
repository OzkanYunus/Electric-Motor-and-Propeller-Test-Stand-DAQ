None#include <Servo.h>
#include <HX711_ADC.h>
#define VIN A0 // define the Arduino pin A0 as voltage input (V in)

String strIRVal = "";
String content = "";
String writeContent = "";
String strLCValue = "";
String strCValue = "";
String strVValue = "";
char character;

const int IRSensor = 3;
const int HX711_dout = 4;
const int HX711_sck = 5;
const int ESCSensor = 6;

int potValue = 0;
int irValue = 0;
float lcValue = 0;

unsigned long t = 0;

HX711_ADC LoadCell(HX711_dout, HX711_sck);
Servo ESC;

const float VCC = 5.0;// supply voltage 5V or 3.3V. If using PCB, set to 5V only.
const int model = 4;   // enter the model (see below)

float cutOffLimit = 1.00;// reading cutt off current. 1.00 is 1 Amper

/*
          "ACS758LCB-050B",// for model use 0
          "ACS758LCB-050U",// for model use 1
          "ACS758LCB-100B",// for model use 2
          "ACS758LCB-100U",// for model use 3
          "ACS758KCB-150B",// for model use 4
          "ACS758KCB-150U",// for model use 5
          "ACS758ECB-200B",// for model use 6
          "ACS758ECB-200U"// for model use  7   
sensitivity array is holding the sensitivy of the  ACS758
current sensors. Do not change.          
*/
float sensitivity[] ={
  40.0,// for ACS758LCB-050B
  60.0,// for ACS758LCB-050U
  20.0,// for ACS758LCB-100B
  40.0,// for ACS758LCB-100U
  13.3,// for ACS758KCB-150B
  16.7,// for ACS758KCB-150U
  10.0,// for ACS758ECB-200B
  20.0,// for ACS758ECB-200U     
}; 

/*         
 *   quiescent Output voltage is factor for VCC that appears at output       
 *   when the current is zero. 
 *   for Bidirectional sensor it is 0.5 x VCC
 *   for Unidirectional sensor it is 0.12 x VCC
 *   for model ACS758LCB-050B, the B at the end represents Bidirectional (polarity doesn't matter)
 *   for model ACS758LCB-100U, the U at the end represents Unidirectional (polarity must match)
 *    Do not change.
 */
float quiescent_Output_voltage [] ={
  0.5, // for ACS758LCB-050B
  0.12,// for ACS758LCB-050U
  0.5, // for ACS758LCB-100B
  0.12,// for ACS758LCB-100U
  0.5, // for ACS758KCB-150B
  0.12,// for ACS758KCB-150U
  0.5, // for ACS758ECB-200B
  0.12,// for ACS758ECB-200U            
};
const float FACTOR = sensitivity[model] / 1000; // set sensitivity for selected model
const float QOV = quiescent_Output_voltage [model] * VCC; // set quiescent Output voltage for selected model
float voltage = 0; // internal variable for voltage
float cutOff = FACTOR / cutOffLimit; // convert current cut off to mV

void setup() {
  Serial.begin(57600);

  // Sensors
  pinMode(IRSensor, INPUT);
  digitalWrite(IRSensor, HIGH);

  // Loadcell
  LoadCell.begin();
  float calibrationValue = 696.0;

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue);
  }

  // ESC
  ESC.attach(ESCSensor, 1000, 2000);
}

void loop() {
  if (Serial.available() > 0) {
    while(Serial.available()) {
      character = Serial.read();
      writeContent.concat(character);
    }

    int startIndex = writeContent.indexOf("<");
    int endIndex = writeContent.indexOf(">");

    if (startIndex == -1 || endIndex == -1) {
      
    } else {
      char command = writeContent.charAt(1);
      if (command == 'q') {   
        potValue = writeContent.substring(startIndex + 3, endIndex).toInt();
        Serial.print("I received: <q:" + String(potValue) + ">");      
      }
    }
    writeContent = "";
  }

  irValue = digitalRead(IRSensor);

  float voltage_raw = (5.0 / 1023.0) * analogRead(VIN); // Read the voltage from sensor
  voltage =  voltage_raw - QOV + 0.007 ; // 0.007 is a value to make voltage zero when there is no current
  float current = voltage / FACTOR;
  if(abs(voltage) > cutOff){
    //Serial.print("V: ");
    //Serial.print(voltage, 3);// print voltage with 3 decimal places
    //Serial.print("V, I: ");
    //Serial.print(current, 2); // print the current with 2 decimal places
    //Serial.println("A");
  } else{
    //Serial.println("No Current");
  }

  int escMapValue = map(potValue, 0, 1023, 0, 180);
  ESC.write(escMapValue);

  strIRVal = String(irValue);
  strVValue = String(voltage);
  strCValue = String(current);
  
  static boolean newDataReady = 0;
  const int serialPrintInterval = 0; // Increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      lcValue = LoadCell.getData();
      strLCValue = String(lcValue);
      newDataReady = 0;
      t = millis();
    }
  }

  content = "<IR:" + strIRVal + ",LC:" + strLCValue + ",V:" + strVValue + ",C:" + strCValue + ">";
  Serial.print(content);
  content = "";
}
