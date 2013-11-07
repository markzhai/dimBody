#include "ofMain.h"
#include "dimApp.h"
#include "dimAppV2.h"
#include "ofAppGlutWindow.h"

/*
	Stage 1 -> Let user choose screen area
	Stage 2 -> Configuration stage, project a black image, get the image from the webcam and set as barrier color
	# difficulty
		auto dim scrren when full black
		different black because of light
	Stage 3 -> Loop to detect and dim screen
				- use harr finder to help locate
*/

//========================================================================
int main( ){
    cout <<"\nScreen Dimmer says hello!\n\n";
    cout << "Compiled with  ";
    cout << ofGetVersionInfo();
    cout << "Programmed for of version: 8\n\n";
	ofAppGlutWindow window;
	//ofSetupOpenGL(&window, 1600, 900, OF_FULLSCREEN);
	ofSetupOpenGL(&window, 1600, 900, OF_WINDOW);			// <-------- setup the GL context
	ofRunApp(new dimAppV2(640, 480));
}