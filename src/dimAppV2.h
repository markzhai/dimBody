#ifndef DIM_APP_V2
#define DIM_APP_V2

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "myGui.h"
#include "screenCapture.h"
#include "zyfDiffImage.h"

#define SCREEN_WIDTH 1600
#define SCREEN_HEIGHT 900

// notes: setup() -> draw() -> update() -> draw() ...
class dimAppV2 : public ofBaseApp {

private:
	int captureWidth;
	int captureHeight;
	myGui gui;
	WindowCapture* capture;
	unsigned long long lastElapsedTime;

	enum dimAppStage {
		   stageSetup,	// Setup function
		   stageInput,	// User input to choose screen area
		   stageConfig1, // Config dim color of left side
		   stageConfig2, // Config dim color of right side
		   stagePrepare,// Prepare to ensure capture the correct image
		   stageOutput  // Loop of core system
	} stage;
	
	ofVideoGrabber videoGrabber;

	ofImage screenImage;
	ofxCvColorImage capturedImage;
	ofxCvColorImage warpCapturedImage;
	ofImage resizedWarpCapturedImage;
	
	/* deprecated */
	ofxCvGrayscaleImage grayImage;
	ofxCvGrayscaleImage grayBg;
	ofxCvGrayscaleImage grayDiff;
	ofxCvContourFinder contourFinder;

	zyfDiffImage resultImage;

public:
	dimAppV2(int width, int height):ofBaseApp() {
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
};

#endif