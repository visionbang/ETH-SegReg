/*
 * Potentials.h
 *
 *  Created on: Nov 24, 2010
 *      Author: gasst
 */

#ifndef _SEGMENTATIONPOTENTIALS_H_
#define _SEGMENTATIONPOTENTIALS_H_
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <utility>
#include <itkStatisticsImageFilter.h>
#include "Potential-SegmentationRegistration-Pairwise.h"

namespace itk{



    template<class TImage>
    class UnaryPotentialSegmentation: public itk::Object{
    public:
        //itk declarations
        typedef UnaryPotentialSegmentation            Self;
        typedef itk::Object Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;

        typedef	TImage ImageType;
        typedef typename ImageType::Pointer ImagePointerType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;

        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::SizeType SizeType;
        typedef typename ImageType::SpacingType SpacingType;
        SizeType m_fixedSize;

        typedef typename itk::StatisticsImageFilter< ImageType > StatisticsFilterType;
    protected:
        ConstImagePointerType m_fixedImage, m_sheetnessImage,m_referenceImage, m_referenceGradientImage;
        ConstImagePointerType m_referenceSegmentation;
        SpacingType m_displacementFactor;
        //LabelImagePointerType m_baseLabelMap;
        bool m_haveLabelMap;
        double m_gradientSigma, m_Sigma;
        double m_gradientScaling;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentation, Object);

        UnaryPotentialSegmentation(){
            this->m_haveLabelMap=false;
        }
        
