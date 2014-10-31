#include "Log.h"

#include <stdio.h>
#include <iostream>
#include "argstream.h"
#include "ImageUtils.h"
#include "FilterUtils.hpp"
#include <fstream>

using namespace std;
using namespace itk;



int main(int argc, char ** argv)
{

	feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
    typedef  float PixelType;
    const unsigned int D=3;
    typedef Image<PixelType,D> ImageType;
    typedef  ImageType::IndexType IndexType;
    typedef  ImageType::PointType PointType;
    typedef  ImageType::DirectionType DirectionType;

    typedef ImageType::Pointer ImagePointerType;
    typedef ImageType::ConstPointer ImageConstPointerType;
 
    argstream * as=new argstream(argc,argv);
    string inFile, outFile;
    double lThresh=std::numeric_limits<PixelType>::min();
    if (!std::numeric_limits<PixelType>::is_integer){
        lThresh=-std::numeric_limits<PixelType>::max();
    }
    double uThresh=std::numeric_limits<PixelType>::max();
    bool negLow=false;
    (*as) >> parameter ("in", inFile, " filename...", true);
    (*as) >> parameter ("out", outFile, " filename...", true);
    (*as) >> parameter ("lt", lThresh, "lower threshold", false);
    (*as) >> option ("negL", negLow, "negatibe lower threshold");
    (*as) >> parameter ("ut", uThresh, "upper threshold", false);
    (*as) >> help();
    as->defaultErrorHandling();
    if (negLow) lThresh=-lThresh;
    ImagePointerType img = ImageUtils<ImageType>::readImage(inFile);

    ImagePointerType outImage=FilterUtils<ImageType>::thresholding(img,lThresh,uThresh);
    //FilterUtils<ImageType>::lowerThresholding(img,thresh);
    
    ImageUtils<ImageType>::writeImage(outFile,outImage);

	return 1;
}