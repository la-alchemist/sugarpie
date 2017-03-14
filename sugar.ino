#include<SPI.h>

#define SCLK 13
#define SDO 12
#define SDI 11
#define FARADAY_LOAD 10
#define CHSEL1 9
#define CHSEL0 8
#define SWING_LOAD 7
#define DDS_LOAD 6
#define CM_LOAD 5
#define TX_L_EN 4
#define TX_M_EN 3
#define TX_S_EN 2

#define TX_L_CM 0.8
#define TX_L_SWING 0.99
#define TX_L_FREQ 3125
#define TX_M_CM 0.8
#define TX_M_SWING 0.99
#define TX_M_FREQ 3125
#define TX_S_CM 0.8
#define TX_S_SWING 0.99
#define TX_S_FREQ 3125
#define TX_R_CM 0.8
#define TX_R_SWING 0.99
#define TX_R_FREQ 3125

#define DAC_SPI_CLOCK_SPEED 1000000 //Upto 1.4MHz
#define DDS_SPI_CLOCK_SPEED 1000000 
#define ADC_SPI_CLOCK_SPEED 8000000 
#define NUM_SAMPLES 300 //250 at 20.833kHz means 14.4ms worth of data
#define DDS_REF_FREQ 8000000
#define FREQ0 0x4000

#define TX_scope 0
#define RX_scope 1

//int buffer_full = 0;
int adc_pointer = 0;
//byte adc_dump[NUM_SAMPLES*3];
byte b0,b1,b2;
signed long rx_int[NUM_SAMPLES];
long temp;

void setup()
{
Serial.begin(115200);
init_all();

//////////***************** TX setup **************//////////////////////
enable_tx('R');
/////////////////////////////////////////////
//////////***************** TX setup **************//////////////////////
delay(1000);//Wait for a second
//////////***************** RX setup **************//////////////////////
////////////////ADC routine///////////////////
read_rx('R');
/////////////////////////////////////////////
//////////***************** RX setup **************//////////////////////
}

void loop() 
{
  if (TX_scope == 1)
  {
    Serial.println(analogRead(A0)/1023.0*5.0);
  }
  if (RX_scope == 1)
  {
    if (adc_pointer < NUM_SAMPLES-1)
    {
      //Is ADC ready? Read it!
      if (digitalRead(MISO) == HIGH)
      {
          read_adc();
      }
    }
    else
    {
      //Dump the data to serial while enclosing triggers
      Serial.println("Trigger");
      for(int i=0;i<NUM_SAMPLES-1;i++)
      {
        Serial.println(rx_int[i],DEC);
      }
      SPI.endTransaction();
      SPI.end();
    }
  }
}

void enable_tx(char channel_id)
{  
  if (channel_id=='L')
  {
    /////////// Set common mode /////////////////
    set_common_mode(TX_L_CM); //Volts
    /////////////////////////////////////////////
    /////////// Set swing //////////////////////
    set_swing(TX_L_SWING); //0.06 to 0.99
    ////////////////////////////////////////////
    /////////// Set frequency //////////////////
    set_freq(TX_L_FREQ); 
    ////////////////////////////////////////////
    digitalWrite(TX_L_EN, LOW);
  }
  else if (channel_id=='M')
  {
    /////////// Set common mode /////////////////
    set_common_mode(TX_M_CM); //Volts
    /////////////////////////////////////////////
    /////////// Set swing //////////////////////
    set_swing(TX_M_SWING); //0.06 to 0.99
    ////////////////////////////////////////////
    /////////// Set frequency //////////////////
    set_freq(TX_M_FREQ); 
    ////////////////////////////////////////////
    digitalWrite(TX_M_EN, LOW);
  }
  else if (channel_id=='S')
  {
    /////////// Set common mode /////////////////
    set_common_mode(TX_S_CM); //Volts
    /////////////////////////////////////////////
    /////////// Set swing //////////////////////
    set_swing(TX_S_SWING); //0.06 to 0.99
    ////////////////////////////////////////////
    /////////// Set frequency //////////////////
    set_freq(TX_S_FREQ); 
    ////////////////////////////////////////////
    digitalWrite(TX_S_EN, LOW);
  }
  else
  {
    /////////// Set common mode /////////////////
    set_common_mode(TX_R_CM); //Volts
    /////////////////////////////////////////////
    /////////// Set swing //////////////////////
    set_swing(TX_R_SWING); //0.06 to 0.99
    ////////////////////////////////////////////
    /////////// Set frequency //////////////////
    set_freq(TX_R_FREQ); 
    ////////////////////////////////////////////
  }
  
}

void read_rx(char channel_id)
{
  select_channel(channel_id);
  //Reset ADC
  reset_adc();
  //Initialize ADC
  init_adc();
}

void reset_adc()
// to reset ADC, we need SCLK HIGH for min of 4 CONVCYCLES
// so here, hold SCLK HIGH for 5 CONVCYCLEs = 1440 usec
{
 //f_CLK = 8000000
 //f_MCLK = f_CLK/6 = 1333333
 //p_MCLK = 1/f_MCLK = 0.75us
 //1 conversion cycle = 384*p_MCLK = 0.288ms
 //5 conversion cycles = 5*60ms = 1.44us (min 4)
 SPI.begin();
 digitalWrite(SCLK, HIGH);
 delayMicroseconds(2);
}

