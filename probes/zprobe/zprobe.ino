// These are standard Ramps1.4 pin definitions, change if needed
#define X_STEP_PIN         54
#define X_DIR_PIN          55
#define X_ENABLE_PIN       38
#define X_MIN_PIN           3
#define X_MAX_PIN           2   //2 //Max endstops default to disabled "-1", set to commented value to enable.

#define Y_STEP_PIN         60
#define Y_DIR_PIN          61
#define Y_ENABLE_PIN       56
#define Y_MIN_PIN          14
#define Y_MAX_PIN          15   //15

#define Z_STEP_PIN         46
#define Z_DIR_PIN          48
#define Z_ENABLE_PIN       62
#define Z_MIN_PIN          18
#define Z_MAX_PIN          19

// LCD
#define BEEPER 33                   // Beeper on AUX-4

#define LCD_PINS_RS 16 
#define LCD_PINS_ENABLE 17
#define LCD_PINS_D4 23
#define LCD_PINS_D5 25 
#define LCD_PINS_D6 27
#define LCD_PINS_D7 29
#define LCD_PINS_RW 39  // set to -1 if not using lcdfast or no r/w

//buttons are directly attached using AUX-2
#define BTN_EN1 37
#define BTN_EN2 35
#define BTN_ENC 31  //the click

#define LCD_WIDTH 20
#define LCD_HEIGHT 4

// comment out if not using an LCD
#define USE_LCD


// the pin used for the Z probe, I use the Z min limit switch, but any pin can be used
#define Z_PROBE_PIN Z_MIN_PIN

// set your steps per mm here, found in Marlin/Configure.h
#define STEPS_PER_MM 400.0F

// set your max speed here in mm/min
#define XY_FEED_RATE 2500.0F


// set the position of your edges in mm, starts at 0,0
#define XMAX 180
#define YMAX 165

#ifdef USE_LCD
#include <LiquidCrystalFast.h>
#if defined(LCD_PINS_RW) && LCD_PINS_RW > -1
LiquidCrystalFast lcd(LCD_PINS_RS, LCD_PINS_RW, LCD_PINS_ENABLE, LCD_PINS_D4, LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7);  //RS,Enable,D4,D5,D6,D7 
#else
LiquidCrystalFast lcd(LCD_PINS_RS, LCD_PINS_ENABLE, LCD_PINS_D4, LCD_PINS_D5,LCD_PINS_D6,LCD_PINS_D7);  //RS,Enable,D4,D5,D6,D7 
#endif
#endif

long zpos= 0;
long xpos= 0;
long ypos= 0;
float zbase= 0.0;

void beep()
{
#ifdef USE_LCD
  //return;
#if (BEEPER > -1)
	{
		pinMode(BEEPER,OUTPUT);
		for(int8_t i=0;i<20;i++){
			digitalWrite(BEEPER,HIGH);
			delay(5);
			digitalWrite(BEEPER,LOW);
			delay(5);
		}
	}
#endif
#endif
}

void buttons_init()
{
#ifdef USE_LCD
	pinMode(BTN_EN1,INPUT);
	pinMode(BTN_EN2,INPUT); 
	pinMode(BTN_ENC,INPUT); 
	//pinMode(SDCARDDETECT,INPUT);
	digitalWrite(BTN_EN1,HIGH);
	digitalWrite(BTN_EN2,HIGH);
	digitalWrite(BTN_ENC,HIGH);
#endif
}

bool getButton() {
#ifdef USE_LCD
	return digitalRead(BTN_ENC) == LOW;
#else
	return false;
#endif
}

void setup()
{
	pinMode(X_STEP_PIN, OUTPUT);
	pinMode(X_DIR_PIN, OUTPUT);
	pinMode(Y_STEP_PIN, OUTPUT);
	pinMode(Y_DIR_PIN, OUTPUT);
	pinMode(Z_STEP_PIN, OUTPUT);
	pinMode(Z_DIR_PIN, OUTPUT);
	
	pinMode(Z_PROBE_PIN, INPUT_PULLUP);
	pinMode(X_ENABLE_PIN, OUTPUT);
	pinMode(Y_ENABLE_PIN, OUTPUT);
	pinMode(Z_ENABLE_PIN, OUTPUT);

	Serial.begin(9600);
	Serial.println("Starting z probe... DZ is difference in micrometers...");

	// disable motors
	digitalWrite(X_ENABLE_PIN, HIGH);
	digitalWrite(Y_ENABLE_PIN, HIGH);
	digitalWrite(Z_ENABLE_PIN, HIGH);
	
	digitalWrite(Z_STEP_PIN, LOW);
	digitalWrite(Z_DIR_PIN, LOW);

#ifdef USE_LCD
	lcd.begin(LCD_WIDTH, LCD_HEIGHT);
	buttons_init();
	lcd.clear();
#endif
	
	xpos= ypos= zpos= 0;

}

void waitForKey() {
#ifdef USE_LCD
	lcd.setCursor(0, 0);
	lcd.print("Click to start");
#endif
	
	Serial.println("Hit any key to start");
	while (Serial.available() && Serial.read()); // empty buffer
	while (!Serial.available()) if(getButton()) break;                 // wait for data
	while (Serial.available() && Serial.read()); // empty buffer again

#ifdef USE_LCD
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Probing....");
#endif

}

bool isProbe() {
	return digitalRead(Z_PROBE_PIN) == LOW;
}

