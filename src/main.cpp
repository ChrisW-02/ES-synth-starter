#include <Arduino.h>
#include <U8g2lib.h>
#include <STM32FreeRTOS.h>
#include <iostream>
#include <ES_CAN.h>
#include <bitset>

// Constants
const uint32_t interval = 100; // Display update interval
volatile int32_t currentStepSize;
volatile uint8_t keyArray[7];
volatile uint8_t finArray[12];
const int sine[360] = {0, 2, 4, 6, 8, 11, 13, 15, 17, 20, 22, 24, 26, 28, 30, 33, 35, 37, 39, 41, 43, 45, 47, 50, 52, 54, 56, 58, 60, 62, 63, 65, 67, 69, 71, 73, 75, 77, 78, 80, 82, 83, 85, 87, 88, 90, 92, 93, 95, 96, 98, 99, 100, 102, 103, 104, 106, 107, 108, 109, 110, 111, 113, 114, 115, 116, 116, 117, 118, 119, 120, 121, 121, 122, 123, 123, 124, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 128, 127, 127, 127, 127, 127, 127, 127, 126, 126, 126, 125, 125, 124, 124, 123, 123, 122, 121, 121, 120, 119, 118, 117, 116, 116, 115, 114, 113, 111, 110, 109, 108, 107, 106, 104, 103, 102, 100, 99, 98, 96, 95, 93, 92, 90, 88, 87, 85, 83, 82, 80, 78, 77, 75, 73, 71, 69, 67, 65, 63, 62, 60, 58, 56, 54, 52, 50, 47, 45, 43, 41, 39, 37, 35, 33, 30, 28, 26, 24, 22, 20, 17, 15, 13, 11, 8, 6, 4, 2, 0, -2, -4, -6, -8, -11, -13, -15, -17, -20, -22, -24, -26, -28, -30, -33, -35, -37, -39, -41, -43, -45, -47, -50, -52, -54, -56, -58, -60, -62, -64, -65, -67, -69, -71, -73, -75, -77, -78, -80, -82, -83, -85, -87, -88, -90, -92, -93, -95, -96, -98, -99, -100, -102, -103, -104, -106, -107, -108, -109, -110, -111, -113, -114, -115, -116, -116, -117, -118, -119, -120, -121, -121, -122, -123, -123, -124, -124, -125, -125, -126, -126, -126, -127, -127, -127, -127, -127, -127, -127, -128, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -124, -123, -123, -122, -121, -121, -120, -119, -118, -117, -116, -116, -115, -114, -113, -111, -110, -109, -108, -107, -106, -104, -103, -102, -100, -99, -98, -96, -95, -93, -92, -90, -88, -87, -85, -83, -82, -80, -78, -77, -75, -73, -71, -69, -67, -65, -64, -62, -60, -58, -56, -54, -52, -50, -47, -45, -43, -41, -39, -37, -35, -33, -30, -28, -26, -24, -22, -20, -17, -15, -13, -11, -8, -6, -4, -2};
std::string keystrArray[7];
int time;
SemaphoreHandle_t keyArrayMutex;
volatile int32_t knob3Rotation = 4;
volatile int32_t knob2Rotation = 4;
// volatile uint8_t TX_Message[8] = {0};
uint8_t RX_Message[8] = {0};
QueueHandle_t msgInQ;

// Pin definitions
// Row select and enable
const int RA0_PIN = D3;
const int RA1_PIN = D6;
const int RA2_PIN = D12;
const int REN_PIN = A5;

// Matrix input and output
const int C0_PIN = A2;
const int C1_PIN = D9;
const int C2_PIN = A6;
const int C3_PIN = D1;
const int OUT_PIN = D11;

// Audio analogue out
const int OUTL_PIN = A4;
const int OUTR_PIN = A3;

// Joystick analogue in
const int JOYY_PIN = A0;
const int JOYX_PIN = A1;

// Output multiplexer bits
const int DEN_BIT = 3;
const int DRST_BIT = 4;
const int HKOW_BIT = 5;
const int HKOE_BIT = 6;

