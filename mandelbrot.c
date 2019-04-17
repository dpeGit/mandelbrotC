#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include "TinyPngOut.h"

#ifndef setheight
#define setheight 1080
#endif
#define setwidth setheight / 9 * 16

#ifndef max
#define max 1000
#endif

#ifndef xShift
#define xShift 0
#endif

#ifndef yShift
#define yShift 0
#endif

#ifndef depth
#define depth 1
#endif

typedef struct compNum {
	long double r;
	long double i;
} compNum;

compNum calcC(int x, int y);
float calcPoint(compNum point);
int makePicture();
void setPalette();
uint8_t* getColor(float iterations);
void hslTorgb(uint8_t *color, float h, float s, float l);

static int printError(enum TinyPngOut_Status status);


const float logOf2 = log2l(2); //constant so we only compute this once

/* we declare this here because in theory allocating this at compile time
 * is slightly faster. In practice this really wont be true due to caching
 * tests will be done later to see if there is an appreciable difference
 */
/* additionally I am using floats here since the accuracy isn't super important for right now
 * in the future this might change when doing zoom's however that will drastically increase memory usage
 */
float pixels[setwidth][setheight];

float palette[max];


int main(){
	long numPixels = (long)setheight * (long)setwidth;
	long numDone = 0;
	float percent = 0;
	//array to map the co-ord plane to;
	for(int x = 0; x < setwidth; ++x){
		for(int y = 0; y < setheight; ++y){
			pixels[x][y] = calcPoint(calcC(x, y));
/*			++numDone;
			float newPercent = ((float) numDone / (float) numPixels) * 100;
			newPercent = trunc(newPercent);
			if(percent < newPercent){
				percent = newPercent;
				printf("%.0f%% done. %ld/%ld pixels.\n", percent, numDone, numPixels);
			}
*/		}

	}
	setPalette();
	//printf("\nDone calculating. Creating the image now.\n");
	return makePicture();
	return 0;
}

/* This takes a point in the image
 * and returns the complex number associated with this
 */
compNum calcC(int x, int y){
	return (compNum) {((4.0L/(setwidth - 1)) * x - 2.5f) * depth + xShift, 
		         (((9.0L /4.0L)/(setheight - 1)) * y - (9.0l / 8.0l)) * depth - yShift};
}

/* This takes a complex point and calculates the fractal at it
 */
float calcPoint(compNum point){
	long double x = 0, y = 0, xSqr = 0, ySqr = 0, xTemp;
	int iteration = 0; 

	while( xSqr + ySqr < 1000 && iteration < max){
		xTemp = xSqr - ySqr + point.r;
		y = 2 * x * y + point.i;
		x = xTemp;
		xSqr = x * x;
		ySqr = y * y;
		++iteration;
	}

	if(iteration == max){
		return iteration; //if it's in the set no need for normalization
	} else{
		float nu = log2(log2(sqrt(xSqr + ySqr))) / logOf2; //normalization
		return iteration - nu;
	}
}
/* This takes list of pixels and writes it to a file
 * given the speed of writing the image (compared to the calculation)
 * there will be no ongoing progress update
 */
int makePicture(){
	char *fileName = (char*) malloc(80 * sizeof(char));
	sprintf(fileName, "Mandelbrot%dx%dat%d.png", setwidth, setheight, max);
	FILE *fout = fopen(fileName, "wb");
	if(fout == NULL){
		perror("Error: fopen");
		return EXIT_FAILURE;
	}
	/* whats nice about this is that it doesn't store the picture in memory */
	struct TinyPngOut pngout;
	enum TinyPngOut_Status status = TinyPngOut_init(&pngout, (uint32_t)setwidth, (uint32_t)setheight, fout);
	if(status != TINYPNGOUT_OK)
		return printError(status);
	for(int y = 0; y < setheight; ++y){
		for(int x = 0; x < setwidth; ++x){
			status = TinyPngOut_write(&pngout, getColor(pixels[x][y]), (size_t) 1);
			if (status != TINYPNGOUT_OK)
				return printError(status);
		}
	}
	if (fclose(fout) != 0) {
		perror("Error: fclose");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* histogram coloring
 */
void setPalette(){
	int total = 0;
	double h = 0;
	unsigned int* histogram = malloc(max * sizeof(int));
	for(int x = 0; x < setwidth; ++x){
		for(int y = 0; y < setheight; ++y){
			if (pixels[x][y] != max){
				histogram[(int) floor(pixels[x][y])]++;
			}
		}
	}
	for(int i = 0; i < max; ++i)
		total += histogram[i];
	for(int i = 0; i < max; ++i){
		h += (float) histogram[i] / total;
		palette[i] = h;
	}
	free(histogram);
}
uint8_t* getColor(float iterations){
	static uint8_t color[3];
	if (iterations == max){
		color[0] = 0x00;
		color[1] = 0x00;
		color[2] = 0x00;
	} else{
		float hLow = 80.0f * palette[(int)floor(iterations)] + 200; //this is the range for the colors, [200, 280]
		float hHigh = 80.0f * palette[(int)ceil(iterations)] + 200;
		float percent = fmod(iterations, 1);
		
		float h = (hHigh - hLow) * percent + hLow;
		float s = 1.0f;
		float l = (iterations / (iterations + 4.0f)) / 1.75f;
		//float l = 0.5;

		hslTorgb(color, h, s, l);
	}
	return color;
}

/* a pretty terrible hslTorgb function
 */
void hslTorgb(uint8_t *color, float h, float s, float l){
	float c = (1 - fabs(2 * l - 1)) * s;
	float x = c * (1 - fabs(fmod((h / 60.0f), 2.0f) - 1));
	float m = l - (c / 2.0f);

	if (h < 60){
		color[0] = (uint8_t) ((c + m) * 255);
		color[1] = (uint8_t) ((x + m) * 255);
		color[2] = (uint8_t) (m * 255);
	} else if (h < 120){
		color[0] = (uint8_t) ((x + m) * 255);
		color[1] = (uint8_t) ((c + m) * 255);
		color[2] = (uint8_t) (m * 255);
	} else if (h < 180){
		color[0] = (uint8_t) (m * 255);
		color[1] = (uint8_t) ((c + m) * 255);
		color[2] = (uint8_t) ((x + m) * 255);
	} else if (h < 240){
		color[0] = (uint8_t) (m * 255);
		color[1] = (uint8_t) ((x + m) * 255);
		color[2] = (uint8_t) ((c + m) * 255);
	} else if (h < 300){
		color[0] = (uint8_t) ((x + m) * 255);
		color[1] = (uint8_t) (m * 255);
		color[2] = (uint8_t) ((c + m) * 255);
	} else if (h < 360){
		color[0] = (uint8_t) ((c + m) * 255);
		color[1] = (uint8_t) (m * 255);
		color[2] = (uint8_t) ((x + m) * 255);
	}
}

/* TPG error statements */
static int printError(enum TinyPngOut_Status status){
	const char *msg;
	switch (status){
		case TINYPNGOUT_OK		:	msg = "OK";			break;
		case TINYPNGOUT_INVALID_ARGUMENT:	msg = "Invalid argument";	break;
		case TINYPNGOUT_IMAGE_TOO_LARGE	:	msg = "Image too large";	break;
		case TINYPNGOUT_IO_ERROR	:	msg = "I/O error";		break;
		default				:	msg = "Unknown error";		break;
	}
	fprintf(stderr, "Error: %s\n", msg);
	return EXIT_FAILURE;
}