// calculate delay in us for feed rate in mm/minute
unsigned int calcDelay(float feed_rate) {
	float spmin= (float)STEPS_PER_MM*feed_rate; // number of steps a minute
	float delayus= 60000000.0/spmin; // delay between steps in us
	//Serial.println(delayus);
	return delayus;
}

void z_up(int mm, float feed_rate) {
	unsigned long steps= (unsigned long)mm * STEPS_PER_MM;
	unsigned int delay= calcDelay(feed_rate);
	
	digitalWrite(Z_DIR_PIN, HIGH);
	for(unsigned long i=0;i<steps;i++) {
		digitalWrite(Z_STEP_PIN, HIGH);
		delayMicroseconds(1);
		digitalWrite(Z_STEP_PIN, LOW);
		delayMicroseconds(delay);
		zpos++;
	}
}

bool z_down(int mm, float feed_rate) {
	unsigned long steps= (unsigned long)mm * STEPS_PER_MM;
	unsigned int delay= calcDelay(feed_rate);

	digitalWrite(Z_DIR_PIN, LOW);
	for(unsigned long i=0;i<steps;i++) {
		digitalWrite(Z_STEP_PIN, HIGH);
		delayMicroseconds(1);
		digitalWrite(Z_STEP_PIN, LOW);
		zpos--;
		if(isProbe()){
			return true;
		}
		delayMicroseconds(delay);
	}
	return false;
}

void move_x(int x) {
	int dir;
	if(x > 0) {
		digitalWrite(X_DIR_PIN, HIGH);
		dir= 1;
	}else {
		digitalWrite(X_DIR_PIN, LOW);
		dir= -1;
	}

	x= abs(x);
	
	unsigned long steps= (unsigned long)x * STEPS_PER_MM;
	unsigned int delay= calcDelay(XY_FEED_RATE);
	for(unsigned long i=0;i<steps;i++) {
		digitalWrite(X_STEP_PIN, HIGH);
		delayMicroseconds(1);
		digitalWrite(X_STEP_PIN, LOW);
		delayMicroseconds(delay);
		xpos+=dir;
	}

}

void move_y(int y) {
	int dir;
	if(y > 0) {
		digitalWrite(Y_DIR_PIN, HIGH);
		dir= 1;
	}else {
		digitalWrite(Y_DIR_PIN, LOW);
		dir= -1;
	}

	y= abs(y);

	unsigned long steps= (unsigned long)y * STEPS_PER_MM;
	unsigned int delay= calcDelay(XY_FEED_RATE);
	for(unsigned long i=0;i<steps;i++) {
		digitalWrite(Y_STEP_PIN, HIGH);
		delayMicroseconds(1);
		digitalWrite(Y_STEP_PIN, LOW);
		delayMicroseconds(delay);
		ypos+=dir;
	}

}

/**
 * probe the 4 corners
 * 
 */

long doProbe() {
	long z;
	while(1) {
		if(z_down(100, 200)) {
			z= zpos;
			break;
		}
	}


	// then another 2mm
	z_up(2, 1000);
	
	return z;
}

void printPos(const char *str, float z) {
	float x= 0, y= 0;
	x= (float)xpos/STEPS_PER_MM;
	y= (float)ypos/STEPS_PER_MM;

	Serial.print(str); Serial.print(": ");
// 	Serial.print(x); Serial.print(",");
	// 	Serial.print(y); Serial.print(",");
	float dz= ((z-zbase)*1000)/STEPS_PER_MM;
	Serial.print(dz); // micro meters
	Serial.println(" um");

#ifdef USE_LCD
	if(strcmp(str, "Front Left") == 0)
		lcd.setCursor(0, 3);
	else if(strcmp(str, "Front Right") == 0)
		lcd.setCursor(10, 3);
	else if(strcmp(str, "Back Left") == 0)
		lcd.setCursor(0, 1);
	else if(strcmp(str, "Back Right") == 0)
		lcd.setCursor(10, 1);
	else if(strcmp(str, "Center") == 0)
		lcd.setCursor(5, 2);

	lcd.print(dz);
#endif
}

void loop()
{
	int delta= 10;
	int xdir= delta;
	int ydir= delta;

	// disable motors so we can set home
	digitalWrite(X_ENABLE_PIN, HIGH);
	digitalWrite(Y_ENABLE_PIN, HIGH);
	digitalWrite(Z_ENABLE_PIN, HIGH);

	waitForKey();

	//enable motors
	digitalWrite(X_ENABLE_PIN, LOW);
	digitalWrite(Y_ENABLE_PIN, LOW);
	digitalWrite(Z_ENABLE_PIN, LOW);
	xpos= ypos= zpos= 0;
	

	// probe front left
	float lastz= 0;
	float maxd= 0;
	for(int i=0;i<4;i++) {
		float z= doProbe();
		if(lastz != 0 && z != lastz) {
			float err= ((lastz-z)*1000)/STEPS_PER_MM;
			Serial.print("Error= "); Serial.print(err); Serial.println(" um");
			maxd= max(z, maxd);
		}
		lastz= z;
	}
	
	zbase= lastz;
	printPos("Front Left", lastz);
	
	// back left
	move_y(YMAX);
	printPos( "Back Left", doProbe() );

	// back right
	move_x(XMAX);
	printPos( "Back Right", doProbe() );

	// front right
	move_y(-YMAX);
	printPos( "Front Right", doProbe() );

        // middle
        move_y(YMAX/2);
        move_x(-XMAX/2);
 	printPos( "Center", doProbe() );
       
	// back home front left
	move_x(-XMAX/2);
        move_y(-YMAX/2);
	printPos( "Front Left", doProbe() );
	
}


