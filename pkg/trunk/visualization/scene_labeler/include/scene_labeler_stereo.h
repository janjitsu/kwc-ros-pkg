#ifndef SCENE_LABELER_STEREO_H
#define SCENE_LABELER_STEREO_H

#include <iostream>
#include <vector>
#include <algorithm>
#include <std_msgs/PointCloudFloat32.h>
#include <std_msgs/ImageArray.h>
#include <std_msgs/Image.h>
#include <std_msgs/String.h>
#include <ros/node.h>
#include <std_msgs/Empty.h>
#include <std_msgs/VisualizationMarker.h>
#include "logging/LogPlayer.h"
#include <newmat10/newmat.h>
#include <newmat10/newmatio.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "opencv/cxcore.h"
#include "image_utils/cv_bridge.h"

#include <smartScan.h>
#include <time.h>
#include <cmath>

void SplitFilename (const string& str, string *dir, string *file)
{
  size_t found;
  found=str.find_last_of("/\\");
  *file = str.substr(found+1);
  *dir = str.substr(0,found);
}

template <class T>
void copyMsg(string name, ros::msg* m, ros::Time t, void* n)
{
  if (m != 0) {
    *((T*)(n)) = *((T*)(m));
  }
}

class scene_labeler_stereo : public ros::node
{
public:
  std_msgs::ImageArray videre_images_msg_;
  std_msgs::ImageArray labeled_images_msg_;
  std_msgs::Image intensity_image_msg_;
  std_msgs::PointCloudFloat32 full_cloud_msg_;
  std_msgs::PointCloudFloat32 videre_cloud_msg_;
  std_msgs::String cal_params_msg_;

  std_msgs::PointCloudFloat32 full_cloud_colored_;
  std_msgs::PointCloudFloat32 videre_cloud_colored_;

  IplImage *mask_, *left_, *disp_;

  scene_labeler_stereo() 
    : ros::node("scene_labeler"), mask_(0), left_(0), disp_(0)
  {
    advertise<std_msgs::PointCloudFloat32>("videre/cloud", 100);
    advertise<std_msgs::VisualizationMarker>("visualizationMarker", 100);
    advertise<std_msgs::ImageArray>("labeled_images", 100);
    advertise<std_msgs::ImageArray>("videre/images", 100);
  }

  void processMsgs(string file1) {
    cout << "Loading messages... "; flush(cout);
    lp.open(file1, ros::Time(0));
    lp.addHandler<std_msgs::ImageArray>(string("videre/images"), &copyMsg<std_msgs::ImageArray>, (void*)(&videre_images_msg_), true);
    lp.addHandler<std_msgs::ImageArray>(string("labeled_images"), &copyMsg<std_msgs::ImageArray>, (void*)(&labeled_images_msg_), true);
    lp.addHandler<std_msgs::PointCloudFloat32>(string("videre/cloud_smallv"), &copyMsg<std_msgs::PointCloudFloat32>, (void*)(&videre_cloud_msg_), true);
    lp.addHandler<std_msgs::String>(string("videre/cal_params"), &copyMsg<std_msgs::String>, (void*)(&cal_params_msg_), true);
    while(lp.nextMsg()); //Load all the messages.
    cout << "Done." << endl;

    // -- Get the labeled mask out of the labeled images array.
    cout << "Extracting images...";
    bridge_mask_ = new CvBridge<std_msgs::Image>(&labeled_images_msg_.images[2], CvBridge<std_msgs::Image>::CORRECT_BGR | CvBridge<std_msgs::Image>::MAXDEPTH_8U);
    assert(bridge_mask_->to_cv(&mask_));

    bridge_left_ = new CvBridge<std_msgs::Image>(&labeled_images_msg_.images[1], CvBridge<std_msgs::Image>::CORRECT_BGR | CvBridge<std_msgs::Image>::MAXDEPTH_8U);
    assert(bridge_left_->to_cv(&left_));

    bridge_disp_ = new CvBridge<std_msgs::Image>(&videre_images_msg_.images[0], CvBridge<std_msgs::Image>::CORRECT_BGR | CvBridge<std_msgs::Image>::MAXDEPTH_8U);
    assert(bridge_disp_->to_cv(&disp_));
    cout << "Done." << endl;
  
/*     // -- Old time stamps anger rostf. */
/*     full_cloud_msg_.header.stamp = ros::Time::now(); */
/*     videre_cloud_msg_.header.stamp = ros::Time::now(); */
    
    // -- Propagate the labels.
    cout << "labeling videre with frame id " << videre_cloud_msg_.header.frame_id << endl;
    labelCloud(&videre_cloud_msg_);
  }

