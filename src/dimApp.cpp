//#define V1
#ifdef V1

#include "dimApp.h"
#include "processing.h"
#include "ofxCv.h"

using processing::constrain;

//----------------------------------------------------------------------------------------------- Global GUI Variables

int g_mouse_pushed = false;
int g_mouse_released = false;
int g_mouse_pressed = false;
int pmouseX, pmouseY;
int request_box = 0;

//----------------------------------------------------------------------------------------------- Global Function Variables
float ringbuffer[20][500];       //general ringbuffer
float lpf_ringbuffer[20][130];   //general ringbuffer for low pass filter [# of ringbuffer][items]

//----------------------------------------------------------------------------------------------- Colors
int c_button_normal =      0x0080ff;
int c_button_over =        0x00c3ff;
int c_button_pushed =      0x00ffff;
int c_button_inactive =    0x606060;

int c_button_text =        0xffffff;
int c_button_triangle =    0xffffff;

int c_trigger_true =       0x57ad00;
int c_trigger_false =      0xad5700;
int c_trigger_true_over =  0x80ff00;
int c_trigger_false_over = 0xff8000;

int c_pos_detected =       0x00ff00;
int c_pos_lpf1 =           0x00ffff;
int c_pos_anticipate =     0x0000ff;
int c_pos_target =         0xff00ff;
int c_pos_real =           0xff0000;

int c_temp1, c_temp2, c_temp3;

//----------------------------------------------------------------------------------------------- Buttons
bool show_pos = true;
bool show_pos_lpf1 = true;
bool show_pos_anticipate = true;
bool show_pos_target = true;
bool scroll_box = true;
bool lock_settings = false;
bool manual_position = true;
bool expo_mode = false;           //triggers between setup and expo mode

int margin = 10;
int roi_top = 0, roi_left = 0, roi_bottom = 0, roi_right = 0;
int curtain_range_left = 0, curtain_range_right = 640;

int scroll_box_size = 150;       //size of scrollbox
int shift_number_position;      //x-position of numbers over scrollbox

//----------------------------------------------------------------------------------------------- Other Globals
int fps_last_millis = 0, fps_new_millis = 0; //timer for calculating video FPS
int second = 0;
int cursor_timeout;
int video_fps;                          //video FPS
float scrollStringPosX = 0;
float blob_weight[21];                   //contains y-position of blob*size
float blob_weight_total;
float pos_detected;
float pos_lpf1;
float pos_lpf1_old;                     //helper variable to calculate speed
float lpf1_speed;                       //contains speed of lowpassfiltered position
float lpf1_speed_old;                   //helper variable to calculate acceleration
float lpf1_accel;                       //contains acceleration of lowpassfiltered position
float pos_anticipate;
float pos_lpf2;
float pos_target;

//--------------------------------------------------------------
void dimApp::setup() {
	ofSetWindowTitle("Auto Dim Screen");
	
	screenImage.allocate(SCREEN_WIDTH, SCREEN_HEIGHT, OF_IMAGE_COLOR_ALPHA);
	colorImage.allocate(captureWidth, captureHeight);
	grayImage.allocate(captureWidth, captureHeight);
	grayBg.allocate(captureWidth, captureHeight);
	grayDiff.allocate(captureWidth, captureHeight);

	ofBackgroundGradient(ofColor::black, ofColor::gray);
	//ofBackground(50, 50, 50);
	
	videoGrabber.listDevices();
	videoGrabber.setDeviceID(0);
	useCamera = videoGrabber.initGrabber(captureWidth, captureHeight);
	videoGrabber.setVerbose(true);
	capture = new WindowCapture(GetDesktopWindow());

	roi_bottom = captureHeight;
	roi_right = captureWidth;

	if (!useCamera) {
		useCamera = false;
		if (!videoPlayer.loadMovie("testvideo.mov"))
			videoPlayer.loadMovie("testvideo.mp4");
		videoPlayer.play();
	}
	gui.setup();
}

