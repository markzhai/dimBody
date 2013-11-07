#define V2
#ifdef V2

#define FULLSCREEN
//#define USE_GRAY_DIFF

#include "dimAppV2.h"
#include "processing.h"
#include "ofxCv.h"
#include <stdlib.h>     /* abs */
#include <windows.h>
using processing::constrain;

//----------------------------------------------------------------------------------------------- Global GUI Variables
int g_mouse_pushed = false;
int g_mouse_released = false;
int g_mouse_pressed = false;
int pmouseX, pmouseY;
bool autoDetect = true;

//----------------------------------------------------------------------------------------------- Colors & Buttons
int c_button_normal =      0x0080ff;
int c_button_over =        0x00c3ff;
int c_button_pushed =      0x00ffff;
int c_button_inactive =    0x606060;
int c_button_text =        0xffffff;
int c_button_triangle =    0xffffff;
int c_temp1, c_temp2, c_temp3;

bool lock_settings = false;
int margin = 10;
ofPoint ptTopLeft, ptBottomLeft, ptTopRight, ptBottomRight;
ofPoint ptWarpTopLeft, ptWarpBottomLeft, ptWarpTopRight, ptWarpBottomRight;

//----------------------------------------------------------------------------------------------- Other Globals
int fps_last_millis = 0, fps_new_millis = 0; // timer for calculating video FPS
int second = 0;
int cursor_timeout;
int video_fps;                          // video FPS
float scrollStringPosX = 0;
float blob_weight[21];                   // contains y-position of blob*size
float blob_weight_total;

cv::Mat screenMatBGR(SCREEN_WIDTH, SCREEN_HEIGHT, 24);
cv::Mat screenMatRGB(SCREEN_WIDTH, SCREEN_HEIGHT, 24);

ofImage dimColorImage;
ofxCvContourFinder contourFinder;
ofxCvGrayscaleImage grayCapturedImage;

//--------------------------------------------------------------
void dimAppV2::setup() {
	stage = stageSetup;

	ofSetWindowTitle("Auto Dim Screen");

	screenImage.allocate(SCREEN_WIDTH, SCREEN_HEIGHT, OF_IMAGE_COLOR);

	capturedImage.allocate(captureWidth, captureHeight);
	grayCapturedImage.allocate(captureWidth, captureHeight);
	warpCapturedImage.allocate(captureWidth, captureHeight);
	resultImage.allocate(screenImage.getWidth(), screenImage.getHeight());
	//resultImage.allocate(ofGetWindowWidth(), ofGetWindowHeight());
#ifdef USE_GRAY_DIFF
	grayImage.allocate(captureWidth, captureHeight);
	grayBg.allocate(captureWidth, captureHeight);
	grayDiff.allocate(captureWidth, captureHeight);
#endif
	resizedWarpCapturedImage.allocate(captureWidth, captureHeight, OF_IMAGE_COLOR);

	ofBackground(ofColor::black);

	videoGrabber.listDevices();
	videoGrabber.setDeviceID(0);
	//videoGrabber.setDesiredFrameRate(120);
	bool useCamera = videoGrabber.initGrabber(captureWidth, captureHeight);
	videoGrabber.setVerbose(true);
	capture = new WindowCapture(GetDesktopWindow());

	ptTopLeft.set(margin, margin, 0);
	ptTopRight.set(margin + captureWidth, margin, 0);
	ptBottomLeft.set(margin, margin + captureHeight, 0);
	ptBottomRight.set(margin + captureWidth, margin + captureHeight, 0);

	if (!useCamera) {
		cerr << "Initialize camera error!" << endl;
		exit();
	}

	gui.setup();

	lastElapsedTime = 0;
	stage = (dimAppStage)(stage + 1);
}

int newFrameCount = 0;
int times = 3;
bool updated = false;

