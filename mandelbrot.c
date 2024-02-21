#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include "TinyPngOut.h"

#ifndef setheight
#define setheight 1080
#endif

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
#define depth 1 //note depth cannot be greater than 1
#endif

#ifndef edgeWidth
#define edgeWidth 0.10f
#endif

#ifndef quiet
#define quiet 1
#endif

#ifndef numThreads
#define numThreads 1
#endif

#ifndef PIXEL_TYPE
#define PIXEL_TYPE float
#endif

typedef struct compNum {
	long double r;
	long double i;
} compNum;


/* prototypes */
void *threadMain(void *args);
compNum calcC(int x, int y);
PIXEL_TYPE calcPoint(compNum point);
int makePicture();
void setPalette();
void getColor(PIXEL_TYPE iterations, uint8_t *color);
void hslTorgb(uint8_t *color, float h, float s, float l);

static int printError(enum TinyPngOut_Status status);


/* computed constants */
const PIXEL_TYPE logOf2 = log2l(2);
const int setwidth = setheight / 9 * 16;


long numPixels;
PIXEL_TYPE pixelArea;

long pagesize;
long pixelsPerChunk;
long chunksPerThread;

PIXEL_TYPE *pixels;
float palette[max];

int main(){
	numPixels = setheight * setwidth;
	pixels = calloc(numPixels, sizeof(PIXEL_TYPE));
	pagesize = sysconf(_SC_PAGESIZE);
	pixelsPerChunk = pagesize / sizeof(PIXEL_TYPE);
	chunksPerThread = numPixels / pixelsPerChunk;

	/* calculating the pixel area */
	compNum	lowerLeft = calcC(0, 0); // lower left coords
	compNum upperRight = calcC(setwidth-1, setheight-1); // upper right
	pixelArea = ((upperRight.r - lowerLeft.r) / setwidth) * ((upperRight.i - lowerLeft.i) / setheight);


	long numDone = 0;
	float percent = 0;
	int curThread = 0;
	int status;
	pthread_t tid[numThreads];
	pthread_mutex_t lock;

	/* thread init */
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("Mutex init failed\n");
		return 1;
	}
	for(int i = 0; i < numThreads; i++){
		int *id = malloc(sizeof(int*));
		*id = i;
		status = pthread_create(&tid[i], NULL, &threadMain, (void*)id);
		if(status != 0){
			printf("Thread %d creation failed with status: %d", i, status);
			return 1;
		}
	}
/*
	if (quiet){
		++numDone;
		float newPercent = ((float) numDone / (float) numPixels) * 100;
		newPercent = trunc(newPercent);
		if(percent < newPercent){
			percent = newPercent;
			printf("%.0f%% done. %ld/%ld pixels.\n", percent, numDone, numPixels);
		}
	}
*/
	/* wait for threads to finish */
	for(int i = 0; i < numThreads; i++){
		pthread_join(tid[i], NULL);
	}
	pthread_mutex_destroy(&lock);

	/* image creation here */
	setPalette();
	if(quiet) printf("\nDone calculating. Creating the image now.\n");
	status = makePicture();
	free(pixels);
	return status;
}

void* threadMain(void *args){
	int id = *((int*)(args));
	long curChunk = id, curPixel, x, y;
	for (long i = 0; i < chunksPerThread; i++) {
		curPixel = curChunk * pixelsPerChunk;
		for(long k = 0; k < pixelsPerChunk && curPixel < numPixels; k++, curPixel++){
			x = curPixel % setwidth;
			y = curPixel / setwidth;
			pixels[curPixel] = calcPoint(calcC(x, y));
		}
		curChunk += numThreads;
	}
	free(args);
	return 0;
}

/*
void* threadMain(void *args){
	int id = *((int*)(args));
	int curPixel = id, x, y;
	for (int i = 0; i < pixelsPerThread; i++) {
		x = curPixel % setwidth;
		y = curPixel / setwidth;
		pixels[curPixel] = calcPoint(calcC(x, y));
		curPixel += numThreads;
	}
	free(args);
	return 0;
}
*/


/* takes a point in the image
 * and returns the complex number associated with this
 */
