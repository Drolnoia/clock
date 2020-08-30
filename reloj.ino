#define SCK PORTB5
#define SS PORTB2
#define MOSI PORTB3
/*
#define TIME_SETUP PORTD2       //trigger time setup routine
#define NUM_UP PORTD4           //up 1 number
#define NUM_DOWN PORTD5         //down 1 number
#define NEXT_NUM PORTD6         //next digit
#define SAVE_TIME PORTD7        //save time
#define NUM_UP_STATUS PIND4     //status of NUM_UP
#define NUM_DOWN_STATUS PIND5   //status of NUM_DOWN
#define NEXT_NUM_STATUS PIND6   //status of NEXT_NUM
#define SAVE_TIME_STATUS PIND7  //status of SAVE_TIME
*/

unsigned char num = 0;
unsigned char units = 0;
unsigned char tens = 0;
bool timeSetupFlag = 0;

void initialize_SPI(){
  DDRB |= (1<<SCK)|(1<<MOSI)|(1<<SS);     //set spi pins as outputs
  SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR1);   //enable spi, master mode, fosc/64
}

void Max_write_data(char address, char dataToPrint){
  PORTB &= ~(1<<SS);
  SPDR = address;               //first write address on the max7219. It's split in two bytes because the MAX72 expects a 16-bit word. MSB First
  while(!(SPSR & (1<<SPIF)));   //wait for shit to get done
  SPDR = dataToPrint;           //then write the actual data on the max7219
  while(!(SPSR & (1<<SPIF)));   //wait for shit to get done
  PORTB |= (1<<SS);
}

void Max_setup(){
  char digitsUsed = 6;
  Max_write_data(0x09, 0xFF);               //0X09 = Decode mode, 0xFF = Decode for all digits
  Max_write_data(0x0A, 8);                  //0x0A = Intensity -> 8 (is this bright enough?)
  Max_write_data(0x0B, digitsUsed-1);       //0x0B = Scan Limit mode, digitsUsed digits
  Max_write_data(0x0C, 1);                  //0x0C = Shutdown, 1 = normal operation (no shutdown)
}

void i2c_setup(){
  TWSR &= ~(1<<TWPS0)&~(1<<TWPS1);    //Set prescaler to 1
  TWBR |= 0x20;                       //DS3231 SCL freq = 200 kHz, CPUfreq = 16 MHz, prescale 1
  TWCR = (1<<TWEN);                   //Enable i2c interface. Internet told me not to use |= operator when working with TWCR
}

void i2c_start(){
  TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);   //set INT Flag, send start condition, enable i2c (datasheet says it should be enabled every time)
  while(!(TWCR & (1<<TWINT)));              //TWINT flag works kinda funny. Not falling into this rabbit hole.
}

void i2c_stop(){
  TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);   //stop condition
}

void i2c_write(uint8_t wordAddress, uint8_t data){
  i2c_start();
  TWDR = 0b11010000;                //0b1101000 is slave address. 0b0 is write mode. SLA + W
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  TWDR =  wordAddress;              //write word address
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  TWDR = data;                      //then write the actual data
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  i2c_stop();
}

void numbers_test(){
  if(num>99){
    num = 0;
  }
  units = num%10;
  tens = num/10;
  
  delay(200);
  Max_write_data(1, units);
  delay(10);
  Max_write_data(2, tens);

  num++;
}

class rtcTime{
  uint8_t hour = 0;
  uint8_t minutes = 0;
  uint8_t seconds = 0;
public:
  Time();
  void setTime();
  void getTime();
  void displayTime();
};

rtcTime::Time(){
  hour = 0;
  minutes = 0;
}

void rtcTime::setTime(){
  i2c_start();
  TWDR = 0b11010000;                //0b1101000 is slave address. 0b0 is write mode. SLA + W
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Set SLA+W: ");
  //Serial.println(TWDR, BIN);
  TWDR =  0x00;                     //write word address. 0x00 is seconds
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Set word address: ");
  //Serial.println(TWDR, HEX);
  TWDR = 0x01;                      //then write the actual data. 1 for test
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Set seconds: ");
  //Serial.println(TWDR, HEX);
  TWDR = 0x02;                      //2 minutes for test
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Set minutes: ");
  //Serial.println(TWDR, HEX);
  TWDR = 0x03;                      //3 hours for test
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Set hour: ");
  //Serial.println(TWDR, HEX);
  i2c_stop();
}

