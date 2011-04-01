/*
 * Potentials.h
 *
 *  Created on: Nov 24, 2010
 *      Author: gasst
 */

#ifndef _NCCPOTENTIAL_H_
#define _NCCPOTENTIAL_H_
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <utility>
#include "itkVector.h"
#include "SRSPotential.h"
namespace itk{

template<class TLabelMapper,class TImage,class TSegmentationInterpolator, class TImageInterpolator>
class NCCRegistrationUnaryPotential : public SegmentationRegistrationUnaryPotential<TLabelMapper,TImage,TSegmentationInterpolator,TImageInterpolator>{
public:
	//itk declarations
	typedef NCCRegistrationUnaryPotential            Self;
	typedef SegmentationRegistrationUnaryPotential<TLabelMapper,TImage,TSegmentationInterpolator,TImageInterpolator>       Superclass;
	typedef SmartPointer<Self>        Pointer;
	typedef SmartPointer<const Self>  ConstPointer;

	typedef	TImage ImageType;
	typedef typename ImageType::Pointer ImagePointerType;
	typedef TLabelMapper LabelMapperType;
	typedef typename LabelMapperType::LabelType LabelType;
	typedef typename ImageType::IndexType IndexType;
	typedef typename ImageType::SizeType SizeType;
	typedef typename ImageType::SpacingType SpacingType;
	typedef TImageInterpolator InterpolatorType;
	typedef typename InterpolatorType::Pointer InterpolatorPointerType;
	typedef typename InterpolatorType::ContinuousIndexType ContinuousIndexType;
	typedef typename LabelMapperType::LabelImagePointerType LabelImagePointerType;
public:
	/** Method for creation through the object factory. */
	itkNewMacro(Self);
	/** Standard part of every itk Object. */
	itkTypeMacro(NCCRegistrationUnaryPotential, Object);

	NCCRegistrationUnaryPotential(){
	}
	virtual double getPotential(IndexType fixedIndex, LabelType label){
		typename itk::ConstNeighborhoodIterator<ImageType> nIt(this->m_radius,this->m_fixedImage, this->m_fixedImage->GetLargestPossibleRegion());
		nIt.SetLocation(fixedIndex);
		double count=0;
		double sff=0.0,smm=0.0,sfm=0.0,sf=0.0,sm=0.0;
		itk::Vector<float,ImageType::ImageDimension> disp=LabelMapperType::getDisplacement(LabelMapperType::scaleDisplacement(label,this->m_displacementFactor));
		for (unsigned int i=0;i<nIt.Size();++i){
			bool inBounds;
			double f=nIt.GetPixel(i,inBounds);
			if (inBounds){
				IndexType neighborIndex=nIt.GetIndex(i);
				//this should be weighted somehow
				ContinuousIndexType idx2(neighborIndex);

				double weight=1.0;
				for (int d=0;d<ImageType::ImageDimension;++d){
					double w=1-(1.0*fabs(neighborIndex[d]-fixedIndex[d]))/(2*this->m_radius[d]);
					idx2[d]+=disp[d];
					weight*=w;
				}
				itk::Vector<float,ImageType::ImageDimension> baseDisp=
						LabelMapperType::getDisplacement(this->m_baseLabelMap->GetPixel(neighborIndex));
				idx2+=baseDisp;
				double m;
				if (!this->m_movingInterpolator->IsInsideBuffer(idx2)){
//					continue;
					m=0;
					for (int d=0;d<ImageType::ImageDimension;++d){
						if (idx2[d]>=this->m_movingInterpolator->GetEndContinuousIndex()[d]){
							idx2[d]=this->m_movingInterpolator->GetEndContinuousIndex()[d]-0.5;
						}
						else if (idx2[d]<this->m_movingInterpolator->GetStartContinuousIndex()[d]){
							idx2[d]=this->m_movingInterpolator->GetStartContinuousIndex()[d]+0.5;
						}
					}
				}
//				else{
					m=this->m_movingInterpolator->EvaluateAtContinuousIndex(idx2);
//				}
				sff+=f*f;
				smm+=m*m;
				sfm+=f*m;
				sf+=f;
				sm+=m;
				//				res+=weight*m_unaryFunction->getPotential(neighborIndex,label);
				count+=1;//weight;
			}

		}
		//		std::cout<<fixedIndex<<" "<<label<<" "<<this->m_fixedImage->GetPixel(fixedIndex)<<" "<<(1-fabs(1.0*sfm/sqrt(smm*sff)))<<" "<<sfm<<" "<<sff<<" "<<smm<<std::endl;

		if (count){
			sff -= ( sf * sf / count );
			smm -= ( sm * sm / count );
			sfm -= ( sf * sm / count );
			double result;
			if (smm*sff){
				result=-(1.0*sfm/sqrt(smm*sff));
//				result=(1-fabs(1.0*sfm/sqrt(smm*sff)));
//								result=(1-(1.0*sfm/sqrt(smm*sff)+1.0)/2);

			}
			else if (sfm>0)result=-1;
			else result=1;
//			std::cout<<sfm<<" "<<smm<<" "<<sff<<" "<<result<<std::endl;
			return this->m_intensWeight*result;
		}
		//no correlation whatsoever
		else return 0;
	}
	virtual ImagePointerType trainClassifiers(){
		return NULL;
	}
};//class

}//namespace
#endif /* POTENTIALS_H_ */