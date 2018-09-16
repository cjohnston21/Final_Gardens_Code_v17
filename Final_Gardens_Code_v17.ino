// Libraries
#include<OneWire.h>
#include<DallasTemperature.h>
#include <Adafruit_CharacterOLED.h>


//PINS

// Temp Sensor Pins
#define ONE_WIRE_BUS 53
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// LCD Pins
const int rs = 23, rw = 25, en = 27, d4 = 29, d5 = 31, d6 = 33, d7 = 35;
Adafruit_CharacterOLED lcd(OLED_V2, rs, rw, en, d4, d5, d6, d7);

// UI Button Pins
const int buttonPinHome = 37; 
const int buttonPinSelect = 39; 
const int buttonPinDown = 41; 
const int buttonPinUp = 43;

// Relay Pins
const int switchPinPowL = 12;
const int switchPinDirL = 11;
const int switchPinPowR = 10;
const int switchPinDirR = 9;

// Hall Sensor Pins
const int hallSensorLBot = 51;
const int hallSensorLTop = 49;
const int hallSensorRBot = 47;
const int hallSensorRTop = 45;

// Piezo Speaker Pin
const int buzzerPin = 52;

// Current Sensor Pin
int ampPin = A9;

//Board T Sensor Pin;
int brdTS = A8;


// GLOBAL VARIABLES

// Temperature Variables
int tempThreshold = 80;
int currentTempUI;
int hysteresis = 3;

// Booleans for moving the winch
boolean lidOpenL = false;
boolean isMovingL = false;
boolean lidOpenR = false;
boolean isMovingR = false;
boolean RIGHT = true;
boolean LEFT = false;
boolean UP = true;
boolean DOWN = false;
boolean manOv = false;
double maxAmps = 7.0;
int maxBoardTemp = 100;

// UI Variables
boolean screenOff = false;
int screenState = 0;
boolean subScreen = false;
boolean whichWinch = LEFT;
int numScreens = 6;
boolean autoMode = true;


// Hall Sensor Vars
int lastHallSensorLBot = HIGH;
int currentHallSensorLBot = HIGH;

int lastHallSensorLTop = HIGH;
int currentHallSensorLTop = HIGH;

int lastHallSensorRBot = HIGH;
int currentHallSensorRBot = HIGH;

int lastHallSensorRTop = HIGH;
int currentHallSensorRTop = HIGH;

// Button States
boolean lastButtonStateHome = LOW;
boolean currentButtonStateHome = LOW;

boolean lastButtonStateSelect = LOW;
boolean currentButtonStateSelect = LOW;

boolean lastButtonStateDown = LOW;
boolean currentButtonStateDown = LOW;

boolean lastButtonStateUp = LOW;
boolean currentButtonStateUp = LOW;

// Time Variables
unsigned long previousMillis = 0;
unsigned long currentMillis = millis();
int intervalUI = 3000000;
int intervalThresh = 10000;
unsigned long previousMillis2 = 0;
unsigned long currentMillis2 = millis();
int maxOpTime = 60000;

// SETUP
void setup() {
  sensors.begin();

  lcd.begin(16, 2);
  lcd.clear();
  lcd.noDisplay();
  lcd.display();
  loadingScreen();
  lcd.clear();
  
  currentTempUI = getTemp();
  
  pinMode(switchPinPowL, OUTPUT);
  pinMode(switchPinDirL, OUTPUT);
  //pinMode(switchPinPowR, OUTPUT);
  //pinMode(switchPinDirR, OUTPUT);

  pinMode(buttonPinHome, INPUT);
  pinMode(buttonPinSelect, INPUT);
  pinMode(buttonPinDown, INPUT);
  pinMode(buttonPinUp, INPUT);

  pinMode(hallSensorLBot, INPUT);
  pinMode(hallSensorLTop, INPUT);
  //pinMode(hallSensorRBot, INPUT);
  //pinMode(hallSensorRTop, INPUT);

  pinMode(buzzerPin, OUTPUT);
  //pinMode(ampPin, INPUT);

 
  
  Serial.begin(9600);
  updateButtonAndHallSensorVars();
  int buttonPressed = checkButtons();
  determineUIScreen(buttonPressed);
  runUIScreen(buttonPressed);
  lidOpenL = currentHallSensorLBot!=LOW;
  /*
   * Dr. G recommended closing frame if open from start.
   * Note if after this lidOpenL still isn't reading we should send an error to the screen
  if(lidOpenL)
  {
    manOv = false;
    makeWarningSound();
    moveLeftWinch(DOWN);
  }
  */
}

