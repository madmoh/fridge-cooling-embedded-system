#include <hidef.h>      /* common defines and macros */
#include "derivative.h" /* derivative-specific definitions */
#include "lcd.h" 				/* include lcd library definitions */
#include "sci1.h"       /* include serial communication interface definitions */


/******* Constants *******/
const unsigned char fs_p = 255;
const unsigned char fs0_ON =   0; // Fan speed 0 (OFF)
const unsigned char fs1_ON =  20; // Fan speed 1 (ON)
const unsigned char fs2_ON = 100; // Fan speed 2 (ON)
const unsigned char fs3_ON = 255; // Fan speed 2 (ON)


/******* Global variables *******/
volatile unsigned char num_of_zones; // Number of zones selected
volatile unsigned char temp1, temp2; // Zones 1 and 2 temperature levels
volatile unsigned char temp1_spec, temp2_spec; // Zones 1 and 2 temperatures
volatile unsigned char cur_temp; // Current temperature
volatile unsigned char ref_has_started, is_ref_on, is_door_open; // Refregirator status
volatile unsigned char f1_ON; // Zone 1 fan speed ON
volatile unsigned char f2_ON; // Zone 2 fan speed ON


/******* Function Headers *******/
void my_delay(unsigned int ms); // A delay function calibrated for mc9s12dp256
void update_ref_status(void); // Sets variables for fan speed
int ATD_CONVERT(); // Returns the temperature value
int key_pad(void); // Returns pressed keypad input

void init_ports(void); // Initializes used ports
void init_timer(void); // Initializes the timer
void ATD_init(void);   // Initializes the ADC (temp sensor)
void init_zones(void); // Displays interface to let user initialize zone settings
void init_temp(void);  // Displays interface to let user initialize temp settings


/******* Main *******/
void main(void) {
	unsigned char temp_cur_temp;
	
	// Re-initialize following variables, because
	  // main() could be called as a program restart
	ref_has_started = is_ref_on = is_door_open = 0;
	f1_ON = f2_ON = 0;
	
	// Run all initialization functions
	SCI1_Init(BAUD_9600);
	init_timer();	
	init_ports();
	init_zones();
	init_temp();
	ATD_init();
	
	// Enable interrupts globally
	__asm CLI;
	
	// Wait for the fridge to be turned ON
	LCDWriteLine(2, "Turn on fridge");
	while (PTH_PTH6 == 0);
	ref_has_started = 1;
	LCD_clear_disp();
	SCI1_OutString("Refrigerator got turned ON");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	
	for(;;) {
		// Mapping DIP switches (bit 0 to 4) from 14 to 45
		  // and setting it as local temperature
		temp_cur_temp =	(PTH & 0b00011111) + 14; // Scenario step 8
		if (temp_cur_temp < 15) {
			cur_temp = 15;
		} else {
			cur_temp = temp_cur_temp;
		}
		
		// Displaying status using LCD and SCI
		LCD_clear_disp();
		LCDWriteLine(1, "Cur Temp: "); // Scenario step 9
		LCDWriteInt(cur_temp);
		LCDWriteChar('F');
		SCI1_OutString("Refrigerator temperature (F): ");SCI1_OutUDec(cur_temp);SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		my_delay(100);
	}
}


/******* Helper functions *******/
 /* Delay for given input (in milliseconds) */
