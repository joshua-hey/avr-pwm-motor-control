#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include "i2c.h"
#include "SSD1306.h"

#define up_count PB5   //pin corresponding to "up" position of RHC button
#define down_count PB7 //pin corresponding to "down" position of RHC button
#define DCmotor PE2
#define optointerrupter PD2
#define SLOTS 2      // number of slots per spinner revolution

double dc_adj=0.0;    //duty cycle adjusting parameter
volatile char dutyCycle = 0;    //global variable containing selected motor speed value
volatile char lastInput = 0;    //variable to store the last state of PORTB
int count_int = 0;               //set pwm value to 0 (no pwm) initially
volatile uint16_t opto_int = 0; 
volatile uint16_t last_opto_initial = 0; // value at start of one second period for RPM calc
volatile uint16_t rpm = 0;  // pulses per sec -> RPM
int overflow_cnt = 0;


ISR(TIMER1_COMPA_vect){  //ISR for when timer1A interrupts
    PORTE &= ~(1<<DCmotor); //turn off PE2 pin
}
ISR(TIMER1_COMPB_vect){  //ISR for when timer1B interrupts
    PORTE |= (1<<DCmotor); //turn on PE2 pin
}
ISR(TIMER2_OVF_vect) {
    overflow_cnt++;
    if (overflow_cnt == 61) {  
        rpm = ((opto_int - last_opto_initial) / SLOTS) * 60;
		last_opto_initial = opto_int;
        overflow_cnt = 0;
    }
}
ISR(INT0_vect) {
	opto_int++;        // increment pulse counter on each slot
}

void OLED_display(){  //this will update the OLED display with the most current info
    OLED_SetCursor(0, 0);
    OLED_Printf("Mode: ");
    OLED_DisplayNumber(C_DECIMAL_U8,dutyCycle/10,2);
    OLED_SetCursor(2, 0);
    OLED_Printf("PWM%: ");
    OLED_DisplayNumber(C_DECIMAL_U8,dutyCycle,3);
    OLED_Printf("%% ");
    OLED_SetCursor(4, 0);
    OLED_Printf("RPM: ");
    OLED_DisplayNumber(C_DECIMAL_U8,rpm,7);
    OLED_SetCursor(6, 0);
    OLED_Printf("Interrupts: ");
    OLED_DisplayNumber(C_DECIMAL_U8,opto_int,6);
}

int main(void){
    OLED_Init(); //INITIALIZE THE OLED
    OLED_Clear(); //CLEAR THE DISPLAY
    DDRB = 0x00;    //set port b pins to inputs (for joystick)
    PORTB = 0xA0;   //set port b pins to have pull-up resistors
    DDRD &= ~(1 << optointerrupter);  // set PD2 to input
    PORTD &= ~(1 << optointerrupter); // disables pullup
    DDRE |= (1<<DCmotor); //set motor pin to output

    // Configures external interrupt INT0
    EICRA |= (1 << ISC01);   // ISC01=1 AND ISC00=0 = falling edge trigger
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);    // enable INT0
    
    // Timer initialization	 
    ICR1 = 65535;  // full PWM period (4.1 ms), no prescale or preload -> (65536-0)*1/16Mhz = 4.096 ms
    OCR1B=0;
    TCCR1A = (1<<COM1A0)|(1<<COM1B0);  //enable timer1a and timer1b comparison
    TCCR1B = (1<<WGM13) | (1<<WGM12) | (1<<CS10);    //enable timer1 with no pre-scaler
    TIMSK1 = (1<<OCIE1A)|(1<<OCIE1B);  //enable timer1A and timer1B interrupts

    TCCR2A = 0;          // Normal mode
    TCCR2B = (1<<CS22)|(1<<CS21)|(1<<CS20);  // 1024 prescaler
    TIMSK2 = (1<<TOIE2); // enable overflow interrupt
    sei();
    
    while (1) {    //infinite loop
        char joystick = (0b11110000&PINB); //get high 4 bits of PORTB (contains up and down button)
        joystick = joystick^0xf0;    //invert value of of up and down button (Pullup resistors pull pins high)
        if(lastInput != joystick){      //check if PORTB has been updated
            if(joystick&(1<<up_count)){ //check for up button being pressed
                if(count_int < 10)       //if the mode is less than 10,
                count_int++; //YES: increment it
            }
            if(joystick&(1<<down_count)){ //check for down button being pressed
                if(count_int > 0)          //if the mode is more than 0,
                count_int--; //NO: decrement it
            }
            dc_adj=count_int * 0.1;
            dutyCycle = count_int*10;//update dutyCycle variable for OLED display
            lastInput = joystick;    //keep the latest PORTB input
        }
        // Explicitly turn motor off if speed is zero
        if (dc_adj <= 0.0) {
            TIMSK1 &= ~((1<<OCIE1A)|(1<<OCIE1B)); //disable PWM interrupts
            PORTE &= ~(1<<DCmotor);               //turn motor off
        } else {
            TIMSK1 |= (1<<OCIE1A)|(1<<OCIE1B);   //enable PWM interrupts
            OCR1A = (uint16_t)(ICR1 * dc_adj);  //reassign timer1 value for On-time pulse
        }
        OLED_display();  //update OLED display
    }
}