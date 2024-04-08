#include <stdio.h>
#include <stdlib.h>

#include "lodepng.h"

//each pixel has 4 values:
//Red (R)
//Green (G)
//Blue (B)
//Transparency (T)

//negative filter (reversing the RGB values)
//0-255   max-val    255-40

__global__ void NegativeFilter(unsigned char * inputImage, unsigned char * outputImage){

//RGBTRGBTRGBTRGBT 1x4   2x2

int r;
int g;
int b;
int t;

int threadIndex = blockDim.x * blockIdx.x + threadIdx.x;

int pixel = threadIndex * 4;

printf("uid = %d\n", pixel);

r = inputImage[pixel];
g = inputImage[pixel+1];
b = inputImage[pixel+2];
t = inputImage[pixel+3];

outputImage[pixel] = 255-r;
outputImage[pixel+1] = 255-g;
outputImage[pixel+2] = 255-b;
outputImage[pixel+3] = t;

}

int main(int argc, char ** argv){

  unsigned int errorDecode; //variable will hold whether there was an issue with loading in the png file
  unsigned char* cpuImage; //this variable will hold all of our image data
  unsigned int width, height; //holds the width and height of image
  
  char * filename = argv[1];
  char * newFilename = argv[2];

  errorDecode = lodepng_decode32_file(&cpuImage, &width, &height, filename); // (where to store the image data, width, height, which file?)
  if(errorDecode){
    printf("error %u: %s\n", errorDecode, lodepng_error_text(errorDecode));
  }
  
  printf("height = %d, width = %d", height, width);
  
  int arraySize = width*height*4;
  int memorySize = arraySize * sizeof(unsigned char);
  
  unsigned char cpuOutImage[arraySize];
  
  unsigned char* gpuInput;
  unsigned char* gpuOutput;
  
  cudaMalloc( (void**) &gpuInput, memorySize);
  cudaMalloc( (void**) &gpuOutput, memorySize);
  
  cudaMemcpy(gpuInput, cpuImage, memorySize, cudaMemcpyHostToDevice);
  
  NegativeFilter<<< height, width >>>(gpuInput, gpuOutput);
  cudaThreadSynchronize();

  cudaMemcpy(cpuOutImage, gpuOutput, memorySize, cudaMemcpyDeviceToHost);
  
  unsigned int errorEncode = lodepng_encode32_file(newFilename, cpuOutImage, width, height);
  if(errorEncode) {
  printf("error %u: %s\n", errorEncode, lodepng_error_text(errorEncode));
  }

  //free(image);

}
