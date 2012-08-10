#include <iostream>
#include "ImageUtils.h"
#include "FilterUtils.hpp"
#include "itkCastImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkApproximateSignedDistanceMapImageFilter.h"
#include "itkFastMarchingImageFilter.h"
#include <map>
#include "itkConnectedComponentImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkLabelShapeKeepNObjectsImageFilter.h"
#include <itkMinimumMaximumImageCalculator.h>
#include <itkHausdorffDistanceImageFilter.h>
#include <map>
#include "argstream.h"
#include <limits>
#include <itkLabelOverlapMeasuresImageFilter.h>


using namespace std;

const unsigned int D=2;
typedef unsigned short Label;
typedef itk::Image< Label, D >  LabelImage;
typedef itk::Image< float, D > TRealImage;
typedef  LabelImage::Pointer LabelImagePointerType;


LabelImagePointerType selectLabel(LabelImagePointerType img, Label l){
    LabelImagePointerType result=ImageUtils<LabelImage>::createEmpty(img);
    typedef itk::ImageRegionIterator<LabelImage> IteratorType;
    IteratorType it1(img,img->GetLargestPossibleRegion());
    IteratorType it2(result,img->GetLargestPossibleRegion());
    for (it1.GoToBegin(),it2.GoToBegin();!it1.IsAtEnd();++it1,++it2){
        it2.Set(it1.Get()==l);
    }
    return result;
}

LabelImagePointerType fixSegmentationImage(LabelImagePointerType segmentationImage, int nSegmentations){
    LabelImagePointerType newImage=ImageUtils<LabelImage>::createEmpty(segmentationImage);
    typedef   itk::ImageRegionConstIterator<LabelImage> ImageConstIterator;
    typedef   itk::ImageRegionIterator<LabelImage> ImageIterator;
    ImageConstIterator imageIt(segmentationImage,segmentationImage->GetLargestPossibleRegion());        
    ImageIterator imageIt2(newImage,newImage->GetLargestPossibleRegion());        
    
    nSegmentations=nSegmentations>0?nSegmentations:2;
    double divisor=FilterUtils<LabelImage>::getMax(segmentationImage)/(nSegmentations-1);
    for (imageIt.GoToBegin(),imageIt2.GoToBegin();!imageIt.IsAtEnd();++imageIt, ++imageIt2){
        imageIt2.Set(floor(1.0*imageIt.Get()/divisor));
    }
    
    return (LabelImagePointerType)newImage;
}