void rtcTime::getTime(){            //reads data from rtc and stores it into rtcTime
  i2c_start();
  TWDR = 0b11010000;                //0b1101000 is slave address. 0b0 is write mode. SLA + W
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Get SLA+W: ");
  //Serial.println(TWDR, BIN);
  TWDR = 0x00;                      //word address to read. First pretend we're writing, then read the address we were supposed to write to. 0x00 is seconds address
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Get word address: ");
  //Serial.println(TWDR, HEX);
  i2c_start();                      //Repeated start condition
  TWDR = 0b11010001;                //0b1101000 is slave address. 0b1 is read mode. SLA + R
  TWCR = (1<<TWINT)|(1<<TWEN);
  while(!(TWCR & (1<<TWINT)));
  //Serial.print("Get SLA+R: ");
  //Serial.println(TWDR, BIN);
  TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);  //Slave sends byte, master sends ack
  while(!(TWCR & (1<<TWINT)));
  seconds = TWDR;
  //Serial.print("Get seconds: ");
  //Serial.println(seconds, HEX);                         //get seconds from TWDR
  TWCR = (1<<TWINT)|(1<<TWEA)|(1<<TWEN);  //Slave sends byte, master sends ack
  while(!(TWCR & (1<<TWINT)));
  minutes = TWDR;                         //get minutes from TWDR
  //Serial.print("Get minutes: ");
  //Serial.println(minutes, HEX);
  TWCR = (1<<TWINT)|(1<<TWEN);            //Slave sends byte, master sends nack
  //Serial.println("flag cleared");
  while(!(TWCR & (1<<TWINT)));
  //Serial.println("flag set");
  hour = TWDR;                            //get hour from TWDR
  //Serial.print("Get hour: ");
  //Serial.println(hour, HEX);
  i2c_stop();
  Serial.print("get hour: ");
  Serial.print(hour, DEC);
  Serial.print(":");
  Serial.print(minutes, DEC);
  Serial.print(":");
  Serial.println(seconds, DEC);
  //Serial.println("--------------------------");
}

void rtcTime::displayTime(){
  delay(100);
  Max_write_data(1, seconds%10);
  delay(100);
  Max_write_data(2, seconds/10);
  delay(100);
  //Serial.print("displayed seconds: ");
  //Serial.println(seconds, DEC);
  Max_write_data(3, minutes%10);
  delay(100);
  Max_write_data(4, minutes/10);
  delay(100);
  //Serial.print("displayed minutes: ");
  //Serial.println(minutes, DEC);
  Max_write_data(5, hour%10);
  delay(100);
  Max_write_data(6, hour/10);
  Serial.print("displayed hour: ");
  Serial.print(hour/10, DEC);
  Serial.print(hour%10, DEC);
  Serial.print(":");
  Serial.print(minutes/10, DEC);
  Serial.print(minutes%10, DEC);
  Serial.print(":");
  Serial.print(seconds/10, DEC);
  Serial.println(seconds%10, DEC);
  Serial.println("--------------------------");
}

rtcTime currentTime;   //time to display

void setup() {
  //extint_setup();
  //buttons_setup();
  initialize_SPI();
  Max_setup();
  i2c_setup();
  Serial.begin(9600);

  currentTime.setTime();
}

void loop(){
  currentTime.getTime();
  currentTime.displayTime();

/*if(timeSetupFlag){
  timeSetup();
}else{

}*/

  //numbers_test();
  
}

/*void extint_setup(){
  SREG |= 0x80;                       //Enable Global interrupts  (1<<I) <- for some reason this doesnt work???
  EICRA &= ~(1<<ISC01)&~(1<<ISC00);   //Low level of INT0 generates an interrupt request (maybe change to any logical change?)
  EIMSK |= (1<<INT0);                 //Enable external interrupt on INT0 
}

ISR(INT0_vect){                       //ExtInt0 interrupt vector
  timeSetupFlag = true;               //Set the flag that enters the time setup routine
}

void buttons_setup(){
  DDRD &= ~(1<<TIME_SETUP)&~(1<<NUM_UP)&~(1<<NUM_DOWN)&~(1<<NEXT_NUM)&~(1<<SAVE_TIME);    //set pins as inputs
  PORTD |= (1<<TIME_SETUP)|(1<<NUM_UP)|(1<<NUM_DOWN)|(1<<NEXT_NUM)|(1<<SAVE_TIME);        //pull-up reistors for buttons
}

void debounce(){
  uint8_t debounce_counter = 0;
  while(debounce_counter<50){
    debounce_counter++;
  }
}

void timeSetup_display(){
  uint8_t digit = 4;          //start on digit 3
}

void timeSetup(){
  uint8_t hour_tens = 0;
  uint8_t hour_units = 0;
  uint8_t minutes_tens = 0;
  uint8_t minutes_units = 0;
  bool flag = true;
  while(flag){
    if(!NUM_UP_STATUS || !NUM_DOWN_STATUS || !NEXT_NUM_STATUS || !SAVE_TIME_STATUS){
      debounce();
      if(!NUM_UP_STATUS)
    }*/
