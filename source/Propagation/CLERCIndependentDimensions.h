#pragma once
#include "matrix.h"
#include "BaseLinearSolver.h"
#include "TransformationUtils.h"
#include "Log.h"
#include "ImageUtils.h"
#include <vector>
#include <sstream>
#include "SolveAquircGlobalDeformationNormCVariables.h"
#include "Metrics.h"
#include "TemporalMedianImageFilter.h"
#include "itkGaussianImage.h"
#include <boost/lexical_cast.hpp>
#include "itkNormalizedCorrelationImageToImageMetric.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkMeanSquaresImageToImageMetricv4.h"
#include "itkCorrelationImageToImageMetricv4.h"
#include "itkRegistrationParameterScalesFromJacobian.h"
#include "itkRegistrationParameterScalesFromPhysicalShift.h"
#include "itkLabelOverlapMeasuresImageFilter.h"
#include "SegmentationFusion.h"
#include <itkGradientMagnitudeImageFilter.h>
#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>
#include <itkDisplacementFieldJacobianDeterminantFilter.h>
#include "SegmentationTools.hxx"
template<class ImageType >
//template<class ImageType, class MetricType=itk::NormalizedCorrelationImageToImageMetric<ImageType,ImageType> >
//template<class ImageType, class MetricType=itk::MeanSquaresImageToImageMetric<ImageType,ImageType> >
class CLERCIndependentDimensions: public AquircGlobalDeformationNormSolverCVariables< ImageType>{
public:
    typedef typename  TransfUtils<ImageType>::DeformationFieldType DeformationFieldType;
    typedef typename  TransfUtils<ImageType>::DisplacementType DisplacementType;
    typedef typename  DeformationFieldType::Pointer DeformationFieldPointerType;
    typedef typename  ImageType::OffsetType OffsetType;
    typedef typename  DeformationFieldType::SpacingType SpacingType;

    typedef typename ImageType::SizeType SizeType;
    typedef typename ImageUtils<ImageType,double>::FloatImagePointerType FloatImagePointerType;
    typedef typename ImageUtils<ImageType,double>::FloatImageType FloatImageType;
    typedef typename itk::ImageRegionIterator<FloatImageType> FloatImageIterator;
    typedef typename itk::ImageRegionIteratorWithIndex<DeformationFieldType> DeformationFieldIterator;
    typedef typename itk::ImageRegionIterator<ImageType> ImageIterator;
    typedef typename DeformationFieldType::PixelType DeformationType;
    typedef typename DeformationFieldType::IndexType IndexType;
    typedef typename DeformationFieldType::PointType PointType;
    typedef typename ImageType::Pointer ImagePointerType;
    typedef typename ImageType::ConstPointer ConstImagePointerType;
    typedef typename ImageType::RegionType RegionType;
    static const unsigned int D=ImageType::ImageDimension;
    static const unsigned int internalD=1;
    typedef GaussianEstimatorScalarImage<FloatImageType> GaussEstimatorType;
    //typedef MinEstimatorScalarImage<FloatImageType> GaussEstimatorType;

    typedef typename TransfUtils<ImageType>::DeformationCacheType DeformationCacheType;

    typedef typename TransfUtils<ImageType,double>::DisplacementFieldTransformType DisplacementFieldTransformType;
    typedef typename DisplacementFieldTransformType::Pointer DisplacementFieldTransformPointer;
    
    typedef map<string,ImagePointerType> ImageCacheType;
    typedef map<string,FloatImagePointerType> FloatImageCacheType;

    typedef map<string, map< string, string> > FileListCacheType;
    

protected:
    int m_nVars,m_nEqs,m_nNonZeroes;
    int m_numImages;
    DeformationCacheType  m_deformationCache, m_trueDeformations, m_updatedDeformationCache, m_downSampledDeformationCache,m_pairwiseGradients,m_estimatedDeformations;
    FileListCacheType m_deformationFileList, m_trueDeformationFileList;
    map<string,string> m_landmarkFileList;
    std::vector<string>  m_imageIDList;
    bool m_additive, m_updateDeformations, m_updateDeformationsGlobalSim;
    RegionType m_regionOfInterest;
    ImagePointerType m_grid;
    int m_nGridPoints;
    ImageCacheType  m_imageList;
    ImageCacheType  *m_maskList;
    ImageCacheType  m_atlasSegmentations,m_groundTruthSegmentations;

    map< int, map <int, GaussEstimatorType > > m_pairwiseInconsistencyStatistics;
    map< string , map <string, FloatImagePointerType > >  m_pairwiseLocalWeightMaps,  m_updatedPairwiseLocalWeightMaps;
    map< string, map <string, double> > m_pairwiseGlobalSimilarity;
    string m_metric;
    std::vector< map< string, int > >m_offsets;
    std::vector< map<string , map<string,FloatImagePointerType> > > m_pairwiseMetricDerivatives;
private:
    bool m_smoothDeformationDownsampling;
    int m_nPixels;// number of pixels/voxels

    int m_nEqFullCircleEnergy; // number of equations for  observation energy E_d
    int m_nVarFullCircleEnergy; // number of variables for each equation of E_d;
    double m_wFullCircleEnergy;

    int m_nEqCircleNorm; // number of equations for energy circular constraint E_circ
    int m_nVarCircleNorm; // number of variables for each equation of E_circ;
    double m_wCircleNorm;

    int m_nEqTransformationSimilarity; // number of equations for Transformation similarity energy E_T
    int m_nVarTransformationSimilarity; // number of variables for each equation of E_T
    double m_wTransformationSimilarity;

    int m_nEqTransformationSymmetry; // number of equations for Transformation similarity energy E_T
    int m_nVarTransformationSymmetry; // number of variables for each equation of E_T
    double m_wTransformationSymmetry;
    
    int m_nEqErrorInconsistency; // number of equations for energy incErrular constraint E_incErr
    int m_nVarErrorInconsistency; // number of variables for each equation of E_incErr;
    double m_wErrorInconsistency;

    int m_nEqErrorNorm; // number of equations for error minimizing energy E_delta
    int m_nVarErrorNorm; // number of variables for each equation of E_delta;
    double m_wErrorNorm;

    int m_nEqDeformationSmootheness; // number of equations for spatial smoothing energy E_s
    int m_nVarDeformationSmootheness; // number of variables for each equation of E_d;
    double m_wDeformationSmootheness;

    int m_nEqErrorSmootheness; // number of equations for spatial smoothing energy E_s
    int m_nVarErrorSmootheness; // number of variables for each equation of E_d;
    double m_wErrorSmootheness;

    int m_nEqErrorStatistics; // number of equations for statistics over error inconsistencies
    int m_nVarErrorStatistics; // number of variables for statistics over error inconsistencies
    double m_wErrorStatistics;
    enum StatisticType {MIN,MAX,MEAN,MEDIAN};
    StatisticType m_TypeErrorStatistics;
        
    string m_optimizer;

    
    double m_wSum;

    double m_sigma;
    
    bool m_locallyUpdateDeformationEstimate;

    double m_exponent;
    
    double m_shearingReduction;
    bool m_linearInterpol;
    int m_ORACLE;
    bool m_haveDeformationEstimate;

    double m_segConsisntencyWeight;

    std::vector<mxArray * > m_results;

    bool m_estDef,m_estError;

    int m_numDeformationsToEstimate,m_nCircles;
    
    double m_resolutionFactor;
    double m_ADE, m_TRE, m_dice,m_Inconsistency;
    bool m_lowResolutionEval,m_bSplineInterpol;
    bool m_filterMetricWithGradient;
    bool m_lineSearch;
    double m_spacingBasedSmoothnessReduction;
    bool m_useConstraints;
    double m_minMinJacobian,m_averageNCC;
    bool m_useTaylor;
    double m_minSim,m_maxSim;
    double 	m_averageJacSTD;
    bool m_lowResSimilarity;
public:
    CLERCIndependentDimensions(){
        m_wTransformationSimilarity=1.0;
        m_wDeformationSmootheness=1.0;
        m_wCircleNorm=1.0;
        m_wErrorNorm=1.0;
        m_wFullCircleEnergy=1.0;
        m_wSum=1.0;
        m_sigma = 10.0;
        m_linearInterpol=false;
        m_haveDeformationEstimate=false;
        //m_updatedDeformationCache = new  map< string, map <string, DeformationFieldPointerType> > ; 
        m_results = std::vector<mxArray * >(D,NULL);
        //m_updateDeformations=true;
        m_updateDeformations=false;
        m_exponent=1.0;
        m_shearingReduction = 1.0;
        m_segConsisntencyWeight = 1.0;
        m_locallyUpdateDeformationEstimate = false;
        m_ORACLE=0;
        m_nEqTransformationSimilarity=0;
        m_metric="lncc";
        m_wTransformationSymmetry=0.0;
        m_resolutionFactor=0.125;
        m_ADE=-1;
        m_TRE=-1;
        m_dice=-1;
        m_Inconsistency=-1;
        m_lowResolutionEval=false;
        m_bSplineInterpol=false;
        m_lineSearch=false;
        m_updateDeformationsGlobalSim=false;
        m_optimizer="csdx100";
        m_maskList=NULL;
        m_useTaylor=false;
        m_minSim=100;
        m_maxSim=-1;
        m_pairwiseMetricDerivatives=std::vector<map<string , map<string,FloatImagePointerType> > >(D);
	m_lowResSimilarity=false;
    }

    double getADE(){return m_ADE;}
    double getTRE(){return m_TRE;}
    double getDice(){return m_dice;}
    double getInconsistency(){return m_Inconsistency;}
    double getMinJac(){return m_minMinJacobian;}
    double getAverageNCC(){ return m_averageNCC;}
    double getAverageJacSTD(){return 	m_averageJacSTD;}
    void setOptimizer(string s){m_optimizer=s;}
    void setDeformationFilenames(FileListCacheType deformationFilenames) {m_deformationFileList = deformationFilenames;}
    void setTrueDeformationFilenames(FileListCacheType trueDeformationFilenames){m_trueDeformationFileList=trueDeformationFilenames;}
    void setLandmarkFilenames(map<string,string> landmarkFilenames){m_landmarkFileList=landmarkFilenames;}
    void setAtlasSegmentations(ImageCacheType  atlasSegmentations){m_atlasSegmentations=atlasSegmentations;}
    void setGroundTruthSegmentations(ImageCacheType  groundTruthSegmentations){m_groundTruthSegmentations = groundTruthSegmentations;}
    void setImageIDs( std::vector<string> imageIDs){m_imageIDList=imageIDs;}
    void setImages(ImageCacheType  inputImages){        m_imageList=inputImages;    };
    void setMasks(ImageCacheType *  masks){        m_maskList=masks;    };
    void setLowResEval(bool b){ m_lowResolutionEval=b;}
    void setBSplineInterpol(bool b){m_bSplineInterpol=b;}
    void setLineSearch(bool b){m_lineSearch=b;}
    void setUseConstraints(bool b){m_useConstraints=b;}
    void setUpdateDeformationsGlobalSim(bool b){ m_updateDeformationsGlobalSim=b;}
    void setUseTaylor(bool b){m_useTaylor=b;}
    void setLowResSim(bool b){m_lowResSimilarity=b;}
    void setROI(ImagePointerType ROI){ 
        this->m_ROI=ROI;
        m_nPixels=this->m_ROI->GetLargestPossibleRegion().GetNumberOfPixels( );
        IndexType startIndex,nullIdx;
        nullIdx.Fill(0);
        PointType startPoint;
        this->m_ROI->TransformIndexToPhysicalPoint(nullIdx,startPoint);
        (m_imageList)[(m_imageIDList)[0]]->TransformPhysicalPointToIndex(startPoint,startIndex);
        m_regionOfInterest.SetIndex(startIndex);
        m_regionOfInterest.SetSize(this->m_ROI->GetLargestPossibleRegion().GetSize());
    }
    void setGrid(ImagePointerType grid){
        this->m_grid=grid;
        m_nGridPoints=grid->GetLargestPossibleRegion().GetNumberOfPixels();

    }