void my_delay(unsigned int ms) {
	int i1, i2, i3;
	int ms1 = ms / 10;
	for (i1 = 0; i1 < 100; i1++)
	for (i2 = 0; i2 < 140; i2++)
	for (i3 = 0; i3 < ms1; i3++)
	asm("NOP\n");
}
/* Toggle LEDs if ON */
void update_ref_status() {
	if (ref_has_started == 1 && is_ref_on == 1) {
		SCI1_OutString("Refrigerator is ON");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		PORTB ^= 0xFF;
	}
	if (ref_has_started == 0) {
		SCI1_OutString("Refrigerator is OFF");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		f1_ON = f2_ON = fs0_ON;
		PORTB == 0;
	}
	is_ref_on = (PTH & 0b01000000) >> 6;
	if (is_ref_on == 0) {
		f1_ON = f2_ON = fs0_ON;
	}
}
/*Get ATD (temperature sensor) value */
int ATD_CONVERT() {
  ATD0CTL5 = 0b10000101; // Right justified data, channel no. 5
  while (!(ATD0STAT0 & 0x80)); // wait for conversion to finish
    return(ATD0DR0); // get and return the value to the caller
}
/* Pressed keypad button */
int key_pad(void) {
	int X;
  for (;;) {
		PORTA = 0xFE;
		X = PORTA;
		if (X == 0xEE) return 0x01;
		if (X == 0xDE) return 0x04;
		if (X == 0xBE) return 0x07;
		if (X == 0x7E) return 0x0E;
		PORTA = 0xFD;
		X = PORTA;
		if (X == 0xED) return 0x02;
		if (X == 0xDD) return 0x05;
		if (X == 0xBD) return 0x08;
		if (X == 0x7D) return 0x00;
		PORTA = 0xFB;
		X = PORTA;
		if (X == 0xEB) return 0x03;
		if (X == 0xDB) return 0x06;
		if (X == 0xBB) return 0x09;
		if (X == 0x7B) return 0x0F;
		PORTA = 0xF7;
		X = PORTA;
		if (X == 0xE7) return 0x0A;
		if (X == 0xD7) return 0x0B;
		if (X == 0xB7) return 0x0C;
		if (X == 0x77) return 0x0D;
	}
}