// LOOP
void loop()
{
      updateButtonAndHallSensorVars();
      int buttonPressed = checkButtons();
      determineUIScreen(buttonPressed);
      runUIScreen(buttonPressed);

      
      if(screenState<1)
        {
           lidOpenR = lidOpenL;
           if(!lidOpenL && !lidOpenR && (currentHallSensorLBot!=LOW)) //|| currentHallSensorRBot!=LOW))
            {
              runErrorProtocolLidState();
              return;
            }
            else if(lidOpenL && lidOpenR && (currentHallSensorLTop!=LOW)) //|| currentHallSensorRTop!=LOW))
            {
              runErrorProtocolLidState();
              return;
            }
          
          int action = checkIfItIsTime();
          int temp;
          if (action!=0)
            temp = getTemp();
          if (action%2==1)
            currentTempUI = temp;
          if (action>=2)
            {
              int winchAction = checkIfWinchShouldMove(temp);
              if(winchAction == 1)
                {
                  makeWarningSound();
                  moveWinchAuto(LEFT, DOWN);
                  //delay(100);
                  //moveWinchAuto(RIGHT, DOWN);
                }
              else if(winchAction == 2)
                {
                  makeWarningSound();
                  moveWinchAuto(LEFT, UP);
                  //delay(100);
                  //moveWinchAuto(RIGHT, UP);
                }
            
            }
        }

        
}

/*
 * FUNCTION: Updates UI button vars and hall sensor vars
 * Inputs: none
 * Outputs: none
 * Global Vars Updated: hall sensor vars, button vars
 */
void updateButtonAndHallSensorVars()
{
  lastHallSensorLBot = currentHallSensorLBot;
  lastHallSensorLTop = currentHallSensorLTop;
  lastHallSensorRBot = currentHallSensorRBot;
  lastHallSensorRTop = currentHallSensorRTop;


  lastButtonStateHome = currentButtonStateHome;
  lastButtonStateSelect = currentButtonStateSelect;
  lastButtonStateDown = currentButtonStateDown;
  lastButtonStateUp = currentButtonStateUp;

  
  currentHallSensorLBot = digitalRead(hallSensorLBot);
  currentHallSensorRBot = digitalRead(hallSensorRBot);
  currentHallSensorLTop = digitalRead(hallSensorLTop);
  currentHallSensorRTop = digitalRead(hallSensorRTop);
  

  currentButtonStateHome = debounce(lastButtonStateHome, buttonPinHome);
  currentButtonStateSelect = debounce(lastButtonStateSelect, buttonPinSelect);
  currentButtonStateDown = debounce(lastButtonStateDown, buttonPinDown);
  currentButtonStateUp = debounce(lastButtonStateUp, buttonPinUp);
}

/*
 * FUNCTION: Debounces button passed to it
 * INPUTS: boolean last (last state of button), int button (button pin)
 * OUTPUT: boolean current (current state of button)
 * Global Vars Updated: none
*/
boolean debounce(boolean last, int button) {
  boolean current = digitalRead(button);
  if (last != current)
  {
    delay(10);
    current = digitalRead(button);
  }
  return current;
}


// UI LOGIC

