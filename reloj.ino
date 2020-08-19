#define SCK PORTB5
#define SS PORTB2
#define MOSI PORTB3
#define TIME_SETUP PORTD2       //trigger time setup routine
#define NUM_UP PORTD4           //up 1 number
#define NUM_DOWN PORTD5         //down 1 number
#define NEXT_NUM PORTD6         //next digit
#define SAVE_TIME PORTD7        //save time
#define NUM_UP_STATUS PIND4     //status of NUM_UP
#define NUM_DOWN_STATUS PIND5   //status of NUM_DOWN
#define NEXT_NUM_STATUS PIND6   //status of NEXT_NUM
#define SAVE_TIME_STATUS PIND7  //status of SAVE_TIME

unsigned char num = 0;
unsigned char units = 0;
unsigned char tens = 0;
//bool time_setup_flag = 0;

uint8_t minutes_units = 0;
uint8_t minutes_tens = 0;
uint8_t hour_units = 0;
uint8_t hour_tens = 0;

void setup() {
  extint_setup();
  buttons_setup();
  initialize_SPI();
  Max_setup();

}

void loop(){
  if(num>99){
    num = 0;
  }
  units = num%10;
  tens = num/10;
  
  delay(200);
  write_data(1, units);
  delay(10);
  write_data(2, tens);

  num++;
}

void initialize_SPI(){
  DDRB |= (1<<SCK)|(1<<MOSI)|(1<<SS);     //set spi pins as outputs
  SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR1);		//enable spi, master mode, fosc/64
}

void write_data(char address, char dataToPrint){
  PORTB &= ~(1<<SS);
  SPDR = address;               //first write address on the max7219. It's split in two bytes because the MAX72 expects a 16-bit word. MSB First
  while(!(SPSR & (1<<SPIF)));   //wait for shit to get done
  SPDR = dataToPrint;           //then write the actual data on the max7219
  while(!(SPSR & (1<<SPIF)));   //wait for shit to get done
  PORTB |= (1<<SS);
}

void Max_setup(){
  char digitsUsed = 2;
  write_data(0x09, 0xFF);               //0X09 = Decode mode, 0xFF = Decode for all digits
  write_data(0x0A, 8);                  //0x0A = Intensity -> 8 (is this bright enough?)
  write_data(0x0B, digitsUsed-1);       //0x0B = Scan Limit mode, 2 digits
  write_data(0x0C, 1);                  //0x0C = Shutdown, 1 = normal operation (no shutdown)
}

void extint_setup(){
  SREG |= 0x80;                       //Enable Global interrupts  (1<<I) <- for some reason this doesnt work???
  EICRA &= ~(1<<ISC01)&~(1<<ISC00);   //Low level of INT0 generates an interrupt request (maybe change to any logical change?)
  EIMSK |= (1<<INT0);                 //Enable external interrupt on INT0 
}

ISR(INT0_vect){                       //ExtInt0 interrupt vector
  //
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

void time_setup(){

}