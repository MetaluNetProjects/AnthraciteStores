/*********************************************************************
 *               analog example for Versa1.0
 *	Analog capture on connectors K1, K2, K3 and K5. 
 *********************************************************************/

#define BOARD 8X2A

#include <fruit.h>
#include <dcmotor.h>
#include <switch.h>
#include <dmx_slave.h>
#include <eeparams.h>

t_delay mainDelay;

DCMOTOR_DECLARE(C);
DCMOTOR_DECLARE(D);

int dmxchan1;
int dmxchan2;

#define DMXMARGIN 20
#define SPEED_MIN 220

void setup(void) {	
//----------- Setup ----------------
	fruitInit();
	switchInit();
	switchSelect(0, CLO);
	switchSelect(1, CHI);
	switchSelect(2, DLO);
	switchSelect(3, DHI);

	delayStart(mainDelay, 5000); 	// init the mainDelay to 5 ms

//----------- dcmotor setup ----------------

	dcmotorInit(C);
	dcmotorInit(D);

//----------- DMX slave setup --------------
	DMXSlaveInit();
	
	EEreadMain();
	
	DMXSlaveSet(dmxchan1, 128);
	DMXSlaveSet(dmxchan2, 128);
}

int dmx2pwm(int chan)
{
	int val = DMXSlaveGet(chan);
	
	if(val < (128 - DMXMARGIN)) return (int)(((128 - DMXMARGIN - (int)val) * (-1023L + SPEED_MIN))/(128 - DMXMARGIN)- SPEED_MIN);
	else if(val > (128 + DMXMARGIN)) return (int)((((int)val - 128 - DMXMARGIN) * (1023L - SPEED_MIN))/(127 - DMXMARGIN) + SPEED_MIN);
	else return 0;
}

unsigned char mainCount;
void loop() {
// ---------- Main loop ------------
	fraiseService();	// listen to Fraise events
	switchService();	// analog management routine

	if(delayFinished(mainDelay)) // when mainDelay triggers :
	{
		delayStart(mainDelay, 5000); 	// re-init mainDelay
		DCMOTOR(C).Vars.PWMConsign = dmx2pwm(dmxchan1);
		DCMOTOR(D).Vars.PWMConsign = dmx2pwm(dmxchan2);
		if((digitalRead(CLO) == 0) && (DCMOTOR(C).Vars.PWMConsign < 0)) DCMOTOR(C).Vars.PWMConsign = 0;
		if((digitalRead(CHI) == 0) && (DCMOTOR(C).Vars.PWMConsign > 0)) DCMOTOR(C).Vars.PWMConsign = 0;
		if((digitalRead(DLO) == 0) && (DCMOTOR(D).Vars.PWMConsign < 0)) DCMOTOR(D).Vars.PWMConsign = 0;
		if((digitalRead(DHI) == 0) && (DCMOTOR(D).Vars.PWMConsign > 0)) DCMOTOR(D).Vars.PWMConsign = 0;
		DCMOTOR_COMPUTE(C,ASYM);
		DCMOTOR_COMPUTE(D,ASYM);
		mainCount++;
		if(mainCount > 20) {
			printf("CM %d %d\n", DCMOTOR(C).Vars.PWMConsign, DCMOTOR(D).Vars.PWMConsign);
			mainCount = 0;
		}
	}
}

// Receiving

void fraiseReceiveChar() // receive text
{
	unsigned char c;
	
	c=fraiseGetChar();
	if(c=='E') { 	// echo text (send it back to host)
		printf("C");
		c = fraiseGetLen(); 			// get length of current packet
		while(c--) printf("%c",fraiseGetChar());// send each received byte
		putchar('\n');				// end of line
	}	
}


void fraiseReceive() // receive raw
{
	unsigned char c;
	c=fraiseGetChar();
	
	switch(c) {
		case 120 : DCMOTOR_INPUT(C) ; break;
		case 121 : DCMOTOR_INPUT(D) ; break;
		case 122 : dmxchan1 = fraiseGetInt(); break;
		case 123 : dmxchan2 = fraiseGetInt(); break;
		case 124 : DMXSlaveSet(dmxchan1, fraiseGetChar()); DMXSlaveSet(dmxchan2, fraiseGetChar()); break;
		case 255 : EEwriteMain(); break;
	}
}

void lowInterrupts()
{
	DMXSlaveISR();
}

void EEdeclareMain()
{
	EEdeclareInt(&dmxchan1);
	EEdeclareInt(&dmxchan2);
}