/*
 * FUNCTION: if-tree determining which button was pushed
 * Inputs: none
 * Outputs: integer representing button 0 - nothing/mulitple, 1 - H, 2 - S, 3 - D, 4 - U
 * Global Vars Updated: none
 */
 int checkButtons()
 {
   int multiple = 0;
   int button = 0;
   if(currentButtonStateHome==HIGH && lastButtonStateHome==LOW)
     {
        button=1;
        multiple+=1;
     }
   if(currentButtonStateSelect==HIGH && lastButtonStateSelect==LOW)
      {
        button=2;
        multiple+=2;
     }
   if(currentButtonStateDown==HIGH && lastButtonStateDown==LOW)
      {
        button=3;
        multiple+=3;
      }
   if(currentButtonStateUp==HIGH && lastButtonStateUp==LOW)
      {
        button=4;
        multiple+=4;
      }
      
   if(button==multiple)
      return button;
   else
      return 0;
 }


/*
 * FUNCTION: if-tree determining and executing actions based on UI button variables
 * Inputs: none
 * Outputs: none
 * Global Vars Updated: screenOnOffCount, subScreen, tempThreshold, hysteresis, manOv, whichWinch, screenState
 */
void determineUIScreen(int button) 
{
  if(button==0)
    return;
  if(screenState == -1)
  {
    if(button==1)
      screenState=0;
  }
  else if (screenState==0)
  {
    if(button==1)
      screenState= -1;
    else if (button==2)
      screenState = 1;
  }
  else if(screenState>0 && subScreen == false)
    {
        if (button==1) 
            screenState = 0;
        else if (button==2 && screenState!=4)
            subScreen = true;
        else if (button==3)
        {
           screenState--;
           if(screenState<1)
              screenState=numScreens-1;
        }  
        else if (button==4)
        {
            if(screenState == numScreens - 1)
              screenState = 1;
            else
              screenState++;
        }
    }
  delay(100);
}

/*
 * FUNCTION: if-tree running the screen the User Interface Displays
 * Inputs: none
 * Outputs: none
 * Global Vars Updated: none
 */
void runUIScreen(int button)
{
  if (screenState == -1)
    lcd.noDisplay();
  else if (screenState == 0)
    {
      lcd.display();
      runHomeScreen();
    }
  else if (screenState == 1)
    {
      if(subScreen)
        runChangeTempThresh(button);
      else
        runTempThreshScreen();
    }
  else if (screenState == 2)
    {
      if(subScreen)
        runManualMode(button);
      else
        runManualModeScreen();
    }
  else if (screenState == 3)
    {
      if(subScreen)
        runChangeHysteresis(button);
      else
        runHysteresisScreen();
    } 
  else if (screenState == 4)
     runLidStateScreen();
  else if (screenState == 5)
    {
      if(subScreen)
        runChangeMaxAmps(button);
      else
        runChangeMaxAmpsScreen();
    }
}


// UI SCREENS

void loadingScreen()
{
  lcd.setCursor(0, 0);
  lcd.print(" Cold Frames UI ");
  lcd.setCursor(0, 1);
  lcd.print("   Loading      ");
  lcd.setCursor(10, 1);
  
  for (int i = 0; i < 3; i++)
  {
    for (int i = 0; i < 3; i++)
    {
      lcd.setCursor(i + 10, 1);
      lcd.print(".");
      delay(500);
    }
    lcd.setCursor(10, 1);
    lcd.print("   ");
    delay(500);
  }
}

void runHomeScreen()
{
  lcd.setCursor(0,0);
  lcd.print("Automatic Mode  ");
  lcd.setCursor(0,1);
  lcd.print("Temp: " + String(currentTempUI) + " F");
  /*
  lcd.setCursor(0, 0);
  lcd.print("Temperature:    ");
  lcd.setCursor(0, 1);
  lcd.print(String(currentTempUI) + " degrees       ");
  */
}

void runTempThreshScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Set Temperature ");
  lcd.setCursor(0, 1);
  lcd.print("Threshold       ");
}

