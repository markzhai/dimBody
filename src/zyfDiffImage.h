#ifndef ZYF_DIFF_IMAGE
#define ZYF_DIFF_IMAGE

#include "ofMain.h"
#include "ofxOpenCv.h"

class zyfDiffImage {
private:
	unsigned threshold;	// 0 ~ 765(255*3)

	ofImage dimColorImage;
	ofColor dimColor;
	ofxCvColorImage diffImage;
	ofxCvGrayscaleImage diffGrayImage;
	ofxCvContourFinder contourFinder;

	vector<ofPoint> vecDiffPoints;
	bool checkDiffPoint(int x, int y);

	unsigned colorDiff(ofColor a, ofColor b);

	bool* diffPoints;
	ofxCvHaarFinder haarFinder;

public:
	bool printDebugInfo;
	int debug_x, debug_y;
	zyfDiffImage();

	void allocate(int w, int h);
	void setFromPixels(const ofPixels &pixels);
	ofPixelsRef getPixelsRef();
	void resize(int w, int h);
	void draw(float x, float y);
	void drawContour(float x, float y);

	void absDiff(ofImage& baseImage, ofImage& barrierImage);

	void setThreshold(int value);
	void setDimcolor(ofColor& color);

	void setDimColorImage(ofImage image) {dimColorImage = image;}

	void drawBlob(ofImage image);
};

#endif