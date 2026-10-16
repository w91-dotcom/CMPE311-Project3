#include <Arduino.h>
#include <limits.h>


/* Custom register definitions for the ATmega328P */
#define MY_TCCR2A (*(volatile unsigned char *)0xB0)
#define MY_TCCR2B (*(volatile unsigned char *)0xB1)
#define MY_TCNT2  (*(volatile unsigned char *)0xB2)
#define MY_OCR2A  (*(volatile unsigned char *)0xB3)
#define MY_TIMSK2 (*(volatile unsigned char *)0x70)
#define MY_TIFR2  (*(volatile unsigned char *)0x37)
#define MY_SREG   (*(volatile unsigned char *)0x5F)


volatile unsigned long myMilliCounter = 0;


int selectedLED = -1;


unsigned long blinkRateOne = 0;
unsigned long blinkRateTwo = 0;


unsigned long blinkTrackerOne = 0;
unsigned long blinkTrackerTwo = 0;


unsigned long inputValue = 0;
int inputStarted = 0;
int inputInvalid = 0;


/* Custom Timer2 compare-match interrupt */
extern "C" void __vector_7(void)
    __attribute__((signal, used, externally_visible));


void __vector_7(void)
{
  myMilliCounter++;
}


/* Returns a copy of the millisecond */
unsigned long myMilli(void)
{
  unsigned char savedStatus;
  unsigned long currentTime;


  savedStatus = MY_SREG;
  __asm__ __volatile__("cli" ::: "memory");


  currentTime = myMilliCounter;


  MY_SREG = savedStatus;
  __asm__ __volatile__("" ::: "memory");


  return currentTime;
}


/* Configures Timer2 to interrupt once every millisecond */
void setupMyMilli(void)
{
  unsigned char savedStatus;


  savedStatus = MY_SREG;
  __asm__ __volatile__("cli" ::: "memory");


  MY_TCCR2B = 0;       /* Stop Timer2 */
  MY_TIMSK2 = 0;       /* Disable Timer2 interrupts */
  MY_TCCR2A = 0x02;    /* Clear timer on compare match */
  MY_TCNT2 = 0;
  MY_OCR2A = 249;      /* 250 counts per interrupt */
  MY_TIFR2 = 0x07;     /* Clear old interrupt flags */


  myMilliCounter = 0;


  MY_TIMSK2 = 0x02;    /* Enable compare-match A interrupt */
  MY_TCCR2B = 0x04;    /* Start Timer2 with prescaler 64 */


  MY_SREG = savedStatus;
  __asm__ __volatile__("" ::: "memory");
}


/* Processes a complete number after Enter is pressed */
void processInput(void)
{
  unsigned long currentTime;


  if (inputInvalid) {
    Serial.println("Invalid input. Enter a whole number.");
  }
  else if (selectedLED == -1) {
    if (inputValue == 1 || inputValue == 2) {
      selectedLED = (int)inputValue;
    }
    else {
      Serial.println("Please choose LED 1 or 2.");
    }
  }
  else {
    currentTime = myMilli();


    if (selectedLED == 1) {
      blinkRateOne = inputValue;
      blinkTrackerOne = currentTime;
      digitalWrite(2, LOW);
    }
    else {
      blinkRateTwo = inputValue;
      blinkTrackerTwo = currentTime;
      digitalWrite(3, LOW);
    }


    selectedLED = -1;
  }


  inputValue = 0;
  inputStarted = 0;
  inputInvalid = 0;


  if (selectedLED == -1) {
    Serial.println("What LED? (1 or 2):");
  }
  else {
    Serial.println("What interval (in msec):");
  }
}


/* Reads one character at a time so LEDs keep blinking */
void taskSerial(void)
{
  char character;
  unsigned long digit;


  if (Serial.available() <= 0) {
    return;
  }


  character = (char)Serial.read();


  if (character == '\r' || character == '\n') {
    if (inputStarted) {
      processInput();
    }
  }
  else if (character >= '0' && character <= '9') {
    inputStarted = 1;
    digit = (unsigned long)(character - '0');


    /* Prevent the input number from overflowing */
    if (inputValue > (ULONG_MAX - digit) / 10UL) {
      inputInvalid = 1;
    }
    else {
      inputValue = inputValue * 10UL + digit;
    }
  }
  else {
    inputStarted = 1;
    inputInvalid = 1;
  }
}


void setup(void)
{
  Serial.begin(9600);


  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);


  digitalWrite(2, LOW);
  digitalWrite(3, LOW);


  setupMyMilli();


  Serial.println("What LED? (1 or 2):");
}


void loop(void)
{
  unsigned long currentTime;
  unsigned long halfInterval;


  currentTime = myMilli();


  /* LED 1 */
  if (blinkRateOne > 0) {
    halfInterval = blinkRateOne / 2;


    if (halfInterval == 0) {
      halfInterval = 1;
    }


    if (currentTime - blinkTrackerOne >= halfInterval) {
      digitalWrite(2, !digitalRead(2));
      blinkTrackerOne = currentTime;
    }
  }


  /* LED 2 */
  if (blinkRateTwo > 0) {
    halfInterval = blinkRateTwo / 2;


    if (halfInterval == 0) {
      halfInterval = 1;
    }


    if (currentTime - blinkTrackerTwo >= halfInterval) {
      digitalWrite(3, !digitalRead(3));
      blinkTrackerTwo = currentTime;
    }
  }


  taskSerial();
}
