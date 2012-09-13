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
#define XMAX 130
#define YMAX 170

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
	Serial.println("Starting...");

	// disable motors
	digitalWrite(X_ENABLE_PIN, HIGH);
	digitalWrite(Y_ENABLE_PIN, HIGH);
	digitalWrite(Z_ENABLE_PIN, HIGH);
	
	digitalWrite(Z_STEP_PIN, LOW);
	digitalWrite(Z_DIR_PIN, LOW);

	xpos= ypos= zpos= 0;

}

int waitForKey() {
	Serial.println("Enter speed in mm/min");
	while (Serial.available() && Serial.read()); // empty buffer
	char buf[32];
	while(1) {
		int n= Serial.readBytesUntil('\n', buf, 31);
		if(n == 0)
			continue;
		buf[n]= 0;
		return atoi(buf);
	}
}


// calculate delay in us for feed rate in mm/minute
unsigned int calcDelay(float feed_rate) {
	float spmin= (float)STEPS_PER_MM*feed_rate; // number of steps a minute
	float delayus= 60000000.0/spmin; // delay between steps in us
	//Serial.println(delayus);
	return delayus;
}


void move_x(int x, float fr) {
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
	unsigned int delay= calcDelay(fr);
	Serial.print("requested delay: "); Serial.println(delay);
	unsigned long cnt= 0;
	unsigned long d= 0;
	for(unsigned long i=0;i<steps;i++) {
		unsigned long n= micros();
		digitalWrite(X_STEP_PIN, HIGH);
		delayMicroseconds(1);
		digitalWrite(X_STEP_PIN, LOW);
		delayMicroseconds(delay-10);
		xpos+=dir;
		d += (micros()-n);
		cnt++;
	}
	Serial.print("actual delay: "); Serial.println(d/cnt);
}

void move_y(int y, float fr) {
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
	unsigned int delay= calcDelay(fr);
	for(unsigned long i=0;i<steps;i++) {
		digitalWrite(Y_STEP_PIN, HIGH);
		delayMicroseconds(1);
		digitalWrite(Y_STEP_PIN, LOW);
		delayMicroseconds(delay);
		ypos+=dir;
	}

}


void loop()
{
	// 2700 seems max
	int delta= 10;
	int xdir= delta;
	int ydir= delta;

	// disable motors so we can set home
	digitalWrite(X_ENABLE_PIN, HIGH);
	digitalWrite(Y_ENABLE_PIN, HIGH);
	digitalWrite(Z_ENABLE_PIN, HIGH);
	int fr= waitForKey();
	Serial.print("speed: "); Serial.println(fr);
	//enable motors
	digitalWrite(X_ENABLE_PIN, LOW);
	digitalWrite(Y_ENABLE_PIN, LOW);
	digitalWrite(Z_ENABLE_PIN, LOW);
	xpos= ypos= zpos= 0;
	
	move_x(150, fr);
	move_x(-150, fr);

	move_y(150, fr);
	move_y(-150, fr);
}


