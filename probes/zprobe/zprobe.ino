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

// the pin used for the Z probe, I use the Z min limit switch, but any pin can be used
#define Z_PROBE_PIN Z_MIN_PIN

// set your steps per mm here, found in Marlin/Configure.h
#define STEPS_PER_MM 400.0F

// set your max speed here in mm/min
#define XY_FEED_RATE 1500.0F


// set the position of your edges in mm, starts at 0,0
#define XMAX 190
#define YMAX 150

long zpos= 0;
long xpos= 0;
long ypos= 0;
float zbase= 0.0;


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

	xpos= ypos= zpos= 0;

}

void waitForKey() {
	Serial.println("Hit any key to start, hit any key to stop");
	while (Serial.available() && Serial.read()); // empty buffer
	while (!Serial.available());                 // wait for data
	while (Serial.available() && Serial.read()); // empty buffer again
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
	Serial.print(((z-zbase)*1000)/STEPS_PER_MM); // micro meters
	Serial.println(" um");
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
	for(int i=0;i<4;i++) {
		float z= doProbe();
		if(lastz != 0 && z != lastz) {
			float err= ((lastz-z)*1000)/STEPS_PER_MM;
			Serial.print("Error= "); Serial.print(err); Serial.println(" um");
		}
		lastz= z;
	}
	
	zbase= lastz;
	printPos("Front Left", zbase);
	
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