// Display driver object
U8G2_SSD1305_128X32_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Function to set outputs using key matrix
void setOutMuxBit(const uint8_t bitIdx, const bool value)
{
  digitalWrite(REN_PIN, LOW);
  digitalWrite(RA0_PIN, bitIdx & 0x01);
  digitalWrite(RA1_PIN, bitIdx & 0x02);
  digitalWrite(RA2_PIN, bitIdx & 0x04);
  digitalWrite(OUT_PIN, value);
  digitalWrite(REN_PIN, HIGH);
  delayMicroseconds(2);
  digitalWrite(REN_PIN, LOW);
}

uint8_t readCols()
{

  int C0 = digitalRead(C0_PIN);
  int C1 = digitalRead(C1_PIN);
  int C2 = digitalRead(C2_PIN);
  int C3 = digitalRead(C3_PIN);

  uint8_t res = 0;
  res |= (C0 << 3);
  res |= (C1 << 2);
  res |= (C2 << 1);
  res |= C3;

  return res;
}

int32_t fss(int frequency)
{
  int32_t step = pow(2, 32) * frequency / 22000;
  return step;
}

void setRow(uint8_t rowIdx)
{
  digitalWrite(REN_PIN, LOW);
  digitalWrite(RA0_PIN, bitRead(rowIdx, 0));
  digitalWrite(RA1_PIN, bitRead(rowIdx, 1));
  digitalWrite(RA2_PIN, bitRead(rowIdx, 2));
  digitalWrite(REN_PIN, HIGH);
}

int mapindex(uint8_t keypress, int rowindex)
{

  int index;

  if (keypress == 7)
  {
    index = 0 + rowindex * 4;
  }
  else if (keypress == 11)
  {
    index = 1 + rowindex * 4;
  }
  else if (keypress == 13)
  {
    index = 2 + rowindex * 4;
  }
  else if (keypress == 14)
  {
    index = 3 + rowindex * 4;
  }
  else
  {
    index = 12;
  }
  return index;
}

class Knob{
  public:
  uint8_t knob;
  uint8_t preknob;
  uint8_t knob_val;
  bool increment;
  int32_t rotation_var = 4;

  int32_t detectknob3rot()
  {

    uint8_t knob = 0;
    knob |= (bitRead(knob_val,2)<<1);
    knob |= bitRead(knob_val,3);  

    if ((knob == 01 && preknob ==00) || (knob == 10 && preknob == 11))
    {
      rotation_var = rotation_var + 1;
      increment = true;
    }
    else if ((knob == 00 && preknob == 01) || (knob == 11 && preknob == 10))
    {
      rotation_var = rotation_var - 1;
      increment = false;
    }
    else if ((knob == 11 && preknob == 00) || (knob == 10 && preknob == 01) || (knob == 01 && preknob == 10) || (knob == 00 && preknob == 11))
    {
      if (increment == true){
        rotation_var = rotation_var + 1;
        increment = true;
      }
      else{
        rotation_var = rotation_var - 1;
        increment = false;
      }
    }
    preknob = knob;
    return rotation_var = constrain(rotation_var, 0, 8);
  }

  int32_t detectknob2rot()
  {

    uint8_t knob = 0;
    knob |= (bitRead(knob_val,0)<<1);
    knob |= bitRead(knob_val,1);

    if ((knob == 01 && preknob ==00) || (knob == 10 && preknob == 11))
    {
      rotation_var = rotation_var + 1;
      increment = true;
    }
    else if ((knob == 00 && preknob == 01) || (knob == 11 && preknob == 10))
    {
      rotation_var = rotation_var - 1;
      increment = false;
    }
    else if ((knob == 11 && preknob == 00) || (knob == 10 && preknob == 01) || (knob == 01 && preknob == 10) || (knob == 00 && preknob == 11))
    {
      if (increment == true){
        rotation_var = rotation_var + 1;
        increment = true;
      }
      else{
        rotation_var = rotation_var - 1;
        increment = false;
      }
    }
    preknob = knob;
    return rotation_var = constrain(rotation_var, 0, 8);
  }
};