void runChangeTempThresh(int button)
{
  if(button==1)
    subScreen=false;
  else if (button==3)
    tempThreshold--;
  else if (button==4)
    tempThreshold++;
    
  lcd.setCursor(0, 0);
  lcd.print("Temp. Threshold:");
  lcd.setCursor(0, 1);
  lcd.print(String(tempThreshold) + " F             ");
}

void runManualModeScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Manual Mode     ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}

void runManualMode(int button)
{
  if(button==1)
    subScreen=false;
  else if (button==2)
    whichWinch = !whichWinch;
  else if (button==3)
    moveWinchManual(whichWinch, DOWN);
  else if (button==4)
    moveWinchManual(whichWinch, UP);

  lcd.setCursor(0, 0);
  lcd.print("Controlling:    ");
  lcd.setCursor(0, 1);
  if (whichWinch == LEFT)
    lcd.print("Left Winch      ");
  else
    lcd.print("Right Winch     ");
}

void runHysteresisScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Change Buffer   ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void runChangeHysteresis(int button)
{
  if(button==1)
    subScreen=false;
  else if (button==3)
    {
      hysteresis--;
      if (hysteresis<0)
        hysteresis=0;
    }
  else if (button==4)
    hysteresis++;
    
  lcd.setCursor(0, 0);
  lcd.print("Buffer:          ");
  lcd.setCursor(0, 1);
  lcd.print(String(hysteresis) + " degrees       ");
}

void runLidStateScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Lid State:      ");
  lcd.setCursor(0, 1);
  if(lidOpenL==true)
    lcd.print("Open            ");
  else
    lcd.print("Closed          ");
}

