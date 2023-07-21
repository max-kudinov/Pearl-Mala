#include <Arduino.h>
#include <FastLED.h>

#define NUM_LEDS 60
#define NUM_SPHERES 19
#define LEDS_PER_SPHERE 3
#define MANUAL_BRIGHTNESS 255
#define AUTO_BRIGHTNESS 200
#define BRIGHTNESS_DELAY 10 
#define INACTIVE_BRIGHTNESS 30
#define NUM_PEARLS_IN_GRADIENT 10
#define PEARL_DELAY 30
#define FIRE_DELAY 30
// #define IDLE_TIMEOUT 600000
#define IDLE_TIMEOUT 10000
// #define NEW_SPHERE_PERIOD 25 * 1000
#define NEW_SPHERE_PERIOD 6000
#define STRIP_PIN A0
#define BUTTON_PIN A1

CRGB leds[NUM_LEDS];

// For testing

CRGB colors[8][3] =	{{{255, 0, 0}, {0, 0, 0}, {0, 0, 0}},				//idle
					{{0, 0, 0}, {0, 255, 0}, {0, 0, 0}},				//1
					{{0, 0, 255}, {0, 0, 0}, {0, 0, 0}},			//2
					{{255, 0, 0}, {0, 255, 0}, {0, 0, 255}},			//3
					{{255, 225, 11}, {255, 153, 0}, {255, 51, 51}},				//4
					{{255, 153, 0}, {255, 51, 51}, {255, 102, 204}},			//5
					{{255, 102, 204}, {153, 255, 255}, {255, 255, 217}},		//6	
					{{255, 255, 11}, {153, 255, 255}, {255, 255, 217}}};		//7

// CRGB colors[8][3] =	{{{255, 0, 0}, {255, 0, 0}, {255, 0, 0}},				//idle
// 					{{0, 255, 0}, {0, 255, 0}, {0, 255, 0}},				//1
// 					{{0, 0, 255}, {0, 0, 0}, {0, 0, 0}},			//2
// 					{{102, 204, 102}, {204, 255, 51}, {255, 225, 11}},			//3
// 					{{255, 225, 11}, {255, 153, 0}, {255, 51, 51}},				//4
// 					{{255, 153, 0}, {255, 51, 51}, {255, 102, 204}},			//5
// 					{{255, 102, 204}, {153, 255, 255}, {255, 255, 217}},		//6	
// 					{{255, 255, 11}, {153, 255, 255}, {255, 255, 217}}};		//7

// CRGB colors[8][3]= {{{0, 51, 204}, {51, 0, 153}, {153, 51, 204}},				//idle
// 							{{0, 51, 204}, {153, 51, 204}, {51, 204, 204}},				//1
// 							{{153, 51, 204}, {51, 204, 204}, {102, 204, 102}},			//2
// 							{{102, 204, 102}, {204, 255, 51}, {255, 225, 11}},			//3
// 							{{255, 225, 11}, {255, 153, 0}, {255, 51, 51}},				//4
// 							{{255, 153, 0}, {255, 51, 51}, {255, 102, 204}},			//5
// 							{{255, 102, 204}, {153, 255, 255}, {255, 255, 217}},		//6	
// 							{{255, 255, 11}, {153, 255, 255}, {255, 255, 217}}};		//7

CRGB fireColors[3] = {{255, 0, 0}, {255, 128, 0}, {255, 255, 0}};


unsigned long previousPress = 0;
unsigned long previousPearlUpdate = 0;
unsigned long previousFireUpdate = 0;
unsigned long previousSphere = 0;
unsigned long previousBrightnessCheck = 0;

bool autoMode = false;
int globalBrightness = MANUAL_BRIGHTNESS;

int colorBlend[NUM_SPHERES];
uint8_t shift [NUM_SPHERES];
uint8_t sphereBrightness[NUM_SPHERES];
uint8_t transitionBrightness[NUM_SPHERES];
bool transition[NUM_SPHERES];
bool descending[NUM_SPHERES];

uint8_t fireColorBlend = 0;
uint8_t fireShift = 0;

uint8_t circle = 3;
uint8_t activeSpheres = 0;

void mainAnimation();
void checkButton();
void finalAnimation();
void fireAnimation();
// void startup();
void autoplay();
void reset();
void newSphere();
void blackout();
void changeBrightness();
void smoothBrightness();
void setBrightness(int sphere, int isActive);
void initStates();

void setup() {
	randomSeed(analogRead(A5));
	initStates();
	FastLED.addLeds<WS2811, STRIP_PIN, GRB>(leds, NUM_LEDS);
	FastLED.setBrightness(MANUAL_BRIGHTNESS);
	pinMode(BUTTON_PIN, INPUT_PULLUP);
	// startup();
	Serial.begin(9600);
}

void loop() {
	if(!autoMode && millis() - previousPress > IDLE_TIMEOUT) {
		autoMode = true;
		reset();
	}

	if(autoMode) {
		autoplay();
	}

	if(millis() - previousPearlUpdate > PEARL_DELAY){
		mainAnimation();
		previousPearlUpdate = millis();
	}

	if(millis() - previousFireUpdate > FIRE_DELAY){
		fireAnimation();
		previousFireUpdate = millis();
	}

	if(millis() - previousBrightnessCheck > BRIGHTNESS_DELAY) {
		changeBrightness();
		previousBrightnessCheck = millis();
	}

	checkButton();

	// for(int i = 0; i < NUM_LEDS; i++) {
	// 	leds[i] = CRGB::White;
	// 	FastLED.show();
	// }
}

