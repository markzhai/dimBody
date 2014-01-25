//#define V1
#define V2

#include "myGui.h"

float version = 2.0;
float versionStrPosX = 0;
int minThresholdRGB = 1, maxThresholdRGB = 765;
int minThresholdHSB = 1, maxThresholdHSB = 360;

myGui::myGui(void) {
	grabberOpened = false;
}

myGui::~myGui(void) {

}

void myGui::setup() {
	myFont.loadFont("SourceCodePro-Regular.ttf", 14, true, true, false);
	mySmallFont.loadFont("SourceCodePro-Regular.ttf", 10, true, true, false);
	ofSetHexColor(0xffffff);
	ofSetVerticalSync(true);

	gui.setDefaultWidth(200);
	//gui.setup("Panel", "settings.xml", ofGetWindowWidth() / 2, ofGetWindowHeight() / 2 + 100);
	gui.setup("Panel", "settings.xml", ofGetWindowWidth()/2 + 360);

	//gui.add(threshold.set("Threshold Gray Value", 24, 0, 255));
	gui.add(thresholdRGB.set("RGB Threshold", 450, minThresholdRGB, maxThresholdRGB));
	gui.add(thresholdHSB.set("HSB Threshold", 80, minThresholdHSB, maxThresholdHSB));

	gui.add(erode.set("Erode", 0, 0, 2));
	gui.add(dilate.set("Dilate", 1, 0, 2));

#ifdef V1
	gui.add(lpf1.set("LPF 1", 3, 0, 40));
	gui.add(lpf2.set("LPF 2", 3, 0, 40));

	ofParameterGroup anticipationFactors;
	anticipationFactors.setName("Anticipation Factor");
	anticipationFactors.add(anticiSpeed.set("Speed", 5, 0, 5000));
	anticipationFactors.add(anticiAccel.set("Acceleration", 5, 0, 50000));
	gui.add(anticipationFactors);
#endif
	gui.add(bFlipHorizontally.set("Flip Horizontally", false));
	gui.add(bFilpVertically.set("Flip Vertically", false));

	gui.add(color.set("color",ofColor(255, 255, 255),ofColor(0,0),ofColor(255,255)));
	gui.add(screenSize.set("screenSize", ""));
	
	ofParameterGroup transformFactors;
	transformFactors.setName("Camera Transform Factor");
	transformFactors.add(angle.set("Angle", 0, -30, 30));
	transformFactors.add(scale.set("Scale", ofVec2f(1, 1), ofVec2f(0.5, 0.5), ofVec2f(1.5, 1.5)));
	transformFactors.add(move.set("Move", ofVec2f(0, 0), ofVec2f(-10, -10), ofVec2f(10, 10)));
	gui.add(transformFactors);

	bHide = true;
}

void myGui::draw() {
	if (versionStrPosX >= ofGetWindowWidth())
		versionStrPosX = 0;
	drawString("Autodimming screen - Version "+ ofToString(version, 2), ++versionStrPosX, ofGetWindowHeight() - 5); //font is loadedp

	if (grabberOpened)
		drawString("Opened camera device successfully", ++versionStrPosX, ofGetWindowHeight() - 5);
	
	if (bHide){
		gui.draw();
	}
}

void myGui::drawString(std::string s, float x, float y) {
	ofSetColor(255, 0, 0);
	myFont.drawString(s, x, y);
	ofSetColor(255, 255, 255);
}

void myGui::drawSmallString(std::string s, float x, float y) {
	mySmallFont.drawString(s, x, y);
}

void myGui::keyPressed(int key){
	if (key == 'h'){
		bHide = !bHide;
	} else if (key == 's') {
		gui.saveToFile("settings.xml");
	} else if (key == 'l') {
		gui.loadFromFile("settings.xml");
	} else if (key == ' ') {
		color = ofColor(255);
	} else if (key == '=' || key == '+') {
		thresholdRGB += 10;
	} else if (key == '-' || key == '_') {
		thresholdRGB -= 10;
	} else if (key == ']' || key == '}') {
		thresholdHSB += 5;
	}  else if (key == '[' || key == '{') {
		thresholdHSB -= 5;
	} 
}