//--------------------------------------------------------------
void dimAppV2::update() {
	videoGrabber.update();
	unsigned long long start_t;
	if (videoGrabber.isFrameNew()) {
		capturedImage.setFromPixels(videoGrabber.getPixels(), captureWidth, captureHeight);

		switch (stage) {
		case dimAppV2::stageSetup:
			cerr << "Error! Stage is setup in update() function!\n";	// should never come here
			exit();
			break;

		case dimAppV2::stageInput:
			ofSetWindowShape(1600, 900);
			capture->captureFrame(screenMatBGR);
			cv::cvtColor(screenMatBGR, screenMatRGB, CV_BGR2RGB);
			ofxCv::toOf(screenMatRGB, screenImage);

			if (autoDetect) {
				grayCapturedImage = capturedImage;
				grayCapturedImage.threshold(150);
				grayCapturedImage.adaptiveThreshold(31);
				contourFinder.findContours(grayCapturedImage, 10000, captureWidth * captureHeight, 10, false);
				if (contourFinder.nBlobs != 0) {
					ofRectangle boundingRect = contourFinder.blobs[0].boundingRect;
					for (int i = 1; i < contourFinder.nBlobs; ++i) {
						if (contourFinder.blobs[i].area > boundingRect.getArea())
							boundingRect = contourFinder.blobs[i].boundingRect;
					}
					boundingRect.translate(margin, margin);
					ptTopLeft = boundingRect.getTopLeft();
					ptTopRight = boundingRect.getTopRight();
					ptBottomLeft = boundingRect.getBottomLeft();
					ptBottomRight = boundingRect.getBottomRight();
				}
			}

			ptWarpTopLeft.set(ptTopLeft.x - margin, ptTopLeft.y - margin);
			ptWarpTopRight.set(ptTopRight.x - margin, ptTopRight.y - margin);
			ptWarpBottomLeft.set(ptBottomLeft.x - margin, ptBottomLeft.y - margin);
			ptWarpBottomRight.set(ptBottomRight.x - margin, ptBottomRight.y - margin);

			warpCapturedImage = capturedImage;
			warpCapturedImage.warpPerspective(ptWarpTopLeft, ptWarpTopRight, ptWarpBottomRight, ptWarpBottomLeft);
			resizedWarpCapturedImage.setFromPixels(warpCapturedImage.getPixelsRef());

			resizedWarpCapturedImage.resize(max(max(ptTopLeft.x, ptBottomLeft.x), max(ptTopRight.x, ptBottomRight.x)) - min(min(ptTopLeft.x, ptBottomLeft.x), min(ptTopRight.x, ptBottomRight.x)), 
				max(max(ptTopLeft.y, ptBottomLeft.y), max(ptTopRight.y, ptBottomRight.y)) - min(min(ptTopLeft.y, ptBottomLeft.y), min(ptTopRight.y, ptBottomRight.y)));

			break;

		case dimAppV2::stageConfig1:	// dim color configuration
			if (lastElapsedTime == 0)
				lastElapsedTime = ofGetElapsedTimeMillis();
			if (ofGetElapsedTimeMillis() - lastElapsedTime >= 1000) {
				lastElapsedTime = 0;
				warpCapturedImage = capturedImage;
				warpCapturedImage.warpPerspective(ptWarpTopLeft, ptWarpTopRight, ptWarpBottomRight, ptWarpBottomLeft);
				resizedWarpCapturedImage.setFromPixels(warpCapturedImage.getPixelsRef());

				resizedWarpCapturedImage.resize(max(max(ptTopLeft.x, ptBottomLeft.x), max(ptTopRight.x, ptBottomRight.x)) - min(min(ptTopLeft.x, ptBottomLeft.x), min(ptTopRight.x, ptBottomRight.x)), 
					max(max(ptTopLeft.y, ptBottomLeft.y), max(ptTopRight.y, ptBottomRight.y)) - min(min(ptTopLeft.y, ptBottomLeft.y), min(ptTopRight.y, ptBottomRight.y)));
				
				if (!dimColorImage.isAllocated())
					dimColorImage.allocate(resizedWarpCapturedImage.width, resizedWarpCapturedImage.height, OF_IMAGE_COLOR);

				for (int i = 0; i < resizedWarpCapturedImage.width / 2; ++i) {
					for (int j = 0; j < resizedWarpCapturedImage.height; ++j) {
						dimColorImage.setColor(i, j, resizedWarpCapturedImage.getPixelsRef().getColor(i, j));
					}
				}
				stage = (dimAppStage)(stage + 1);
			}
			break;
		case dimAppV2::stageConfig2:	// dim color configuration
			if (lastElapsedTime == 0)
				lastElapsedTime = ofGetElapsedTimeMillis();
			if (ofGetElapsedTimeMillis() - lastElapsedTime >= 1000) {
				lastElapsedTime = 0;
				warpCapturedImage = capturedImage;
				warpCapturedImage.warpPerspective(ptWarpTopLeft, ptWarpTopRight, ptWarpBottomRight, ptWarpBottomLeft);
				resizedWarpCapturedImage.setFromPixels(warpCapturedImage.getPixelsRef());

				resizedWarpCapturedImage.resize(max(max(ptTopLeft.x, ptBottomLeft.x), max(ptTopRight.x, ptBottomRight.x)) - min(min(ptTopLeft.x, ptBottomLeft.x), min(ptTopRight.x, ptBottomRight.x)), 
					max(max(ptTopLeft.y, ptBottomLeft.y), max(ptTopRight.y, ptBottomRight.y)) - min(min(ptTopLeft.y, ptBottomLeft.y), min(ptTopRight.y, ptBottomRight.y)));
				for (int i = resizedWarpCapturedImage.width / 2; i < resizedWarpCapturedImage.width; ++i) {
					for (int j = 0; j < resizedWarpCapturedImage.height; ++j) {
						dimColorImage.setColor(i, j, resizedWarpCapturedImage.getPixelsRef().getColor(i, j));
					}
				}
				resultImage.setDimColorImage(dimColorImage);
				stage = (dimAppStage)(stage + 1);
			}
			break;
		case dimAppV2::stagePrepare:
			start_t = ofGetElapsedTimeMillis();

			capture->captureFrame(screenMatBGR);
			cv::cvtColor(screenMatBGR, screenMatRGB, CV_BGR2RGB);
			ofxCv::toOf(screenMatRGB, screenImage);
			screenImage.update();
			//cout << ofGetWidth() << " : " << ofGetHeight() << endl;
			//screenImage.resize(ofGetWidth(), ofGetHeight());
			resultImage.setFromPixels(screenImage);
			cout << "Update Stage[prepare] costs seconds: " << (ofGetElapsedTimeMillis() - start_t) / 1000.0 << endl;
			break;
		case dimAppV2::stageOutput:
			start_t = ofGetElapsedTimeMillis();
			// Grab screenshot
			capture->captureFrame(screenMatBGR);					//0.04s
			cv::cvtColor(screenMatBGR, screenMatRGB, CV_BGR2RGB);	//0.005s
			ofxCv::toOf(screenMatRGB, screenImage);					//0.001s
			screenImage.update();
			//screenImage.resize(ofGetWidth(), ofGetHeight());		//0.4s

			//cout << "update stageOutput" << endl;

			warpCapturedImage = capturedImage;
			warpCapturedImage.warpPerspective(ptWarpTopLeft, ptWarpTopRight, ptWarpBottomRight, ptWarpBottomLeft);
			resizedWarpCapturedImage.setFromPixels(warpCapturedImage.getPixelsRef());

			resizedWarpCapturedImage.resize(max(max(ptTopLeft.x, ptBottomLeft.x), max(ptTopRight.x, ptBottomRight.x)) - min(min(ptTopLeft.x, ptBottomLeft.x), min(ptTopRight.x, ptBottomRight.x)), 
				max(max(ptTopLeft.y, ptBottomLeft.y), max(ptTopRight.y, ptBottomRight.y)) - min(min(ptTopLeft.y, ptBottomLeft.y), min(ptTopRight.y, ptBottomRight.y)));
			
			resultImage.setFromPixels(screenImage);
			resultImage.setThreshold(gui.getThreshold());
			
			resultImage.absDiff(screenImage, resizedWarpCapturedImage);
			cout << "Update stage[Output] costs seconds: " << (ofGetElapsedTimeMillis() - start_t) / 1000.0 << endl;
			//resultImage.resize(ofGetWidth(), ofGetHeight());
#ifdef USE_GRAY_DIFF
			grayImage.setROI(cropcapturedImage.getROI());
			grayImage.resize(cropcapturedImage.width, cropcapturedImage.height);
			grayImage = cropcapturedImage;
			grayBg.setROI(screencapturedImage.getROI());
			grayBg.resize(screencapturedImage.width, screencapturedImage.height);
			grayBg = screencapturedImage;
			grayDiff.resize(ptBottomRight.x - ptTopLeft.x, ptBottomRight.y - ptTopLeft.y);
			grayDiff.absDiff(grayBg, grayImage);	// compare current captured image and beforehand captured image
			grayDiff.threshold(gui.getThreshold());
			for (int i = 1; i <= gui.getErode(); ++i) {
				grayDiff.erode();
			}
			for (int i = 1; i <= gui.getDilate(); ++i) {
				grayDiff.dilate();
			}

			grayDiff.mirror(true, false);            //start to find blobs from bottom of image
			contourFinder.findContours(grayDiff, 30, 200000, 20, false);
			grayDiff.mirror(true, false);

			// weight blobs according to their position and size
			blob_weight_total = 0;
			// cycle through all detected blobs
			for (int i = 0; i < contourFinder.nBlobs; ++i) {
				// as blobs were detected in y-flipped image: y-flip centroid of blobs
				contourFinder.blobs[i].centroid.y = captureHeight - contourFinder.blobs[i].centroid.y;
				//just care about blobs inside ROI
				if (contourFinder.blobs[i].centroid.y >= ptTopLeft.y && contourFinder.blobs[i].centroid.y <= ptBottomLeft.y) {
					// weight blobs according to their height (lower = more important)
					blob_weight[i] = contourFinder.blobs[i].area * (contourFinder.blobs[i].centroid.y - ptTopLeft.y);
					// weight blobs according to curtain range (more outside range=less important)
					blob_weight[i] = blob_weight[i] * constrain(processing::map((float)contourFinder.blobs[i].centroid.x, 0, ptTopLeft.x, 0, 1), 0, 1);
					blob_weight[i] = blob_weight[i] * constrain(processing::map((float)contourFinder.blobs[i].centroid.x, captureWidth, ptTopRight.x, 0, 1), 0, 1);
					// blobs which are close to real curtain position are weighted three times as strong
					//if (contourFinder.blobs[i].centroid.x > pos_real - slider_close_area_size && contourFinder.blobs[i].centroid.x < pos_real + slider_close_area_size)
					//	blob_weight[i] *= 3;

				} else {
					blob_weight[i] = 0;
				}
				blob_weight_total += blob_weight[i];
			}

			// avoid division by zero if no blobs detected; use old position value instead
			if (blob_weight_total > 0) {
				// turn absolute blob_weight into relative value (0-1)
				for (int i = 0; i < int(constrain((float)contourFinder.nBlobs, 0, 20)); ++i) {
					blob_weight[i] = blob_weight[i] / blob_weight_total;
				}
			}
#endif
			break;
		default:
			break;
		}
	}
}