        virtual void Init(){}
        virtual void freeMemory(){
        }
        void SetGradientScaling(double s){m_gradientScaling=s;}
        void SetFixedImage(ConstImagePointerType fixedImage){
            this->m_fixedImage=fixedImage;
            this->m_fixedSize=this->m_fixedImage->GetLargestPossibleRegion().GetSize();
        }
        void SetFixedGradientImage(ConstImagePointerType sheetnessImage){
            this->m_sheetnessImage=sheetnessImage;
            
            typename StatisticsFilterType::Pointer filter=StatisticsFilterType::New();
            filter->SetInput(this->m_sheetnessImage);
            filter->Update();
            this->m_gradientSigma=filter->GetSigma();
            this->m_gradientSigma*=this->m_gradientSigma;
            std::cout<<"Gradient variance: "<<m_gradientSigma<<std::endl;
            filter->SetInput(this->m_fixedImage);
            filter->Update();
            this->m_Sigma=filter->GetSigma();
            this->m_Sigma*=this->m_Sigma;
            
        }
        virtual void SetReferenceSegmentation(ConstImagePointerType im){
            m_referenceSegmentation=im;
        }
        virtual void SetReferenceGradient(ConstImagePointerType im){
            m_referenceGradientImage=im;
        }
        virtual void SetReferenceImage(ConstImagePointerType im){
            m_referenceImage=im;
        }
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            int s= this->m_sheetnessImage->GetPixel(fixedIndex);
            double imageIntensity=this->m_fixedImage->GetPixel(fixedIndex);
            double segmentationProb=1;
            int tissue=-500;
            int bone=300;
            if (segmentationLabel>0) {
                if (imageIntensity < tissue)
                    segmentationProb = fabs(tissue-imageIntensity);
                else if (imageIntensity <bone)
                    segmentationProb = 0.69;
                else
                    segmentationProb = 0;
            }else{
                if (imageIntensity > bone && s>0)
                    segmentationProb = fabs(imageIntensity-bone);
                else if (imageIntensity > tissue)
                    segmentationProb = 0.69;
                else
                    segmentationProb = 0;
            }
            return segmentationProb;
        }

        virtual double getWeight(IndexType idx1, IndexType idx2){
            int s1=this->m_sheetnessImage->GetPixel(idx1);
            int s2=this->m_sheetnessImage->GetPixel(idx2);
            double edgeWeight=fabs(s1-s2);
            edgeWeight*=edgeWeight;

            //int i1=this->m_fixedImage->GetPixel(idx1);
            //int i2=this->m_fixedImage->GetPixel(idx2);
            //double intensityDiff=(i1-i2)*(i1-i2);
            edgeWeight=(s1 < s2) ? 1.0 : exp( - 40* (edgeWeight/this->m_gradientSigma) );
            //edgeWeight=(s1 < s2) ? 1.0 : exp( - 0.05* edgeWeight );
            //edgeWeight= exp( - 0.5 * 0.5*(edgeWeight/this->m_gradientSigma) +intensityDiff/this->m_Sigma);
            //edgeWeight= 0.5 * 0.5*(edgeWeight/this->m_gradientSigma +intensityDiff/this->m_Sigma);
            //edgeWeight= 0.5 * (edgeWeight/this->m_gradientSigma);
            //edgeWeight= 1;//0.5 * intensityDiff/this->m_Sigma;
            return edgeWeight;
        }
    };//class
    template<class TImage>
    class UnaryPotentialSegmentationArtificial: public UnaryPotentialSegmentation<TImage>{
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationArtificial            Self;
        typedef  UnaryPotentialSegmentation<TImage> Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;

        typedef	TImage ImageType;
        typedef typename ImageType::IndexType IndexType;

    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationArtificial, Object);
        
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            double imageIntensity=this->m_fixedImage->GetPixel(fixedIndex);
            double segmentationProb=1;
            switch (segmentationLabel) {
            case 1  :
                segmentationProb = (imageIntensity < 128 ) ? 1 : 0;
                break;
            case 0:
                segmentationProb = ( imageIntensity > 128)  ? 1 : 0;
                break;
            default:
                assert(false);
            }
            //        std::cout<<fixedIndex<<" "<<segmentationLabel<<" " << imageIntensity <<" "<<segmentationProb<<std::endl;
            return segmentationProb;
        }

      
    };//class
    template<class TImage>
    class UnaryPotentialSegmentationArtificial2: public  UnaryPotentialSegmentation<TImage>{
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationArtificial2            Self;
        typedef UnaryPotentialSegmentation<TImage> Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
        typedef TImage ImageType;
        typedef typename ImageType::IndexType IndexType;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationArtificial2, Object);
        
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            double imageIntensity=this->m_fixedImage->GetPixel(fixedIndex);
            double segmentationProb=1;
            if (segmentationLabel>=1) {
                segmentationProb = (imageIntensity < 85 || imageIntensity>170  ) ? 1 : 0;
            }
            else{
                segmentationProb =  (imageIntensity > 85 && imageIntensity<170  )  ? 1 : 0;
            }
            //        std::cout<<fixedIndex<<" "<<segmentationLabel<<" " << imageIntensity <<" "<<segmentationProb<<std::endl;
            return segmentationProb;
        }

    };//class

 

    template<class TImage>
    class UnaryPotentialSegmentationProb: public UnaryPotentialSegmentation<TImage>{
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationProb            Self;
        typedef  UnaryPotentialSegmentation<TImage> Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
        typedef TImage ImageType;
        typedef typename ImageType::IndexType IndexType;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationProb, Object);
        
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            double imageIntensity=1.0*this->m_fixedImage->GetPixel(fixedIndex)/255;
            double segmentationProb=1;
            if (segmentationLabel>0) {
                segmentationProb = 1-imageIntensity;//(imageIntensity < 0.7 ) ? 1 : 0;
            }else{
                segmentationProb = imageIntensity;//( imageIntensity > 0.4) ? 1 : 0;
            }
            //   std::cout<<fixedIndex<<" "<<segmentationLabel<<" " << imageIntensity <<" "<<segmentationProb<<std::endl;
            
            return segmentationProb;
        }

    };//class

    template<class TImage>
    class UnaryPotentialSegmentationUnsignedBone: public UnaryPotentialSegmentation<TImage> {
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationUnsignedBone            Self;
        typedef UnaryPotentialSegmentation<TImage> Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
        
        typedef TImage ImageType;
        typedef typename ImageType::IndexType IndexType;
    public:
        
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationUnsignedBone, Object);

        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            int s=this->m_sheetnessImage->GetPixel(fixedIndex);
            int bone=(300+1000)*255.0/2000;
            int tissue=(-500+1000)*255.0/2000;
            double imageIntensity=this->m_fixedImage->GetPixel(fixedIndex);
            double segmentationProb=1;
            if (segmentationLabel>0) {
                if (imageIntensity < tissue)
                    segmentationProb =fabs(imageIntensity-tissue);
                else if (imageIntensity < bone) 
                    segmentationProb = 0.69; //log (0.5);
                else
                    segmentationProb = 0.00000001;
            }else{
                if ((imageIntensity >  bone)  && s>128)
                    segmentationProb = fabs(imageIntensity-bone);
                else if (imageIntensity >tissue)
                    segmentationProb =0.69 ;
                else
                    segmentationProb = 0.00000001;
                
                //            if (segmentationLabel>0) {
                //                segmentationProb = (imageIntensity < (-500+1000)*255.0/2000 ) ? 1 : 0;
                //            }else{
                //                segmentationProb = ( imageIntensity > (300+1000)*255.0/2000 ) && ( s > 128 ) ? 1 : 0;
            }
            return segmentationProb;
        }

        virtual double getWeight(IndexType idx1, IndexType idx2){
            int s1=this->m_sheetnessImage->GetPixel(idx1);
            int s2=this->m_sheetnessImage->GetPixel(idx2);
            double edgeWeight=fabs(s1-s2);
            //edgeWeight*=edgeWeight;

            int i1=this->m_fixedImage->GetPixel(idx1);
            int i2=this->m_fixedImage->GetPixel(idx2);
            double intensityDiff=(i1-i2)*(i1-i2);
            edgeWeight=(s1 < s2) ? 0.99999999 : exp( - 2*(edgeWeight) );
            //edgeWeight=(s1 < s2) ? 1.0 : exp( - 0.05* edgeWeight );
            //edgeWeight= exp( - 0.5 * 0.5*(edgeWeight/this->m_gradientSigma) +intensityDiff/this->m_Sigma);
            //edgeWeight= 0.5 * 0.5*(edgeWeight/this->m_gradientSigma +intensityDiff/this->m_Sigma);
            //edgeWeight= 0.5 * (edgeWeight/this->m_gradientSigma);
            //edgeWeight= 1;//0.5 * intensityDiff/this->m_Sigma;
#if 0
            edgeWeight=1-edgeWeight;
            if (edgeWeight<=0) edgeWeight=0.00001;
            edgeWeight=-log(edgeWeight);
#endif
            return edgeWeight;
        }
    };

    template<class TImage, class TClassifier>
    class UnaryPotentialSegmentationClassifier: public UnaryPotentialSegmentation<TImage> {
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationClassifier            Self;
        typedef UnaryPotentialSegmentation<TImage> Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
        
        typedef TImage ImageType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;

        typedef TClassifier ClassifierType;
        typedef typename ClassifierType::Pointer ClassifierPointerType;
        
    protected:
        ConstImagePointerType m_deformationPrior;
        double m_alpha;
        ClassifierPointerType m_classifier;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationClassifier, Object);
          
        virtual void Init(){
            
            m_classifier=  ClassifierType::New();
            m_classifier->setNIntensities(256);
            m_classifier->setData(this->m_referenceImage,this->m_referenceSegmentationImage,(ConstImagePointerType)this->m_referenceGradientImage);
            //m_classiifier->setData(movingImage,movingSegmentationImage);
#if 1
            m_classifier->train();
            m_classifier->saveProbs("test.probs");
#else
            m_classifier->loadProbs("test.probs");
#endif
            //m_classifier->evalImage(targetImage);
            //m_classifier->evalImage(targetImage,fixedGradientImage);
        }
        
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            double imageIntensity=this->m_fixedImage->GetPixel(fixedIndex);
            int s= this->m_sheetnessImage->GetPixel(fixedIndex);

            //prob of inverse segmentation label
            //double prob=m_classifier->px_l(imageIntensity,s,(segmentationLabel));
           
            //cout<<imageIntensity<<" "<<s<<" "<<segmentationLabel<<" "<<prob<<" "<< -log(prob) <<std::endl ;
            //penalize only if prob <0.6
