#include "PointOfView.h"


using cv::Mat;
using cv::Vec4d;
using cv::Vec3d;
using cv::Vec2d;
using cv::Vec;
using cv::Range;
using cv::Ptr;
using std::vector;
using std::string;
using cv::imread;

namespace OpencvSfM{

  PointOfView::PointOfView(cv::Ptr<Camera> device, Mat rotation /*=Mat::eye(3, 3, CV_64F)*/, Vec3d translation /*=Vec(0.0,0.0,0.0)*/ )
    : projection_matrix_(3, 4, CV_64F)
  {
    CV_Assert( rotation.rows==3 && rotation.cols==3 );
    CV_Assert( !device.empty() );

    this->device_=device;

    this->rotation_=projection_matrix_(Range::all(),Range(0,3));
    rotation.copyTo( this->rotation_ );

    this->translation_ = projection_matrix_(Range::all(),Range(3,4));
    Mat(translation).copyTo( this->translation_ );

    this->config_=0;//everything should be estimated...

    //as we are a new point of view related to a device, we should add our address into device_:
    device_->pointsOfView_.push_back(this);
  };

  PointOfView::~PointOfView(void)
  {
    this->projection_matrix_.release();

    //remove the reference in device_->pointsOfView_:
    vector<PointOfView*>::iterator ourRef=device_->pointsOfView_.begin();
    bool isFound=false;
    while(!isFound && ourRef != device_->pointsOfView_.end())
    {
      //be aware of NULL pointor:
      if( (*ourRef) == this )
      {
        isFound=true;
        (*ourRef)=NULL;
      }
      ourRef++;
    }
  }

  Vec2d PointOfView::project3DPointIntoImage(Vec3d point) const
  {
    //As we don't know what type of camera we use (with/without disportion, fisheyes...)
    //we can't use classic projection matrix P = K . [R|t]
    //Instead, we first compute points transformation into camera's system and then compute
    //pixel coordinate using camera device function.

    //As we need Mat object to compute projection, we create temporary objects:
    Mat mat3DNorm(4,1,CV_64F);
    double* point3DNorm=(double*)mat3DNorm.data;
    Mat mat2DNorm(3,1,CV_64F);
    double* point2DNorm=(double*)mat2DNorm.data;

    vector<Vec2d> pointsOut;
    point3DNorm[0] = point[0];
    point3DNorm[1] = point[1];
    point3DNorm[2] = point[2];
    point3DNorm[3] = 1;

    //transform points into camera's coordinates:
    mat2DNorm = ( projection_matrix_ * mat3DNorm);
    pointsOut.push_back(Vec2d(point2DNorm[0]/point2DNorm[2],point2DNorm[1]/point2DNorm[2]));

    //transform points into pixel coordinates using camera intra parameters:
    pointsOut = device_->normImageToPixelCoordinates(pointsOut);
    return pointsOut[0];
  }
  vector<Vec2d> PointOfView::project3DPointsIntoImage(vector<Vec3d> points) const
  {
    //As we don't know what type of camera we use (with/without disportion, fisheyes...)
    //we can't use classic projection matrix P = K . [R|t]
    //Instead, we first compute points transformation into camera's system and then compute
    //pixel coordinate using camera device function.

    //As we need Mat object to compute projection, we create temporary objects:
    Mat mat3DNorm(4,1,CV_64F);
    double* point3DNorm=(double*)mat3DNorm.data;
    Mat mat2DNorm(3,1,CV_64F);
    double* point2DNorm=(double*)mat2DNorm.data;
    
    vector<Vec2d> pointsOut;
    vector<Vec3d>::iterator point=points.begin();
    while(point!=points.end())
    {
      point3DNorm[0] = (*point)[0];
      point3DNorm[1] = (*point)[1];
      point3DNorm[2] = (*point)[2];
      point3DNorm[3] = 1;

      //transform points into camera's coordinates:
      mat2DNorm = ( projection_matrix_ * mat3DNorm);

      pointsOut.push_back(Vec2d(point2DNorm[0]/point2DNorm[2],point2DNorm[1]/point2DNorm[2]));

      point++;
    }
    //transform points into pixel coordinates using camera intra parameters:
    return device_->normImageToPixelCoordinates(pointsOut);
  }

  bool PointOfView::pointInFrontOfCamera(cv::Vec4d point) const
  {
    Mat pointTranspose= (Mat) point;
    double condition_1 = this->projection_matrix_.row(2).dot(pointTranspose.t()) * point[3];
    double condition_2 = point[2] * point[3];
    if( condition_1 > 0 && condition_2 > 0 )
      return true;
    else
      return false;
  }
  Mat PointOfView::getProjectionMatrix() const
  {
    return device_->getIntraMatrix() * projection_matrix_;
  };
}