//--------------------------------------------------------------
void dimAppV2::draw() {
	ofSetHexColor(0xffffff);
	switch (stage) {
	case dimAppV2::stageInput:
		gui.draw();
		capturedImage.draw(margin, margin);
		//if (autoDetect) {
		//	contourFinder.draw(margin, margin);
		//	for (int i = 0; i < contourFinder.nBlobs; ++i)
		//		contourFinder.blobs[i].draw(margin, margin);
		//}
		resizedWarpCapturedImage.draw(margin, captureHeight + 1.5 * margin);
		screenImage.resize(captureWidth, captureHeight);
		screenImage.draw(resizedWarpCapturedImage.getWidth() + 2 * margin, captureHeight + 1.5 * margin);

#ifdef USE_GRAY_DIFF
		grayDiff.draw(captureWidth + margin * 4, margin);// draw difference image
		grayBg.draw(margin, captureHeight + 1.5 * margin);
		grayBg.draw(screenImage.getWidth(), captureHeight);
		grayImage.draw(grayBg.width + 2 * margin, captureHeight + 1.5 * margin);
		grayImage.draw(captureWidth + resizedWarpCapturedImage.getWidth(), captureHeight);
#endif
		//---------------------------------------------------------- draw vertical ROI
		c_temp1 = c_button_normal;
		c_temp2 = c_button_over;
		c_temp3 = c_button_pushed;
		c_button_normal = 0xa0a0a0;
		c_button_over = 0xcccccc;
		c_button_pushed = 0xdddddd;

		if (g_mouse_pushed == 1) {
			ptTopLeft.set(mouseX, mouseY, 0);
		} else if (g_mouse_pushed == 2) {
			ptBottomLeft.set(mouseX, mouseY, 0);
		} else if (g_mouse_pushed == 3) { 
			ptTopRight.set(mouseX, mouseY, 0);
		} else if (g_mouse_pushed == 4) {
			ptBottomRight.set(mouseX, mouseY, 0);
		}

		button(1, "", ptTopLeft.x - margin / 2, ptTopLeft.y - margin / 2, margin, margin, !lock_settings);
		button(2, "", ptBottomLeft.x - margin / 2, ptBottomLeft.y - margin / 2, margin, margin, !lock_settings);
		button(3, "", ptTopRight.x - margin / 2, ptTopRight.y - margin / 2, margin, margin, !lock_settings);
		button(4, "", ptBottomRight.x - margin / 2, ptBottomRight.y - margin / 2, margin, margin, !lock_settings);

		c_button_normal = c_temp1;
		c_button_over = c_temp2;
		c_button_pushed = c_temp3;

		ofSetHexColor(0xa0a0a0);
		ofLine(ptTopLeft, ptBottomLeft);
		ofLine(ptBottomLeft, ptBottomRight);
		ofLine(ptTopRight, ptBottomRight);
		ofLine(ptTopLeft, ptTopRight);
		break;
	case dimAppV2::stageConfig1:
		ofSetColor(ofColor::white);
		ofRect(ofGetWindowWidth() - 100, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::red);
		ofRect(ofGetWindowWidth() - 200, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::green);
		ofRect(ofGetWindowWidth() - 300, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::blue);
		ofRect(ofGetWindowWidth() - 400, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::aqua);
		ofRect(ofGetWindowWidth() - 500, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::magenta);
		ofRect(ofGetWindowWidth() - 600, 0, 100, ofGetWindowHeight());
		break;
	case dimAppV2::stageConfig2:
		ofSetColor(ofColor::white);
		ofRect(0, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::red);
		ofRect(100, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::green);
		ofRect(200, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::blue);
		ofRect(300, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::aqua);
		ofRect(400, 0, 100, ofGetWindowHeight());
		ofSetColor(ofColor::magenta);
		ofRect(500, 0, 100, ofGetWindowHeight());
		break;
	case dimAppV2::stagePrepare:
		screenImage.draw(0, 0);
		//resultImage.resize(ofGetWidth(), ofGetHeight());
		//resultImage.draw(0, 0);	// draw difference image
		break;
	case dimAppV2::stageOutput:
		//cout << "draw stageOutput" << endl;
		screenImage.draw(0, 0);
		resultImage.drawContour(0, 0);

		//resultImage.drawBlob(screenImage);
		//resultImage.draw(0, 0);	// draw difference image
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------
void dimAppV2::keyPressed(int key) {
	if (key == 'n'){
		if (stage == stageOutput)
			stage = stageInput;
		else 
			stage = (dimAppStage)(stage + 1);
	}
	gui.keyPressed(key);
}

//--------------------------------------------------------------
void dimAppV2::keyReleased(int key) {

}

//--------------------------------------------------------------
void dimAppV2::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void dimAppV2::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void dimAppV2::mousePressed(int x, int y, int button) {
	g_mouse_pressed = true;
	autoDetect = false;

	//cout << x << " : " << y << endl;
	resultImage.printDebugInfo = true;
	resultImage.debug_x = x;
	resultImage.debug_y = y;
}

//--------------------------------------------------------------
void dimAppV2::mouseReleased(int x, int y, int button) {
	g_mouse_pressed = false;
}

//--------------------------------------------------------------
void dimAppV2::windowResized(int w, int h) {
	gui.windowResized(w, h);
}

//--------------------------------------------------------------
void dimAppV2::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void dimAppV2::dragEvent(ofDragInfo dragInfo) { 

}

bool dimAppV2::button(int index, string name, int x_pos, int y_pos, int x_size, int y_size, bool active) {
	bool mouse_over = false;
	bool released = false;
	if (active) {
		if (mouseX >= x_pos && mouseX <= x_pos + x_size && mouseY >= y_pos && mouseY <= y_pos + y_size)
			mouse_over = true;
		if (mouse_over && g_mouse_pushed == 0 && g_mouse_pressed)
			g_mouse_pushed = index;
		if (g_mouse_pushed == index && !g_mouse_pressed) {
			g_mouse_pushed = 0;
			if (mouse_over) {
				g_mouse_released = index;
				released = true;
			}
		}
	}
	if (active) {
		ofSetHexColor(c_button_normal);			// normal button color
		if (mouse_over && g_mouse_pushed == 0)
			ofSetHexColor(c_button_over);		// over button color
		if (g_mouse_pushed == index)
			ofSetHexColor(c_button_pushed);		// pushed button color
	} else {
		ofSetHexColor(c_button_inactive);
	}
	ofRect(x_pos, y_pos, x_size, y_size);
	ofSetHexColor(c_button_text); //font color
	gui.drawSmallString(name, x_pos + 6, y_pos + y_size / 2 + 4);
	return released;
}
#endif