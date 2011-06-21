#include "PointsToTrack.h"

using cv::Mat;
using cv::Scalar;
using cv::KeyPoint;
using cv::Point;
using std::vector;
using cv::line;
using cv::circle;

namespace OpencvSfM{
  PointsToTrack::PointsToTrack(std::vector<KeyPoint> keypoints/*=std::vector<cv::KeyPoint>(0)*/,Mat descriptors/*=cv::Mat()*/)
  {
  }


  PointsToTrack::~PointsToTrack(void)
  {
    keypoints_.clear();
    descriptors_.release();
  }
  
  int PointsToTrack::computeKeypointsAndDesc(bool forcing_recalculation)
  {
    int nbPoints;
    if( !forcing_recalculation )
    {
      if( keypoints_.empty() )
        nbPoints=computeKeypoints();
      else
        nbPoints=keypoints_.size();

      if( descriptors_.empty() )
        computeDescriptors();
    }
    else
    {
      nbPoints=computeKeypoints();
      computeDescriptors();
    }

    return nbPoints;
  }


  int PointsToTrack::computeKeypoints()
  {//we don't have data to compute keypoints...
    return keypoints_.size();
  }


  void PointsToTrack::computeDescriptors()
  {//we don't have data to compute descriptors...
  }

  void PointsToTrack::addKeypoints(std::vector<cv::KeyPoint> keypoints,cv::Mat descriptors/*=cv::Mat()*/,bool computeMissingDescriptor/*=false*/)
  {
    //add the keypoints to the end of our points vector:
    this->keypoints_.insert( this->keypoints_.end(),keypoints.begin(),keypoints.end());


    cv::KeyPointsFilter::runByKeypointSize( keypoints_,
      std::numeric_limits<float>::epsilon() );

    if(!computeMissingDescriptor)
    {
      if(!descriptors_.empty())
      {
        Mat newDescriptors( this->keypoints_.size(), this->descriptors_.cols,
          this->descriptors_.type());
        newDescriptors( 
          cv::Rect(0, 0, this->descriptors_.cols,this->descriptors_.rows) ) = 
          this->descriptors_;

        newDescriptors( cv::Rect(0, this->descriptors_.rows,
          this->descriptors_.cols,descriptors.rows) ) = descriptors;

        this->descriptors_=newDescriptors;
      }
    }
    else
    {
      this->computeDescriptors();
    }
  }
  void PointsToTrack::printPointsOnImage(const Mat &image, Mat& outImg, const Scalar& color/*=Scalar::all(-1)*/, int flags/*=DrawMatchesFlags::DEFAULT*/) const
  {
    if(outImg.empty())
      outImg=image.clone();
    cv::drawKeypoints(image, keypoints_, outImg, color, flags);
  }
  void PointsToTrack::read( const cv::FileNode& node, PointsToTrack& points )
  {
    std::string myName=node.name();
    if( myName != "PointsToTrack")
      return;//this node is not for us...
    cv::FileNode& node_keypoints = node["keypoints"];
    if( node_keypoints.empty() )
      CV_Error( CV_StsError, "PointsToTrack FileNode is not correct!" );

    cv::FileNodeIterator it = node_keypoints.begin(), it_end = node_keypoints.end();
    while( it != it_end )
    {
      KeyPoint kpt;
      it >> kpt.pt.x >> kpt.pt.y >> kpt.size >> kpt.angle >> kpt.response >> kpt.octave >> kpt.class_id;
      points.keypoints_.push_back(kpt);
      //it++ is not needed as the >> operator increment automatically it!
    }

    cv::FileNode node_descriptors = node["descriptors"];
    if( node_descriptors.empty() )
      CV_Error( CV_StsError, "PointsToTrack FileNode is not correct!" );
    node_descriptors >> points.descriptors_;

  };
  void PointsToTrack::write(cv::FileStorage& fs, const PointsToTrack& keypoints)
  {
    fs << "{" << "PointsToTrack" << "{";
    cv::write( fs, "keypoints", keypoints.keypoints_ );

    fs << "descriptors" << keypoints.descriptors_;
    fs << "}" << "}";
  }
}