int main(int argc, char * argv [])
{


    argstream as(argc, argv);
	string groundTruth,segmentationFilename,outputFilename="";
    bool hausdorff=false;
    double threshold=-9999999;
    bool convertFromClassified=false;
    bool multilabel=false;
    bool connectedComponent=false;
    int evalLabel=-1;
    int nSegmentations=2;
	as >> parameter ("g", groundTruth, "groundtruth image (file name)", true);
	as >> parameter ("s", segmentationFilename, "segmentation image (file name)", true);
	as >> parameter ("o", outputFilename, "output image (file name)", false);
    as >> parameter ("t", threshold, "threshold segmentedImage (threshold)", false);
	as >> parameter ("c", convertFromClassified, "convert from classified segmentation (after normalization) (0,1)", false);
    as >> option ("h", hausdorff, "compute hausdorff distance(0,1)");
    as >> option ("m", multilabel, "convert from multilabel segmentation");
	as >> parameter ("e", evalLabel, "label to evaluate", false);
    as >> parameter ("n", nSegmentations, "number of segmentation labels in both images (must be consistent!)", false);

	as >> option ("l", connectedComponent, "use largest connected component in segmentation");
	as >> help();
	as.defaultErrorHandling();

 
    LabelImage::Pointer groundTruthImg =
        fixSegmentationImage(ImageUtils<LabelImage>::readImage(groundTruth),nSegmentations);
    LabelImage::Pointer segmentedImg =
        fixSegmentationImage(ImageUtils<LabelImage>::readImage(segmentationFilename),nSegmentations);

    
    unsigned totalPixels = 0;

    TRealImage::Pointer distancesOutsideTruthBone;
    TRealImage::Pointer distancesInsideTruthBone;
    TRealImage::Pointer distanceMap;
    float maxAbsDistance = 0;
    float maxDistance = -std::numeric_limits<float>::max();
    double minSum=0,maxSum=0;
    int minDistance = std::numeric_limits<int>::max();
    double minCount=0, maxCount=0;
    int sum = 0;
    unsigned totalEdges = 0;
    float mean=0;
    if (evalLabel>-1){
        groundTruthImg=selectLabel(groundTruthImg,evalLabel);   
        segmentedImg=selectLabel(segmentedImg,evalLabel);
    }
    ImageUtils<LabelImage>::writeImage("test.png",ImageUtils<LabelImage>::multiplyImageOutOfPlace(segmentedImg,65535));
    if (multilabel){
        //segmentedImg=convertToBinaryImageFromMultiLabel(segmentedImg);
        std::cout<<"NYI"<<std::endl;
        exit(0);
    }
    else{
        if (threshold!=-9999999){
            segmentedImg= FilterUtils<LabelImage>::binaryThresholdingLow(segmentedImg, threshold);         
        }
    }    
    typedef LabelImage::ConstPointer ConstType;
    if (connectedComponent){  
        typedef itk::MinimumMaximumImageCalculator <LabelImage>
            ImageCalculatorFilterType;
        typedef itk::ConnectedComponentImageFilter<LabelImage,LabelImage>  ConnectedComponentImageFilterType;
        ConnectedComponentImageFilterType::Pointer filter =
            ConnectedComponentImageFilterType::New();
        filter->SetInput(segmentedImg);
        filter->Update();
    
        typedef itk::LabelShapeKeepNObjectsImageFilter< LabelImage > LabelShapeKeepNObjectsImageFilterType;
        LabelShapeKeepNObjectsImageFilterType::Pointer labelShapeKeepNObjectsImageFilter = LabelShapeKeepNObjectsImageFilterType::New();
        labelShapeKeepNObjectsImageFilter->SetInput( filter->GetOutput() );
        labelShapeKeepNObjectsImageFilter->SetBackgroundValue( 0 );
        labelShapeKeepNObjectsImageFilter->SetNumberOfObjects( 1);
        labelShapeKeepNObjectsImageFilter->SetAttribute( LabelShapeKeepNObjectsImageFilterType::LabelObjectType::NUMBER_OF_PIXELS);
        labelShapeKeepNObjectsImageFilter->Update();
        segmentedImg =  FilterUtils<LabelImage>::binaryThresholdingLow(labelShapeKeepNObjectsImageFilter->GetOutput(), 0.1);     //;//f->GetOutput();// FilterUtils<LabelImage>::binaryThresholding(labelShapeKeepNObjectsImageFilter->GetOutput(),1,10000);//filter->GetOutput(),1,1);
    }
   
   
    if (hausdorff){
        typedef itk::HausdorffDistanceImageFilter<LabelImage, LabelImage> HausdorffDistanceFilterType;
        typedef HausdorffDistanceFilterType::Pointer HDPointerType;
        HDPointerType hdFilter=HausdorffDistanceFilterType::New();;
        hdFilter->SetInput1(groundTruthImg);
        hdFilter->SetInput2(segmentedImg);
        hdFilter->SetUseImageSpacing(true);
        hdFilter->Update();
        mean=hdFilter->GetAverageHausdorffDistance();
        maxAbsDistance=hdFilter->GetHausdorffDistance();
    }

    typedef itk::LabelOverlapMeasuresImageFilter<LabelImage> OverlapMeasureFilterType;
    OverlapMeasureFilterType::Pointer filter = OverlapMeasureFilterType::New();
    filter->SetSourceImage(groundTruthImg);
    filter->SetTargetImage(segmentedImg);
    filter->Update();
    double dice=filter->GetDiceCoefficient();
    std::cout<<"Dice " << dice ;
    std::cout<<"  Mean "<< mean;
    std::cout<<" MaxAbs "<< maxAbsDistance;
    std::cout<< std::endl;
    // std::cout<<"EvalG - % of bone segmented "<< float(glob.truePos) / ((glob.truePos + glob.falseNeg) / 100)<< std::endl;

    

  


	return EXIT_SUCCESS;
}