    virtual void Initialize(){
        m_numImages=m_imageIDList.size();
        m_numDeformationsToEstimate=0;
        m_nCircles=0;
        //calculate number of deformations and number of deformation circles
        for (int s = 0;s<m_numImages;++s){                            
            int source=s;
            string sourceID=(m_imageIDList)[source];
            if (m_maskList && m_maskList->find(sourceID)!=m_maskList->end()){
                (*m_maskList)[sourceID]=FilterUtils<ImageType>::NNResample((*m_maskList)[sourceID],this->m_ROI,false);
                int dilation=1;
                if (m_bSplineInterpol)
                    dilation=2;
                //dilate mask to avoid interpolation artefacts at mask borders. 
                //dilation needs to be larger for bspline because of larger support area
                (*m_maskList)[sourceID]=FilterUtils<ImageType>::dilation((*m_maskList)[sourceID],dilation);
                if (s==0)
                    m_nPixels=int(FilterUtils<ImageType>::sum((*m_maskList)[sourceID]));
                else{
                    //use largest mask as estimate for calculation of number of free variables
                    m_nPixels=max(m_nPixels,int(FilterUtils<ImageType>::sum((*m_maskList)[sourceID])));
                }
            }
            
                            

            for (int t=0;t<m_numImages;++t){
                if (t!=s){
                    int target=t;
                    string targetID=(m_imageIDList)[target];


                    bool estSourceTarget=false;
                    bool skip=false;
                    if (findDeformation(m_deformationFileList,sourceID,targetID)){
                        estSourceTarget=true;
                    }else{
                        if (findDeformation(m_trueDeformationFileList,sourceID,targetID) &&  (m_trueDeformations)[sourceID][targetID].IsNull() ){
                            (m_trueDeformations)[sourceID][targetID] = ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[sourceID][targetID]);
                            (m_trueDeformations)[sourceID][targetID]=TransfUtils<ImageType>::linearInterpolateDeformationField( (m_trueDeformations)[sourceID][targetID],this->m_ROI,m_smoothDeformationDownsampling);
                        }
                        else
                            skip=true;
                    }
                    m_numDeformationsToEstimate+=estSourceTarget;

                    for (int i=0;i<m_numImages;++i){ 
                        if (t!=i && i!=s){
                            //define a set of 3 images
                            int intermediate=i;
                            string intermediateID=(m_imageIDList)[i];
                            
                            bool estSourceIntermediate=false;
                            if (findDeformation(m_deformationFileList,sourceID,intermediateID)){
                                estSourceIntermediate=true;
                            }else{
                                if (findDeformation(m_trueDeformationFileList,sourceID,intermediateID) &&  (m_trueDeformations)[sourceID][targetID].IsNull())
                                    {
                                        (m_trueDeformations)[sourceID][intermediateID] = ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[sourceID][intermediateID]);
                                        (m_trueDeformations)[sourceID][intermediateID]=TransfUtils<ImageType>::linearInterpolateDeformationField( (m_trueDeformations)[sourceID][intermediateID],this->m_ROI,m_smoothDeformationDownsampling);
                 
                                    }
                                else
                                    skip=true;
                            }

                            bool estIntermediateTarget=false;
                            if (findDeformation(m_deformationFileList,intermediateID,targetID)){
                                estIntermediateTarget=true;
                            }else{
                                if (findDeformation(m_trueDeformationFileList,intermediateID,targetID) &&  (m_trueDeformations)[sourceID][targetID].IsNull())
                                    {
                                        (m_trueDeformations)[intermediateID][targetID] = ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[intermediateID][targetID]);
                                        (m_trueDeformations)[intermediateID][targetID]=TransfUtils<ImageType>::linearInterpolateDeformationField( (m_trueDeformations)[intermediateID][targetID],this->m_ROI,m_smoothDeformationDownsampling);
                                    }
                                else
                                    skip = true;
                            }
                            
                            //check if any of the deformations of the loop should be estimated
                            if (! skip && (estIntermediateTarget || estSourceTarget || estSourceIntermediate)){
                                ++m_nCircles;
                            }
                        }
                    }
                }
            }
        }
        LOGV(1)<<VAR(m_nCircles)<<" "<<VAR(m_numDeformationsToEstimate)<<endl;
        if (m_nCircles == 0 || m_numDeformationsToEstimate == 0){
            LOG<<"No deformations to estimate, or not able to form any circles from input registrations, aborting!"<<endl;
            exit(-1);
        }
        
        //normalize weights according to number of deformation pairs/circles
        m_wFullCircleEnergy /=m_nCircles;
        m_wCircleNorm/=m_nCircles;
        m_wErrorInconsistency/=m_nCircles;
        m_wDeformationSmootheness/=m_numDeformationsToEstimate;
        m_wErrorSmootheness/=m_numDeformationsToEstimate;
        m_wErrorNorm/=m_numDeformationsToEstimate;
        m_wErrorStatistics/=m_numDeformationsToEstimate;
        m_wTransformationSimilarity/=m_numDeformationsToEstimate;
        if (m_nPixels!=m_nGridPoints){
            m_wTransformationSimilarity*=1.0*m_nGridPoints/(m_nPixels);
        }
        m_wTransformationSymmetry/=m_numDeformationsToEstimate;
        
        LOGV(2)<<VAR(m_nPixels)<<endl;
      
        if (m_maskList){
            map<string,int> dummy;
            dummy["000"]=-1;
            m_offsets=std::vector< map< string, int > >(m_numImages*(m_numImages-1),dummy);
        }


        //compute errors, weights, and so on and so forth?
        DoALot();

    }
    
    void setOracle(int o){m_ORACLE=o;}
    void setWeightFullCircleEnergy(double w){m_wFullCircleEnergy=w;}
    void setWeightTransformationSimilarity(double w,bool update=false){
        m_wTransformationSimilarity=w;  
        if (update && m_nEqTransformationSimilarity){
            m_wTransformationSimilarity/=m_numDeformationsToEstimate;
        }
    }
    void setWeightTransformationSymmetry(double w, bool update=false){
        m_wTransformationSymmetry=w;  
        if (update && m_nEqTransformationSymmetry){
            m_wTransformationSymmetry/=m_numDeformationsToEstimate;
        }
    }
  
    void setWeightDeformationSmootheness(double w){m_wDeformationSmootheness=w;}
    void setWeightErrorSmootheness(double w){m_wErrorSmootheness=w;}
    void setWeightErrorNorm(double w){m_wErrorNorm=w;}
    void setWeightErrorStatistics(double w){m_wErrorStatistics=w;}
    void setWeightCircleNorm(double w){m_wCircleNorm=w;}
    void setWeightSum(double w){m_wSum=w;}
    void setWeightInconsistencyError(double w){m_wErrorInconsistency=w;}
    void setLinearInterpol(bool i){m_linearInterpol=i;}
    void setSigma(double s){m_sigma=s;}

    void setLocalWeightExp(double e){ m_exponent=e;}
    void setShearingReduction(double r){m_shearingReduction = r;}
    void setScalingFactorForConsistentSegmentation(double scalingFactorForConsistentSegmentation){ m_segConsisntencyWeight = scalingFactorForConsistentSegmentation;}
    void setUpdateDeformations(bool b){m_updateDeformations=b;}
    void setLocallyUpdateDeformations(bool s){m_locallyUpdateDeformationEstimate=s;}
    void setSmoothDeformationDownsampling(bool s){m_smoothDeformationDownsampling=s;}
    void setMetric(string m){m_metric=m;}
    void setFilterMetricWithGradient(bool b){m_filterMetricWithGradient=b;}
    virtual void createSystem(){
        //set up ROI

      
        int interpolationFactor;
        if (m_linearInterpol){
            interpolationFactor = pow(2,D); //linear interpolation
        }
        else
            interpolationFactor = 1 ; //NNinterpolation;
        


        //compute number of variables
        m_nEqCircleNorm =  (m_wCircleNorm>0.0)* m_nGridPoints * internalD * m_nCircles;//m_numImages*(m_numImages-1)*(m_numImages-2); //again all components of all triples
        m_nVarCircleNorm = interpolationFactor+2 ; // only one/2^D variables per pair
       
        m_nEqDeformationSmootheness =  (m_wDeformationSmootheness>0.0)* D* m_nGridPoints * internalD * m_numImages*(m_numImages-1); //every pixel in each registration has D neighbors (in one direction), and each component separately
        m_nVarDeformationSmootheness = 3; //for piecewise linear regularization, 2 for piecewise constant
        
        m_nEqTransformationSimilarity =  (m_wTransformationSimilarity>0.0)*m_nPixels * internalD * m_numDeformationsToEstimate;//m_numImages*(m_numImages-1); //same as ErrorNorm
        m_nVarTransformationSimilarity=m_nPixels!=m_nGridPoints?interpolationFactor:1;
      


        long int m_nEQsTripls= m_nEqCircleNorm;
        m_nEqs= m_nEqCircleNorm+ m_nEqDeformationSmootheness + m_nEqTransformationSimilarity ; // total number of equations
        if (m_metric == "gradient") m_nEqs+= (m_wTransformationSimilarity>0.0)*m_nPixels * internalD * m_numDeformationsToEstimate; //additional bounds on stepsize?
        long int m_nEQsPairs=m_nEqs-m_nEQsTripls;


        
        m_estError= false ;
        m_estDef =  m_nEqTransformationSimilarity ||  m_nEqDeformationSmootheness ;
        m_nVars= m_numImages*(m_numImages-1)*m_nGridPoints*internalD *(m_estError + m_estDef); // total number of free variables (error and deformation)

        long int m_nNonZeroesTripls=m_nEqCircleNorm * m_nVarCircleNorm; //maximum number of non-zeros        
        m_nNonZeroes=m_nEqCircleNorm * m_nVarCircleNorm + m_nEqDeformationSmootheness*m_nVarDeformationSmootheness + m_nEqTransformationSimilarity*m_nVarTransformationSimilarity; //maximum number of non-zeros
       	if (m_metric == "gradient")m_nNonZeroes +=(m_wTransformationSimilarity>0.0)*m_nPixels * internalD * m_numDeformationsToEstimate; //additional bounds on stepsize?

        long int m_nNonZerosPairs=m_nNonZeroes-m_nNonZeroesTripls;


        LOGV(1)<<"Creating equation system.."<<endl;
        LOGV(1)<<VAR(m_numImages)<<" "<<VAR(m_nGridPoints)<<" "<<VAR(m_nEqs)<<" "<<VAR(m_nVars)<<" "<<VAR(m_nNonZeroes)<<endl;
        LOGV(1)<<VAR(  m_wTransformationSimilarity)<<" "<<VAR(        m_wDeformationSmootheness)<<" "<<VAR(m_wCircleNorm)<<" "<<VAR(m_wErrorNorm)<<" "<<VAR(m_wErrorStatistics)<<" "<<VAR(m_wFullCircleEnergy)<<" "<<VAR(m_wSum)<<" "<<VAR(m_wErrorSmootheness)<<" "<<VAR(m_sigma)<<" "<<VAR(m_locallyUpdateDeformationEstimate)<<" "<<VAR(m_exponent)<<endl;
        double totalInconsistency = 0.0;
        int totalCount = 0;

        bool haveLocalWeights=false;

      

        mxArray *mxInit=mxCreateDoubleMatrix((mwSize)m_nVars,1,mxREAL);
        mxArray *mxUpperBound=mxCreateDoubleMatrix((mwSize)m_nVars,1,mxREAL);
        mxArray *mxLowerBound=mxCreateDoubleMatrix((mwSize)m_nVars,1,mxREAL);
     
      

        double * init=mxGetPr(mxInit);
        double * lb=mxGetPr(mxLowerBound);
        std::fill(lb,lb+m_nVars,-200);
        double * ub=mxGetPr(mxUpperBound);
        std::fill(ub,ub+m_nVars,200);
        long int cForConsistency, eqForConsistency;
        for (unsigned int d = 0; d< D; ++d){

          
            LOGV(1)<<"creating"<<VAR(d)<<endl;

           
     


            char buffer[256+1];
            buffer[256] = '\0';
            engOutputBuffer(this->m_ep, buffer, 256);
      
            //attention matlab index convention?!?
            long int eq = 1;
            long int c=0;
        
            if (m_useTaylor || d==0){
                //only need to create inconsistency matrix once!
                LOGV(1)<<"Creating sparse matrix for triplets"<<endl;
                LOGV(2)<<"Allocating memory"<<endl;

                mxArray *mxX=mxCreateDoubleMatrix((mwSize)m_nNonZeroesTripls,1,mxREAL);
                mxArray *mxY=mxCreateDoubleMatrix((mwSize)m_nNonZeroesTripls,1,mxREAL);
                mxArray *mxV=mxCreateDoubleMatrix((mwSize)m_nNonZeroesTripls,1,mxREAL);
                mxArray *mxB=mxCreateDoubleMatrix((mwSize)m_nEQsTripls,1,mxREAL);
                if ( !mxX || !mxY || !mxV || !mxB || !mxInit){
                    LOG<<"couldn't allocate memory vor triplet matrix!"<<endl;
                    exit(0);
                }

                LOGV(2)<<"initialising memory"<<endl;
                double * x=( double *)mxGetData(mxX);    //    std::fill(x,x+m_nNonZeroesTripls,-1);
                double * y=( double *)mxGetData(mxY);    //    std::fill(y,y+m_nNonZeroesTripls,m_nVars);
                double * v=( double *)mxGetData(mxV);
                double * b=mxGetPr(mxB);        //std::fill(b,b+m_nEQsTripls,-999999);

                
            

                LOGV(2)<<"Computing triplet topology structure"<<endl;
                computeTripletEnergies( x,  y, v,  b, c,  eq,d);
                cForConsistency=c;
                eqForConsistency=eq;
                LOGV(2)<<"Passing variables to matlab"<<endl;
#if 1
                //put variables into workspace and immediately destroy them
                engPutVariable(this->m_ep,"xCord",mxX);
                mxDestroyArray(mxX);
                engPutVariable(this->m_ep,"yCord",mxY);
                mxDestroyArray(mxY);
                engPutVariable(this->m_ep,"val",mxV);
                mxDestroyArray(mxV);
                engPutVariable(this->m_ep,"b",mxB);
                mxDestroyArray(mxB);
#endif
                LOGV(2)<<"resizing arrays"<<endl;
                //remove unused entries
                ostringstream nEqs;
                nEqs<<"nEq="<<eq<<";";
                engEvalString(this->m_ep,nEqs.str().c_str());
                ostringstream nNz;
                nNz<<"nNz="<<c<<";";
                engEvalString(this->m_ep,nNz.str().c_str());
#if 0
                engEvalString(this->m_ep,"xCord=xCord(1:nNz);");
                engEvalString(this->m_ep,"yCord=yCord(1:nNz);");
                engEvalString(this->m_ep,"val=val(1:nNz);");
                engEvalString(this->m_ep,"b=b(1:nEq-1);");
#else
                engEvalString(this->m_ep,"xCord(nNz+1:end)=[];");
                engEvalString(this->m_ep,"yCord(nNz+1:end)=[];");
                engEvalString(this->m_ep,"val(nNz+1:end)=[];");
                engEvalString(this->m_ep,"b(nEq:end)=[];");
#endif
                //transform indexing of variables to be 1..nVariables
                //LOGV(2)<<"re-indexing variables.."<<endl;
                //engEvalString(this->m_ep,"oldCode=unique(sort(yCord));newCode=1:size(oldCode,1);newCode=newCode'; [a1 b1]=ismember(yCord,oldCode);yCord=newCode(b1(a1));");
                LOGV(2)<<"Creating actual sparse matrix.."<<endl;
                engEvalString(this->m_ep,"A=sparse(xCord,yCord,val);" );
                //clear unnneeded variables from matlab workspace
                //engEvalString(this->m_ep,"clear xCord yCord val b1 a1;t=toc;" );
                LOGV(1)<<"done, cleaning up"<<endl;

            }else{
                c=cForConsistency;
                eq=eqForConsistency;
            }
            //need to create pairwise entries for every dimension
            //TODO: it would be sufficient to only change the RHS!
            mxArray *mxPairsX=mxCreateDoubleMatrix((mwSize)m_nNonZerosPairs,1,mxREAL);
            mxArray *mxPairsY=mxCreateDoubleMatrix((mwSize)m_nNonZerosPairs,1,mxREAL);
            mxArray *mxPairsV=mxCreateDoubleMatrix((mwSize)m_nNonZerosPairs,1,mxREAL);
            mxArray *mxPairsB=mxCreateDoubleMatrix((mwSize)m_nEQsPairs,1,mxREAL);
            if ( !mxPairsX || !mxPairsY || !mxPairsV || !mxPairsB || !mxInit){
                LOG<<"couldn't allocate memory vor pairwise matrix!"<<endl;
                exit(0);
            }
            double * x=( double *)mxGetData(mxPairsX);        std::fill(x,x+m_nNonZerosPairs,-1);
            double * y=( double *)mxGetData(mxPairsY);        std::fill(y,y+m_nNonZerosPairs,m_nVars);
            double * v=( double *)mxGetData(mxPairsV);
            double * b=mxGetPr(mxPairsB);        std::fill(b,b+m_nEQsPairs,-999999);
            long int cPair=0,eqPair=1;
            computePairwiseEnergiesAndBounds( x,  y, v,  b, init, lb, ub, cPair,  eqPair,d);
            LOGV(1)<<VAR(eq)<<" "<<VAR(c)<<endl;
            //put variables into workspace and immediately destroy them
            engPutVariable(this->m_ep,"xCordPairs",mxPairsX);
            mxDestroyArray(mxPairsX);
            engPutVariable(this->m_ep,"yCordPairs",mxPairsY);
            mxDestroyArray(mxPairsY);
            engPutVariable(this->m_ep,"valPairs",mxPairsV);
            mxDestroyArray(mxPairsV);
            engPutVariable(this->m_ep,"bPairs",mxPairsB);
            mxDestroyArray(mxPairsB);
            //remove unused entries
            ostringstream nEqs;
            nEqs<<"nEq="<<eqPair<<";";
            engEvalString(this->m_ep,nEqs.str().c_str());
            ostringstream nNz;
            nNz<<"nNz="<<cPair<<";";
            engEvalString(this->m_ep,nNz.str().c_str());
            engEvalString(this->m_ep,"xCordPairs=xCordPairs(1:nNz)");
            engEvalString(this->m_ep,"yCordPairs=yCordPairs(1:nNz)");
            engEvalString(this->m_ep,"valPairs=valPairs(1:nNz)");
            engEvalString(this->m_ep,"bPairs=bPairs(1:nEq-1);");
            LOGI(6,engEvalString(this->m_ep,"save('early.mat');" ));
            LOGV(3)<<VAR(eqPair)<<" "<<VAR(cPair)<<endl;
            LOGV(1)<<"Creating sparse matrix for pairs"<<endl;

            //transform indexing of variables to be 1..nVariables
            //LOGV(2)<<"re-indexing variables.."<<endl;
            //engEvalString(this->m_ep,"oldCode=unique(sort(yCord));newCode=1:size(oldCode,1);newCode=newCode'; [a1 b1]=ismember(yCord,oldCode);yCord=newCode(b1(a1));");
            LOGV(2)<<"converting in matrix format.."<<endl;
            engEvalString(this->m_ep,"APairs=sparse(xCordPairs,yCordPairs,valPairs);" );
            LOGV(1)<<"done, cleaning up"<<endl;
            engEvalString(this->m_ep,"clear xCordPairs yCordPairs valPairs;" );
            //clear unnneeded variables from matlab workspace
            //engEvalString(this->m_ep,"clear xCord yCord val b1 a1;t=toc;" );
            if (m_useTaylor || d==0){
                LOGV(2)<<"concatenating matrices.."<<endl;
                engEvalString(this->m_ep,"A=vertcat(A,APairs);");
                engEvalString(this->m_ep,"b=vertcat(b,bPairs);");
                
            }
            else{
                LOGV(2)<<"updating pairwise matrix.."<<endl;
                engEvalString(this->m_ep,"A((size(A,1)-size(APairs,1)+1):end,:)=APairs;");
                engEvalString(this->m_ep,"b((size(A,1)-size(APairs,1)+1):end,:)=bPairs;");

            }
            
            engEvalString(this->m_ep,"clear bPairs APairs;" );
            LOGV(2)<<"done."<<endl;

            
            
            

            //concatenate 
            //engEvalString(this->m_ep,"xCord=vertcat(xCord,xCordPairs);");
            //            engEvalString(this->m_ep,"yCord=vertcat(yCord,yCordPairs);");
            //            engEvalString(this->m_ep,"val=vertcat(val,valPairs);");
            //            engEvalString(this->m_ep,"b=vertcat(b,bPairs);");
            //clear


           

         

            engPutVariable(this->m_ep,"init",mxInit);
            this->haveInit=true;
            LOGV(1)<<"Solving "<<VAR(d)<<endl;
            if (1){
                engEvalString(this->m_ep, "lb=[-200000*ones(size(A,2),1)];");
                engEvalString(this->m_ep, "ub=[200000*ones(size(A,2),1);]");
                engEvalString(this->m_ep, "init=init(1:size(A,2));");
            }else{
                engPutVariable(this->m_ep,"lb",mxLowerBound);
                engPutVariable(this->m_ep,"ub",mxUpperBound);
            }
         
            LOGI(6,engEvalString(this->m_ep,"save('sparse.mat');" ));
            engEvalString(this->m_ep, "norm(A*init-b)");
            LOGI(2,printf("initialisation residual %s", buffer+2));

            string opt, params="";
	  
            char delim='x';
            std::vector<string> p=split(m_optimizer,delim);
            opt=p[0];
            if (p.size()>1){
                params=p[1];
            }
            if ( opt!="lsqlin" ){
                ostringstream minFunc; 		    
                string iters=params!=""?params:"100";
                minFunc<<"tic;[x fval] =grad_solve(A,b,0,"<<iters<<",'"<<opt<<"',init);t=toc;";
                TIME(engEvalString(this->m_ep, minFunc.str().c_str()));
                TIME(engEvalString(this->m_ep, "res=norm(A*x-b);"));

                LOGV(1)<<"Done with grad_solve()"<<endl;
            }else{
                //solve using trust region method
	      
                engEvalString(this->m_ep, "options=optimset(optimset('lsqlin'),'Display','iter','TolFun',1e-54,'LargeScale','on');");//,'Algorithm','active-set' );");
                TIME(engEvalString(this->m_ep, "tic;[x resnorm residual flag  output lambda] =lsqlin(A,b,[],[],[],[],lb,ub,init,options);t=toc;"));
                TIME(engEvalString(this->m_ep, "res=norm(A*x-b);"));

                mxArray * flag=engGetVariable(this->m_ep,"flag");
                if (flag !=NULL){
                    mxDestroyArray(flag);
                }else{
                    LOG<<"LSQLIN large scale failed, trying medium scale algorithm"<<endl;
                    TIME(engEvalString(this->m_ep, "tic;[x resnorm residual flag output lambda] =lsqlin(A,b,[],[],[],[],[],[],[]);t=toc"));
                }
                       
            }
            mxArray * mxtime=engGetVariable(this->m_ep,"t");
            double* t = ( double *) mxGetData(mxtime);
	   
            mxArray * mxRes=engGetVariable(this->m_ep,"res");
            double* res = ( double *) mxGetData(mxRes);
	   
            LOGV(1)<<"Finished optimizer "<<opt<<" for dimension "<<d<<" in "<<t[0]<<" seconds, result: "<<res[0]<<std::endl;
            mxDestroyArray(mxtime);
            mxDestroyArray(mxRes);

       

            //backtransform indexing
            //engEvalString(this->m_ep, "newX=zeros(max(oldCode),1);newX(oldCode)=x;x=newX;");


            LOGI(6,engEvalString(this->m_ep,"save('test.mat');" ));
            if ((m_results[d] = engGetVariable(this->m_ep,"x")) == NULL)
                printf("something went wrong when getting the variable.\n Result is probably wrong. \n");
            //engEvalString(this->m_ep,"clearvars" );

            if (m_useTaylor){
                engEvalString(this->m_ep,"clearvars" );
            }

            
        }//dimensions
        engEvalString(this->m_ep,"clearvars" );
        mxDestroyArray(mxInit);
        mxDestroyArray(mxLowerBound);
        mxDestroyArray(mxUpperBound);
    }
    virtual void solve(){}

    virtual void storeResult(string directory){
        //std::vector<double> result(m_nVars);
        std::vector<double*> rData(D);
        for (int d= 0; d<D ; ++d){
            rData[d]=mxGetPr(this->m_results[d]);
        }

        ImagePointerType mask;
        
        for (int s = 0;s<m_numImages;++s){
            for (int t=0;t<m_numImages;++t){
                if (s!=t){
                    string sourceID=(this->m_imageIDList)[s];
                    string targetID = (this->m_imageIDList)[t];
                    //slightly(!!!) stupid creation of empty image
                    DeformationFieldPointerType estimatedError=TransfUtils<ImageType>::createEmpty(this->m_grid);
                    DeformationFieldIterator itErr(estimatedError,estimatedError->GetLargestPossibleRegion());
                    DeformationFieldPointerType estimatedDeform=TransfUtils<ImageType>::createEmpty(this->m_grid);
                    DeformationFieldIterator itDef(estimatedDeform,estimatedDeform->GetLargestPossibleRegion());
                    itErr.GoToBegin();
                    itDef.GoToBegin();
                                                                        
                    
                    if (m_maskList && m_maskList->find(targetID)!=m_maskList->end()){
                        mask=(*m_maskList)[targetID];
                    }else{
                        mask=TransfUtils<ImageType>::createEmptyImage(estimatedDeform);
                        mask->FillBuffer(0);
                        typename ImageType::SizeType size=mask->GetLargestPossibleRegion().GetSize();
                        IndexType offset;
                        //double fraction=0.9;
                        double fraction=1.0;
                        for (int d=0;d<D;++d){
                            offset[d]=(1.0-fraction)/2*size[d];
                            size[d]=fraction*size[d];
                        }
                        
                        typename ImageType::RegionType region;
                        region.SetSize(size);
                        region.SetIndex(offset);
                        LOGV(6)<<VAR(region)<<endl;
                        ImageUtils<ImageType>::setRegion(mask,region,1);
                    }
                    
                    
                    DeformationFieldPointerType oldDeformation;
                    
                    if ((m_downSampledDeformationCache)[sourceID][targetID].IsNotNull()){
                        oldDeformation=(m_downSampledDeformationCache)[sourceID][targetID];

                        DeformationFieldIterator origIt(oldDeformation,oldDeformation->GetLargestPossibleRegion());
                        origIt.GoToBegin();

                        DeformationFieldIterator trueIt;
                        if ((m_trueDeformations)[sourceID][targetID].IsNotNull()){
                            trueIt=DeformationFieldIterator((m_trueDeformations)[sourceID][targetID],(m_trueDeformations)[sourceID][targetID]->GetLargestPossibleRegion());
                            trueIt.GoToBegin();
                        }

                   
                        for (int p=0;!itErr.IsAtEnd();++itErr,++itDef,++origIt){
                            //get solution of eqn system
                            DeformationType estimatedError,estimatedDeformation,originalDeformation;
                            IndexType idx = itErr.GetIndex();
                            originalDeformation=origIt.Get();
                            if (mask->GetPixel(idx)){
                                for (unsigned int d=0;d<D;++d,++p){
                                    // minus 1 to correct for matlab indexing
                                    if (m_estError){
                                        estimatedError[d]=rData[d][edgeNumError(s,t,idx,d)-1];
                                    }
                                    if (m_estDef)
                                        estimatedDeformation[d]=rData[d][edgeNumDeformation(s,t,idx,d)-1];
                                    
                                    if (!m_estError && m_estDef){
                                        estimatedError[d]= originalDeformation[d]-estimatedDeformation[d];
                                    } else if (!m_estDef && m_estError)
                                        estimatedDeformation[d]= originalDeformation[d]-estimatedError[d];
                                }
                            }else{
                                estimatedDeformation=(originalDeformation);
                                estimatedError.Fill(0.0);
                            }
                            itErr.Set(estimatedError);
                            itDef.Set(estimatedDeformation);


                            if (mask->GetPixel(itErr.GetIndex()) && (m_trueDeformations)[sourceID][targetID].IsNotNull()){
                                DeformationType trueErr = origIt.Get()-trueIt.Get();
                                DeformationType estimatedDiffErr = estimatedDeformation - trueIt.Get();
                                //LOGV(5)<<VAR(trueErr.GetNorm())<<" "<<VAR(estimatedError.GetNorm())<<endl;
                                //LOGV(5)<<VAR(trueErr.GetNorm())<<" "<<VAR(estimatedDiffErr.GetNorm())<<endl;
                                LOGV(5)<<VAR(trueErr)<<" "<<VAR(estimatedError)<<endl;
                                LOGV(5)<<VAR(trueErr)<<" "<<VAR(estimatedDiffErr)<<endl;
                           
                                ++trueIt;
                            }
                        }





                        if ((m_trueDeformations)[sourceID][targetID].IsNotNull()){
                            mask=TransfUtils<ImageType>::createEmptyImage(estimatedDeform);
                            mask->FillBuffer(0);
                            typename ImageType::SizeType size=mask->GetLargestPossibleRegion().GetSize();
                            IndexType offset;
                            double fraction=0.9;
                            for (int d=0;d<D;++d){
                                offset[d]=(1.0-fraction)/2*size[d];
                                size[d]=fraction*size[d];
                            }
                        
                            typename ImageType::RegionType region;
                            region.SetSize(size);
                            region.SetIndex(offset);
                            LOGV(6)<<VAR(region)<<endl;
                            ImageUtils<ImageType>::setRegion(mask,region,1);
                            //mask = TransfUtils<ImageType>::warpImage(mask,estimatedDeform);

                            if (false){
                                double newError=TransfUtils<ImageType>::computeDeformationNormMask(TransfUtils<ImageType>::subtract(estimatedDeform,(m_trueDeformations)[sourceID][targetID]),mask);
                                //double newError=TransfUtils<ImageType>::computeDeformationNorm(TransfUtils<ImageType>::subtract(estimatedDeform,(m_trueDeformations)[sourceID][targetID]),1);
                                //mask->FillBuffer(1);
                                //mask = TransfUtils<ImageType>::warpImage(mask,(m_downSampledDeformationCache)[sourceID][targetID]);
                                double oldError=TransfUtils<ImageType>::computeDeformationNormMask(TransfUtils<ImageType>::subtract((m_downSampledDeformationCache)[sourceID][targetID],(m_trueDeformations)[sourceID][targetID]),mask);
                                //double oldError=TransfUtils<ImageType>::computeDeformationNorm(TransfUtils<ImageType>::subtract((m_downSampledDeformationCache)[sourceID][targetID],(m_trueDeformations)[sourceID][targetID]),1);
                                LOGV(1)<<VAR(s)<<" "<<VAR(t)<<" "<<VAR(oldError)<<" "<<VAR(newError)<<endl;
                            }
                        }
                      
                        m_estimatedDeformations[sourceID][targetID]=estimatedDeform;//Error;
                      
                   
                    }else{
                        (m_downSampledDeformationCache)[sourceID][targetID]=NULL;
                        (m_updatedDeformationCache)[sourceID][targetID]=NULL;
                    }
                }
            }
        }
      
        for (int d= 0; d<D ; ++d){
            mxDestroyArray(this->m_results[d]);
        }
       
       
    }

    std::vector<double> getResult(){
        std::vector<double> result(m_nVars);
        return result;
    }

