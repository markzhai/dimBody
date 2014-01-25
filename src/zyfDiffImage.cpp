#include "zyfDiffImage.h"
#include "processing.h"

zyfDiffImage::zyfDiffImage() {
	dimColor.r = 255;
	dimColor.g = 255;
	dimColor.b = 255;
}

void zyfDiffImage::allocate(int w, int h) {
	diffImage.allocate(w, h);
	diffGrayImage.allocate(w, h);
}

void zyfDiffImage::setFromPixels(const ofPixels &pixels) {
	diffImage.setFromPixels(pixels);
	diffImage.flagImageChanged();
}

ofPixelsRef zyfDiffImage::getPixelsRef() {
	return diffImage.getPixelsRef();
}

// Image Transformation Operations
//--------------------------------------------------------------------------------
void zyfDiffImage::resize(int w, int h) {
	diffImage.resize(w, h);
	diffGrayImage.resize(w, h);
}

void zyfDiffImage::draw(float x, float y) {
	diffImage.draw(x, y);
}

void zyfDiffImage::drawContour(float x, float y) {
	diffGrayImage = diffImage;
	if (isErode)
		diffGrayImage.erode();
	if (isDilate)
		diffGrayImage.dilate();
	contourFinder.findContours(diffGrayImage, 10000, 600000, 5, true);
	ofPushStyle();
	ofSetHexColor(0x00FFFF);
	ofSetColor(ofColor::black);
	for (int i = 0; i < contourFinder.blobs.size(); i++) {
		ofFill();
		ofBeginShape();
		for (int j = 0; j < contourFinder.blobs[i].nPts; j++) {
			ofVertex(contourFinder.blobs[i].pts[j].x, contourFinder.blobs[i].pts[j].y);
		}
		ofEndShape();
	}
	ofPopStyle();
}

// take 0.06-0.08seconds
void zyfDiffImage::absDiff(ofImage& baseImage, ofImage& barrierImage) {
	unsigned long long start_t = ofGetElapsedTimeMillis();
	//barrierImage.resize(barrierImage.getWidth() / 3, barrierImage.getHeight() / 3);
	//barrierImage.update();
	ofPixelsRef pixels = diffImage.getPixelsRef();
	ofPixelsRef barrierPixels = barrierImage.getPixelsRef();
	ofPixelsRef basePixels = baseImage.getPixelsRef();
	
	unsigned absDiff, absDiffHSB;
	int mapX, mapY, tillX, tillY;
	//dimColorImage.saveImage("dim.jpg");
	ofColor blackColor(0, 0, 0);
	
	if (printDebugInfo) {
		mapX = processing::map(debug_x, 0, ofGetWidth(), 0, barrierImage.getWidth());
		mapY = processing::map(debug_y, 0, ofGetHeight(), 0, barrierImage.getHeight());
		printf("X: %d (%d), Y: %d (%d)\n", debug_x, mapX, debug_y, mapY);
		cout << "Dim    : " << dimColorImage.getColor(mapX, mapY) << endl;
		cout << "Barrier: " << barrierImage.getColor(mapX, mapY) << endl;
		cout << "Base   : " << baseImage.getColor(debug_x, debug_y) << endl;

		printf("Barrier(HSB): %.2f\t%.2f\t%.2f\n", barrierImage.getColor(mapX, mapY).getHue(), barrierImage.getColor(mapX, mapY).getSaturation(), barrierImage.getColor(mapX, mapY).getBrightness());
		printf("Base   (HSB): %.2f\t%.2f\t%.2f\n", baseImage.getColor(debug_x, debug_y).getHue(), baseImage.getColor(debug_x, debug_y).getSaturation(), baseImage.getColor(debug_x, debug_y).getBrightness());
		printDebugInfo = false;
	}

	for (int i = 0; i < barrierImage.getWidth(); ++i) {
		for (int j = 0; j < barrierImage.getHeight(); ++j) {
			mapX = processing::map(i, 0, barrierImage.getWidth(), 0, baseImage.getWidth());
			mapY = processing::map(j, 0, barrierImage.getHeight(), 0, baseImage.getHeight());

			tillX = processing::map(i + 1, 0, barrierImage.getWidth(), 0, baseImage.getWidth());
			tillY = processing::map(j + 1, 0, barrierImage.getHeight(), 0, baseImage.getHeight());
			
			if (colorDiffRGB(barrierPixels.getColor(i, j), dimColorImage.getColor(i, j)) < 60) {
				//cout << "detect " << barrierPixels.getColor(i, j) << " - " << dimColorImage.getColor(i, j) << endl;
				continue;	// Ignore dimming color pixels
			}
			
			for (int m = mapX; m < tillX; ++m) {
				for (int n = mapY; n < tillY; ++n) {
					// it supposed to be black according to base image
					//if (colorDiff(ofColor::black, basePixels.getColor(m, n)) < 20) continue;
					absDiff = colorDiffRGB(barrierPixels.getColor(i, j), basePixels.getColor(m, n));
					absDiffHSB = colorDiffHSB(barrierPixels.getColor(i, j), basePixels.getColor(m, n));
					if (absDiff > thresholdRGB || absDiffHSB > thresholdHSB) {
						pixels.setColor(m, n, blackColor);
					}
				}
			}
		}
	}
	diffImage.flagImageChanged();
	//cout << (ofGetElapsedTimeMillis() - start_t) / 1000.0;
}

void zyfDiffImage::setThreshold(int rgb, int hsb) {
	if (rgb > 755)
		thresholdRGB = 755;
	else if (rgb < 0)
		thresholdRGB = 0;
	else
		thresholdRGB = rgb;
	if (hsb > 360)
		thresholdHSB = 360;
	else if (hsb < 0)
		thresholdHSB = 0;
	else
		thresholdHSB = hsb;
}

void zyfDiffImage::setDimcolor(ofColor& color) {
	float blue = color.b;
	float red = color.r;
	float green = color.g;
	cout << "Dim color - R: " << red << "  G: " << green << "  B: " << blue << endl;
	dimColor.set(color);
}

unsigned zyfDiffImage::colorDiffRGB(ofColor a, ofColor b) {
	unsigned result = abs(a.r - b.r) + abs(a.g - b.g) + abs(a.b - b.b);
	return result;
}

unsigned zyfDiffImage::colorDiffHSB(ofColor a, ofColor b) {
	unsigned result = 0;
	float hue1, sat1, brightness1;
	a.getHsb(hue1, sat1, brightness1);
	float hue2, sat2, brightness2;
	b.getHsb(hue2, sat2, brightness2);

	if (sat1 >= 50 && sat2 >= 50)
		result = abs(hue1 - hue2);

	return result;
}

ofxCvColorImage cvImage;
void zyfDiffImage::drawBlob(ofImage image) {
	//cout << image.width << " " << image.height << endl;
	if (!cvImage.bAllocated) {
		cvImage.allocate(image.width, image.height);
	}
	cvImage.setFromPixels(image.getPixelsRef());
	
	diffGrayImage = diffImage;
	contourFinder.findContours(diffGrayImage, 7000, 400000, 5, true);
	for (int i = 0; i < contourFinder.blobs.size(); i++) {
		cvImage.drawBlobIntoMe(contourFinder.blobs[i], 0);
	}
	cvImage.draw(0, 0);
}