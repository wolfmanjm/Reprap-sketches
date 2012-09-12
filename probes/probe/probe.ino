
// Define pin-assignments
/*
#define STEPPING_DDR       DDRD
#define STEPPING_PORT      PORTD
#define X_STEP_BIT         2  // Uno Digital Pin 2
#define Y_STEP_BIT         3  // Uno Digital Pin 3
#define Z_STEP_BIT         4  // Uno Digital Pin 4
#define X_DIRECTION_BIT    5  // Uno Digital Pin 5
#define Y_DIRECTION_BIT    6  // Uno Digital Pin 6
#define Z_DIRECTION_BIT    7  // Uno Digital Pin 7

#define STEPPERS_DISABLE_DDR    DDRB
#define STEPPERS_DISABLE_PORT   PORTB
#define STEPPERS_DISABLE_BIT    0  // Uno Digital Pin 8

#define LIMIT_DDR     DDRB
#define LIMIT_PIN     PINB
#define X_LIMIT_BIT   1  // Uno Digital Pin 9
#define Y_LIMIT_BIT   2  // Uno Digital Pin 10
#define Z_LIMIT_BIT   3  // Uno Digital Pin 11
*/

#define GRBL

#ifdef GRBL
#define X_STEP_PIN 2
#define X_DIR_PIN 5
#define Y_STEP_PIN 3
#define Y_DIR_PIN 6
#define Z_STEP_PIN 4
#define Z_DIR_PIN 7

#define Z_LIMIT_PIN 11
#define XYZ_ENABLE_PIN 8
#else
// marlin

#endif

#define STEPS_PER_MM 400.0F
#define XY_FEED_RATE 800.0F

long zpos= 0;
long xpos= 0;
long ypos= 0;

void setup()
{
	pinMode(X_STEP_PIN, OUTPUT);
	pinMode(X_DIR_PIN, OUTPUT);
	pinMode(Y_STEP_PIN, OUTPUT);
	pinMode(Y_DIR_PIN, OUTPUT);
	pinMode(Z_STEP_PIN, OUTPUT);
	pinMode(Z_DIR_PIN, OUTPUT);
	
	pinMode(Z_LIMIT_PIN, INPUT_PULLUP);
	pinMode(XYZ_ENABLE_PIN, OUTPUT);

	Serial.begin(9600);
	Serial.println("Starting z probe... DZ is difference in micrometers...");

	// disable motors
	digitalWrite(XYZ_ENABLE_PIN, HIGH);
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
	return digitalRead(Z_LIMIT_PIN) == LOW;
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
 * Scan over the table and probe the table height at each stop
 * do two probes at each point and take average
 * returns steps in difference from first reference probe
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

void loop()
{
	int delta= 10;
	float x= 0, y= 0;
	int xdir= delta;
	int ydir= delta;
	float zbase= 0.0;
	bool first= true;

	// disable motors so we can set home
	digitalWrite(XYZ_ENABLE_PIN, HIGH);
	waitForKey();

	//enable motors
	digitalWrite(XYZ_ENABLE_PIN, LOW);
	xpos= ypos= zpos= 0;
	
	while(1) {
		if(Serial.available()){
			Serial.println("Cancelled");
			break;
		}
		
		float z1= doProbe();
		if(first) {
			float z2= doProbe();
			if(z1 != z2) {
				float err= ((z1-z2)*1000)/STEPS_PER_MM;
				Serial.print("Error= "); Serial.print(err); Serial.println(" um");
			}
			zbase= z2;
			first= false;
		}

		x= (float)xpos/STEPS_PER_MM;
		y= (float)ypos/STEPS_PER_MM;
		
		Serial.print(x); Serial.print(",");
		Serial.print(y); Serial.print(",");
		Serial.println(((z1-zbase)*1000)/STEPS_PER_MM); // micro meters

		if(x >= 100.0) {
			if(xdir > 0){
				move_y(ydir);
				xdir= -delta;
			}else
				move_x(xdir);
			

		}else if(x <= 0.0) {
			if(xdir < 0) {
				move_y(ydir);
				xdir= delta;
			}else
				move_x(xdir);
		
		}else{
			move_x(xdir);
		}
		
		if(y >= 100.0) {
			ydir= -delta;
		}else if(y <= 0.0){
			ydir= delta;
		}
	}
}