  void publishAll() {

/*     // -- Put the points from the cloud into the mask image. */
/*     for (unsigned int i=0; i<n; i++) { */
/*       int c = floor(projected(1, i+1)+.5); */
/*       int r = floor(projected(2, i+1)+.5); */
/*       if(r >= 0 && r < mask_->height && // */
/* 	 c >= 0 && c < mask_->width) { */
/* 	CvScalar s=cvGet2D(mask_,r,c); */
/* 	s.val[0] = 100; */
/* 	cvSet2D(mask_, r, c, s); */
/*       } */
/*     } */
/*     cvNamedWindow( "mask_", CV_WINDOW_AUTOSIZE); */
/*     cvShowImage("mask_", mask_); */
/*     cout << " waiting for keypress..." << endl; */
/*     cvWaitKey(); */

    // -- Display the images for debugging.
/*     cvNamedWindow( "mask_", 0); cvResizeWindow("mask_", mask_->width, mask_->height); */
/*     cvShowImage("mask_", mask_); */
/*     cvNamedWindow( "left_", CV_WINDOW_AUTOSIZE); */
/*     cvShowImage("left_", left_); */
/*     cvNamedWindow( "disp_", CV_WINDOW_AUTOSIZE); */
/*     cvShowImage("disp_", disp_); */

/*     cout << " waiting for keypress..." << endl; */
/*     cvWaitKey(); */

    // -- Publish the messages.
    cout << "Publishing... " << endl;
    videre_cloud_colored_ = colorPointCloud(videre_cloud_msg_); 
    publish("videre/cloud", videre_cloud_colored_);
    publish("labeled_images", labeled_images_msg_);
    publish("videre/images", videre_images_msg_);
  }



 private:

  CvBridge<std_msgs::Image> *bridge_mask_, *bridge_left_, *bridge_disp_;
  LogPlayer lp;
  
  void labelCloud(std_msgs::PointCloudFloat32 *ptcld) {
       
    // -- Set up smallv to image plane transformation.
    //Assumes the points are in mm:
    string cal = cal_params_msg_.data;
    string proj = cal.substr(cal.find("proj"), cal.find("rect"));

    NEWMAT::Real trnsele[12];

    //This is terrible.
    sscanf(proj.c_str(), "%*s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %*s", &trnsele[0], &trnsele[1], &trnsele[2], &trnsele[3], &trnsele[4], &trnsele[5], &trnsele[6], &trnsele[7], &trnsele[8], &trnsele[9], &trnsele[10], &trnsele[11]);
    
    NEWMAT::Matrix trns(3,4);
    trns << trnsele;
    cout << "trns: " << endl << trns << endl;

    // -- Put the videre_cloud_msg_ into a NEWMAT matrix and do the projection.
    unsigned int n = ptcld->get_pts_size();
    NEWMAT::Matrix vid(4, n);
    NEWMAT::Real videle[4*n];
    for (unsigned int i=0; i<n; i++) {
      videle[i] = ptcld->pts[i].x * 1000; //in millimeters
      videle[i + n] = ptcld->pts[i].y  * 1000;
      videle[i + 2*n] = ptcld->pts[i].z * 1000;
      videle[i + 3*n] = 1;
    }
    vid << videle;
    NEWMAT::Matrix projected = trns * vid;

    // -- Normalize so z = 1.
    for ( unsigned int i=1; i<=n; i++) {
      projected(1,i) = projected(1,i) / projected(3,i);
      projected(2,i) = projected(2,i) / projected(3,i);
      projected(3,i) = 1;
    } 

    /* cout << endl << "Max x: " << projected.Row(1).Maximum() << "   Min x: " << projected.Row(1).Minimum() << endl; */
/*     cout << endl << "Max y: " << projected.Row(2).Maximum() << "   Min y: " << projected.Row(2).Minimum() << endl; */
/*     cout << endl << "Max z: " << projected.Row(3).Maximum() << "   Min z: " << projected.Row(3).Minimum() << endl; */

    // -- Add the channel with the labeling.
    std_msgs::PointCloudFloat32 tmp;
    tmp.set_pts_size(n);
    tmp.set_chan_size(2);
    //cout << "copying " << ptcld->chan[0].name.data << endl;
    tmp.chan[0] = ptcld->chan[0];
    tmp.header = ptcld->header;
    //tmp.chan[0].set_vals_size(n);
    tmp.chan[1].set_vals_size(n);
    //tmp.chan[0].name = "intensities";
    tmp.chan[1].name = "labels";
    cout << "hxw " << mask_->height << " " << mask_->width << endl;
    for (unsigned int i=1; i<=n; i++) {
      tmp.pts[i-1] = ptcld->pts[i-1];
      int c = floor(projected(1, i)+.5);
      int r = floor(projected(2, i)+.5);
      if(r >= 0 && r < mask_->height && //
	 c >= 0 && c < mask_->width && //
	 cvGet2D(mask_, r, c).val[0] != 0)
	tmp.chan[1].vals[i-1] = cvGet2D(mask_, r, c).val[0];
      else
        tmp.chan[1].vals[i-1] = 0;
    }

    ptcld->set_chan_size(2);
    *ptcld = tmp;
    cout << "label: chan size is " << ptcld->get_chan_size() << " and npts is " << ptcld->get_pts_size() << endl;
  }

  std_msgs::PointCloudFloat32 colorPointCloud(std_msgs::PointCloudFloat32 ptcld) {
    if(ptcld.get_chan_size() == 1) {
      cout << "chan size is " << ptcld.get_chan_size() << endl;
      cout << "Not coloring..." << endl;
      return ptcld;
    }

    for (unsigned int i=0; i<ptcld.get_pts_size(); i++) {
      if(ptcld.chan[1].vals[i] != 0) 
	ptcld.chan[0].vals[i] = 255;
      else
	ptcld.chan[0].vals[i] = 100;
    }
    ptcld.chan[0].vals[0] = 0; //Because ogre vis scales the colors.

    return ptcld;

  }

};
#endif