/*

int32_t detectknob3rot(uint8_t &preknob3, uint8_t keypress, bool &flag)
{
  uint8_t knob3 = 0;
  knob3 |= (bitRead(keypress, 2) << 1);
  knob3 |= bitRead(keypress, 3);
  if ((knob3 == 01 && preknob3 == 00) || (knob3 == 10 && preknob3 == 11))
  {
    knob3Rotation = knob3Rotation + 1;
    flag = true;
  }
  else if ((knob3 == 00 && preknob3 == 01) || (knob3 == 11 && preknob3 == 10))
  {
    knob3Rotation = knob3Rotation - 1;
    flag = false;
  }
  else if ((knob3 == 11 && preknob3 == 00) || (knob3 == 10 && preknob3 == 01) || (knob3 == 01 && preknob3 == 10) || (knob3 == 00 && preknob3 == 11))
  {
    if (flag == true)
    {
      knob3Rotation = knob3Rotation + 1;
      flag = true;
    }
    else
    {
      knob3Rotation = knob3Rotation - 1;
      flag = false;
    }
  }
  preknob3 = knob3;
  return knob3Rotation = constrain(knob3Rotation, 0, 8);
}
*/

void sampleISR()
{
  static uint32_t phaseAcc[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
  const int32_t keys[13] = {fss(261.63), fss(277.18), fss(293.66), fss(311.13), fss(329.63), fss(349.23), fss(369.99), fss(392), fss(415.3), fss(440), fss(466.16), fss(493.88), 0};
  int32_t Vfin = 0;
  for (int i=0; i<12; i++){
      int temp = finArray[i];
      if(temp==0){
        int32_t currStepsize = keys[i];
        if(knob2Rotation>4){
          currStepsize = currStepsize << (knob2Rotation-4);
        }
        else if(knob2Rotation<4){
          currStepsize = currStepsize >> (4-knob2Rotation);
        }
        
        phaseAcc[i] += currStepsize; //sawtooth
        int32_t Vout = (phaseAcc[i] >> 24) - 128;
        Vout = Vout >> (8 - knob3Rotation);
        Vfin += Vout;

        // for sine wave
        // int idx = (((keys[i]<<1)*time)>>22)%360;
        // Vfin += sine[idx];
        // (Vfin + 128)>> 25>> (8 - knob3Rotation)
      }
    }
  analogWrite(OUTR_PIN, Vfin + 128);
  // time += 1;
}


void scanKeysTask(void *pvParameters)
{
  const TickType_t xFrequency = 20 / portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();
  uint8_t prestate2, prestate3;
  
  Knob knob3;
  knob3.preknob = prestate3;
  knob3.increment = false;
  
  Knob knob2;
  knob2.preknob = prestate2;
  knob2.increment = false;

  bool flag = false;
  uint8_t TX_Message[8] = {0};
  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
    const int32_t keys[13] = {fss(261.63), fss(277.18), fss(293.66), fss(311.13), fss(329.63), fss(349.23), fss(369.99), fss(392), fss(415.3), fss(440), fss(466.16), fss(493.88), 0};
    // volatile int32_t currentStepSize;
    // uint8_t keyArray[7];

    int32_t stepSizes;
    int32_t prestepSizes;
    int32_t localknob3; //for volumn
    int32_t localknob2; //for octave
    bool press_key;
    press_key = false;
    int preindex;
    // uint8_t TX_Message[8] = {0};


    for (int i = 0; i < 4; i++)
    {
      uint8_t rowindex = i;
      setRow(rowindex);
      delayMicroseconds(3);
      keyArray[i] = readCols();
      // std::bitset<4> KeyBits(keys);
      // std::string keystr = KeyBits.to_string();
      // // change the name of this
      // keystrArray[i] = keystr;
    }


    // getting finArray
    uint32_t temp1 = keyArray[0];
    uint32_t temp2 = keyArray[1];
    uint32_t temp3 = keyArray[2];
    uint32_t tmp = temp3;
    for (int i=0; i<12; i++){
      if(i == 4){
        tmp = temp2;
      }
      else if(i == 8){
        tmp = temp1;
      }
      finArray[11-i] = tmp %2;
      tmp = tmp >> 1;
    }    


    for (int i = 0; i < 4; i++)
    {
      xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
      uint8_t keypresse = keyArray[i];

      xSemaphoreGive(keyArrayMutex);
      if (mapindex(keypresse, i) != 12 && press_key == false)
      {
        stepSizes = keys[mapindex(keypresse, i)];
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 12)
      {
        press_key = false;
      }
      if (keyArray[0] == 15 && keyArray[1] == 15 && keyArray[2] == 15)
      {
        press_key = false;
        stepSizes = 0;
      }


      if ((prestepSizes != stepSizes) && press_key == true)
      {
        TX_Message[0] = 'P';
        TX_Message[1] = 4;
        TX_Message[2] = mapindex(keypresse, i);
        preindex = mapindex(keypresse, i);
      }
      if ((prestepSizes != stepSizes) && press_key == false)
      {
        TX_Message[0] = 'R';
        TX_Message[1] = 4;
        TX_Message[2] = preindex;
      }
      __atomic_store_n(&currentStepSize, stepSizes, __ATOMIC_RELAXED);
      prestepSizes = stepSizes;
      if (i == 3)
      {
        xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
        knob3.knob_val = keyArray[i];
        knob2.knob_val = keyArray[i];
        //knob3.readknob();
        localknob3 = knob3.detectknob3rot();  
        localknob2 = knob2.detectknob2rot(); 
        prestate3 = knob3.preknob;    
        prestate2 = knob2.preknob; 
        //localknob3 = detectknob3rot(prestate, keypresse, flag);
        xSemaphoreGive(keyArrayMutex);
        __atomic_store_n(&knob3Rotation, localknob3, __ATOMIC_RELAXED);
        __atomic_store_n(&knob2Rotation, localknob2, __ATOMIC_RELAXED);
      }
    }
    CAN_TX(0x123, TX_Message);
  }
}