/******* Initialization functions *******/
/* Ports initializations */
void init_ports(void) {
	// LCD
	LCD_Init(); // Initialize the LCD
	
	// Keypad
	DDRA = 0x0F; // Set Port A dir as input (keypad)
	PUCR = 0x01; // Activate pull-up resistor for the keypad
	
	// DIP switches w/ interrupt
	DDRH = 0x00; // Set Port H dir as input (DIP swithces)
	PIEH_PIEH7 = 0xFF; // Enable PTH interrupt
	PPSH_PPSH7 = 0xFF; // Make it Falling Edge-Trig
	
	// LEDs
	DDRB = 0xFF; // Set Port B dir as output (LEDs/7-seg)
	DDRJ = 0xFF; // Set Port J dir as output (serial clock)
	PTJ  = 0x00; // Set serial clock to 0
	PTP  = 0x0F; // Set common cathod to disable 7-seg
	
	// Buzzer
	DDRT = 0xFF; // Set PORT T dir as output
	
	// IRQ
	INTCR = 0xC0; // Enable and set IRQ to falling-edge trigger
}
/* Timer initializations */
void init_timer(void) {
	// General
	TSCR1 = 0x80; // Enable timer counter 
	TSCR2 = 0x86; // Enable timer overflow interrupt, set prescaler to 8 
	
	// Timer overflow
	TFLG2 = TFLG2_TOF_MASK; // Reset timer overflow flag
	
	// Output compare channel 0 (zone 1 fan)
	TFLG1 |= TFLG1_C0F_MASK; // Clear channel 0 timer interrupt flags
	TIE_C0I = 1; // Enable channel 0 interrupt 
	TIOS_IOS0 = 1; // Enable output compare channel 0
	TCTL2 = 0b00000010; // Set OC0 to low initially 
	
	// Output compare channel 7 (zone 2 fan)
	TFLG1 |= TFLG1_C7F_MASK; // Clear channel 7 timer interrupt flags
	TIE_C7I = 1; // Enable channel 7 interrupt 
	TIOS_IOS7 = 1; // Enable output compare channel 7
	TCTL1 = 0b10000000; // Set OC7 to low initially	 
}
/* ADC initialization */
void ATD_init(void) {
  ATD0CTL2_ADPU = 1; // Power up ATD channel 0, disable interrupts 
  delay(1); // Wait for ADC to warm up 
  ATD0CTL4 = 0b10000101; // 8-bit resolution, prescaler of 5 
}
/* Zones initialization */
void init_zones() {
	LCDWriteLine(1, "Enter #Zones"); // Scenario step 1
	num_of_zones = key_pad(); // Scenario step 2
	while (num_of_zones != 1 && num_of_zones != 2) {
		LCDWriteLine(1, "Either 1 or 2"); // Scenario step 3
		num_of_zones = key_pad();
	}
	LCD_clear_disp();
	my_delay(100); // Wait for 0.1 second
	LCDWriteLine(1, "#Zones: ");
	LCDWriteInt(num_of_zones); // Scenario step 4
	SCI1_OutString("Number of zones chosen: ");SCI1_OutUDec(num_of_zones);SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	
	my_delay(1000); // Wait for 1 second
	LCD_clear_disp();
}
/* Temperature initialization */
void init_temp() {
	// Setup zone 1 temperature level
	LCDWriteLine(1, "Enter Z1 Temp"); // Scenario step 5
	temp1 = key_pad(); // Scenario step 6
	while (temp1 != 1 && temp1 != 2 && temp1 != 3) {
		LCDWriteLine(1, "Either 1, 2 or 3");
		temp1 = key_pad();
	}
	switch (temp1) { // Specifying cooling temperature of zone 1
		case 1: temp1_spec = 20; break;
		case 2: temp1_spec = 30; break;
		case 3: temp1_spec = 40; break;
	}
	SCI1_OutString("Zone 1 temperature specified is ");SCI1_OutUDec(temp1_spec);SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	LCD_clear_disp();
	my_delay(100);
	
	// Setup zone 2 temperature level (if it exists)
	if (num_of_zones == 2) {
		LCDWriteLine(1, "Enter Z2 Temp"); // Scenario step 5
		temp2 = key_pad(); // Scenario step 6
		while (temp2 != 1 && temp2 != 2 && temp2 != 3) {
			LCDWriteLine(1, "Either 1, 2 or 3");
			temp2 = key_pad();
		}
		switch (temp2) { // Specifying cooling temperature of zone 2
			case 1: temp2_spec = 20; break;
			case 2: temp2_spec = 30; break;
			case 3: temp2_spec = 40; break;
		}
		SCI1_OutString("Zone 1 temperature specified is ");SCI1_OutUDec(temp2_spec);SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		LCD_clear_disp();
		my_delay(100);
	}
	
	// Display zone 1 temperature level
	LCDWriteLine(1, "Z1 Temp: "); // Scenario step 7
	LCDWriteInt(temp1);
	LCDWriteChar(' ');LCDWriteChar('[');LCDWriteInt(temp1_spec);LCDWriteChar('F');LCDWriteChar(']');
	
	// Display zone 2 temperature level (if it exists)
	if (num_of_zones == 2) {
		LCDWriteLine(2, "Z2 Temp: "); // Scenario step 7
		LCDWriteInt(temp2);	
		LCDWriteChar(' ');LCDWriteChar('[');LCDWriteInt(temp2_spec);LCDWriteChar('F');LCDWriteChar(']');
	}
	
	my_delay(2000);
	LCD_clear_disp();
}