//--------------------------------------------------------------
void dimApp::update() {
	bool newFrame;
	grabScreenshot();
	if (useCamera) {
		videoGrabber.update();
		newFrame = videoGrabber.isFrameNew();
	} else {
		videoPlayer.update();
		newFrame = videoPlayer.isFrameNew();
	}
	if (newFrame) {
		fps_last_millis = fps_new_millis;
		fps_new_millis = ofGetElapsedTimeMillis();
		video_fps = 1000 / (fps_new_millis - fps_last_millis);

		if (useCamera) {
			colorImage.setFromPixels(videoGrabber.getPixels(), captureWidth, captureHeight);
		} else{
			colorImage.setFromPixels(videoPlayer.getPixels(), captureWidth, captureHeight);
		}

		colorImage.mirror(gui.isFilpVertically(), gui.isFlipHorizontally());
		grayImage = colorImage;
		grayDiff.absDiff(grayBg, grayImage);	// compare current captured image and beforehand captured image
		grayBg = grayImage;
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
		for (int i = 0; i < int(constrain((float)contourFinder.nBlobs, 0, 20)); ++i) {
			// as blobs were detected in y-flipped image: y-flip centroid of blobs
			contourFinder.blobs[i].centroid.y = captureHeight - contourFinder.blobs[i].centroid.y;
			//just care about blobs inside ROI
			if (contourFinder.blobs[i].centroid.y >= roi_top && contourFinder.blobs[i].centroid.y <= roi_bottom) {
				// weight blobs according to their height (lower = more important)
				blob_weight[i] = contourFinder.blobs[i].area * (contourFinder.blobs[i].centroid.y - roi_top);
				// weight blobs according to curtain range (more outside range=less important)
				blob_weight[i] = blob_weight[i] * constrain(processing::map((float)contourFinder.blobs[i].centroid.x, 0, curtain_range_left, 0, 1), 0, 1);
				blob_weight[i] = blob_weight[i] * constrain(processing::map((float)contourFinder.blobs[i].centroid.x, captureWidth, (float)curtain_range_right, 0, 1), 0, 1);
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
			// weight x-positions according to blob_weight; calculate detected position
			if (contourFinder.nBlobs > 0)
				pos_detected = 0;  //reset detected positions if any blobs are found
			for (int i = 0; i < int(constrain((float)contourFinder.nBlobs, 0, 20)); ++i) {
				pos_detected += blob_weight[i]*contourFinder.blobs[i].centroid.x;
			}
		}

		// position filter pipeline
		pos_lpf1 = lpf(3, pos_detected, gui.getLpf1());
		lpf1_speed = (pos_lpf1 - pos_lpf1_old) / ((float)(fps_new_millis - fps_last_millis));
		pos_lpf1_old = pos_lpf1;

		lpf1_accel = (lpf1_speed - lpf1_speed_old) / ((float)(fps_new_millis - fps_last_millis));
		lpf1_speed_old = lpf1_speed;

		pos_anticipate = constrain(pos_lpf1 + (lpf1_speed * gui.getAnticiSpeed()) + (lpf1_accel * gui.getAnticiAccel()), 
			(float)curtain_range_left, (float)curtain_range_right);
		pos_lpf2 = lpf(4, pos_anticipate, gui.getLpf2());
		pos_target = constrain(pos_lpf2, (float)curtain_range_left, (float)curtain_range_right);

		// update ringbuffer scrollbox
		if (scroll_box) {
			float f;
			fillRingBuffer(5, (float)second, scroll_box_size);
			fillRingBuffer(6, constrain(pos_detected, 0, captureWidth), scroll_box_size);
			fillRingBuffer(7, constrain(pos_lpf1, 0, captureWidth), scroll_box_size);
			fillRingBuffer(8, constrain(pos_anticipate, 0, captureWidth), scroll_box_size);
			fillRingBuffer(9, constrain(pos_target, 0, captureWidth), scroll_box_size);
		}
	}
}

//--------------------------------------------------------------
void dimApp::draw() {
	gui.draw();
	
	processScreenImage();
	screenImage.draw(margin, captureHeight + margin); // draw the incoming image

	ofSetHexColor(0xffffff);
	colorImage.draw(margin, margin); // draw the incoming image
	ofSetHexColor(0x008888);
	grayDiff.draw(captureWidth + margin * 4, margin);// draw difference image

	//---------------------------------------------------------- draw vertical ROI
	c_temp1 = c_button_normal;
	c_temp2 = c_button_over;
	c_temp3 = c_button_pushed;
	c_button_normal = 0xa0a0a0;
	c_button_over = 0xcccccc;
	c_button_pushed = 0xdddddd;

	if (g_mouse_pushed == 1) {
		roi_top = int(constrain((float)mouseY - margin, 0, (float)roi_bottom - margin * 3));
	}
	if (g_mouse_pushed == 2) {
		roi_bottom = int(constrain((float)mouseY - margin, (float)roi_top + margin * 3, captureHeight));
	}

	button(1, "", captureWidth, roi_top + margin / 2, margin, margin, !lock_settings&&!request_box);
	button(2, "", captureWidth, roi_bottom + margin / 2, margin, margin, !lock_settings&&!request_box);

	c_button_normal = c_temp1;
	c_button_over = c_temp2;
	c_button_pushed = c_temp3;

	ofSetHexColor(0xa0a0a0);
	ofLine(margin, roi_top + margin, captureWidth + margin * 2, roi_top + margin);	//roi lines in video image left
	ofLine(margin, roi_bottom + margin, captureWidth + margin * 2, roi_bottom + margin);
	ofLine(captureWidth + margin * 2.5, roi_top + margin * 1.5, captureWidth + margin * 2.5, (roi_top + roi_bottom) / 2 + 2);
	ofLine(captureWidth + margin * 2.5, (roi_top + roi_bottom) / 2 + 18, captureWidth + margin * 2.5, roi_bottom + margin / 2);
	gui.drawString("ROI", captureWidth + margin * 1.5, (roi_top + roi_bottom) / 2 + margin * 1.5 - 1);
	ofLine(captureWidth + margin * 4, roi_top + margin, captureWidth * 2 + margin * 4, roi_top + margin);//roi lines in difference image right
	ofLine(captureWidth + margin * 4, roi_bottom + margin, captureWidth * 2 + margin * 4, roi_bottom + margin);

	//---------------------------------------------------------- draw horizon ROI
	c_temp1 = c_button_normal; c_temp2 = c_button_over; c_temp3 = c_button_pushed;
	c_button_normal = 0xa0a0a0;c_button_over = 0xcccccc;c_button_pushed = 0xdddddd;

	if (g_mouse_pushed == 3) { 
		curtain_range_left = int(constrain((float)mouseX - margin, 0, (float)curtain_range_right - 90));
	}
	if (g_mouse_pushed == 4) {
		curtain_range_right = int(constrain((float)mouseX-10, (float)curtain_range_left + 90, captureWidth));
	}

	button(3, "", curtain_range_left + margin / 2, captureHeight + margin * 2, margin, margin, !lock_settings && !request_box);
	button(4, "", curtain_range_right + margin / 2, captureHeight + margin * 2, margin, margin, !lock_settings && !request_box);
	c_button_normal = c_temp1;c_button_over = c_temp2;c_button_pushed = c_temp3;
	ofSetHexColor(0xa0a0a0);
	// curtain range lines in video image left
	ofLine(curtain_range_left + margin, margin, curtain_range_left + margin, captureHeight + margin * 2);
	ofLine(curtain_range_right + margin, margin, curtain_range_right + margin, captureHeight + margin * 2);
	ofLine(curtain_range_left + margin * 1.5, captureHeight + margin * 2.5, (curtain_range_left + curtain_range_right)/2-27, captureHeight + margin * 2.5);
	ofLine((curtain_range_left + curtain_range_right) / 2 + 48, captureHeight + margin * 2.5,curtain_range_right + margin / 2, captureHeight + margin * 2.5);
	gui.drawString("ROI", (curtain_range_left + curtain_range_right) / 2 - margin * 2.5 + 1, captureHeight + margin * 2.5 + 3);
	// curtain range lines in difference image right
	ofLine(captureWidth + margin * 4 + curtain_range_left, margin, captureWidth + margin * 4 + curtain_range_left, captureHeight + margin);
	ofLine(captureWidth + margin * 4 + curtain_range_right, margin, captureWidth + margin * 4 + curtain_range_right, captureHeight + margin);

	//---------------------------------------------------------- display weighted blobs
	ofNoFill();
	ofSetHexColor(0xff0000);
	for (int i = 0; i < int(constrain((float)contourFinder.nBlobs, 0, 20)); i++) {
		ofCircle(contourFinder.blobs[i].centroid.x + captureWidth + margin * 4, contourFinder.blobs[i].centroid.y + margin, blob_weight[i] * 20);
		ofLine(contourFinder.blobs[i].centroid.x + captureWidth + margin * 4 - 3, contourFinder.blobs[i].centroid.y + margin, 
			contourFinder.blobs[i].centroid.x + captureWidth + margin * 4, contourFinder.blobs[i].centroid.y + margin);
		ofLine(contourFinder.blobs[i].centroid.x + captureWidth + margin * 4, contourFinder.blobs[i].centroid.y + margin - 3, 
			contourFinder.blobs[i].centroid.x + captureWidth + margin * 4, contourFinder.blobs[i].centroid.y + margin + 3);
	}
	ofFill();

	//---------------------------------------------------------- display lines in video images
	if (show_pos) {
		ofSetHexColor(c_pos_detected);
		ofLine(int(constrain(pos_detected, 0, captureWidth)) + captureWidth + margin * 4, margin, int(constrain(pos_detected, 0, captureWidth)) + captureWidth + margin * 4, captureHeight + margin);
		ofLine(int(constrain(pos_detected, 0, captureWidth)) + margin, margin, int(constrain(pos_detected, 0, captureWidth)) + margin, captureHeight + margin);
	}
	if (show_pos_lpf1) {
		ofSetHexColor(c_pos_lpf1);
		ofLine(int(constrain(pos_lpf1, 0, captureWidth)) + captureWidth + margin * 4, margin, int(constrain(pos_lpf1, 0, captureWidth)) + captureWidth + margin * 4, captureHeight + margin);
		ofLine(int(constrain(pos_lpf1, 0, captureWidth)) + margin, margin, int(constrain(pos_lpf1, 0, captureWidth)) + margin, captureHeight + margin);
	}
	if (show_pos_anticipate) {
		ofSetHexColor(c_pos_anticipate);
		ofLine(int(constrain(pos_anticipate, 0, captureWidth)) + captureWidth + margin * 4, margin, int(constrain(pos_anticipate, 0, captureWidth)) + captureWidth + margin * 4, captureHeight + margin);
		ofLine(int(constrain(pos_anticipate, 0, captureWidth)) + margin, margin, int(constrain(pos_anticipate, 0, captureWidth)) + margin, captureHeight + margin);
	}
	if (show_pos_target) {
		ofSetHexColor(c_pos_target);
		ofLine(int(constrain(pos_target, 0, captureWidth)) + captureWidth + margin * 4, margin, int(constrain(pos_target, 0, captureWidth)) + captureWidth + margin * 4, captureHeight + margin);
		ofLine(int(constrain(pos_target, 0, captureWidth)) + margin, margin, int(constrain(pos_target, 0, captureWidth)) + margin, captureHeight + margin);
	}

	//---------------------------------------------------------- draw scrollbox
	for(int i = 0; i < scroll_box_size; ++i){                         //scrollbox, display seconds
		if (readRingBuffer(5, i, scroll_box_size) > readRingBuffer(5, i + 1,scroll_box_size))
			ofLine(captureWidth + margin * 4, captureHeight + margin * 4 + i, captureWidth * 2 + margin * 4, captureHeight + margin * 4 + i);
	}

	//---------------------------------------------------------- scrollbox, display colored lines
	for (int i = 1; i < scroll_box_size; ++i) {
		ofSetHexColor(c_pos_detected);
		if (show_pos)
			ofLine(readRingBuffer(6, i, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + i, 
			readRingBuffer(6, i + 1, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + 1 + i);
		ofSetHexColor(c_pos_lpf1);
		if (show_pos_lpf1)
			ofLine(readRingBuffer(7, i, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + i, 
			readRingBuffer(7, i + 1, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + 1 + i);
		ofSetHexColor(c_pos_anticipate);
		if (show_pos_anticipate)
			ofLine(readRingBuffer(8, i, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + i, 
			readRingBuffer(8, i + 1, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + 1 + i);
		ofSetHexColor(c_pos_target);
		if (show_pos_target)
			ofLine(readRingBuffer(9, i, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + i, 
			readRingBuffer(9, i + 1, scroll_box_size) + captureWidth + margin * 4, captureHeight + margin * 4 + 1 + i);
		ofSetHexColor(c_pos_real);
	}

	//---------------------------------------------------------- display values as numbers if mouse over scrollbox
	if (mouseX >= captureWidth + margin * 4 && mouseX <= captureWidth * 2 + margin * 4 && 
		mouseY >= captureHeight + margin * 4 && mouseY <= scroll_box_size + captureHeight + margin * 4) {
		ofSetHexColor(0x202020);
		ofRect(captureWidth + margin * 4, captureHeight + 14, captureWidth, 22);                                 //scrollbox,  gray bgnd box  -  colored numbers
		ofSetHexColor(0xffffff);
		ofLine(captureWidth + margin * 4, mouseY, captureWidth * 2 + margin * 4, mouseY);
		gui.drawString(ofToString(mouseY - captureHeight - margin * 4), captureWidth + margin * 4.5, captureHeight + margin * 2.5 + 3);
		shift_number_position = 25;

		if (show_pos) {
			ofSetHexColor(c_pos_detected);
			gui.drawString(ofToString(int(readRingBuffer(6, mouseY - captureHeight - margin * 4, scroll_box_size))), captureWidth + margin * 7.5 + shift_number_position, captureHeight + margin * 2.5 + 3);
			shift_number_position += 50;
		}
		if (show_pos_lpf1) {
			ofSetHexColor(c_pos_lpf1);
			gui.drawString(ofToString(int(readRingBuffer(7, mouseY - captureHeight - margin * 4, scroll_box_size))), captureWidth + margin * 7.5 + shift_number_position, captureHeight + margin * 2.5 + 3);
			shift_number_position += 50;
		}
		if (show_pos_anticipate) {
			ofSetHexColor(c_pos_anticipate);
			gui.drawString(ofToString(int(readRingBuffer(8, mouseY - captureHeight - margin * 4, scroll_box_size))), captureWidth + margin * 7.5 + shift_number_position, captureHeight + margin * 2.5 + 3);
			shift_number_position += 50;
		}
		if (show_pos_target) {
			ofSetHexColor(c_pos_target);
			gui.drawString(ofToString(int(readRingBuffer(9, mouseY - captureHeight - margin * 4, scroll_box_size))), captureWidth + margin * 7.5 + shift_number_position, captureHeight + margin * 2.5 + 3);
			shift_number_position += 50;
		}
	}

	//---------------------------------------------------------- draw scrollbox size
	c_temp1 = c_button_normal; c_temp2 = c_button_over; c_temp3 = c_button_pushed;
	c_button_normal = 0xa0a0a0; c_button_over=0xcccccc; c_button_pushed = 0xdddddd;
	if (g_mouse_pushed == 24) {
		scroll_box_size = int(constrain((float)mouseY - captureHeight - margin * 4, 1, 500));
	}
	button(24, "",  captureWidth + margin * 2,  scroll_box_size + captureHeight + margin * 3.5, 10, 10, !lock_settings && !request_box);
	c_button_normal=c_temp1; c_button_over=c_temp2; c_button_pushed=c_temp3;
	// scrollbox line
	ofSetHexColor(0xa0a0a0);
	ofLine(captureWidth + margin * 3, scroll_box_size + captureHeight + margin * 4, captureWidth * 2 + margin * 4, scroll_box_size + captureHeight + margin * 4);
	
	//---------------------------------------------------------- draw scrollbox size
	if (show_pos)
		ofSetHexColor(c_pos_detected);
	else
		ofSetHexColor(0x505050);
	ofRect(captureWidth * 2 + 54, captureHeight + margin * 3, 16, 16);
	show_pos = trigger(8, "Detected Position", captureWidth * 2 + 78, captureHeight + margin * 3, 144, 16, show_pos, !lock_settings&&!request_box);

	if (show_pos_lpf1)
		ofSetHexColor(c_pos_lpf1);
	else
		ofSetHexColor(0x505050);
	ofRect(captureWidth * 2 + 54, captureHeight + margin * 6, 16, 16);
	show_pos_lpf1 = trigger(10, "LPF Position", captureWidth * 2 + 78, captureHeight + margin * 6, 144, 16, show_pos_lpf1, !lock_settings&&!request_box);
	
	if (show_pos_anticipate)
		ofSetHexColor(c_pos_anticipate);
	else
		ofSetHexColor(0x505050);
	ofRect(captureWidth * 2 + 54, captureHeight + margin * 9, 16, 16);
	show_pos_anticipate = trigger(12, "Anticipated Position", captureWidth * 2 + 78, captureHeight + margin * 9, 144, 16, show_pos_anticipate, !lock_settings&&!request_box);
	
	if (show_pos_target)
		ofSetHexColor(c_pos_target);
	else
		ofSetHexColor(0x505050);
	ofRect(captureWidth * 2 + 54, captureHeight + margin * 12, 16, 16);
	show_pos_target = trigger(15, "Target Position", captureWidth * 2 + 78, captureHeight + margin * 12, 144, 16, show_pos_target, !lock_settings&&!request_box);

	//----------------------------------------------------------Draw Scroll String
	if (scrollStringPosX >= ofGetWindowWidth())
		scrollStringPosX = 0;
	else
		++ scrollStringPosX;
	gui.drawSmallString("Program LPS: " + ofToString((int)ofGetFrameRate()), ofGetWidth() - scrollStringPosX - 130, 10);
	gui.drawSmallString("FPS: " + ofToString(video_fps), ofGetWidth() - scrollStringPosX, 10);
}

//--------------------------------------------------------------
void dimApp::keyPressed(int key) {
	gui.keyPressed(key);
}

//--------------------------------------------------------------
void dimApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void dimApp::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void dimApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void dimApp::mousePressed(int x, int y, int button) {
	g_mouse_pressed = true;
}

//--------------------------------------------------------------
void dimApp::mouseReleased(int x, int y, int button) {
	g_mouse_pressed = false;
}

//--------------------------------------------------------------
void dimApp::windowResized(int w, int h) {
	gui.windowResized(w, h);
}

//--------------------------------------------------------------
void dimApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void dimApp::dragEvent(ofDragInfo dragInfo) { 

}

bool dimApp::button(int index, string name, int x_pos, int y_pos, int x_size, int y_size, bool active) {
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

// Low pass filter
float dimApp::lpf(int index, float new_value, int buffer_length) {
	float average = 0;
	for (int i = 0; i < buffer_length; ++i) {
		lpf_ringbuffer[index][i] = lpf_ringbuffer[index][i + 1];
		average += lpf_ringbuffer[index][i];
	}
	lpf_ringbuffer[index][buffer_length] = new_value;
	average += new_value;
	average /= (float)buffer_length + 1;
	return average;
}

void dimApp::fillRingBuffer(int index, float new_value, int buffer_length) {
	/*  for (int i = buffer_length;i>0;i--) {
	ringbuffer[index][i] = ringbuffer[index][i-1];
	}
	ringbuffer[index][0] = new_value;*/
	if (ringbuffer[index][0] == 0)
		ringbuffer[index][0] = 1;
	ringbuffer[index][int(ringbuffer[index][0])] = new_value;	//index0 = position of read/fill
	++ ringbuffer[index][0];
	if (ringbuffer[index][0] > buffer_length)
		ringbuffer[index][0] = 1;
}

float dimApp::readRingBuffer(int index, int position, int buffer_length) {
	return ringbuffer[index][(int(ringbuffer[index][0]) + (buffer_length-position) - 1) % buffer_length + 1];
}

bool dimApp::trigger(int index, string name, int x_pos, int y_pos, int x_size, int y_size, bool value, bool active){
	int c_temp1 = c_button_normal;
	int c_temp2 = c_button_over;
	int c_temp3 = c_button_pushed;
	if (value) {
		c_button_normal = c_trigger_true;
		c_button_over = c_trigger_true_over;
		c_button_pushed = c_trigger_false;
	} else {
		c_button_normal = c_trigger_false;
		c_button_over = c_trigger_false_over;
		c_button_pushed = c_trigger_true;
	}
	if (button(index,name,x_pos,y_pos,x_size,y_size,active) && active)
		value =! value;
	c_button_normal = c_temp1;
	c_button_over = c_temp2;
	c_button_pushed = c_temp3;
	return value;
}

//--------------------------------------------------------------Process screenshot image
void dimApp::processScreenImage() {
	ofPixelsRef pixels = screenImage.getPixelsRef();
	ofPixelsRef grayDiffPixels = grayDiff.getPixelsRef();
	cout << contourFinder.nBlobs;

	for (int i = 0; i < (float)contourFinder.nBlobs; ++i) {
		int x = contourFinder.blobs[i].centroid.x;
		int y = contourFinder.blobs[i].centroid.y;
		
		for (int j = - blob_weight[i]; j < blob_weight[i]; ++j) {
			for (int k = - blob_weight[i]; k < blob_weight[i]; ++k) {
				ofColor c = pixels.getColor(x + j, y + k);
				ofColor newC(255, 255, 255);	// ofColor(r, g, b, alpha)
				pixels.setColor(x + j, y + k, newC);
			}
		}
	}

	for (int i = 0; i < pixels.getWidth(); ++i) {
		for (int j = 0;  j < pixels.getHeight(); ++j) {
			ofColor c = grayDiffPixels.getColor(i, j);
			if (c.r == 0 && c.g == 0 && c.b == 0)
				continue;
			ofColor newC(0, 0, 0);
			pixels.setColor(i, j, newC);
		}
		cout << endl;
	}

	screenImage.update();
	return;
}


cv::Mat screenMatBGR(SCREEN_WIDTH, SCREEN_HEIGHT, 24);
cv::Mat screenMatRGB(SCREEN_WIDTH, SCREEN_HEIGHT, 24);

void dimApp::grabScreenshot() {
	capture->captureFrame(screenMatBGR);

	cv::cvtColor(screenMatBGR, screenMatRGB, CV_BGR2RGB);
	ofxCv::toOf(screenMatRGB, screenImage);

	/*
	ofPixelsRef pixels = screenImage.getPixelsRef();

	for (int i = 0; i < screenImage.getWidth(); ++i) {
		for (int j = 0;  j < screenImage.getHeight(); ++j) {
			ofColor c = pixels.getColor(i, j);
			ofColor newC(c.b, c.g, c.r, 255);	// ofColor(r, g, b, alpha)
			pixels.setColor(i, j, newC);
		}
	}
	screenImage.update();*/
	screenImage.resize(roi_right - roi_left, roi_bottom - roi_top);
}

#endif