void displayUpdateTask(void *pvParameters)
{

  const TickType_t xFrequency = 100 / portTICK_PERIOD_MS;
  TickType_t xLastWakeTime = xTaskGetTickCount();

  while (1)
  {
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

    bool press_key;
    press_key = false;
    uint32_t ID;
    //uint8_t RX_Message[8] = {0};

    u8g2.clearBuffer();                 // clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

    for (int i = 0; i < 3; i++)
    {
      xSemaphoreTake(keyArrayMutex, portMAX_DELAY);
      uint8_t keypresse = keyArray[i];
      u8g2.setCursor(2, 20);
      u8g2.print(knob3Rotation, HEX);
      u8g2.setCursor(2, 10);
      u8g2.print(knob2Rotation, HEX);
      xSemaphoreGive(keyArrayMutex);
      if (mapindex(keypresse, i) == 0 && press_key == false)
      {
        u8g2.drawStr(2, 30, "C");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 1 && press_key == false)
      {
        u8g2.drawStr(2, 30, "C#");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 2 && press_key == false)
      {
        u8g2.drawStr(2, 30, "D");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 3 && press_key == false)
      {
        u8g2.drawStr(2, 30, "D#");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 4 && press_key == false)
      {
        u8g2.drawStr(2, 30, "E");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 5 && press_key == false)
      {
        u8g2.drawStr(2, 30, "F");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 6 && press_key == false)
      {
        u8g2.drawStr(2, 30, "F#");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 7 && press_key == false)
      {
        u8g2.drawStr(2, 30, "G");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 8 && press_key == false)
      {
        u8g2.drawStr(2, 30, "G#");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 9 && press_key == false)
      {
        u8g2.drawStr(2, 30, "A");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 10 && press_key == false)
      {
        u8g2.drawStr(2, 30, "A#");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 11 && press_key == false)
      {
        u8g2.drawStr(2, 30, "B");
        press_key = true;
      }
      else if (mapindex(keypresse, i) == 12 && press_key == false)
      {
        u8g2.drawStr(2, 30, " ");
        press_key = false;
      }

      // __atomic_store_n(&currentStepSize, stepSizes, __ATOMIC_RELAXED);
      // String display = mapkey(mapindex(keyArray,i));
      // u8g2.print(display);
      // u8g2.print(mapkey(mapindex(keyArray[i],i)));
      
    }
    // for(int j=0;j++;j<12){
    //     std::cout<<"test"<<std::endl;
    //     std::cout << finArray[j] << ",";
    // }
    // int test1 = finArray[4];
    // std::cout << test1 << std::endl;
    while (CAN_CheckRXLevel())
    u8g2.setCursor(66, 30);
    u8g2.print((char)RX_Message[0]);
    u8g2.print(RX_Message[1]);
    u8g2.print(RX_Message[2]);
    u8g2.sendBuffer();

    // Toggle LED
    digitalToggle(LED_BUILTIN);
  }
}
  void CAN_RX_ISR(void)
  {
    uint8_t RX_Message_ISR[8];
    uint32_t ID;
    CAN_RX(ID, RX_Message_ISR);
    xQueueSendFromISR(msgInQ, RX_Message_ISR, NULL);
  }

