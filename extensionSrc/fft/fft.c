#include "fft.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846f

typedef struct{
    float real;
    float imaginary;
}complex;

static complex roots[SAMPLE_NUM/2];
static complex buffer[SAMPLE_NUM];
static bool rootsInitialised = false;
static complex coeffs[SAMPLE_NUM];


static float magnitude(complex value){
    return sqrtf(value.real * value.real + value.imaginary * value.imaginary);
}

static complex rootOfUnity(int k,int n){
    return (complex){cosf((2*k*PI)/n),-sinf((2*k*PI)/n)};
}

static complex multiplyComplex(complex c1,complex c2){
    return (complex){c1.real * c2.real - c1.imaginary * c2.imaginary,
                   c1.imaginary * c2.real + c2.imaginary * c1.real};
}

static complex addComplex(complex c1, complex c2){
    return (complex){c1.real + c2.real,c1.imaginary + c2.imaginary};
}

static complex negateComplex(complex c1){
    return (complex){-c1.real,-c1.imaginary};
}

static void initialiseRoots(void){
    if(rootsInitialised){
        return;
    }
    for(int k =0; k<SAMPLE_NUM/2;k++){
        roots[k] = rootOfUnity(k,SAMPLE_NUM);
    }
    rootsInitialised = true;
}

static bool isPowerOfTwo(int n){
    return n>0 && ((n&(n-1)) == 0);
}


//This function initially will take in the samples with an imaginary part of 0
// It will then split it into two polynomials- which are the odds and the evens
// And then recursively evaluate those using the same method
// It will then take those two inputs and combine them and pass them up

// The stride parameter allows the list to be easily 'split' into odds and evens 
// And the start parameter tells the algorithm which point in the list to start at 
// This allows fft to be done only using one buffer rather than a new even and odd array each time
static void fftRecursive(complex *coeffs,complex *buffer,int numCoeffs,int start,int stride){
    //In the base case where there is one element, we return.
    // This is because the polynomial has 1 coefficient, which is constant
    // So the elements in the array will be the correct evaluation
    if(numCoeffs == 1){
        return;
    }
    // If its not, we split it into the even and odd coefficients
    //Evens
    fftRecursive(coeffs,buffer,numCoeffs/2,start,stride * 2);
    //Odds
    fftRecursive(coeffs,buffer,numCoeffs/2,start +stride, stride * 2);

    complex currentEven;
    complex currentOdd;
    complex root;
    complex rootProduct;

    //This loop loads the new values into a buffer
    //This is needed because otherwise the calls would overwrite values
    // that need to be used in the future
    for (int n = 0;n<numCoeffs/2;n++){
        //The two lines below get the odd even pair used in FFT
        currentEven = coeffs[start + 2*n*stride];
        currentOdd = coeffs[start + (2*n+1) * stride];
        //This gets the associated root of unity and calculates its product
        root = roots[n*SAMPLE_NUM/numCoeffs];
        rootProduct = multiplyComplex(root,currentOdd);
        //This stores the values from this level of recursion in the buffer
        buffer[n] = addComplex(currentEven, rootProduct);
        buffer[n + numCoeffs/2] = addComplex(currentEven, negateComplex(rootProduct));
    }

    //This puts the values from the buffer into the actual array
    for(int n = 0; n<numCoeffs;n++){
        coeffs[start + stride*n] = buffer[n];
    }
    return;
}

//This function takes in the array of floats from the sample
// and will alter the magnitudes array to give the magnitude
// of each bin, which will be used to represent a range of frequencies
void fft(float *samples,int numCoeffs, float *bins){

    if (samples == NULL || bins == NULL) {
        fprintf(stderr, "NULL pointer passed to fft\n");
        exit(EXIT_FAILURE);
    }
    //This generalises it to allow any power of 2 less than 1024
    if(numCoeffs > SAMPLE_NUM || !isPowerOfTwo(numCoeffs)){
        fprintf(stderr,"Incorrect sample size\n");
        exit(1);
    }

    initialiseRoots();
    //Converts the samples to complex numbers with 0 imaginary part
    for (int i = 0; i < numCoeffs; i++) {
        coeffs[i].real = samples[i];
        coeffs[i].imaginary = 0.0f;
    }

    fftRecursive(coeffs,buffer, numCoeffs, 0, 1);

    for (int i = 0; i < numCoeffs / 2; i++) {
        bins[i] = magnitude(coeffs[i]);
    }
}

