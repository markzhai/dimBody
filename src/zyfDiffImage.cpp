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

	diffPoints = (bool*)calloc(w * h, sizeof(int));	//realloc()
	memset(diffPoints, 0x0, sizeof(bool) * w * h);
	//cout << diffPoints[w*h - 1];
	//free((void**)diffPoints);
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
	contourFinder.findContours(diffGrayImage, 7000, 400000, 20, true);
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
	
	unsigned absDiff;
	int mapX, mapY, tillX, tillY;
	dimColorImage.saveImage("dim.jpg");
	ofColor blackColor(0, 0, 0);
	if (printDebugInfo) {
		int mapX = processing::map(debug_x, 0, ofGetWidth(), 0, barrierImage.getWidth());
		int	mapY = processing::map(debug_y, 0, ofGetHeight(), 0, barrierImage.getHeight());
		cout << "Dim Color    : " << dimColorImage.getColor(mapX, mapY) << endl;
		cout << "Barrier Color: " << barrierImage.getColor(mapX, mapY) << endl;
		printDebugInfo = false;
	}

	for (int i = 0; i < barrierImage.getWidth(); ++i) {
		for (int j = 0; j < barrierImage.getHeight(); ++j) {
			mapX = processing::map(i, 0, barrierImage.getWidth(), 0, baseImage.getWidth());
			mapY = processing::map(j, 0, barrierImage.getHeight(), 0, baseImage.getHeight());

			tillX = processing::map(i + 1, 0, barrierImage.getWidth(), 0, baseImage.getWidth());
			tillY = processing::map(j + 1, 0, barrierImage.getHeight(), 0, baseImage.getHeight());
			
			if (colorDiff(barrierPixels.getColor(i, j), dimColorImage.getColor(i, j)) < 50) {
				//cout << "detect " << barrierPixels.getColor(i, j) << " - " << dimColorImage.getColor(i, j) << endl;
				continue;	// Ignore dimming color pixels
			}

			for (int m = mapX; m < tillX; ++m) {
				for (int n = mapY; n < tillY; ++n) {
					// it supposed to be black according to base image
					if (colorDiff(ofColor::black, basePixels.getColor(m, n)) < 50)
						continue;
					absDiff = colorDiff(barrierPixels.getColor(i, j), basePixels.getColor(m, n));
					if (absDiff > threshold) {
						pixels.setColor(m, n, blackColor);
					}
				}
			}
		}
	}
	diffImage.flagImageChanged();
	//vecDiffPoints.clear();
	
	//cout << (ofGetElapsedTimeMillis() - start_t) / 1000.0;
}

bool zyfDiffImage::checkDiffPoint(int x, int y) {
	for (vector<ofPoint>::const_iterator iter = vecDiffPoints.begin(); iter != vecDiffPoints.end(); ++ iter) {
		if (iter->y > y)
			return false;
		if (iter->y == y && iter->x > x)
			return false;
		if (iter->y == y && iter->x == x)
			return true;
	}
	return false;
}

void zyfDiffImage::setThreshold(int value) {
	if (value > 755)
		threshold = 755;
	else if (value < 0)
		threshold = 0;
	else
		threshold = value;
}

void zyfDiffImage::setDimcolor(ofColor& color) {
	float blue = color.b;
	float red = color.r;
	float green = color.g;
	cout << "Dim color - R: " << red << "  G: " << green << "  B: " << blue << endl;

	unsigned diff = colorDiff(color, dimColor);
	dimColor.set(color);
}

unsigned zyfDiffImage::colorDiff(ofColor a, ofColor b) {
	unsigned result = 0;
	result += abs(a.r - b.r) + abs(a.g - b.g) + abs(a.b - b.b);
	return result;
}

ofxCvColorImage cvImage;
void zyfDiffImage::drawBlob(ofImage image) {
	//cout << image.width << " " << image.height << endl;
	if (! cvImage.bAllocated) {
		cvImage.allocate(image.width, image.height);
	}
	cvImage.setFromPixels(image.getPixelsRef());
	
	diffGrayImage = diffImage;
	contourFinder.findContours(diffGrayImage, 7000, 400000, 20, true);
	for (int i = 0; i < contourFinder.blobs.size(); i++) {
		cvImage.drawBlobIntoMe(contourFinder.blobs[i], 0);
	}
	cvImage.draw(0, 0);
}