public:
  
   

protected:
    //return fortlaufende number of pairs n1,n2, 0..(n*(n-1)-1)
    inline long int edgeNum(int n1,int n2){ return ((n1)*(m_numImages-1) + n2 - (n2>n1));}
 

    inline long int edgeNumDeformation(int n1,int n2,IndexType idx, int d){ 
        long int edgeNumber=(edgeNum(n1,n2));
        long int offset;
        if (m_maskList==NULL){
            offset= this->m_grid->ComputeOffset(idx);
        }else{
            map<string,int> & omap=m_offsets[edgeNumber];
            stringstream ss;
            ss<<idx;
            string idxStream=ss.str();
            map<string,int>::iterator  it=omap.find(idxStream);
            if (it == omap.end()){
                --it;
                int c=it->second +1;
                omap[idxStream]=c;
                offset=c;
            }else{
                offset=it->second;
            }
            //LOGV(3)<<VAR(edgeNumber)<<" "<<VAR(idx)<<" "<<VAR(offset)<<endl;
            
        }
        //return offset*internalD+edgeNum(n1,n2)*m_nGridPoints*internalD + 1 ;
        double result= internalD*edgeNumber*m_nGridPoints+offset + 1 ;
        if (result > m_nVars){
            LOG<<VAR(result)<<" "<<VAR(n1)<<" "<<VAR(n2)<<" "<<VAR(idx)<<endl;
        }
        return result;
    }

    inline long int edgeNumError(int n1,int n2,IndexType idx, int d){ 
        double result = m_estDef*m_nGridPoints*internalD*(m_numImages-1)*(m_numImages) + edgeNumDeformation(n1,n2,idx,d);
        if (result>m_nVars){
            LOG<<m_nVars<<" "<<VAR(result)<<" "<<VAR(n1)<<" "<<VAR(n2)<<" "<<VAR(idx)<<endl;
            sqrt(-1);
        }
        return result;
    }


    //compose 3 deformations. order is left-to-right
    DeformationFieldPointerType composeDeformations(DeformationFieldPointerType d1,DeformationFieldPointerType d2,DeformationFieldPointerType d3){
        return TransfUtils<ImageType>::composeDeformations(d3,TransfUtils<ImageType>::composeDeformations(d2,d1));

    }


    inline bool getLinearNeighbors(const DeformationFieldPointerType def, const PointType & point, std::vector<std::pair<IndexType,double> > & neighbors){
        bool inside=true;
        neighbors= std::vector<std::pair<IndexType,double> >(pow(2,D));
        int nNeighbors=0;
        IndexType idx1;
        inside = def->TransformPhysicalPointToIndex(point,idx1);
  
        LOGV(8)<<VAR(point)<<" "<<VAR(idx1)<<" "<<VAR(inside)<<" "<<VAR(def->GetOrigin())<<endl;
        
        if (!inside) return false;
        PointType pt1;
        def->TransformIndexToPhysicalPoint(idx1,pt1);
        DeformationType dist=point-pt1;
        if (inside){
            neighbors[nNeighbors++]=std::make_pair(idx1,getWeight(dist,def->GetSpacing()));
        }
        OffsetType off;
        off.Fill(0);
        double sum=0.0;
        for (int i=1;i<pow(2,D);++i){
            int spill=1;
            for (int d=0;d<D;++d){
                off[d]+=spill*sign(dist[d]);
                if (fabs(off[d])>1){
                    spill=1;off[d]=0;
                }else{
                    break;
                }

            }
            IndexType idx=idx1+off;
            PointType pt;
            def->TransformIndexToPhysicalPoint(idx,pt);
            DeformationType delta=point-pt;
            if (def->GetLargestPossibleRegion().IsInside(idx)){
                //LOGV(1)<<VAR(def->GetLargestPossibleRegion().GetSize())<<" "<<VAR(idx)<<endl;
                double w=getWeight(delta,def->GetSpacing());
                sum+=w;
                neighbors[nNeighbors++]=std::make_pair(idx,w);
                inside=true;
            }
        }
        neighbors.resize(nNeighbors);
        return inside;
    } 
    inline bool getLinearNeighbors(const ImagePointerType img, const PointType & point, std::vector<std::pair<IndexType,double> > & neighbors){
        bool inside=true;
        neighbors= std::vector<std::pair<IndexType,double> >(pow(2,D));
        int nNeighbors=0;
        IndexType idx1;
        inside = img->TransformPhysicalPointToIndex(point,idx1);
  
        LOGV(8)<<VAR(point)<<" "<<VAR(idx1)<<" "<<VAR(inside)<<" "<<VAR(img->GetOrigin())<<endl;
        
        if (!inside) return false;
        PointType pt1;
        img->TransformIndexToPhysicalPoint(idx1,pt1);
        DeformationType dist=point-pt1;
        if (inside){
            neighbors[nNeighbors++]=std::make_pair(idx1,getWeight(dist,img->GetSpacing()));
        }
        OffsetType off;
        off.Fill(0);
        double sum=0.0;
        for (int i=1;i<pow(2,D);++i){
            int spill=1;
            for (int d=0;d<D;++d){
                off[d]+=spill*sign(dist[d]);
                if (fabs(off[d])>1){
                    spill=1;off[d]=0;
                }else{
                    break;
                }

            }
            IndexType idx=idx1+off;
            PointType pt;
            img->TransformIndexToPhysicalPoint(idx,pt);
            DeformationType delta=point-pt;
            if (img->GetLargestPossibleRegion().IsInside(idx)){
                //LOGV(1)<<VAR(def->GetLargestPossibleRegion().GetSize())<<" "<<VAR(idx)<<endl;
                double w=getWeight(delta,img->GetSpacing());
                if (w>0.001){
                    sum+=w;
                    neighbors[nNeighbors++]=std::make_pair(idx,w);
                    inside=true;
                }
            }
        }
        neighbors.resize(nNeighbors);
        return inside;
    }

    inline bool getGaussianNeighbors(const DeformationFieldPointerType def, const PointType & point, std::vector<std::pair<IndexType,double> > & neighbors, double sigma){
        bool inside=true;
        int nNeighbors=0;
        IndexType idx1;
        int pixelSigma=max(1.0,sigma/def->GetSpacing()[0]+0.5);
        sigma=max(0.5,sigma);
        neighbors= std::vector<std::pair<IndexType,double> >(pow(pixelSigma*2,D));

        inside = def->TransformPhysicalPointToIndex(point,idx1);

        LOGV(8)<<VAR(point)<<" "<<VAR(idx1)<<" "<<VAR(inside)<<" "<<VAR(def->GetOrigin())<<" "<<VAR(sigma)<<" "<<VAR(pixelSigma)<<endl;
        
        if (!inside) return false;
        PointType pt1;
        def->TransformIndexToPhysicalPoint(idx1,pt1);
        //get bottom left index
        typename ImageUtils<ImageType>::ContinuousIndexType cIdx;
        def->TransformPhysicalPointToContinuousIndex(pt1,cIdx);
        for (int d=0;d<D;++d)
            idx1[d]=int(cIdx[d]);

        OffsetType off;
        off.Fill(-pixelSigma+1);
        double sum=0.0;
        for (int i=0;i<pow(2*pixelSigma,D);++i){
            if (i>0){
                for (int d=0;d<D;++d){
                    off[d]++;
                    if (off[d]>pixelSigma){
                        off[d]=-pixelSigma+1;
                    }else{
                        break;
                    }
                }
            }
            IndexType idx=idx1+off;
            if (def->GetLargestPossibleRegion().IsInside(idx)){
                PointType pt;
                def->TransformIndexToPhysicalPoint(idx,pt);
                DeformationType delta=point-pt;
                double w=exp(- delta.GetSquaredNorm()/(sigma*sigma));
                LOGV(8)<<VAR(nNeighbors)<<" "<<VAR(idx)<<" "<<VAR(off)<<" "<<VAR(w)<<" "<<VAR(point)<<" "<<VAR(pt)<<" "<<VAR(delta.GetSquaredNorm())<<endl;
                sum+=w;
                neighbors[nNeighbors++]=std::make_pair(idx,w);
                inside=true;
            }
        }
        neighbors.resize(nNeighbors);
        for (int i=0;i<neighbors.size();++i){
            neighbors[i].second/=sum;
        }
        return inside;
    }

    inline bool getNearestNeighbors(const DeformationFieldPointerType def, const PointType & point, std::vector<std::pair<IndexType,double> > & neighbors){
        bool inside=false;
        neighbors= std::vector<std::pair<IndexType,double> >(1);
        int nNeighbors=0;
        IndexType idx1;
        def->TransformPhysicalPointToIndex(point,idx1);
        inside=inside || def->GetLargestPossibleRegion().IsInside(idx1);
        PointType pt1;
        def->TransformIndexToPhysicalPoint(idx1,pt1);
        DeformationType dist=point-pt1;
        if (inside){
            neighbors[nNeighbors++]=std::make_pair(idx1,1.0);
        }
       
        return inside;
    }

    inline double getWeight(const DeformationType & dist, const SpacingType & space){
        double w=1.0;
        for (int d=0;d<D;++d){
            w*=(1-fabs(dist[d])/space[d]);
        }
        LOGV(9)<<VAR(dist)<<" "<<VAR(space)<<" "<<VAR(w)<<endl;
        return w;
    }
    inline int sign(const double s){

        if (s>=0) return 1;
        if (s<0) return -1;
        return 0;
    }

    void computeTripletEnergies(double * x, double * y, double * v, double * b, long int &c,long int & eq, unsigned int d){
        double maxAbsDisplacement=0.0;
        //0=min,1=mean,2=max,3=median,-1=off,4=gauss;
        int accumulate=4;
        double manualResidual=0.0;
        for (int s = 0;s<m_numImages;++s){                            
            int source=s;
            string sourceID=(m_imageIDList)[source];
        
            for (int t=0;t<m_numImages;++t){
                if (t!=s){
                    int target=t;
                    string targetID=(m_imageIDList)[target];
                    DeformationFieldPointerType dSourceTarget;
                    bool estSourceTarget=false;
                    bool skip=false;
                    if ((m_downSampledDeformationCache)[sourceID][targetID].IsNotNull()){
                        dSourceTarget=(m_downSampledDeformationCache)[sourceID][targetID];
                        estSourceTarget=true;
                    }else{
                        if ((m_trueDeformations)[sourceID][targetID].IsNotNull())
                            dSourceTarget=(m_trueDeformations)[sourceID][targetID];
                        else
                            skip=true;
                    }
                    
                   
                    
                    //triplet energies
                    for (int i=0;i<m_numImages;++i){ 
                        if (t!=i && i!=s){
                            //define a set of 3 images
                            int intermediate=i;
                            string intermediateID=(m_imageIDList)[i];
                            DeformationFieldPointerType dSourceIntermediate;
                            
                            bool estSourceIntermediate=false;
                            if ((m_downSampledDeformationCache)[sourceID][intermediateID].IsNotNull()){
                                dSourceIntermediate=(m_downSampledDeformationCache)[sourceID][intermediateID];
                                estSourceIntermediate=true;
                            }else{
                                if ((m_trueDeformations)[sourceID][intermediateID].IsNotNull())
                                    dSourceIntermediate=(m_trueDeformations)[sourceID][intermediateID];
                                else
                                    skip=true;
                            }

                            DeformationFieldPointerType dIntermediateTarget;
                            
                            bool estIntermediateTarget=false;
                            if ((m_downSampledDeformationCache)[intermediateID][targetID].IsNotNull()){
                                dIntermediateTarget=(m_downSampledDeformationCache)[intermediateID][targetID];
                                estIntermediateTarget=true;
                            }else{
                                if ((m_trueDeformations)[intermediateID][targetID].IsNotNull())
                                    dIntermediateTarget=(m_trueDeformations)[intermediateID][targetID];
                                else
                                    skip = true;
                            }
                            

                            //skip also if none of the registrations in the circle are to be re-estimated
                            skip= skip || (!(estIntermediateTarget || estSourceTarget || estSourceIntermediate));
                            LOGV(4)<<VAR(skip)<<" "<<VAR(estIntermediateTarget)<<" "<<VAR(estSourceTarget)<<" "<<VAR(estSourceIntermediate)<<" "<<VAR(sourceID)<<" "<<VAR(targetID)<<" "<<VAR(intermediateID)<<endl;
                            if ( ! skip ){

                                //use updated deform for constructing circle
                                if (false && m_ORACLE && m_ORACLE<4){// && m_trueDeformations[intermediateID][targetID].IsNotNull()){
                                    LOGV(3)<<"ORACLE! "<<VAR(intermediateID)<<" " <<VAR(targetID)<<endl;
                                    if (findDeformation(m_trueDeformationFileList,intermediateID,targetID)){
                                        dIntermediateTarget =ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[intermediateID][targetID]);
                                        dIntermediateTarget= TransfUtils<ImageType>::linearInterpolateDeformationField(dIntermediateTarget,this->m_grid,false);
                                    }else{
                                        LOGV(3)<<"ORACLE FAILED"<<VAR(intermediateID)<<" " <<VAR(targetID)<<" MISSING"<<endl;
                                        exit(-1);
                                    }
                                }else if (estIntermediateTarget && m_haveDeformationEstimate && (m_updatedDeformationCache)[intermediateID][targetID].IsNotNull()){
                                    dIntermediateTarget=(m_updatedDeformationCache)[intermediateID][targetID];
                                }
                                

                                //TODO: improve approx using gradients?
                                //compute gradients of 2nd transf
                                FloatImagePointerType gradSourceIntermed;
                                typename FilterUtils<FloatImageType>::LinearInterpolatorPointerType gradientInterpol;
                                if (m_useTaylor){
                                    
                                    if ( m_ORACLE && m_ORACLE<4){// && m_trueDeformations[intermediateID][targetID].IsNotNull()){
                                        gradSourceIntermed=TransfUtils<ImageType,float,double,double>::computeDirectedGradient(
                                                                                                                               TransfUtils<ImageType>::linearInterpolateDeformationField(
                                                                                                                                                                                         ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[sourceID][intermediateID])
                                                                                                                                                                                         ,this->m_grid,false)
                                                                                                                               ,d);
                                    }else{
                                        gradSourceIntermed=TransfUtils<ImageType,float,double,double>::computeDirectedGradient(dSourceIntermediate,d);
                                    }
                                    gradientInterpol= FilterUtils<FloatImageType>::LinearInterpolatorType::New();
                                    gradientInterpol->SetInputImage(gradSourceIntermed);
                                }

                                //compute norm
                                //DeformationFieldIterator it(dSourceTarget,dSourceTarget->GetLargestPossibleRegion());
                                ImageIterator it(m_grid,m_grid->GetLargestPossibleRegion());
                                it.GoToBegin();
                            
                              
                                bool haveMasks=(m_maskList && m_maskList->find(targetID)!=m_maskList->end()) && (m_maskList->find(intermediateID)!=m_maskList->end());
                                
                             
                                SizeType roiSize=m_regionOfInterest.GetSize();
                            
                                // LOGV(1)<<VAR(dir)<<" "<<VAR(start)<<endl;
                                for (;!it.IsAtEnd();++it){

                                    bool valid=true;

                                    //get index in target domain
                                    IndexType gridIndex=it.GetIndex(),intermediateIndex,idx1,targetIndex;
                                    LOGV(9)<<VAR(gridIndex)<<endl;
                                    PointType ptIntermediate,ptTarget,ptSourceDirect;
                                    IndexType roiIntermediateIndex,roiTargetIndex;
                                
                                    //get physical point in target domain
                                    m_grid->TransformIndexToPhysicalPoint(gridIndex,ptTarget);
                                    dSourceTarget->TransformPhysicalPointToIndex(ptTarget,targetIndex);
                                    //get corresponding point in intermediate deform
                                    DeformationType dIntermediate=dIntermediateTarget->GetPixel(targetIndex);
                                    ptIntermediate= ptTarget + dIntermediate;
                                
                                    //get point in source image where direct deformation points to
                                    ptSourceDirect = ptTarget + dSourceTarget->GetPixel(targetIndex);
                                
                                    //get neighbors of that point
                                    std::vector<std::pair<IndexType,double> >ptIntermediateNeighbors,ptIntermediateNeighborsCircle;
                                    
                                    
                                    bool inside=true;

                                    if (haveMasks){
                                        inside=m_maskList->find(targetID)->second->GetPixel(targetIndex);
                                    }
                                    if (m_linearInterpol){
                                        //inside=inside && getLinearNeighbors(dIntermediateTarget,ptIntermediate,ptIntermediateNeighbors);
                                        inside=inside && getLinearNeighbors(m_grid,ptIntermediate,ptIntermediateNeighbors);
                                        //inside=getGaussianNeighbors(dIntermediateTarget,ptIntermediate,ptIntermediateNeighbors,(ptIntermediate-ptSourceDirect).GetNorm());
                                    }else{
                                        inside=inside && getNearestNeighbors(dIntermediateTarget,ptIntermediate,ptIntermediateNeighbors);
                                    }

 
                                        
                                    this->m_ROI->TransformPhysicalPointToIndex(ptTarget,roiTargetIndex);
                                    LOGV(9)<<VAR(targetIndex)<<" "<<VAR(roiTargetIndex)<<endl;
                                
                                    if (inside){

                                        //if there are atlas segmentations, we're only forming circles at atlas segmentation points
                                        //and at all points otherwise
                                        double val= m_atlasSegmentations.size()==0;
                                        
                                        //val*=getIndexBasedWeight(roiTargetIndex,roiSize);

                                      

                                        std::vector<bool> insideIntermediate(ptIntermediateNeighbors.size(),true);
                                        if (haveMasks){
                                            bool insideInterm=false;
                                            for (int i=0;i<ptIntermediateNeighbors.size();++i){
                                                insideIntermediate[i]= m_maskList->find(intermediateID)->second->GetPixel(ptIntermediateNeighbors[i].first);
                                                insideInterm=insideInterm ||insideIntermediate[i];
                                                if (!insideInterm){
                                                    val=-1;
                                                }
                                            
                                            }
                                        }
                                 
                                        if (val>0){
                                            //statisticIt.Set(statisticIt.Get()+(disp));


                                            LOGV(9)<<VAR(source)<<" "<<VAR(intermediate)<<" "<<VAR(target)<<" "<<VAR(roiTargetIndex)<<" "<<VAR(d)<<endl;
                                            LOGV(9)<<VAR(edgeNumError(intermediate,target,roiTargetIndex,d))<<" "<<VAR(edgeNumDeformation(intermediate,target,roiTargetIndex,d))<<endl;
                                       
                                            //set w_circ
                                            if (m_wCircleNorm>0.0){
                                                //val/=m_Inconsistency;
                                                double RHS=0.0;
                                                if (estIntermediateTarget){
                                                    double taylorWeight=0.0;
                                                    if (m_useTaylor){
                                                        taylorWeight=gradientInterpol->Evaluate(ptIntermediate);
                                                        RHS+=taylorWeight*dIntermediateTarget->GetPixel(gridIndex)[d];
                                                        //val*=1.0/(1.0+100*taylorWeight*taylorWeight);
                                                        //LOGV(3)<<VAR(taylorWeight)<<" "<<VAR(val)<<endl;
                                                    }
                                                    //indirect
                                                    x[c]=eq;
                                                    y[c]=edgeNumDeformation(intermediate,target,gridIndex,d);
                                                    v[c]=val* m_wCircleNorm*(1.0+taylorWeight);
                                            
                                                    ++c;
                                                }else{
                                                    RHS-=dIntermediateTarget->GetPixel(gridIndex)[d];
                                                }
                                               
                                        
                                                double defSum=0.0;
                                                for (int i=0;i<ptIntermediateNeighbors.size();++i){
                                                    if (estSourceIntermediate    && insideIntermediate[i]){
                                                        x[c]=eq;
                                                        y[c]=edgeNumDeformation(source,intermediate,ptIntermediateNeighbors[i].first,d); // this is an APPROXIMIATION!!! might be bad :o
                                                        v[c]=ptIntermediateNeighbors[i].second*val* m_wCircleNorm;
                                                        ++c;
                                                        LOGV(8)<<VAR(roiTargetIndex)<<" "<<VAR(i)<<" "<<VAR(ptIntermediateNeighbors[i].first)<<" "<<VAR(ptIntermediateNeighbors[i].second)<<endl;
                                                    }else{
                                                        //RHS-=ptIntermediateNeighbors[i].second*dSourceIntermediate->GetPixel(ptIntermediateNeighbors[i].first)[d];
                                                        LOG<<"NYI"<<endl;
                                                        exit(0);
                                                    }
                                                }

                                                if (estSourceTarget){
                                                    //minus direct
                                                    x[c]=eq;
                                                    y[c]=edgeNumDeformation(source,target,gridIndex,d);
                                                    v[c]= - val* m_wCircleNorm;
                                                    ++c;
                                                }else{
                                                    RHS+=dSourceTarget->GetPixel(gridIndex)[d];
                                                }
                                        
                                            
                                                b[eq-1]=val*m_wCircleNorm*RHS;
                                                ++eq;
                                            }
                                    
                                       
                                        }//val >0
                                    }//inside

                                }//image iterator

                            }//if anything is to estimate :)
                        }//if
                        
                    }//intermediate
                }//if
            }//target
        }//source
        LOGV(1)<<VAR(manualResidual)<<endl;
    }//compute triplets

    void computePairwiseEnergiesAndBounds(double * x, 
                                          double * y,
                                          double * v, 
                                          double * b, 
                                          double * init, 
                                          double * lb, 
                                          double * ub, 
                                          long int & c , 
                                          long int & eq , 
                                          unsigned int  d)
    {
        double dblSpacing=this->m_ROI->GetSpacing()[d];
        
        for (int s = 0;s<m_numImages;++s){                            
            int source=s;
            for (int t=0;t<m_numImages;++t){
                if (t!=s){
                    int target=t;
                    //pairwise energies!
                    string sourceID=(this->m_imageIDList)[source];
                    string targetID = (this->m_imageIDList)[target];
                  
                    //only compute pairwise energies if deformation is to be estimated
                    if ((m_downSampledDeformationCache)[sourceID][targetID].IsNotNull()){
                      
                        FloatImagePointerType lncc;
                        FloatImageIterator lnccIt;
                        if (  m_sigma>0.0 && (m_wErrorNorm>0.0 || m_wTransformationSimilarity)){
                            lncc=(m_pairwiseLocalWeightMaps)[sourceID][targetID];
                            lnccIt=FloatImageIterator(lncc,lncc->GetLargestPossibleRegion());
                            lnccIt.GoToBegin();
                        }
                        DeformationFieldPointerType trueDef;

#if 0

                        if (findDeformation(m_trueDeformationFileList,sourceID,targetID)){
                            LOGV(3)<<"Evaluating true def! "<<VAR(sourceID)<<" " <<VAR(targetID)<<endl;
                            trueDef =ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[sourceID][targetID]);
                            trueDef=TransfUtils<ImageType>::linearInterpolateDeformationField(trueDef,this->m_ROI,false);
                        }
#endif                   
                        DeformationFieldPointerType dSourceTarget=(this->m_downSampledDeformationCache)[sourceID][targetID];
                        DeformationFieldIterator it(dSourceTarget,m_regionOfInterest);
                        it.GoToBegin();
                  
                        LOGV(1)<<VAR(dSourceTarget->GetSpacing())<<" "<<m_grid->GetSpacing()<<endl;
                        
                        FloatImagePointerType newLocalWeights;
                        if (m_locallyUpdateDeformationEstimate && ! m_updateDeformations && m_haveDeformationEstimate && (m_updatedDeformationCache)[sourceID][targetID].IsNotNull()){
                            newLocalWeights=(m_updatedPairwiseLocalWeightMaps)[sourceID][targetID];
                        }

                   


                        //iterate over ROI to create similarity based penalties
                        for (;!it.IsAtEnd();++it){
                            DeformationType localDef=it.Get();
                            IndexType idx=it.GetIndex();
                            LOGV(8)<<VAR(eq)<<" "<<VAR(localDef)<<endl;
                            if (m_maskList && m_maskList->find(targetID)!=m_maskList->end()){
                                if (!(*m_maskList)[targetID]->GetPixel(idx)){
                                    //don't add pairwise adherence for any pixel which is not inside the mask, if such mask exists
                                    continue;
                                }
                            }

                          

                            //intensity based weight
                            double weight=1.0;
                            if (lncc.IsNotNull()){
                                weight = max(0.00001,lnccIt.Get());
                                //weight= weight>0.95?1000:weight;
                                double err=0;
                                if (trueDef.IsNotNull()){
                                    err=trueDef->GetPixel(idx).GetNorm();
                                }
                                LOGV(4)<<VAR(weight)<<" "<<VAR(err)<<endl;
                                ++lnccIt;
                            }
                            //weight*=pow(abs( (m_pairwiseGlobalSimilarity[sourceID][targetID]-m_minSim)/(m_maxSim-m_minSim)),1.0);
                            //weight=pow(abs( (m_pairwiseGlobalSimilarity[sourceID][targetID])),m_sigma);
                            
                    
                        
                            double localWeight=-1;
                            double localUpdatedDef;
                            //get local similarity weight for updated deformation if available
                            if (m_locallyUpdateDeformationEstimate  &&  m_haveDeformationEstimate && ! m_updateDeformations){
                                localWeight=newLocalWeights->GetPixel(idx);
                                localUpdatedDef=(m_updatedDeformationCache)[sourceID][targetID]->GetPixel(idx)[d];
                            }
                        
                            //set w_T
                            //set eqn for soft constraining the estimated true deformation to be similar to the original deformation
                            if (m_wTransformationSimilarity>0.0){
                                if (m_metric=="gradient"){
                                    map<string , map<string,FloatImagePointerType> > &cache=m_pairwiseMetricDerivatives[d];
                                    map<string, FloatImagePointerType> & mmap1=cache[sourceID];
                                    FloatImagePointerType metricGradient=mmap1[targetID];
                                    double localGradient=metricGradient->GetPixel(idx);
                                    int edgeNumDef=edgeNumDeformation(source,target,idx,d);
                                    x[c]    = eq;
                                    y[c]    = edgeNumDef;
                                    //v[c++]  = 1.0*m_wTransformationSimilarity* localGradient;
                                    //b[eq-1] = (weight+localGradient-weight+localDef[d]*localGradient)*m_wTransformationSimilarity ;
#if 0
#define CONTINUOUSDERIVATIVE
                                    v[c++]  = 1.0*m_wTransformationSimilarity*(weight+abs(localGradient));
                                    b[eq-1] = (localGradient+localDef[d])*(weight+abs(localGradient))*m_wTransformationSimilarity ;

                                                                        
#else
                                    v[c++]  = 1.0*m_wTransformationSimilarity;
                                    double update=0.0;//localGradient;
#define CONTINUOUSDERIVATIVE
#ifdef CONTINUOUSDERIVATIVE
                                    
                                    double magnitude=this->m_ROI->GetSpacing()[d];
                                    if (true && localGradient!=0){
                                        //update=(weight+fabs(localGradient))/localGradient;
                                        //update=(1.0-weight)/localGradient;
                                        //update=localGradient>0?magnitude:-magnitude;
                                        update=localGradient*magnitude;
                                        if (update>magnitude) update=magnitude;
                                        if ( update<-magnitude) update=-magnitude;
                                    }
#else
                                    update=localGradient;
#endif
                                    b[eq-1] = (update+localDef[d])*m_wTransformationSimilarity ;
#endif
                                    //v[c++]  = 1.0*m_wTransformationSimilarity;
                                    //b[eq-1] = (localGradient+localDef[d])*m_wTransformationSimilarity ;
                                    ++eq;
                                    weight=0.0;

                                }
                                //weight=1.0/sqrt(fabs(meanInconsistency));
                                double localDisp  =localDef[d];
                                if (localWeight>=weight){
                                    localDisp=localUpdatedDef;
                                    weight=localWeight;
                                }
                                //weight/=(1.0+m_averageNCC);
                                PointType targetPoint;
                                dSourceTarget->TransformIndexToPhysicalPoint(idx,targetPoint);
                                std::vector<std::pair<IndexType,double> >ptNeighbors;
                                getLinearNeighbors(m_grid,targetPoint,ptNeighbors);
			
                                //if (weight>0.0){
                                for (int i=0;i<ptNeighbors.size();++i){
				    
                                    int edgeNumDef=edgeNumDeformation(source,target,ptNeighbors[i].first,d);
                                    //int edgeNumDef=edgeNumDeformation(source,target,idx,d);
                                    // LOGV(3)<<VAR(eq)<<" "<<VAR(c)<<" "<<VAR(idx)<<" "<<VAR(i)<<" "<<VAR(ptNeighbors[i].first)<<" "<<VAR(ptNeighbors[i].second)<<endl;
                                    x[c]    = eq;
                                    y[c]    = edgeNumDef;
                                    v[c++]  = 1.0*m_wTransformationSimilarity *weight*ptNeighbors[i].second;
                                }
                                b[eq-1] = localDisp*m_wTransformationSimilarity * weight;
                                ++eq;
                                LOGV(8)<<VAR(source)<<" "<<VAR(target)<<" "<<VAR(idx)<<" "<<VAR(d)<<endl;
                            }
                            //}
                            
                        }//iter ROI
                          

                        if (m_ORACLE>1){
                            LOGV(3)<<"ORACLE! "<<VAR(sourceID)<<" " <<VAR(targetID)<<endl;
                            if (findDeformation(m_trueDeformationFileList,sourceID,targetID)){
                                dSourceTarget =ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[sourceID][targetID]);
                                dSourceTarget= TransfUtils<ImageType>::linearInterpolateDeformationField(dSourceTarget,this->m_ROI,false);
                            }else{
                                LOGV(3)<<"ORACLE FAILED"<<VAR(sourceID)<<" " <<VAR(targetID)<<" MISSING"<<endl;
                            }
                        }
                        ImageIterator it2(m_grid,m_grid->GetLargestPossibleRegion());
                        it2.GoToBegin();
						
                        //iterate over grid to generate coarse constraints.
                        for (;!it2.IsAtEnd();++it2){
                            IndexType idx=it2.GetIndex();
                            int edgeNumDef=edgeNumDeformation(source,target,idx,d);
                            IndexType targetIndex;
                            PointType targetPoint;
                            m_grid->TransformIndexToPhysicalPoint(idx,targetPoint);
                            dSourceTarget->TransformPhysicalPointToIndex(targetPoint,targetIndex);
                            //TODO: should be interpolated?
                            DeformationType localDef;
                            if (! (m_ORACLE>1) && m_locallyUpdateDeformationEstimate  &&  m_haveDeformationEstimate && ! m_updateDeformations){
                                localDef=(m_updatedDeformationCache)[sourceID][targetID]->GetPixel(idx);
                            }else{
                                localDef=dSourceTarget->GetPixel(targetIndex);
                            }
			  
#define CURVATURE
#ifdef CURVATURE
                            //spatial smootheness of estimated deformations
                            if (m_wDeformationSmootheness>0.0){
                                for (unsigned int n=0;n<D;++n){
                                    OffsetType off,off2;
                                    off.Fill(0);
                                    off2=off;
                                    off[n]=1;
                                    off2[n]=-1;
                                    double smoothenessWeight =this->m_wDeformationSmootheness*m_spacingBasedSmoothnessReduction;
                                    if (n!=d){
                                        //smoothenss for shearing is different
                                        smoothenessWeight*=m_shearingReduction;
                                    }
                                    IndexType neighborIndexRight=idx+off;
                                    IndexType neighborIndexLeft=idx+off2;
                                    if (dSourceTarget->GetLargestPossibleRegion().IsInside(neighborIndexRight) &&dSourceTarget->GetLargestPossibleRegion().IsInside(neighborIndexLeft) ){
                                        x[c]=eq;
                                        y[c]=edgeNumDef;
                                        v[c++]=-2.0*smoothenessWeight;
                                        x[c]=eq;
                                        y[c]=edgeNumDeformation(source,target,neighborIndexRight,d);
                                        v[c++]=smoothenessWeight;
                                        x[c]=eq;
                                        y[c]=edgeNumDeformation(source,target,neighborIndexLeft,d);
                                        v[c++]=smoothenessWeight;
                                        b[eq-1]=0.0;
                                        ++eq;
                                    }
                                }//inside
                            }

#else

                            //spatial smootheness of estimated deformations
                            if (m_wDeformationSmootheness>0.0){
                                for (unsigned int n=0;n<D;++n){
                                    OffsetType off,off2;
                                    off.Fill(0);
                                    off2=off;
                                    off[n]=1;
                                    off2[n]=-1;
                                    double smoothenessWeight =this->m_wDeformationSmootheness*m_spacingBasedSmoothnessReduction;
                                    if (n!=d){
                                        //smoothenss for shearing is different
                                        smoothenessWeight*=m_shearingReduction;
                                    }
                                    IndexType neighborIndexRight=idx+off;
                                    if (dSourceTarget->GetLargestPossibleRegion().IsInside(neighborIndexRight)){
                                        x[c]=eq;
                                        y[c]=edgeNumDef;
                                        v[c++]=-1.0*smoothenessWeight;
                                        x[c]=eq;
                                        y[c]=edgeNumDeformation(source,target,neighborIndexRight,d);
                                        v[c++]=smoothenessWeight;
                                        b[eq-1]=0.0;
                                        ++eq;
                                    }
                                }//inside
                            }

#endif
                          
                           
                          
                            if (m_estDef){
                                lb[edgeNumDef-1]=localDef[d] -dblSpacing;
                                ub[edgeNumDef-1]=localDef[d] +dblSpacing;
                                init[edgeNumDef-1] =  localDef[d];
                            }
                           
                        }//for

                    }//if
                }//check if estimation is necessary
            }//target
        }//source
    }//computePairwiseEnergiesAndBounds



    void computeConstraints(double * x, 
                            double * y,
                            double * v, 
                            double * b, 
                            int d
                            )
    {

        int c=0;
        int eq=1;
        double dblSpacing=this->m_ROI->GetSpacing()[d];
        for (int s = 0;s<m_numImages;++s){                            
            int source=s;
            for (int t=0;t<m_numImages;++t){
                if (t!=s){
                    int target=t;
                    //pairwise energies!
                    string sourceID=(this->m_imageIDList)[source];
                    string targetID = (this->m_imageIDList)[target];
                  
                    //only compute pairwise energies if deformation is to be estimated
                    if ((m_downSampledDeformationCache)[sourceID][targetID].IsNotNull()){
                        DeformationFieldPointerType dSourceTarget=(this->m_downSampledDeformationCache)[sourceID][targetID];
                        DeformationFieldIterator it(dSourceTarget,m_regionOfInterest);
                        it.GoToBegin();
                        for (;!it.IsAtEnd();++it){
                            DeformationType localDef=it.Get();
                            IndexType idx=it.GetIndex();
                            LOGV(8)<<VAR(eq)<<" "<<VAR(localDef)<<endl;
                            int edgeNumDef=edgeNumDeformation(source,target,idx,d);

                            //constraints
                            for (unsigned int n=0;n<D;++n){
                                OffsetType off,off2;
                                off.Fill(0);
                                off2=off;
                                off[n]=2;
                                off2[n]=-1;

                                //only constraint in-line neighbors to prevent folding.
                                // d1-d2<spacing
                                IndexType neighborIndexRight=idx+off;
                                if (dSourceTarget->GetLargestPossibleRegion().IsInside(neighborIndexRight)){
                                    if (n == d ){
                                        x[c]=eq;
                                        y[c]=edgeNumDef;
                                        v[c++]=1.0;
                                        x[c]=eq;
                                        y[c]=edgeNumDeformation(source,target,neighborIndexRight,d);
                                        v[c++]=-1.0;
                                        b[eq-1]=1.8*dblSpacing;
                                        ++eq;
                                    }//n==d
#if 1
                                    else{
                                        x[c]=eq;
                                        y[c]=edgeNumDef;
                                        v[c++]=1.0;
                                        x[c]=eq;
                                        y[c]=edgeNumDeformation(source,target,neighborIndexRight,d);
                                        v[c++]=-1.0;
                                        b[eq-1]=dblSpacing*1.1;
                                        ++eq;
                                        
                                        x[c]=eq;
                                        y[c]=edgeNumDef;
                                        v[c++]=-1.0;
                                        x[c]=eq;
                                        y[c]=edgeNumDeformation(source,target,neighborIndexRight,d);
                                        v[c++]=1.0;
                                        b[eq-1]=dblSpacing*1.1;
                                        ++eq;
                                    }
#endif
                                }//inside
                            }//neighbors
                        }//for
                    }//if
                }//check if estimation is necessary
            }//target
        }//source
    }//computeConstraints