void init_adc()
{
  SPI.begin();
  SPI.beginTransaction(SPISettings(ADC_SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE0));
  digitalWrite(SCLK, LOW);
}

void select_channel(char channel_id)
{
  if (channel_id=='L')
  {
    pinMode(CHSEL0, LOW);
    pinMode(CHSEL1, LOW);
  }
  else if (channel_id=='M')
  {
    pinMode(CHSEL0, LOW);
    pinMode(CHSEL1, HIGH);
  }
  else if (channel_id=='S')
  {
    pinMode(CHSEL0, HIGH);
    pinMode(CHSEL1, LOW);
  }
  else
  {
    pinMode(CHSEL0, HIGH);
    pinMode(CHSEL1, HIGH);
  }
}

void set_freq(long freq)
{
  SPI.begin();
  SPI.beginTransaction(SPISettings(DDS_SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE2));
  set_dds(freq);
  SPI.endTransaction();
  SPI.end();
}

void set_dds(long freq)
{

  long freq_data = freq * pow(2, 28) / DDS_REF_FREQ;
  int freq_MSB = (int)(freq_data >> 14) | FREQ0;
  int freq_LSB = (int)(freq_data & 0x3FFF) | FREQ0;
  
  digitalWrite(SCLK, HIGH);
  digitalWrite(DDS_LOAD, LOW);
  delayMicroseconds(1);
  SPI.transfer16(0x2100); //Reset
  SPI.transfer16(freq_LSB); //Frequency LSB
  SPI.transfer16(freq_MSB); //Frequency MSB
  SPI.transfer16(0xC000); //Phase
  SPI.transfer16(0x2000); //Release reset
  delayMicroseconds(1);
  digitalWrite(DDS_LOAD, HIGH);
  digitalWrite(SCLK, HIGH);
}

void set_swing(float ratio)
{
  float res = 1000/(1/ratio-1);
  int code = floor((res-60)/10000*255);
  //Serial.println(code, HEX);
  digitalWrite(SWING_LOAD, LOW);
  delayMicroseconds(1);
  SPI.begin();
  SPI.beginTransaction(SPISettings(DAC_SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE1));
  SPI.transfer(code);
  delayMicroseconds(1);
  digitalWrite(SWING_LOAD, HIGH);
  delayMicroseconds(1);
  SPI.endTransaction();
  SPI.end();
}

void set_common_mode(float cm)
{
  //cm = 2*cm-0.326;
  SPI.begin();
  SPI.beginTransaction(SPISettings(DAC_SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE0));
  delay(10);
  SPI.transfer16(float(cm/2.048)*0x1000);
  delay(10);
  digitalWrite(CM_LOAD, LOW);
  delay(10);
  digitalWrite(CM_LOAD, HIGH);
  delay(10);
  SPI.endTransaction();
  SPI.end();
  //Print message
  //float val = cm;
  //String message = String("Common mode set to ");
  //message = message + val + "V";
  //Serial.println(message);
}

void drdy_wait()
// wait for DRDY to pass and to reach start-point of DOUT
{
 // 36*p_MCLK = 27us
 delayMicroseconds(27);
}

void read_adc()
{
 drdy_wait();
 // go to drdy_wait routine, where we wait for
 // DRDY phase to pass, and thus for DOUT phase to begin

// adc_dump[adc_pointer] = SPI.transfer(0x00);
// adc_dump[adc_pointer+1] = SPI.transfer(0x00);
// adc_dump[adc_pointer+2] = SPI.transfer(0x00);
b0 = SPI.transfer(0x00);
b1 = SPI.transfer(0x00);
b2 = SPI.transfer(0x00);
//rx_int[adc_pointer] = ((((rx_int[adc_pointer]|b0)<<8)|b1)<<8)|b2;
temp = b0;
temp = temp*256 + b1;
temp = temp*256 + b2;
rx_int[adc_pointer] = temp;

 //Increment pointer
 adc_pointer = adc_pointer+1;
}


void init_all()
{
  pinMode(FARADAY_LOAD,OUTPUT);//Slave select
  pinMode(CHSEL1,OUTPUT);
  pinMode(CHSEL0,OUTPUT);
  pinMode(SWING_LOAD,OUTPUT);
  pinMode(DDS_LOAD,OUTPUT);
  pinMode(CM_LOAD,OUTPUT);
  pinMode(TX_L_EN,OUTPUT);
  pinMode(TX_M_EN,OUTPUT);
  pinMode(TX_S_EN,OUTPUT);
  pinMode(SCLK,OUTPUT);
  pinMode(SDI,OUTPUT);
  pinMode(SDO,INPUT);
  // Pull up all enables
  digitalWrite(FARADAY_LOAD,HIGH);
  digitalWrite(CHSEL1,HIGH);
  digitalWrite(CHSEL0,HIGH);
  digitalWrite(SWING_LOAD,HIGH);
  digitalWrite(DDS_LOAD,HIGH);
  digitalWrite(CM_LOAD,HIGH);
  digitalWrite(TX_L_EN,HIGH);
  digitalWrite(TX_M_EN,HIGH);
  digitalWrite(TX_S_EN,HIGH);
  digitalWrite(SCLK,HIGH);
  digitalWrite(SDI,HIGH);
}