#if 1
#if 0
            double prob=1-m_classifier->px_l(imageIntensity,s,!(segmentationLabel>0));
            if (prob<=0) prob=0.00000000001;
            return -log(prob);
#else
            //double prob=m_classifier->px_l(imageIntensity,(segmentationLabel>0));
            double prob=m_classifier->px_l(imageIntensity,(segmentationLabel>0),s);
            //            if (prob<=0) prob=0.00000000001;
            //if (segmentationLabel && prob<0.5) prob=0.5;
            return -log(prob);
#endif
#else
            double result=0.0;
            if (segmentationLabel){
                if (imageIntensity<50 || s<100){
                    result=1;//fabs(imageIntensity - 50);
                }
            }
            else{
                if (imageIntensity>120 ){
                    result=1;//fabs(imageIntensity - 120);
                }
            }
            return result;
#endif
        }

        virtual double getWeight(IndexType idx1, IndexType idx2){
            assert(false);
            return -1;
        }
      
    };

  

    template<class TImage, class TClassifier>
    class UnaryPotentialSegmentationClassifierWithPrior: 
        public UnaryPotentialSegmentationClassifier<TImage,TClassifier>  
    {
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationClassifierWithPrior            Self;
        typedef UnaryPotentialSegmentationClassifier<TImage,TClassifier> UnarySuperclass;


        
        
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
        
        typedef TImage ImageType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;
        typedef TClassifier ClassifierType;
        typedef typename ClassifierType::Pointer ClassifierPointerType;

        typedef PairwisePotentialSegmentationRegistration<TImage> SRSPotentialType;
        typedef typename SRSPotentialType::Pointer SRSPotentialPointerType;
        
    protected:
        double m_alpha;
        SRSPotentialPointerType m_srsPotential;
        itk::Vector<float,ImageType::ImageDimension> zeroDisplacement;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationClassifier, Object);

        UnaryPotentialSegmentationClassifierWithPrior(){
            zeroDisplacement.Fill(0.0);
        }
        void SetAlpha(double alpha){this->m_alpha=alpha;}     
        void SetSRSPotential(SRSPotentialPointerType pot){m_srsPotential=pot;}
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            double origPotential=UnarySuperclass::getPotential(fixedIndex,segmentationLabel);
            
            double priorPotential=m_srsPotential->getPotential(fixedIndex,fixedIndex,zeroDisplacement,segmentationLabel);
            return origPotential+m_alpha*priorPotential;
        }
    };//class


    template<class TImage>
    class UnaryPotentialSegmentationUnsignedBoneMarcel: public UnaryPotentialSegmentation<TImage> {
    public:
        //itk declarations
        typedef UnaryPotentialSegmentationUnsignedBoneMarcel           Self;
        typedef UnaryPotentialSegmentation<TImage> Superclass;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
        
        typedef TImage ImageType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename TImage::ConstPointer ConstImagePointerType;
    private:
        ConstImagePointerType m_tissuePrior;
    public:
        
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(UnaryPotentialSegmentationUnsignedBone, Object);
        
        void SetTissuePrior(ConstImagePointerType img){m_tissuePrior=img;}
        
        virtual double getPotential(IndexType fixedIndex, int segmentationLabel){
            int s=this->m_sheetnessImage->GetPixel(fixedIndex);
            int bone=(300+1000)*255.0/2000;
            int tissue=(-500+1000)*255.0/2000;
            double imageIntensity=this->m_fixedImage->GetPixel(fixedIndex);
            double totalCost=1;
            
            switch (segmentationLabel) {
            case 0:
                totalCost = ( imageIntensity > bone) && ( s > 0 ) ? 1 : 0;
                break;
            default  :
                bool tissuePrior = (this->m_tissuePrior->GetPixel(fixedIndex))>0;
                totalCost = (tissuePrior || imageIntensity < tissue) ? 1 : 0;
                break;
            }
            return totalCost;
        }
    };//class
}//namespace
#endif /* POTENTIALS_H_ */