void runChangeMaxAmpsScreen()
{
  lcd.setCursor(0, 0);
  lcd.print("Change Max Amps ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void runChangeMaxAmps(int button)
{
  if(button==3)
    maxAmps=maxAmps-0.25;
  else if (button==4)
    maxAmps=maxAmps+0.25;
  else if (button==1)
    subScreen = false;
    
  lcd.setCursor(0, 0);
  lcd.print("Max Amps Allowed");
  lcd.setCursor(0, 1);
  lcd.print(String(maxAmps) + " amps           ");
}

void runErrorProtocolLidState()
{
  lcd.setCursor(0, 0);
  lcd.print("ERROR: LID STATE");
  lcd.setCursor(0,1);
  lcd.print("!= FRAME SENSOR ");
  updateButtonAndHallSensorVars();
  
  while(currentButtonStateHome==LOW)
  {
    updateButtonAndHallSensorVars();
    
    if(currentHallSensorLBot==LOW && !lidOpenL && !lidOpenR)
      break;
    if(currentHallSensorLTop==LOW && lidOpenL && lidOpenR)
        break;
  }

  if(currentButtonStateHome==HIGH && lastButtonStateHome==LOW)
    screenState=1;
  
  
  delay(100);
}

void runErrorProtocolMaxAmps()
{
  lcd.setCursor(0, 0);
  lcd.print("ERROR: MAX AMPS ");
  lcd.setCursor(0,1);
  lcd.print("THING ON FRAME? ");
  while(currentButtonStateHome==LOW)
  {
    updateButtonAndHallSensorVars();
  }

  screenState=1;
  

  delay(100);
}





// LOGIC FOR CONTROLLING WINCH

/*
 * FUNCTION: if-tree running the screen the User Interface Displays
 * Inputs: none
 * Outputs: int action (can be 0,1,2,3 and determines if UI temp should be updated or if the temp Threshold should be checked)
 * Global Vars Updated: currentMillis, currentMillis2, previousMillis, previousMillis2
 */
int checkIfItIsTime() 
{
  currentMillis = millis();
  currentMillis2 = currentMillis;
  int action = 0;
  
  if ((unsigned long)(currentMillis - previousMillis) >= intervalUI) 
  {
    previousMillis = currentMillis;
    action += 1;
  }
  
  if ((unsigned long)(currentMillis2 - previousMillis2) >= intervalThresh) 
  {
    previousMillis2 = currentMillis2;
    action += 2;
  } 
  return action;
}

/*
 * FUNCTION: Records Temperatures from T sensors. 
 * Inputs: none
 * Outputs: int tempF (temp. in fahrenheit)
 * Global Vars Updated: none
*/
int getTemp()
{
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");

  // Gets Temperature Reading From 0th Temperature Sensor
  double t1 = sensors.getTempCByIndex(0);
  
  int tempF = convertTempToF(t1);
  
  return tempF;
}

/* 
 * FUNCTION: Converts Celsius to Fahrenheit
 * Inputs: double tempC (temp, in C)
 * Outputs: int T (temp in F)
 * Global Vars Updated: none
*/
int convertTempToF(double tempC)
{
  int T = (int)(tempC * 9.0 / 5.0 + 32);
  return T;
}

/*
 * FUNCTION: Checks to see if Temperature is Above/Below Threshold and calls makeWarningSound() and the moveWinch() methods
 * Inputs: int temp (current temp. read by the T sensors)
 * Outputs: none
 * Global Vars Updated: whichWinch
*/
int checkIfWinchShouldMove(int temp)
{
  if (abs(tempThreshold - temp) > hysteresis && temp < tempThreshold && lidOpenL)// && lidOpenR)
      return 1;
  else if (abs(tempThreshold - temp) > hysteresis && temp > tempThreshold && !lidOpenL)// && !lidOpenR)
      return 2;
 
  return 0;
}

/*
 * FUNTION: Tones the buzzer down by the cold frames to alert surrounding people that the winch is about to move
 * Inputs: none
 * Outputs: none
 * Global Vars Updated: none
 */
void makeWarningSound()
{
  tone(buzzerPin, 4000, 500);
  delay(1000);
  tone(buzzerPin, 4000, 500);
  delay(1000);
  tone(buzzerPin, 4000, 500);
  delay(250);
}

/*
 * FUNCTION: Controls relays to move the winches during manual mode
 * Inputs: boolean dir (DOWN or UP), boolean winch (which winch we are controling) 
 * Outpus: none
 * Global Vars Updated: isMovingL, openLidL, updateButtonAndHallSensorVars(), openLidR, isMovingL
 */
void moveWinchManual(boolean winch, boolean dir)
{
  if(winch==LEFT)
  {
        digitalWrite(switchPinDirL, dir);
        digitalWrite(switchPinPowL, HIGH);
        isMovingL = true;
     
        while ((currentButtonStateUp == HIGH && dir == UP) || (currentButtonStateDown == HIGH && dir == DOWN && currentHallSensorLBot!=LOW))
        {
          updateButtonAndHallSensorVars();
     
          if(measureCurrent()>maxAmps)
            break;
          /*
          if(mainBoardTemp()>maxBoardTemp)
            break;
           */
          //Serial.println(getTemp());
          Serial.print("Amps: ");
          Serial.println(measureCurrent());
        }
        if (lidOpenL && currentHallSensorLBot==LOW && dir==DOWN) 
          delay(2000);
          
        digitalWrite(switchPinPowL, LOW);
        digitalWrite(switchPinDirL, LOW);
        isMovingL = false;
        lidOpenL = currentHallSensorLBot!=LOW;
  }
/*
 else
 {
    digitalWrite(switchPinDirR, dir);
    digitalWrite(switchPinPowR, HIGH);
    isMovingR = true;
    unsigned long winchStartTime = millis();
    unsigned long winchCurrentOpTime = winchStartTime;
    
    while (((currentButtonStateUp == HIGH && dir == UP) || (currentButtonStateDown == HIGH && dir == DOWN && currentHallSensorRBot==HIGH) || !manOv) && (isMovingR && !checkHallSensors(dir)))
    {
      updateButtonAndHallSensorVars();
      
      unsigned long winchPrevDiff = abs(winchCurrentOpTime - winchStartTime);
      winchCurrentOpTime = millis();
      if(winchPrevDiff>abs(winchCurrentOpTime - winchStartTime))
        winchStartTime = -winchPrevDiff;
      
      if(!manOv && abs(winchCurrentOpTime - winchStartTime)>maxOpTime)
        isMovingR = false;

      //senseCurrent();
    }
    if (dir==DOWN) 
      delay(500);
    digitalWrite(switchPinPowR, LOW);
    digitalWrite(switchPinDirR, LOW);
    isMovingR = false;
    lidOpenR = currentHallSensorRBot!=LOW;
 }
*/
   
}

/*
 * FUNCTION: Controls relays to move winches during Automatic Mode
 * Inputs: boolean dir (DOWN or UP), boolean winch (which winch)
 * Outpus: none
 * Global Vars Updated: isMovingR, openLidR, updateButtonAndHallSensorVars()
 */
void moveWinchAuto(boolean winch, boolean dir)
{
  if(winch==LEFT)
  {
    digitalWrite(switchPinDirL, dir);
    digitalWrite(switchPinPowL, HIGH);
    unsigned long winchStartTime = millis();
    unsigned long winchCurrentOpTime = winchStartTime;
    //Serial.println(currentHallSensorLBot);
    String error = "";
 
    while ((currentHallSensorLTop != LOW && dir==UP) || (currentHallSensorLBot != LOW && dir==DOWN))
    {
      updateButtonAndHallSensorVars();
      
      winchCurrentOpTime = millis();
      
      if(abs(winchCurrentOpTime - winchStartTime)>maxOpTime)
        break;
      if(measureCurrent()>maxAmps)
        {
          error = "amps";
          break;
        }
      if(currentButtonStateHome==HIGH)
        {
          screenState=1;
          break;
        }
        
      /*
      if(mainBoardTemp()>maxBoardTemp)
        break;
       */
      //Serial.println(getTemp());
      Serial.print("Amps: ");
      Serial.println(measureCurrent());
    }
    if (lidOpenL && currentHallSensorLBot==LOW && dir==DOWN) 
        delay(2000);
      
    
    digitalWrite(switchPinPowL, LOW);
    digitalWrite(switchPinDirL, LOW);
    lidOpenL = currentHallSensorLBot!=LOW;
    if(error.equals("amps"))
    {
      runErrorProtocolMaxAmps();
    }
    
  }
  /*
  else
  {
    digitalWrite(switchPinDirR, dir);
    digitalWrite(switchPinPowR, HIGH);
    isMovingR = true;
        unsigned long winchStartTime = millis();
        unsigned long winchCurrentOpTime = winchStartTime;
        
        while (!manOv && currentHallSensorRTop == LOW && lastHallSensorRTop == HIGH && dir==UP) || (currentHallSensorRBot == LOW && lastHallSensorRBot == HIGH && dir==DOWN)
        {
          updateButtonAndHallSensorVars();
          
          unsigned long winchPrevDiff = abs(winchCurrentOpTime - winchStartTime);
          winchCurrentOpTime = millis();
          if(winchPrevDiff>abs(winchCurrentOpTime - winchStartTime))
            winchStartTime = -winchPrevDiff;
          
          if(!manOv && abs(winchCurrentOpTime - winchStartTime)>maxOpTime)
            isMovingR = false;
    
          //senseCurrent();
        }
        
        digitalWrite(switchPinPowR, LOW);
        digitalWrite(switchPinDirR, LOW);
        isMovingR = false;
        lidOpenR = currentHallSensorRBot!=LOW;
        delay(500);
  }
  */
}

/*
 * FUNCTION: Measures the current of the winches. Eventually incorporated into moveLeftWinch or moveRightWinch while statement
 * Inputs: none
 * Outputs: none
 * Global Vars Updated: none
 */ 
 double measureCurrent()
{
  int sensorValue = analogRead(ampPin);
  double amps = ((512 - sensorValue)*75.75/1023);
  return amps;
}