void decodeText(void *pvParameters)
{
  while (1)
  {
  xQueueReceive(msgInQ, RX_Message, portMAX_DELAY);
  const int32_t keys[13] = {fss(261.63), fss(277.18), fss(293.66), fss(311.13), fss(329.63), fss(349.23), fss(369.99), fss(392), fss(415.3), fss(440), fss(466.16), fss(493.88), 0};
  int32_t stepSizes;
  if(RX_Message[0] == 'P'){
    stepSizes = keys[RX_Message[2]];
  }
  else if(RX_Message[0] == 'R'){
    stepSizes = 0;
  }
  
  }
}

void setup()
{
  // put your setup code here, to run once:

  // Set pin directions
  pinMode(RA0_PIN, OUTPUT);
  pinMode(RA1_PIN, OUTPUT);
  pinMode(RA2_PIN, OUTPUT);
  pinMode(REN_PIN, OUTPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(OUTL_PIN, OUTPUT);
  pinMode(OUTR_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(C0_PIN, INPUT);
  pinMode(C1_PIN, INPUT);
  pinMode(C2_PIN, INPUT);
  pinMode(C3_PIN, INPUT);
  pinMode(JOYX_PIN, INPUT);
  pinMode(JOYY_PIN, INPUT);

  // Initialise display
  setOutMuxBit(DRST_BIT, LOW); // Assert display logic reset
  delayMicroseconds(2);
  setOutMuxBit(DRST_BIT, HIGH); // Release display logic reset
  u8g2.begin();
  setOutMuxBit(DEN_BIT, HIGH); // Enable display power supply

  // Initialise UART
  Serial.begin(9600);
  Serial.println("Hello World");

  // Create the mutex and assign its handle
  keyArrayMutex = xSemaphoreCreateMutex();
  // initialise queue handler
  msgInQ = xQueueCreate(36, 8);

  // initialise CAN
  CAN_Init(true);
  CAN_RegisterRX_ISR(CAN_RX_ISR);
  setCANFilter(0x123, 0x7ff);
  CAN_Start();

  // creat a timer
  TIM_TypeDef *Instance = TIM1;
  HardwareTimer *sampleTimer = new HardwareTimer(Instance);
  sampleTimer->setOverflow(22000, HERTZ_FORMAT);
  sampleTimer->attachInterrupt(sampleISR);
  sampleTimer->resume();

  TaskHandle_t scanKeysHandle = NULL;
  xTaskCreate(
      scanKeysTask,     /* Function that implements the task */
      "scanKeys",       /* Text name for the task */
      64,               /* Stack size in words, not bytes */
      NULL,             /* Parameter passed into the task */
      1,                /* Task priority */
      &scanKeysHandle); /* Pointer to store the task handle */

  TaskHandle_t displayHandle = NULL;
  xTaskCreate(
      displayUpdateTask,   /* Function that implements the task */
      "displayupdatetask", /* Text name for the task */
      256,                 /* Stack size in words, not bytes */
      NULL,                /* Parameter passed into the task */
      1,                   /* Task priority */
      &displayHandle);     /* Pointer to store the task handle */

  vTaskStartScheduler();
}

void loop()
{
  // put your main code here, to run repeatedly:

  // static uint32_t next = millis();
  // static uint32_t count = 0;

  // if (millis() > next) {
  //   next += interval;

  //   //Update display
  //   u8g2.clearBuffer();         // clear the internal memory
  //   u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font

  //   uint32_t* k = NULL;

  //   scanKeysTask(k);

  //   u8g2.sendBuffer();
  //   //currentStepSize = stepSizes[index];

  //   //Toggle LED
  //   digitalToggle(LED_BUILTIN);
  // }
}