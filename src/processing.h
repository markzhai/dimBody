#ifndef _PROCESSING_H
#define _PROCESSING_H

namespace processing {
	inline float map(float value, float range_min1, float range_max1, float range_min2, float range_max2) {
		if (range_max1 == range_min1){  //avoiding division by zero
			value = range_max2;
		} else {
			value -= range_min1;
			value /= (range_max1 - range_min1);
			value *= (range_max2 - range_min2);
			value += range_min2;
		}
		return value;
	}

	inline float constrain (float value, float min, float max) {
		if (value < min)
			value = min;
		if (value > max)
			value = max;
		return value;
	}
}

#endif