void mainAnimation() {
	for(int i = 0; i < NUM_SPHERES; i++) {
		int inc = 8;
		if(i < activeSpheres && !descending[i]) inc *= 2;
        colorBlend[i] += inc;
        if (colorBlend[i] > 255) 
		{
			colorBlend[i] = 0;
			shift[i]++;
		}
	}

	for(int i = 0; i < (NUM_LEDS - LEDS_PER_SPHERE); i += LEDS_PER_SPHERE) {
		int currentSphere = i / LEDS_PER_SPHERE;
		int currentShift = shift[currentSphere];
		int currentBlend = colorBlend[currentSphere];
		int isActive = 0;

        if(currentSphere < activeSpheres && !descending[currentSphere]) isActive = 1;
		setBrightness(currentSphere, isActive);

        for(int j = 0; j < 3; j++) {
            CRGB color1 = colors[circle + isActive][(currentShift + j) % 3];
            CRGB color2 = colors[circle + isActive][(currentShift+ j + 1) % 3];
            leds[i + j] = blend(color1, color2, currentBlend);
			leds[i + j] = blend(CRGB::Black, leds[i + j], sphereBrightness[currentSphere]);
        }

        if(transition[currentSphere]) {
            if(descending[currentSphere]) {
                transitionBrightness[currentSphere] -= 25;
                if (transitionBrightness[currentSphere] == 0) descending[currentSphere] = false;
            }
            else {
                transitionBrightness[currentSphere] += 5;
                if (transitionBrightness[currentSphere] == 250) transition[currentSphere] = false;
            }

            for(int j = 0; j < 3; j++) {
				leds[i + j] = blend(CRGB::Black, leds[i + j], transitionBrightness[currentSphere]);
            }
        }
	}
	FastLED.show();
}

void fireAnimation() {
	fireColorBlend += 8;
	if(fireColorBlend == 0) fireShift++;
	for(int j = 0; j < 3; j++) {
		CRGB color1 = fireColors[(fireShift + j) % 3];
		CRGB color2 = fireColors[(fireShift + j + 1) % 3];
		leds[NUM_LEDS - LEDS_PER_SPHERE + j] = blend(color1, color2, fireColorBlend);
	}
}

void setBrightness(int sphere, int isActive) {
	if(isActive == 1) {
		int position = activeSpheres - sphere + 1;
		int brightGradient = 255 - ((255 - INACTIVE_BRIGHTNESS + 30) / NUM_PEARLS_IN_GRADIENT) * position;

		if(position < NUM_PEARLS_IN_GRADIENT && sphereBrightness[sphere] < brightGradient) sphereBrightness[sphere]++;
		else if(position < NUM_PEARLS_IN_GRADIENT && sphereBrightness[sphere] > brightGradient) sphereBrightness[sphere]--;
		else if (position >= NUM_PEARLS_IN_GRADIENT && sphereBrightness[sphere] > INACTIVE_BRIGHTNESS + 30) sphereBrightness[sphere]--;
	}
	else if(isActive == 0 && sphereBrightness[sphere] > INACTIVE_BRIGHTNESS) sphereBrightness[sphere]--;
}

void finalAnimation() {
	for(int i = 0; i < NUM_LEDS; i++) {
		leds[i] = CRGB::White;
		FastLED.show();
		delay(20);
	}
	blackout();
	// startup();
}

// void startup() {
// 	fireAnimation();
// 	for(int i = 0; i < 255; i++) {
// 		for(int j = 0; j < NUM_LEDS - LEDS_PER_SPHERE; j += LEDS_PER_SPHERE) {
// 			leds[j] = blend(CRGB::Black, colors[circle][0], i);
// 			leds[j+1] = blend(CRGB::Black, colors[circle][1], i);
// 			leds[j+2] = blend(CRGB::Black, colors[circle][2], i);
// 			setBrightness(j, 0);
// 			setBrightness(j + 1, 0);
// 			setBrightness(j + 2, 0);
// 		}
// 		FastLED.show();
// 		delay(5);
// 	}
// }

void newSphere() {
	activeSpheres++;

	if(activeSpheres == NUM_SPHERES + 1) {
		activeSpheres = 0;
		circle++;
		if(circle == 8) reset();
	}
	else {
		transitionBrightness[activeSpheres - 1] = 250;
		transition[activeSpheres - 1] = true;
		descending[activeSpheres - 1] = true;
	}
}

void autoplay() {
	if(millis() - previousSphere > NEW_SPHERE_PERIOD) {
		newSphere();
		previousSphere = millis();
	}
}

void reset() {
	circle = 0;
	activeSpheres = 0;
	finalAnimation();
}

void blackout() {
	for(int i = 0; i < NUM_LEDS; i++) {
		leds[i] = CRGB::Black;
		FastLED.show();
		delay(20);
	}
}

void changeBrightness() {
	if(autoMode && globalBrightness != AUTO_BRIGHTNESS) {
		if (globalBrightness > AUTO_BRIGHTNESS) globalBrightness--;
		else if (globalBrightness < AUTO_BRIGHTNESS) globalBrightness++;
	}
	if(!autoMode && globalBrightness != MANUAL_BRIGHTNESS) {
		if (globalBrightness > MANUAL_BRIGHTNESS) globalBrightness--;
		else if (globalBrightness < MANUAL_BRIGHTNESS) globalBrightness++;
	}
	FastLED.setBrightness(globalBrightness);
}

void checkButton() {
	if(millis() - previousPress > 250 && !digitalRead(BUTTON_PIN)){
		if(autoMode) {
			autoMode = false;
			reset();
		}
		else newSphere();
		previousPress = millis();
	}
}

void initStates() {
	for(int i = 0; i < NUM_SPHERES; i++) {
		colorBlend[i] = random(0, 32) * 8;
		shift[i] = random(0, 3);
		sphereBrightness[i] = INACTIVE_BRIGHTNESS;
		transitionBrightness[i] = 250;
		transition[i] = false;
		descending[i] = false;
	}
}

