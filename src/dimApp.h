#ifndef DIM_APP
#define DIM_APP

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "myGui.h"
#include "screenCapture.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

class dimApp : public ofBaseApp {
private:
	int captureWidth;
	int captureHeight;
	myGui gui;
	bool useCamera;
	WindowCapture* capture;

	void grabScreenshot();

public:
	dimApp(int width, int height):ofBaseApp() {
		captureWidth = width;
		captureHeight = height;
	}

	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	bool button(int index, string name, int x_pos, int y_pos, int x_size, int y_size, bool active);
	bool trigger(int index, string name, int x_pos, int y_pos, int x_size, int y_size,bool value, bool active);
	float lpf(int index, float new_value, int buffer_length);
	void fillRingBuffer(int index, float new_value, int buffer_length);
	float readRingBuffer(int index, int position, int buffer_length);

	void processScreenImage();

	ofVideoGrabber videoGrabber;
	ofVideoPlayer videoPlayer;

	ofImage screenImage;
	ofxCvColorImage colorImage;
	ofxCvGrayscaleImage grayImage;
	ofxCvGrayscaleImage grayBg;
	ofxCvGrayscaleImage grayDiff;
	ofxCvContourFinder contourFinder;
	ofSerial serial;
};

#endif