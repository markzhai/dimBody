//#define V1
#define V2

#include "myGui.h"

float version = 2.0;
float versionStrPosX = 0;

int minThreshold = 1;
int maxThreshold = 700;

myGui::myGui(void) {
	grabberOpened = false;
}

myGui::~myGui(void) {

}

void myGui::setup() {
	myFont.loadFont("SourceCodePro-Regular.ttf", 12, true, true, false);
	mySmallFont.loadFont("SourceCodePro-Regular.ttf", 9, true, true, false);
	ofSetHexColor(0xffffff);
	ofSetVerticalSync(true);

	gui.setDefaultWidth(200);
	//gui.setup("Panel", "settings.xml", ofGetWindowWidth() / 2, ofGetWindowHeight() / 2 + 100);
	gui.setup("Panel", "settings.xml", ofGetWindowWidth()/2 + 360);

	//gui.add(threshold.set("Threshold Gray Value", 24, 0, 255));
	gui.add(threshold.set("Threshold", 450, minThreshold, maxThreshold));


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

	//gui.add(center.set("center", ofVec2f(ofGetWidth()*.5,ofGetHeight()*.5),ofVec2f(0,0),ofVec2f(ofGetWidth(),ofGetHeight())));
	gui.add(color.set("color",ofColor(255, 255, 255),ofColor(0,0),ofColor(255,255)));
	//gui.add(circleResolution.set("circleRes", 5, 3, 90));
	//gui.add(twoCircles.setup("twoCircles"));
	//gui.add(ringButton.setup("ring"));
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
	//splashLogo.draw(0, 0);
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
	myFont.drawString(s, x, y);
}

void myGui::drawSmallString(std::string s, float x, float y) {
	mySmallFont.drawString(s, x, y);
}

void myGui::keyPressed(int key){
	if (key == 'h'){
		bHide = !bHide;
	}
	else if (key == 's') {
		gui.saveToFile("settings.xml");
	}
	else if (key == 'l') {
		gui.loadFromFile("settings.xml");
	}
	else if (key == ' ') {
		color = ofColor(255);
	}
	else if (key == '=') {
		threshold = threshold + 5;
	}
	else if (key == '-') {
		threshold = threshold - 5;
	}
}