public:
  
    double getIndexBasedWeight(IndexType idx,SizeType size){
        double weight=100.0;
        for (int d=0;d<D;++d){
            //first center
            double halfSize=0.5*size[d];
            double localWeight=fabs(1.0*idx[d]-halfSize);
            double distance=localWeight-halfSize;
            if (distance==0.0){
                localWeight=0.0;
            }else{
                //compute falloff
                localWeight=max(0.0,1.0+1.0/(distance));
            }
            //take max
            weight=min(weight,localWeight);
            LOGV(8)<<VAR(idx[d])<<" "<<VAR(size[d])<<" "<<VAR(localWeight)<<endl;

        }
        return weight;
    }

 

    bool findDeformation( map<string, map<string, string> > cache, string id1, string id2){
        
        typename map<string, map<string, string> >::iterator it;
        
        
        it=cache.find(id1);
        
        if (it !=cache.end()){
            typename map<string, string>::iterator it2;
            it2=it->second.find(id2);
            if (it2!=it->second.end()){
                return true;
            }
        }
        return false;
    }

    void doubleImageResolution(){
        m_resolutionFactor*=2;
        m_resolutionFactor=max(1.0,m_resolutionFactor);
    }

   
   


    void DoALot(string directory=""){
        typedef typename itk::LabelOverlapMeasuresImageFilter<ImageType> OverlapMeasureFilterType;
        m_spacingBasedSmoothnessReduction=1.0/this->m_grid->GetSpacing()[0];
        //initialize ADE with zero if it is going to be computed.
        m_ADE=(m_trueDeformationFileList.size()>0)?0:-1;
        m_dice=0;
        m_TRE=0;
        double m_averageMinJac=0.0;
	m_averageJacSTD=0.0;
        m_averageNCC=0.0;
        m_minMinJacobian=std::numeric_limits<double>::max();
        int count=0, treCount=0;
        bool firstRun=false;
        bool computeDownsampledMetrics=m_lowResSimilarity;
        for (int target=0;target<m_numImages;++target){
            for (int source=0;source<m_numImages;++source){
                if (source!=target){
                    string targetID=(m_imageIDList)[target];
                    string sourceID=(m_imageIDList)[source];
                    DeformationFieldPointerType deformation,updatedDeform, knownDeformation;
                    bool estDef=false;
                    bool updateThisDeformation=false;
                    ImagePointerType targetImage=(m_imageList)[targetID];
                    ImagePointerType sourceImage=(m_imageList)[sourceID];
                    if (computeDownsampledMetrics){
                        targetImage=FilterUtils<ImageType>::LinearResample(targetImage,this->m_ROI,true);
                        sourceImage=FilterUtils<ImageType>::LinearResample(sourceImage,this->m_ROI,true);
                    }
                    double nCC=-5;;
                    double dice=-1,tre=-1;
                    ImagePointerType warpedImage;
                    FloatImagePointerType lncc;

                    if (findDeformation(m_deformationFileList,sourceID,targetID)){
                      

                        LOGV(5)<<"preparing deformation to be estimated for "<<sourceID<<" to " <<targetID <<endl;
                        estDef=true;
                        if (m_estimatedDeformations[sourceID][targetID].IsNotNull()){
                            if (m_lowResolutionEval){
                                DeformationFieldPointerType estimatedDeformation=m_estimatedDeformations[sourceID][targetID];
                                if (m_bSplineInterpol){
                                    updatedDeform=TransfUtils<ImageType>::bSplineInterpolateDeformationField(estimatedDeformation,targetImage,false);
                                    //updatedDeform=TransfUtils<ImageType>::computeDeformationFieldFromBSplineTransform(estimatedDeformation,targetImage);
                                }else{
                                    updatedDeform=TransfUtils<ImageType>::linearInterpolateDeformationField(estimatedDeformation,targetImage,false);
                                }
                                
                            }else{
                                //read full resolution deformations
                                deformation=ImageUtils<DeformationFieldType>::readImage(m_deformationFileList[sourceID][targetID]);
                                //resample deformation to target image space
                                deformation=TransfUtils<ImageType>::linearInterpolateDeformationField(deformation,targetImage,m_smoothDeformationDownsampling);
                                //update deformation
                                DeformationFieldPointerType estimatedDeformation=m_estimatedDeformations[sourceID][targetID];
#if 0
#else
                               
                                //downsample original deformation with smoothing
                                DeformationFieldPointerType downSampledDeformation;
                                if (m_bSplineInterpol){
                                    //downSampledDeformation=TransfUtils<ImageType>::computeDeformationFieldFromBSplineTransform(deformation,this->m_ROI);
                                    downSampledDeformation=TransfUtils<ImageType>::bSplineInterpolateDeformationField(deformation,m_estimatedDeformations[sourceID][targetID],m_smoothDeformationDownsampling);
                                }else{
                                    downSampledDeformation=TransfUtils<ImageType>::linearInterpolateDeformationField(deformation,estimatedDeformation,m_smoothDeformationDownsampling);
                                }
                                //subtract low resolution error estimate (sampled to the correct resolution in case it was estimated for a lower resolution)
                                DeformationFieldPointerType estimatedError=TransfUtils<ImageType>::subtract(downSampledDeformation,estimatedDeformation);
                                
                                
                                if (m_bSplineInterpol){
                                    estimatedError=TransfUtils<ImageType>::bSplineInterpolateDeformationField(estimatedError,targetImage,m_smoothDeformationDownsampling);
                                    //fullResolutionErrorEstimate=TransfUtils<ImageType>::computeBSplineTransformFromDeformationField(estimatedError,targetImage);
                                }else{
                                    estimatedError=TransfUtils<ImageType>::linearInterpolateDeformationField(estimatedError,targetImage,m_smoothDeformationDownsampling);
                                }
                                
                                if (m_lineSearch){
                                    //search for updating weight which improves the metric most
                                    //doesnt work well:D
                                    double alpha=2.0;
                                    double bestAlpha=1.0;
                                    double bestNCC=2;

                                    for (int i=0;i<10;++i){
                                        DeformationFieldPointerType def=TransfUtils<ImageType>::subtract(deformation,TransfUtils<ImageType>::multiplyOutOfPlace(estimatedError,alpha));
                                        double nCC=Metrics<ImageType>::nCC( targetImage,sourceImage,def);
                                        LOGV(2)<<VAR(sourceID)<<" "<<VAR(targetID)<<" "<<VAR(alpha)<<" "<<VAR(nCC)<<endl;
                                        if (nCC<bestNCC){
                                            updatedDeform=def;
                                            bestNCC=nCC;
                                        }
                                        alpha/=2.0;
                                    }
                                    
                                }else{
                                    updatedDeform=TransfUtils<ImageType>::subtract(deformation,estimatedError);
                                }
#endif
                                LOGV(3)<<"updated deformation"<<endl;
                            }
                        }else{
                            //read full resolution deformations
                            deformation=ImageUtils<DeformationFieldType>::readImage(m_deformationFileList[sourceID][targetID]);
                            //resample deformation to target image space
                            deformation=TransfUtils<ImageType>::linearInterpolateDeformationField(deformation,targetImage,false);
                            updatedDeform=deformation;
                        }
                        //compare segmentations

                        if (m_groundTruthSegmentations[targetID].IsNotNull() && m_groundTruthSegmentations[sourceID].IsNotNull()){
                            ImagePointerType deformedSeg=TransfUtils<ImageType>::warpImage(  m_groundTruthSegmentations[sourceID] , updatedDeform,true);
                            typename OverlapMeasureFilterType::Pointer filter = OverlapMeasureFilterType::New();
                            filter->SetSourceImage((m_groundTruthSegmentations)[targetID]);
                            filter->SetTargetImage(deformedSeg);
                            filter->SetCoordinateTolerance(1e-4);
                            filter->Update();
                            dice=filter->GetDiceCoefficient();
                            LOGV(1)<<VAR(sourceID)<<" "<<VAR(targetID)<<" "<<VAR(dice)<<endl;
                            typedef typename SegmentationTools<ImageType>::OverlapScores OverlapScores;
                            std::vector<OverlapScores> scores=SegmentationTools<ImageType>::computeOverlapMultilabel((m_groundTruthSegmentations)[targetID],deformedSeg);
                            LOGV(1)<<VAR(sourceID)<<" "<<VAR(targetID)<<" ";
                            dice=0.0;
                            for (int s=0;s<scores.size();++s){
                                if (mylog.getVerbosity()>1){std::cout<<" "<<VAR(scores[s].labelID)<<" "<<scores[s].dice;}
                                dice+=scores[s].dice;
                            }
                            if (mylog.getVerbosity()>1){                                std::cout<<endl;}
                            dice=dice/(scores.size());
                            LOGV(1)<<"AverageDice : "<<dice<<endl;
                            
                            m_dice+=dice;
                        }
                        

                        // compare landmarks

                        if (m_landmarkFileList.size()){
                            if (m_landmarkFileList.find(targetID)!=m_landmarkFileList.end() && m_landmarkFileList.find(sourceID)!=m_landmarkFileList.end()){
                                //hope that all landmark files are available :D
                                if (m_maskList && m_maskList->find(targetID)!=m_maskList->end())
                                    tre=TransfUtils<ImageType>::computeTRE(m_landmarkFileList[targetID], m_landmarkFileList[sourceID],updatedDeform,targetImage,(*m_maskList)[targetID],(*m_maskList)[sourceID]);
                                else
                                    tre=TransfUtils<ImageType>::computeTRE(m_landmarkFileList[targetID], m_landmarkFileList[sourceID],updatedDeform,targetImage);
                                treCount+=1;
                                m_TRE+=tre;
                            }
                        }
                        
                        
                        
                        //compute LNCC
                        warpedImage = TransfUtils<ImageType>::warpImage(  sourceImage , updatedDeform,m_metric == "categorical");
                        double samplingFactor=1.0*warpedImage->GetLargestPossibleRegion().GetSize()[0]/this->m_ROI->GetLargestPossibleRegion().GetSize()[0];
                        double imageResamplingFactor=min(1.0,8.0/(samplingFactor));
                        nCC= Metrics<ImageType>::nCC(targetImage,warpedImage);
                        if (-nCC>m_maxSim) m_maxSim=-nCC;
                        if (-nCC<m_minSim) m_minSim=-nCC;
                        m_averageNCC+=nCC;
                        //if (m_sigma>0){
                        if (m_metric != "categorical"){
                            warpedImage=FilterUtils<ImageType>::LinearResample(warpedImage,imageResamplingFactor,true);
                            targetImage=FilterUtils<ImageType>::LinearResample(targetImage,imageResamplingFactor,true);
                        }else{
                            warpedImage=FilterUtils<ImageType>::NNResample(warpedImage,imageResamplingFactor,false);
                            targetImage=FilterUtils<ImageType>::NNResample(targetImage,imageResamplingFactor,false);

                        }
                        LOGV(3)<<"Computing metric "<<m_metric<<" on images downsampled by "<<samplingFactor<<" to size "<<warpedImage->GetLargestPossibleRegion().GetSize()<<endl;
                        if (m_metric == "lncc"){
                            //lncc= Metrics<ImageType,FloatImageType>::efficientLNCC(warpedImage,targetImage,m_sigma,m_exponent);
                            lncc= Metrics<ImageType,FloatImageType,long double>::efficientLNCC(warpedImage,targetImage,m_sigma,m_exponent);
                        }else if (m_metric == "itklncc"){
                            lncc= Metrics<ImageType,FloatImageType,float>::ITKLNCC(warpedImage,targetImage,m_sigma,m_exponent, this->m_ROI);
                        }else if (m_metric == "lnccAbs"){
                            lncc= Metrics<ImageType,FloatImageType>::efficientLNCCNewNorm(warpedImage,targetImage,m_sigma,m_exponent);
                        }else if (m_metric == "lnccAbsMultiscale"){
                            lncc= Metrics<ImageType,FloatImageType>::multiScaleLNCCAbs(warpedImage,targetImage,m_sigma,m_exponent);
                        }else if (m_metric == "lsad"){
                            lncc= Metrics<ImageType,FloatImageType>::LSADNorm(warpedImage,targetImage,m_sigma,m_exponent);
                          
                        }else if (m_metric == "lssd"){
                            lncc= Metrics<ImageType,FloatImageType>::LSSDNorm(warpedImage,targetImage,m_sigma,m_exponent);
                        }else if (m_metric == "localautocorrelation"){
                            //lncc= Metrics<ImageType,FloatImageType>::LSSD(warpedImage,(m_imageList)[targetID],m_sigma);
                            lncc= Metrics<ImageType,FloatImageType>::localMetricAutocorrelation(warpedImage,targetImage,m_sigma,2,"lssd",m_exponent);
                        }else if (m_metric == "gradient"){
                            //lncc= Metrics<ImageType,FloatImageType>::LSSD(warpedImage,(m_imageList)[targetID],m_sigma);
                            lncc=  Metrics<ImageType,FloatImageType>::efficientLNCC(warpedImage,targetImage,m_sigma,m_exponent);
                        }else if (m_metric == "categorical"){
                            lncc=  Metrics<ImageType,FloatImageType>::CategoricalDiffNorm(warpedImage,targetImage,m_sigma,m_exponent);
                        }else if (m_metric == "deedsSSC"){
                            lncc=  Metrics<ImageType,FloatImageType,float>::deedsMIND(warpedImage,targetImage,m_sigma,m_exponent);
                        }
                        else if (m_metric == "deedsLCC"){
                            lncc=  Metrics<ImageType,FloatImageType,float>::deedsLCC(warpedImage,targetImage,m_sigma,m_exponent);
                        }else{
                            LOG<<"do not understand "<<VAR(m_metric)<<",aborting."<<endl;
                            exit(-1);
                        } 

                        //multiply image with normed gradients of target and warped source image
                        if (m_filterMetricWithGradient){
                            //typedef typename itk::GradientMagnitudeImageFilter<ImageType,FloatImageType> GradientFilter;
                            typedef typename itk::GradientMagnitudeRecursiveGaussianImageFilter<ImageType,FloatImageType> GradientFilter;

                            //get gradient of target image
                            typename GradientFilter::Pointer filter=GradientFilter::New();
                            filter->SetInput((ConstImagePointerType)targetImage);
                            filter->SetSigma(m_sigma);
                            filter->Update();
                            FloatImagePointerType gradient=filter->GetOutput();
                            ImageUtils<FloatImageType>::expNormImage(gradient,0.0);
                            ImageUtils<FloatImageType>::multiplyImage(gradient,-1.0);
                            ImageUtils<FloatImageType>::add(gradient,1.0);

                            //multiply lncc with target gradient
                            lncc=ImageUtils<FloatImageType>::multiplyImageOutOfPlace(lncc,gradient);

                        }
                        if (m_metric == "gradient"){
                            //double value;
                            //computeMetricAndDerivative(targetImage, m_imageList[sourceID] ,updatedDeform ,   (m_pairwiseGradients)[sourceID][targetID] ,  value);
                            //updatedDeform= (m_pairwiseGradients)[sourceID][targetID];
                            for (int d=0;d<D;++d){
                                double magnitude=this->m_ROI->GetSpacing()[d];
                                
                                DisplacementType plusOne,minusOne; plusOne.Fill(0);minusOne.Fill(0); plusOne[d]=magnitude; minusOne[d]=-magnitude;

                                //calculate lncc shifted right
                                ImagePointerType shiftedImage = TransfUtils<ImageType>::translateImage(  warpedImage , plusOne);
                                FloatImagePointerType lnccPlusOne= Metrics<ImageType,FloatImageType, double>::efficientLNCC(shiftedImage,targetImage,m_sigma,m_exponent);
                                //lncc shifted left
                                shiftedImage = TransfUtils<ImageType>::translateImage(  warpedImage , minusOne);
                                FloatImagePointerType lnccMinusOne= Metrics<ImageType,FloatImageType, double>::efficientLNCC(shiftedImage,targetImage,m_sigma,m_exponent);
#ifdef CONTINUOUSDERIVATIVE
                                FloatImagePointerType lnccDiffRightLeft=FilterUtils<FloatImageType>::substract(lnccPlusOne,lnccMinusOne);
                                map<string , map<string,FloatImagePointerType> > &cache=m_pairwiseMetricDerivatives[d];
                                map<string, FloatImagePointerType> & mmap1=cache[sourceID];
                                mmap1[targetID] = FilterUtils<FloatImageType>::LinearResample(lnccDiffRightLeft,FilterUtils<ImageType,FloatImageType>::cast(this->m_ROI),true);
#else
                                //calcuate wen lncc is better than input when shifted right
                                FloatImagePointerType lnccDiffRight=FilterUtils<FloatImageType>::substract(lnccPlusOne,lncc);
                                ostringstream oss1;
                                double meanLNCC_center=FilterUtils<FloatImageType>::getMean(lncc);
                                LOGV(1)<<VAR(meanLNCC_center)<<endl;

                                FloatImagePointerType lnccUpdateRight=FilterUtils<FloatImageType>::binaryThresholdingLow(lnccDiffRight,0);
                                oss1<<m_metric<<"-diffRight-directon"<<d<<"-"<<sourceID<<"-TO-"<<targetID<<".mha";
                                LOGI(6,ImageUtils<FloatImageType>::writeImage(oss1.str(),lnccUpdateRight));
                                oss1.str("");
                                oss1.clear();
                                ImageUtils<FloatImageType>::multiplyImage(lnccUpdateRight,magnitude);
                                bool test=true;
                                //TESTDEFORM
                                if (test){
                                    DeformationFieldPointerType updateDefSimple=ImageUtils<DeformationFieldType>::duplicate(updatedDeform);
                                    FloatImagePointerType directionD=TransfUtils<ImageType,float,double,double>::getComponent(updateDefSimple,d);
                                    directionD=FilterUtils<FloatImageType>::add(directionD,lnccUpdateRight);
                                    TransfUtils<ImageType,float,double,double>::setComponent(updateDefSimple,directionD,d);
                                    ImagePointerType warpedImage2=TransfUtils<ImageType>::warpImage( sourceImage,updateDefSimple);
                                    FloatImagePointerType lnccWarpedUpdate= Metrics<ImageType,FloatImageType, double>::efficientLNCC(warpedImage2,targetImage,m_sigma,m_exponent);
                                    double meanLNCC_updateRight=FilterUtils<FloatImageType>::getMean(lnccWarpedUpdate);
                                    LOGV(1)<<VAR(meanLNCC_updateRight)<<endl;
                                }
                              
                                
                                //calculate improvement over initival lncc
                                FloatImagePointerType lnccDiffLeft=FilterUtils<FloatImageType>::substract(lnccMinusOne,lncc);
                               
                                FloatImagePointerType lnccUpdateLeft=FilterUtils<FloatImageType>::binaryThresholdingLow(lnccDiffLeft,0);
                                ImageUtils<FloatImageType>::multiplyImage(lnccUpdateLeft,-magnitude);
                                //TESTDEFORM
                                 if (test){
                                    DeformationFieldPointerType updateDefSimple=ImageUtils<DeformationFieldType>::duplicate(updatedDeform);
                                    FloatImagePointerType directionD=TransfUtils<ImageType,float,double,double>::getComponent(updateDefSimple,d);
                                    directionD=FilterUtils<FloatImageType>::add(directionD,lnccUpdateLeft);
                                    TransfUtils<ImageType,float,double,double>::setComponent(updateDefSimple,directionD,d);
                                    ImagePointerType warpedImage2=TransfUtils<ImageType>::warpImage( sourceImage,updateDefSimple);
                                    FloatImagePointerType lnccWarpedUpdate= Metrics<ImageType,FloatImageType, double>::efficientLNCC(warpedImage2,targetImage,m_sigma,m_exponent);
                                    double meanLNCC_updateLeft=FilterUtils<FloatImageType>::getMean(lnccWarpedUpdate);
                                    LOGV(1)<<VAR(meanLNCC_updateLeft)<<endl;
                                }

                                //compare left and right shift lncc
                                FloatImagePointerType lnccDiffLeftRight=FilterUtils<FloatImageType>::substract(lnccPlusOne,lnccMinusOne);
                                //1 if right is better, 0 if left is better
                                FloatImagePointerType lnccUpdateLeftRight=FilterUtils<FloatImageType>::binaryThresholdingLow(lnccDiffLeftRight,0);
                                //keep only entries in lnccUpdateRight when it is also better than the update left
                                ImageUtils<FloatImageType>::multiplyImage(lnccUpdateRight,lnccUpdateLeftRight);
                                    //TESTDEFORM
                                 if (test){
                                    DeformationFieldPointerType updateDefSimple=ImageUtils<DeformationFieldType>::duplicate(updatedDeform);
                                    FloatImagePointerType directionD=TransfUtils<ImageType,float,double,double>::getComponent(updateDefSimple,d);
                                    directionD=FilterUtils<FloatImageType>::add(directionD,lnccUpdateRight);
                                    TransfUtils<ImageType,float,double,double>::setComponent(updateDefSimple,directionD,d);
                                    ImagePointerType warpedImage2=TransfUtils<ImageType>::warpImage( sourceImage,updateDefSimple);
                                    FloatImagePointerType lnccWarpedUpdate= Metrics<ImageType,FloatImageType, double>::efficientLNCC(warpedImage2,targetImage,m_sigma,m_exponent);
                                    double meanLNCC_updateRightexcludingLeft=FilterUtils<FloatImageType>::getMean(lnccWarpedUpdate);
                                    LOGV(1)<<VAR(meanLNCC_updateRightexcludingLeft)<<endl;
                                }
                                
                               
                                //invert mask
                                FloatImagePointerType lnccUpdateRightLeft=FilterUtils<FloatImageType>::invert(lnccUpdateLeftRight);
                                //keep only entries in lnccUpdateLeft when it is also better than the update right
                                ImageUtils<FloatImageType>::multiplyImage(lnccUpdateLeft,lnccUpdateRightLeft);
                                 if (test){
                                    DeformationFieldPointerType updateDefSimple=ImageUtils<DeformationFieldType>::duplicate(updatedDeform);
                                    FloatImagePointerType directionD=TransfUtils<ImageType,float,double,double>::getComponent(updateDefSimple,d);
                                    directionD=FilterUtils<FloatImageType>::add(directionD,lnccUpdateLeft);
                                    TransfUtils<ImageType,float,double,double>::setComponent(updateDefSimple,directionD,d);
                                    ImagePointerType warpedImage2=TransfUtils<ImageType>::warpImage(sourceImage,updateDefSimple);
                                    FloatImagePointerType lnccWarpedUpdate= Metrics<ImageType,FloatImageType, double>::efficientLNCC(warpedImage2,targetImage,m_sigma,m_exponent);
                                    double meanLNCC_updateLeftexcludingRight=FilterUtils<FloatImageType>::getMean(lnccWarpedUpdate);
                                    LOGV(1)<<VAR(meanLNCC_updateLeftexcludingRight)<<endl;
                                }

                                FilterUtils<FloatImageType>::add(lnccUpdateRight,lnccUpdateLeft);
                                   if (test){
                                    DeformationFieldPointerType updateDefSimple=ImageUtils<DeformationFieldType>::duplicate(updatedDeform);
                                    FloatImagePointerType directionD=TransfUtils<ImageType,float,double,double>::getComponent(updateDefSimple,d);
                                    directionD=FilterUtils<FloatImageType>::add(directionD,lnccUpdateRight);
                                    TransfUtils<ImageType,float,double,double>::setComponent(updateDefSimple,directionD,d);
                                    ImagePointerType warpedImage2=TransfUtils<ImageType>::warpImage( sourceImage,updateDefSimple);
                                    FloatImagePointerType lnccWarpedUpdate= Metrics<ImageType,FloatImageType, double>::efficientLNCC(warpedImage2,targetImage,m_sigma,m_exponent);
                                    double meanLNCC_updateFinal=FilterUtils<FloatImageType>::getMean(lnccWarpedUpdate);
                                    LOGV(1)<<VAR(meanLNCC_updateFinal)<<endl;
                                }
                                  map<string , map<string,FloatImagePointerType> > &cache=m_pairwiseMetricDerivatives[d];
                                  map<string, FloatImagePointerType> & mmap1=cache[sourceID];
                                  mmap1[targetID] = FilterUtils<FloatImageType>::LinearResample(lnccUpdateRight,FilterUtils<ImageType,FloatImageType>::cast(this->m_ROI),true);
#endif
                                  ostringstream oss;
                                oss<<m_metric<<"-DERIVATIVE-directon"<<d<<"-"<<sourceID<<"-TO-"<<targetID<<".mha";
                                LOGI(6,ImageUtils<FloatImageType>::writeImage(oss.str(),mmap1[targetID]));

                           
                      
                                
                            }
                        }

                        LOGV(3)<<"done."<<endl;
                        ostringstream oss;
                        oss<<m_metric<<"-"<<sourceID<<"-TO-"<<targetID<<".mha";

                        LOGI(6,ImageUtils<FloatImageType>::writeImage(oss.str(),lncc));
                        //resample lncc result
                         
                        //resample with smoothing
                        lncc=FilterUtils<FloatImageType>::LinearResample(lncc,FilterUtils<ImageType,FloatImageType>::cast(this->m_ROI),false);
                         
                        oss<<"-resampled";
                        if (D==2){
                            oss<<".png";
                        }else
                            oss<<".nii";
                        LOGI(6,ImageUtils<ImageType>::writeImage(oss.str(),FilterUtils<FloatImageType,ImageType>::cast(ImageUtils<FloatImageType>::multiplyImageOutOfPlace(lncc,255))));
                        //}
                        //store...
                        if (m_updateDeformations || !m_pairwiseLocalWeightMaps[sourceID][targetID].IsNotNull()){

                            if (m_pairwiseLocalWeightMaps[sourceID][targetID].IsNull()){
                                //certainly update local weights maps when it was not set before ;)
                                m_pairwiseLocalWeightMaps[sourceID][targetID]=lncc;
                                m_pairwiseGlobalSimilarity[sourceID][targetID]=nCC;
                                updateThisDeformation=true;
                            }else{
                                if (! m_updateDeformationsGlobalSim || 
                                    (nCC <   m_pairwiseGlobalSimilarity[sourceID][targetID] ))
                                    {
                                        //update if global Sim improved or if it shouldn't be considered anyway
                                        m_pairwiseLocalWeightMaps[sourceID][targetID]=lncc;
                                        m_pairwiseGlobalSimilarity[sourceID][targetID]=nCC;
                                        updateThisDeformation=true;

                                    }
                            }
                        }else{
                            m_pairwiseGlobalSimilarity[sourceID][targetID]=nCC;
                            m_updatedPairwiseLocalWeightMaps[sourceID][targetID]=lncc;
                            //upsample if necessary.. note that here, quite some accuracy of the lncc is lost :( in fact, lncc should be recomputed
                            if (m_pairwiseLocalWeightMaps[sourceID][targetID]->GetLargestPossibleRegion().GetSize()!=this->m_ROI->GetLargestPossibleRegion().GetSize()){
                                //update the pairwise similarity of the original deformation to the correct resolution
                                warpedImage = TransfUtils<ImageType>::warpImage(  sourceImage , deformation,m_metric == "categorical");
                                if (m_metric != "categorical"){
                                    warpedImage=FilterUtils<ImageType>::LinearResample(warpedImage,imageResamplingFactor,true);
                                }else{
                                    warpedImage=FilterUtils<ImageType>::NNResample(warpedImage,imageResamplingFactor,false);
                                }
                                if (m_metric == "lncc"){
                                    //lncc= Metrics<ImageType,FloatImageType>::efficientLNCC(warpedImage,targetImage,m_sigma,m_exponent);
                                    lncc= Metrics<ImageType,FloatImageType, double>::efficientLNCC(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "lnccAbs"){
                                    lncc= Metrics<ImageType,FloatImageType>::efficientLNCCNewNorm(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "lnccAbsMultiscale"){
                                    lncc= Metrics<ImageType,FloatImageType>::multiScaleLNCCAbs(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "deedsSSC"){
                                    lncc=  Metrics<ImageType,FloatImageType,float>::deedsMIND(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "deedsLCC"){
                                    lncc=  Metrics<ImageType,FloatImageType,float>::deedsLCC(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "itklncc"){
                                    lncc= Metrics<ImageType,FloatImageType>::ITKLNCC(warpedImage,targetImage,m_sigma,m_exponent, this->m_ROI);
                                }else if (m_metric == "lsad"){
                                    lncc= Metrics<ImageType,FloatImageType>::LSADNorm(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "lssd"){
                                    lncc= Metrics<ImageType,FloatImageType>::LSSDNorm(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "localautocorrelation"){
                                    //lncc= Metrics<ImageType,FloatImageType>::LSSD(warpedImage,(m_imageList)[targetID],m_sigma);
                                    lncc= Metrics<ImageType,FloatImageType>::localMetricAutocorrelation(warpedImage,targetImage,m_sigma,2,"lssd",m_exponent);
                                }else if (m_metric == "gradient"){
                                    //lncc= Metrics<ImageType,FloatImageType>::LSSD(warpedImage,(m_imageList)[targetID],m_sigma);
                                    lncc=  Metrics<ImageType,FloatImageType>::efficientLNCC(warpedImage,targetImage,m_sigma,m_exponent);
                                }else if (m_metric == "categorical"){
                                    lncc=  Metrics<ImageType,FloatImageType>::CategoricalDiffNorm(warpedImage,targetImage,m_sigma,m_exponent);
                                }else{
                                    LOG<<"do not understand "<<VAR(m_metric)<<",aborting."<<endl;
                                    exit(-1);
                                } 
                                lncc=FilterUtils<FloatImageType>::LinearResample(lncc,FilterUtils<ImageType,FloatImageType>::cast(this->m_ROI),false);
                                m_pairwiseLocalWeightMaps[sourceID][targetID]=lncc;
                            }
                        }
                    }
                    double ade=0.0;
                    //read true deformation if it is a) in the file list and b) we want to either compare it to the estimate or it needs to be cached.
                    if (findDeformation(m_trueDeformationFileList,sourceID,targetID) && (estDef || !m_trueDeformations[sourceID][targetID].IsNotNull())){
                        knownDeformation=ImageUtils<DeformationFieldType>::readImage(m_trueDeformationFileList[sourceID][targetID]);
                        if (estDef){
                            ImagePointerType mask=TransfUtils<ImageType>::createEmptyImage(knownDeformation);
                            mask->FillBuffer(0);
                            typename ImageType::SizeType size=mask->GetLargestPossibleRegion().GetSize();
                            IndexType offset;
                            double fraction=0.9;
                            for (int d=0;d<D;++d){
                                offset[d]=(1.0-fraction)/2*size[d];
                                size[d]=fraction*size[d];
                            }
                        
                            typename ImageType::RegionType region;
                            region.SetSize(size);
                            region.SetIndex(offset);
                            ImageUtils<ImageType>::setRegion(mask,region,1);
                            DeformationFieldPointerType defo;
                            if (m_bSplineInterpol){
                                defo=TransfUtils<ImageType>::linearInterpolateDeformationField(updatedDeform,knownDeformation,m_smoothDeformationDownsampling);
                            }else{
                                defo=TransfUtils<ImageType>::bSplineInterpolateDeformationField(updatedDeform,knownDeformation,m_smoothDeformationDownsampling);
                            }

                            DeformationFieldPointerType diff=TransfUtils<ImageType>::subtract(defo,knownDeformation);
                            //m_ADE+=TransfUtils<ImageType>::computeDeformationNorm(diff);
                            ade=TransfUtils<ImageType>::computeDeformationNormMask(diff,mask);
                            m_ADE+=ade;
                        }
                    }


                    //compute ADE
                    //compute inconsistency? baeh.
                    
                    //store updated deformation in the appropriate place
                    if (estDef){
                        //write full resolution deformation to disk
                        ostringstream outfile;
                        if (directory != ""){
                            //outfile<<directory<<"/estimatedLocalComposedDeformationError-FROM-"<<sourceID<<"-TO-"<<targetID<<".mha";
                            //ImageUtils<DeformationFieldType>::writeImage(outfile.str().c_str(),estimatedError);
                            ostringstream outfile2;
                            outfile2<<directory<<"/estimatedLocalComposedDeformation-FROM-"<<sourceID<<"-TO-"<<targetID<<".mha";
                            ImageUtils<DeformationFieldType>::writeImage(outfile2.str().c_str(),updatedDeform);
                        }

                        
                        typedef typename itk::DisplacementFieldJacobianDeterminantFilter<DeformationFieldType,double> DisplacementFieldJacobianDeterminantFilterType;
                        typename DisplacementFieldJacobianDeterminantFilterType::Pointer jacobianFilter = DisplacementFieldJacobianDeterminantFilterType::New();
                        jacobianFilter->SetInput(updatedDeform);
                        jacobianFilter->SetUseImageSpacingOff();
                        jacobianFilter->Update();
                        FloatImagePointerType jac=jacobianFilter->GetOutput();
                        double minJac = FilterUtils<FloatImageType>::getMin(jac);
			double stdDevJac=sqrt(FilterUtils<FloatImageType>::getVariance(jac));
			m_averageJacSTD+=stdDevJac;
                        LOGV(2)<<VAR(sourceID)<<" "<<VAR(targetID)<< " " << VAR(minJac) <<" " <<VAR(stdDevJac)<<" "<<VAR(nCC)<<" "<<VAR(ade)<<" "<<VAR(dice)<<" "<<VAR(tre)<<endl;
                        m_averageMinJac+=minJac;
                        if (minJac<m_minMinJacobian){
                            m_minMinJacobian=minJac;
                        }

                        //sample to correct resolution (downsample)
                        
                        if (m_bSplineInterpol){
                            updatedDeform=TransfUtils<ImageType>::bSplineInterpolateDeformationField(updatedDeform,this->m_ROI,m_smoothDeformationDownsampling);
                            //updatedDeform=TransfUtils<ImageType>::computeBSplineTransformFromDeformationField(updatedDeform,this->m_ROI);
                        }else{
                            updatedDeform=TransfUtils<ImageType>::linearInterpolateDeformationField(updatedDeform,this->m_ROI,m_smoothDeformationDownsampling);
                        }
                       
                        if (updateThisDeformation){//m_updateDeformations ||  !m_downSampledDeformationCache[sourceID][targetID].IsNotNull()){
                            
                            firstRun=true;// 
			   
			   
                            m_downSampledDeformationCache[sourceID][targetID] = updatedDeform;

                        }else{

                            (m_updatedDeformationCache)[sourceID][targetID] = updatedDeform;
                            
#if 0
                            locallyUpdateDeformation( m_downSampledDeformationCache[sourceID][targetID],(m_updatedDeformationCache)[sourceID][targetID], m_pairwiseLocalWeightMaps[sourceID][targetID],m_updatedPairwiseLocalWeightMaps[sourceID][targetID]);
                            //locallyUpdateDeformation((m_updatedDeformationCache)[sourceID][targetID],m_downSampledDeformationCache[sourceID][targetID],m_updatedPairwiseLocalWeightMaps[sourceID][targetID], m_pairwiseLocalWeightMaps[sourceID][targetID]);
                            //updatedDeform=(m_updatedDeformationCache)[sourceID][targetID];(m_updatedDeformationCache)[sourceID][targetID]=(m_updatedDeformationCache)[sourceID][targetID];(m_updatedDeformationCache)[sourceID][targetID]=updatedDeform;
                            //jac=m_updatedPairwiseLocalWeightMaps[sourceID][targetID];m_updatedPairwiseLocalWeightMaps[sourceID][targetID]=m_pairwiseLocalWeightMaps[sourceID][targetID];m_pairwiseLocalWeightMaps[sourceID][targetID]=jac;
#endif

                            m_haveDeformationEstimate = true;
                            //resample deformation if resolution or origin was changed
                            if (this->m_ROI->GetLargestPossibleRegion().GetSize() != m_downSampledDeformationCache[sourceID][targetID]->GetLargestPossibleRegion().GetSize()
                                || this->m_ROI->GetOrigin() != m_downSampledDeformationCache[sourceID][targetID]->GetOrigin() )
                                {
                                    m_downSampledDeformationCache[sourceID][targetID]=TransfUtils<ImageType>::linearInterpolateDeformationField(deformation,this->m_ROI,m_smoothDeformationDownsampling);
                                }

                        }
                        ++count;
                    }

                }//target=source
            }
        }
        m_ADE/=count;
        m_dice/=count;
        if (treCount)
            m_TRE/=treCount;
        m_averageMinJac/=count;
        m_averageNCC/=count;
	m_averageJacSTD/=count;
        //LOG<<VAR(m_averageMinJac)<<" "<<VAR(minMinJac)<<" "<<VAR(m_averageNCC)<<endl;
        if (m_updateDeformations || firstRun ){
            m_Inconsistency=TransfUtils<ImageType>::computeInconsistency(&m_downSampledDeformationCache,&m_imageIDList, &m_trueDeformations,m_maskList);
        }else{
            m_Inconsistency=TransfUtils<ImageType>::computeInconsistency(&m_updatedDeformationCache,&m_imageIDList, &m_trueDeformations,m_maskList);
        }
    }//DoALot
    
    
    
    double computeTRE(string targetLandmarks, string refLandmarks, DeformationFieldPointerType def,ImagePointerType reference){
        typedef typename  ImageType::DirectionType DirectionType;
        typedef typename itk::VectorLinearInterpolateImageFunction<DeformationFieldType> DefInterpolatorType;
        typedef typename DefInterpolatorType::ContinuousIndexType CIndexType;
        PointType p;
        p.Fill(0.0);
        typename DefInterpolatorType::Pointer defInterpol=DefInterpolatorType::New();
        defInterpol->SetInputImage(def);
        ifstream ifs(refLandmarks.c_str());
        int i=0;
        double TRE=0.0;
        int count=0;
        vector<PointType> landmarksReference, landmarksTarget;
        DirectionType refDir=reference->GetDirection();
        DirectionType targetDir=def->GetDirection();

        while ( not ifs.eof() ) {
            PointType point;
            for (int d=0;d<D;++d){
                ifs>>point[d];
                point[d]=point[d]*refDir[d][d];
            }
            //LOG<<point<<endl;
            landmarksReference.push_back(point);
            
        } 
        //std::cout<<"read "<<landmarksReference.size()<<" landmarks"<<std::endl;
        ifstream ifs2(targetLandmarks.c_str());
        i=0;
        for (;i<landmarksReference.size()-1;++i){
            PointType pointTarget;
            for (int d=0;d<D;++d){
                ifs2>>pointTarget[d];
                pointTarget[d]=pointTarget[d]*targetDir[d][d];
            }        
            IndexType indexTarget,indexReference;
            def->TransformPhysicalPointToIndex(pointTarget,indexTarget);
           
            PointType deformedReferencePoint;
            reference->TransformPhysicalPointToIndex(landmarksReference[i],indexReference);
                        
          
            CIndexType cindex;
            def->TransformPhysicalPointToContinuousIndex(pointTarget,cindex);
            if (def->GetLargestPossibleRegion().IsInside(cindex)){
                deformedReferencePoint= pointTarget+defInterpol->EvaluateAtContinuousIndex(cindex);
                double localError=(deformedReferencePoint - landmarksReference[i]).GetNorm();
                LOGI(2,std::cout<<"pt"<<i<<": "<<(localError)<<" ");
                TRE+=localError;
                ++count;
            }
        }
        LOGI(2,std::cout<<std::endl);
        return TRE/count;
    }


    double computeLandmarkRegistrationError(DeformationCacheType * deformations, map<string,string> landmarkFilenames,std::vector<string> imageIDs, ImageCacheType * images){

        int nImages=imageIDs.size();
        typedef typename  ImageType::DirectionType DirectionType;
        
        typedef typename itk::VectorLinearInterpolateImageFunction<DeformationFieldType> DefInterpolatorType;
        
        typedef typename DefInterpolatorType::ContinuousIndexType CIndexType;
        
        PointType p;
        p.Fill(0.0);
        
        double sumSquareError=0.0;
        int count = 0;

        for (int source=0;source<nImages;++source){
            string sourceID=imageIDs[source];
            for (int target=0;target<nImages;++target){
                if (source!=target){
                    string targetID=imageIDs[target];
                    DeformationFieldPointerType def=(deformations)[sourceID][targetID];
                    ImagePointerType reference=(images)[targetID];
                    DirectionType refDir=reference->GetDirection();

                    if (def->GetLargestPossibleRegion().GetSize() != reference->GetLargestPossibleRegion().GetSize()){
                        def=TransfUtils<ImageType>::linearInterpolateDeformationField(def,reference);
                    }
                    typename DefInterpolatorType::Pointer defInterpol=DefInterpolatorType::New();
        
                    defInterpol->SetInputImage(def);
                    DirectionType targetDir=def->GetDirection();
                    vector<PointType> landmarksReference, landmarksTarget;
                    string refLandmarks=landmarkFilenames[sourceID];
                    string targetLandmarks=landmarkFilenames[targetID];
                    ifstream ifs(refLandmarks.c_str());
                    int i=0;
                      
                    while ( not ifs.eof() ) {
                        PointType point;
                        for (int d=0;d<D;++d){
                            ifs>>point[d];
                            point[d]=point[d]*refDir[d][d];
                        }
                        //LOG<<point<<endl;
                        landmarksReference.push_back(point);
                          
                    } 
                    //std::cout<<"read "<<landmarksReference.size()<<" landmarks"<<std::endl;
                    ifstream ifs2(targetLandmarks.c_str());
                    i=0;
                    for (;i<landmarksReference.size()-1;++i){
                        PointType pointTarget;
                        for (int d=0;d<D;++d){
                            ifs2>>pointTarget[d];
                            pointTarget[d]=pointTarget[d]*targetDir[d][d];
                        }        
                        IndexType indexTarget,indexReference;
                        def->TransformPhysicalPointToIndex(pointTarget,indexTarget);
                        //LOG<<VAR(def->GetOrigin())<<endl;
                        //LOG<<VAR(pointTarget)<<" "<<VAR(indexTarget)<<endl;
                        PointType deformedReferencePoint;
                        reference->TransformPhysicalPointToIndex(landmarksReference[i],indexReference);
                          
                        //std::cout<<VAR(targetPoint)<<endl;
                        //deformedReferencePoint= pointTarget+def->GetPixel(indexTarget);
                        CIndexType cindex;
                        def->TransformPhysicalPointToContinuousIndex(pointTarget,cindex);
                        //LOG<<VAR(landmarksReference[i])<<" "<<VAR(indexReference)<<" "<<VAR(cindex)<<endl;
                        if (def->GetLargestPossibleRegion().IsInside(cindex)){
                            deformedReferencePoint= pointTarget+defInterpol->EvaluateAtContinuousIndex(cindex);
                              
                            //LOG<< VAR(pointTarget) << endl;
                            double localSquaredError=(deformedReferencePoint - landmarksReference[i]).GetNorm();
                           
                              
                            LOGI(2,std::cout<<"pt"<<i<<": "<<(localSquaredError)<<" ");
                            sumSquareError+=localSquaredError;
                            ++count;
                        }
                    }
                    LOGI(2,std::cout<<endl);
                      
                }
            }
        }
        return sumSquareError/count;
      
    }  

    void computeMetricAndDerivative(ImagePointerType img1, ImagePointerType img2, DeformationFieldPointerType def, DeformationFieldPointerType deriv, double & value){

        typedef typename itk::CorrelationImageToImageMetricv4<FloatImageType,FloatImageType> MetricType;
    
        //typedef typename itk::MeanSquaresImageToImageMetricv4<FloatImageType,FloatImageType> MetricType;
        typedef typename MetricType::Pointer MetricPointer;
        typedef typename MetricType::DerivativeType MetricDerivativeType;

        //typedef itk::RegistrationParameterScalesFromJacobian<MetricType> ScalesEstimatorType;
        typedef itk::RegistrationParameterScalesFromPhysicalShift<MetricType> ScalesEstimatorType;
        typedef typename ScalesEstimatorType::Pointer ScalesEstimatorPointer;

        m_resolutionFactor=4.0*def->GetLargestPossibleRegion().GetSize()[0]/img1->GetLargestPossibleRegion().GetSize()[0];
        m_resolutionFactor=min(1.0,m_resolutionFactor);
        FloatImagePointerType fimg1=FilterUtils<ImageType,FloatImageType>::LinearResample(img1,m_resolutionFactor,true);
        FloatImagePointerType fimg2=FilterUtils<ImageType,FloatImageType>::LinearResample(img2,m_resolutionFactor,true);
        typename TransfUtils<ImageType,float,double>::OutputDeformationFieldPointerType dblDef=TransfUtils<ImageType,float,double>::cast(def);

#if 0
        typedef typename  itk::DisplacementFieldTransform<double, D> DisplacementFieldTransformType;
        typedef typename DisplacementFieldTransformType::Pointer DisplacementFieldTransformPointer;
        DisplacementFieldTransformPointer defTransf=DisplacementFieldTransformType::New();
        defTransf->SetDisplacementField(dblDef);
        //    LOG<<defTransf<<endl;
#else

        typedef typename  itk::BSplineDeformableTransform<double, D,3> BSplineDeformableTransformType;
        typedef typename BSplineDeformableTransformType::Pointer BSplineDeformableTransformPointer;
        typename BSplineDeformableTransformType::ImagePointer paramImages[D];
         
        BSplineDeformableTransformPointer defTransf=BSplineDeformableTransformType::New();
        for (int d=0;d<D;++d){
            paramImages[d]=TransfUtils<ImageType,double,double,double>::getComponent(dblDef,d);
        }
        defTransf->SetCoefficientImages(paramImages);
#endif
        //LOG<<VAR(defTransf->GetNumberOfParameters())<<endl;
         
     
        MetricPointer metric=MetricType::New();
        metric->SetFixedImage(fimg1);
        metric->SetMovingImage(fimg2);
        metric->SetTransform(defTransf);
       
        metric->Initialize();
        MetricDerivativeType derivative;
      
        metric->GetValueAndDerivative(value, derivative);

        ScalesEstimatorPointer scalesEstimator=ScalesEstimatorType::New();
        scalesEstimator->SetMetric(metric);
        typename ScalesEstimatorType::ScalesType scales,localScales;
        scalesEstimator->EstimateScales(scales);
        //scalesEstimator->EstimateLocalScales(derivative,localScales);
        float learningRate;
        float stepScale;
        float maxStepSize;
        //modify gradient by scales
        for (int i=0;i<derivative.size();++i){
            LOGV(4)<<derivative[i]<<"  "<<i<<" "<<i%scales.size()<<" "<<scales[i%scales.size()]<<endl;
            derivative[i]/=scales[i%scales.size()];
        }
        stepScale=scalesEstimator->EstimateStepScale(derivative);
        //estimate learning rate
        maxStepSize=scalesEstimator->EstimateMaximumStepSize();
        learningRate=2*maxStepSize/stepScale;
        
        LOGV(1)<<VAR(value)<<endl;
        

        //deriv=TransfUtils<ImageType>::createEmpty(def);
        int numberOfPixels=deriv->GetBufferedRegion().GetNumberOfPixels();
        typedef typename itk::ImageRegionIterator<DeformationFieldType> DeformationIteratorType;
        DeformationIteratorType defIt(deriv,deriv->GetLargestPossibleRegion());
        DeformationFieldPointerType bestDeriv;
        int countBad=0;
        for (int i=0;i<10;++i){
            int p=0;
            for (defIt.GoToBegin();!defIt.IsAtEnd();++defIt,++p){
                DeformationType disp;
                for (int d=0;d<D;++d){
                    LOGV(4)<<VAR(p+d*numberOfPixels)<<" "<<VAR(derivative[p+d*numberOfPixels])<<" "<<VAR(scales[p+d*numberOfPixels])<<" "<<VAR(stepScale)<<" "<<VAR(maxStepSize)<<" "<<VAR(learningRate)<<endl;
                    disp[d]=derivative[p+d*numberOfPixels]*learningRate;///scales[p+d*numberOfPixels];
                }
                defIt.Set(disp);

            }
            typename TransfUtils<ImageType,float,double>::OutputDeformationFieldPointerType newDef=TransfUtils<ImageType,double>::add(dblDef,TransfUtils<ImageType,float,double>::cast(deriv));
            for (int d=0;d<D;++d){
                paramImages[d]=TransfUtils<ImageType,double,double,double>::getComponent(newDef,d);
            }
            defTransf->SetCoefficientImages(paramImages);
            metric->SetTransform(defTransf);
            metric->Initialize();
            double newValue=metric->GetValue();
            LOGV(1)<<VAR(i)<<" "<<VAR(value)<<" "<<VAR(newValue)<<" "<<VAR(learningRate)<<endl;
            if (newValue<value){
                value=newValue; learningRate*=2;
                bestDeriv=deriv;
                countBad=0;
            }else{
                if (countBad>2) return;
                learningRate/=2;
                countBad+=1;
            }
        }
        DeformationFieldPointerType newDef=TransfUtils<ImageType>::add(TransfUtils<ImageType,double,float>::cast(dblDef),deriv);
        deriv=newDef;
        ImageUtils<DeformationFieldType>::writeImage("derivative.mha",deriv);
        
    }

    void locallyUpdateDeformation(DeformationFieldPointerType & def, DeformationFieldPointerType &updatedDef, FloatImagePointerType & similarity, FloatImagePointerType updatedSimilarity){
        typedef typename itk::ImageRegionIterator<DeformationFieldType> DeformationIteratorType;
        DeformationIteratorType defIt(def,def->GetLargestPossibleRegion());
        DeformationIteratorType updatedDefIt(updatedDef,def->GetLargestPossibleRegion());
        typedef typename itk::ImageRegionIterator<FloatImageType> FloatImageIteratorType;
        FloatImageIteratorType simIt(similarity,similarity->GetLargestPossibleRegion());
        FloatImageIteratorType updatedSimIt(updatedSimilarity,similarity->GetLargestPossibleRegion());
        defIt.GoToBegin();
        updatedDefIt.GoToBegin();
        simIt.GoToBegin();
        updatedSimIt.GoToBegin();
        for (;!defIt.IsAtEnd();++simIt,++updatedSimIt,++defIt,++updatedDefIt){
            
            if (updatedSimIt.Get()>=simIt.Get()){
                simIt.Set(updatedSimIt.Get());
                defIt.Set(updatedDefIt.Get());
            }

        }


    }
 
};
