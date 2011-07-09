//Set to 1 if you want to test the points detection and matching
//But be aware to set other tests to 0...
#include "../src/SequenceAnalyzer.h"
#include "../src/PointsMatcher.h"
#include "../src/PointsToTrackWithImage.h"
#include "../src/StructureEstimator.h"
#include "../src/PointOfView.h"
#include "../src/CameraPinhole.h"
#include <opencv2/calib3d/calib3d.hpp>
#include <fstream>


#include "test_data_sets.h"

NEW_TUTO(Triangulation_tests, "Test of triangulation",
  "Using points and cameras parameters, try to compute motion and object. Only for debug!"){

  int nviews = 5;
  int npoints = 6;
  std::ifstream inPoints("logPoints.txt");
  string skeepWord;

  // Collect P matrices together.
  vector<PointOfView> cameras;
  for (int j = 0; j < nviews; ++j) {
    double R[9];
    double T[3];
    double K[9];
    inPoints >> skeepWord;
    while( skeepWord.find("ProjectionsMat")==std::string::npos && !inPoints.eof() )
      inPoints >> skeepWord;
    inPoints >> K[0] >> K[1] >> K[2];
    inPoints >> K[3] >> K[4] >> K[5];
    inPoints >> K[6] >> K[7] >> K[8];
    Ptr<Camera> device=Ptr<Camera>(new CameraPinhole(Mat(3,3,CV_64F,K)) );
    inPoints >> R[0] >> R[1] >> R[2];
    inPoints >> R[3] >> R[4] >> R[5];
    inPoints >> R[6] >> R[7] >> R[8];
    inPoints >> T[0] >> T[1] >> T[2];
    //create the PointOfView:
    cameras.push_back( PointOfView(device, Mat(3,3,CV_64F,R), Vec3d(T) ) );
  }

  vector<cv::Vec3d> points3D;
  vector<cv::Ptr<PointsToTrack>> points2D;
  vector<vector<cv::KeyPoint>> tracks_keypoints(nviews);
  for (int i = 0; i < npoints; ++i) {
    inPoints >> skeepWord;
    while( skeepWord != "2D" && !inPoints.eof() )
      inPoints >> skeepWord;
    inPoints >> skeepWord;//skip Point
    double X[3];
    inPoints >> X[0] >> X[1] >> X[2];
    points3D.push_back( Vec3d( X[0], X[1], X[2] ) );

    // Collect the image of point i in each frame.
    // inPoints >> skeepWord;
    for (int j = 0; j < nviews; ++j) {
      cv::Vec2f point;
      inPoints >> point[0] >> point[1];
      tracks_keypoints[j].push_back( KeyPoint(cv::Point2f(point[0], point[1]),1) );
    }
  }
  for (int j = 0; j < nviews; ++j) {
    points2D.push_back( cv::Ptr<PointsToTrack>(
      new PointsToTrack(j, tracks_keypoints[j] )) );
  }
  /*
  vector<Mat> Ks(nviews);
  vector<Mat> Ps(nviews);
  vector<cv::Vec3d> points3D;
  vector<vector<cv::Vec2d>> points2D;
  */
  //create an empty sequence of images:
  vector<Mat> images;
  for (int j = 0; j < nviews; ++j) {
    Mat img=Mat::zeros(800,600,CV_8SC1);
    images.push_back( img );
  }


  Ptr<DescriptorMatcher> matcher;
  matcher=Ptr<DescriptorMatcher>(new FlannBasedMatcher());
  Ptr<PointsMatcher> matches_algo ( new PointsMatcher(matcher) );
  SequenceAnalyzer motion_estim(images, points2D,matches_algo);

  //create tracks:
  vector<TrackPoints> tracks;
  for(int i=0; i<npoints; ++i)
  {
    TrackPoints tp;
    for(int j=0;j<nviews;j++)
    {
      tp.addMatch(j,i);
    }
    tracks.push_back(tp);
  }
  motion_estim.addTracks(tracks);

  StructureEstimator structure (motion_estim, cameras);
  structure.computeStructure(tracks);
  
  //now for each point of view, we draw the picture and these points projected:
  vector<PointOfView>::iterator itPoV=cameras.begin();
  unsigned int index_image=0;
  while ( itPoV!=cameras.end() )
  {
    Mat imgTmp=images[index_image];//get the current image
    if(imgTmp.empty())
      break;//end of sequence: quit!
    index_image++;

    //create the vector of 3D points viewed by this camera:
    vector<Vec3d> points3D;
    vector<KeyPoint> points2DOrigine;
    //motion_estim_loaded.filterPoints(triangulated,index_image,points3D,points2DOrigine);
    vector<Vec2d> pixelProjected=itPoV->project3DPointsIntoImage(tracks);
    //convert Vec2d into KeyPoint:
    vector<KeyPoint> points2D;
    for(unsigned int j=0;j<pixelProjected.size();j++)
      points2D.push_back( KeyPoint( (float)pixelProjected[j][0],
      (float)pixelProjected[j][1], 10.0 ) );

    Mat imgTmp1,imgTmp2;
    drawKeypoints(imgTmp,points2DOrigine,imgTmp1,Scalar(255,255,255));
    drawKeypoints(imgTmp,points2D,imgTmp2,Scalar(255,255,255));
    imshow("Points origine...",imgTmp1);
    imshow("Points projected...",imgTmp2);
    cv::waitKey(0);
    itPoV++;
  }

}