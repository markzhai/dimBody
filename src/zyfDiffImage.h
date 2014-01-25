#ifndef ZYF_DIFF_IMAGE
#define ZYF_DIFF_IMAGE

#include "ofMain.h"
#include "ofxOpenCv.h"

class zyfDiffImage {
private:
	unsigned thresholdRGB, thresholdHSB;

	ofImage dimColorImage;
	ofColor dimColor;
	ofxCvColorImage diffImage;
	ofxCvGrayscaleImage diffGrayImage;
	ofxCvContourFinder contourFinder;
	ofxCvHaarFinder haarFinder;

	unsigned colorDiffRGB(ofColor a, ofColor b);
	unsigned colorDiffHSB(ofColor a, ofColor b);

public:
	bool printDebugInfo;
	bool isErode, isDilate;
	int debug_x, debug_y;
	zyfDiffImage();

	void allocate(int w, int h);
	void setFromPixels(const ofPixels &pixels);
	ofPixelsRef getPixelsRef();
	void resize(int w, int h);
	void draw(float x, float y);
	void drawContour(float x, float y);

	void absDiff(ofImage& baseImage, ofImage& barrierImage);

	void setThreshold(int rgb, int hsb);
	void setDimcolor(ofColor& color);

	void setDimColorImage(ofImage image) {dimColorImage = image;}

	void drawBlob(ofImage image);
};

#endif