/******* Interrupts *******/
/* Timer Overflow */
#pragma CODE_SEG NON_BANKED
void interrupt (((0x10000-Vtimovf)/2)-1) TIMOVF_ISR(void) {
	unsigned char z1_temp_diff, z2_temp_diff;
	
	// Zone 1
	z1_temp_diff = cur_temp - temp1_spec;
	if (z1_temp_diff <= 0) {
		f1_ON = fs0_ON;	// Turn off zone 1 fan
		SCI1_OutString("Zone 1 fan is OFF");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} else if (z1_temp_diff <= 5) {
		f1_ON = fs1_ON;	// Turn on zone 1 fan to speed level 1
		SCI1_OutString("Zone 1 fan speed is at level 1");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} else if (z1_temp_diff <= 10) {
		f1_ON = fs2_ON;	// Turn on zone 1 fan to speed level 2
		SCI1_OutString("Zone 1 fan speed is at level 2");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} else {
		f1_ON = fs3_ON;	// Turn on zone 1 fan to speed level 3
		SCI1_OutString("Zone 1 fan speed is at level 3");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	}
	
	// Zone 2
	z2_temp_diff = cur_temp - temp2_spec;
	if (z2_temp_diff <= 0) {
		f2_ON = fs0_ON;	// Turn off zone 2 fan
		SCI1_OutString("Zone 2 fan is OFF");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} else if (z2_temp_diff <= 5) {
		f2_ON = fs1_ON;	// Turn on zone 2 fan to speed level 1
		SCI1_OutString("Zone 2 fan speed is at level 1");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} else if (z2_temp_diff <= 10) {
		f2_ON = fs2_ON;	// Turn on zone 2 fan to speed level 2
		SCI1_OutString("Zone 2 fan speed is at level 2");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} else {
		f2_ON = fs3_ON;	// Turn on zone 2 fan to speed level 3
		SCI1_OutString("Zone 2 fan speed is at level 3");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
	} 
	
	update_ref_status();
	
	if ((ATD_CONVERT() * 100.0) / 51 > 27) {
		SCI1_OutString("Overheating");;SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		main();
	}
	
	TFLG2 = TFLG2_TOF_MASK; // Clear timer interrupt flag
}  	 
/* Output Compare Channel 0 (Zone 1) */
#pragma CODE_SEG NON_BANKED
void interrupt (((0x10000-Vtimch0)/2)-1) TIMCH0_ISR(void) {
	if (is_ref_on == 1 && ref_has_started == 1) {
		SCI1_OutString("Zone 1 fan is operating");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		if (TCTL2_OL0 == 1) {
			TC0 += f1_ON;
			TCTL2 = 0b00000010;
			} else if (TCTL2_OL0 == 0) {
			TC0 += (fs_p - f1_ON);
			TCTL2 = 0b00000011;
		}
	} else {
		TC0 += f1_ON;
	  TCTL2 = 0b00000010;
	}
	TFLG1 = TFLG1_C0F_MASK; // Reset channel 0 interrupt
}
/* Output Compare Channel 7 (Zone 2) */
#pragma CODE_SEG NON_BANKED
void interrupt (((0x10000-Vtimch7)/2)-1) TIMCH7_ISR(void) {
	if (is_ref_on == 1 && ref_has_started == 1 && num_of_zones == 2) {
		SCI1_OutString("Zone 2 fan is operating");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D);
		if (TCTL1_OL7 == 1) {
			TC7 += f2_ON;
			TCTL1 = 0b10000000;
			} else if (TCTL1_OL7 == 0) {
			TC7 += (fs_p - f2_ON);
			TCTL1 = 0b11000000;
		}
	} else {
		TC7 += f2_ON;
	  TCTL1 = 0b10000000;
	}
	TFLG1 = TFLG1_C7F_MASK;
}
/* IRQ switch */
#pragma CODE_SEG NON_BANKED // Access victor priority table
interrupt 6 void IRQ_ISR(void) { /// When IRQ interrupt is activated
	LCD_clear_disp();
	LCDWriteLine(1, "Operation is");
	LCDWriteLine(2, "stopped");
	my_delay(3000);	
	update_ref_status();
	main();               
}
/* DIP switches interrupt */
#pragma CODE_SEG NON_BANKED // Access victor priority table
interrupt (((0x10000-Vporth)/2)-1) void PORTH_ISR(void) {
	int m = 0, s = 0;
	if (PIFH_PIFH7 == 1) {
		LCD_clear_disp();
		LCDWriteLine(1, "WARNING!");
		LCDWriteLine(2, "Door is open");
		while (PTH_PTH7 == 1) {	
			PTT ^= 0b00100000; // Toggle PT5 for Buzzer
			SCI1_OutString("WARNING! Door is open");SCI1_OutChar(0x0A);SCI1_OutChar(0x0D); 
			PORTB ^= 0xFF;
			delay(10);
		}
		LCD_clear_disp();
	}
	PORTB = 0x00;
	PTP = 0x0F;
  PIFH = PIFH | 0xFF; // Clear PTH Interrupt Flags for the next round
} 
