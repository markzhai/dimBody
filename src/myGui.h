#pragma once

#include "ofMain.h"
#include "ofxGui.h"

class myGui {
public:
	myGui(void);
	~myGui(void);
	void setup();
	void draw();
	void update();
	void drawString(std::string s, float x, float y);
	void drawSmallString(std::string s, float x, float y);
	void windowResized(int w, int h) {screenSize = ofToString(w) + "x" + ofToString(h);};
	void keyPressed(int key);

	void setGrabber(bool opened) {
		grabberOpened = opened;
	};

	int getThreshold() {return threshold;}
	int getErode() {return erode;}
	int getDilate() {return dilate;}
	int getLpf1() {return lpf1;}
	int getLpf2() {return lpf2;}
	float getAnticiSpeed() {return anticiSpeed;}
	float getAnticiAccel() {return anticiAccel;}
	
	bool isFilpVertically() {return bFilpVertically;}
	bool isFlipHorizontally() {return bFlipHorizontally;}

	float getAngle() {return angle;}
	ofVec2f getCenter() {return center;}
	ofVec2f getScale() {return scale;}
	ofVec2f getMove() {return move;}

private:
	ofImage splashLogo;
	ofTrueTypeFont myFont;
	ofTrueTypeFont mySmallFont;
	bool grabberOpened;
	bool videoOpened;

	bool bHide;
	
	ofParameter<int> threshold;
	ofParameter<int> erode;
	ofParameter<int> dilate;
	ofParameter<int> lpf1;
	ofParameter<int> lpf2;
	ofParameter<bool> bFlipHorizontally;
	ofParameter<bool> bFilpVertically;
	
	ofParameter<float> anticiSpeed;
	ofParameter<float> anticiAccel;

	ofParameter<ofColor> color;
	
	ofParameter<float> angle;
	ofParameter<ofVec2f> center;
	ofParameter<ofVec2f> scale;
	ofParameter<ofVec2f> move;

	ofParameter<int> circleResolution;
	ofxButton twoCircles;
	ofxButton ringButton;
	ofParameter<string> screenSize;

	ofxPanel gui;
};