compNum calcC(int x, int y){
	return (compNum) {((4.0L / (setwidth - 1)) * x - 2.5f) * depth + xShift, 
		         (((9.0L / 4.0L)/(setheight - 1)) * y - (9.0l / 8.0l)) * depth + yShift};
}

/* takes a complex point and calculates the set at it */
PIXEL_TYPE calcPoint(compNum point){
	long double x = 0, y = 0, xSqr = 0, ySqr = 0, xTemp, derCTemp;
	compNum derC;
	derC.r = 1;
	derC.i = 0;
	int iteration = 0; 

	while (xSqr + ySqr < 1000 && iteration < max) {
		derCTemp = 2 * (derC.r*x - derC.i*y) + 1;
		derC.i = 2 * (derC.r*y + derC.i*x);
		derC.r = derCTemp;
		xTemp = xSqr - ySqr + point.r;
		y = 2 * x * y + point.i;
		x = xTemp;
		xSqr = x * x;
		ySqr = y * y;
		++iteration;
	}

	if (iteration == max) {
		return iteration; // if it's in the set no need for normalization
	} else { // edge detection
		if ((xSqr + ySqr) * pow(log2(xSqr + ySqr), 2) < ((derC.r * derC.r) + (derC.i * derC.i)) * edgeWidth * pixelArea) {
			return -1.0f; // marker for the edge color this can eventually be reworked so the edge is its own iterative color
		} else { // normalization
			PIXEL_TYPE nu = log2(log2(sqrt(xSqr + ySqr))) / logOf2;
			return iteration - nu;
		}
	}
}

/* This takes list of pixels and writes it to a file
 * given the speed of writing the image (compared to the calculation)
 * there will be no ongoing progress update
 */
int makePicture(){
	char fileName[80];
	sprintf(fileName, "Mandelbrot%dx%dat%d.png", setwidth, setheight, max);
	FILE *fout = fopen(fileName, "wb");
	if (fout == NULL) {
		perror("Error: fopen");
		return EXIT_FAILURE;
	}
	/* whats nice about this is that it doesn't store the picture in memory */
	struct TinyPngOut pngout;
	enum TinyPngOut_Status status = TinyPngOut_init(&pngout, (uint32_t)setwidth, (uint32_t)setheight, fout);
	uint8_t color[3];
	if (status != TINYPNGOUT_OK)
		return printError(status);
	for (int i = 0; i < numPixels; ++i) {
			getColor(pixels[i], color);
			status = TinyPngOut_write(&pngout, color, (size_t) 1);
			if (status != TINYPNGOUT_OK)
				return printError(status);
	}
	if (fclose(fout) != 0) {
		perror("Error: fclose");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* histogram coloring */
void setPalette(){
	int total = 0;
	double h = 0;
	unsigned int *histogram = (int*) calloc(max, sizeof(int));
	for (int i = 0; i < numPixels; ++i) {
		if (pixels[i] != max && pixels[i] != -1) {
			histogram[(int) floor(pixels[i])]++;
			histogram[(int) ceil(pixels[i])]++;
		}
	}
	for (int i = 0; i < max; ++i)
		total += histogram[i];
	for (int i = 0; i < max; ++i) {
		h += (float) histogram[i] / total;
		palette[i] = h;
	}
	//free(histogram);
}

void getColor(PIXEL_TYPE iterations, uint8_t *color){ 
	/* set color */
	if (iterations == max) {
		color[0] = 0x00;
		color[1] = 0x00;
		color[2] = 0x00;
	/* edge color */
	} else if (iterations == -1) {
		color[0] = 0xFF;
		color[1] = 0xEC;
		color[2] = 0xB8;
	/* normalized color */
	} else {
		float hLow = 80.0f * palette[(int)floor(iterations)] + 200; // this is the range for the colors, [200, 280]
		float hHigh = 80.0f * palette[(int)ceil(iterations)] + 200;
  		float percent = fmod(iterations, 1); 
		
		float h = (hHigh - hLow) * percent + hLow;
		float s = 1.0f;
		float l = (iterations / (iterations + 4.0f)) / 1.75f;
		// float l = 0.5;

		hslTorgb(color, h, s, l);
	}
}

/* a pretty terrible hslTorgb function */
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
