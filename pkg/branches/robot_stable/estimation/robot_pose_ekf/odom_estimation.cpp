/*********************************************************************
* Software License Agreement (BSD License)
* 
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
* 
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
* 
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
* 
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

#include "odom_estimation.h"

//#define __EKF_DEBUG_COUT__


using namespace MatrixWrapper;
using namespace BFL;
using namespace tf;
using namespace std;
using namespace ros;


namespace estimation
{
  // constructor
  odom_estimation::odom_estimation():
    _prior(NULL),
    _filter(NULL),
    _filter_initialized(false),
    _odom_initialized(false),
    _imu_initialized(false),
    _vo_initialized(false)
  {
    // create SYSTEM MODEL
    ColumnVector sysNoise_Mu(6);  sysNoise_Mu = 0;
    SymmetricMatrix sysNoise_Cov(6); sysNoise_Cov = 0;
    sysNoise_Cov(1,1) = pow(1000.0,2);
    sysNoise_Cov(2,2) = pow(1000.0,2);
    sysNoise_Cov(3,3) = pow(1000.0,2);
    sysNoise_Cov(4,4) = pow(1000.0,2);
    sysNoise_Cov(5,5) = pow(1000.0,2);
    sysNoise_Cov(6,6) = pow(1000.0,2);  
    Gaussian system_Uncertainty(sysNoise_Mu, sysNoise_Cov);
    _sys_pdf   = new NonLinearAnalyticConditionalGaussianOdo(system_Uncertainty);
    _sys_model = new AnalyticSystemModelGaussianUncertainty(_sys_pdf);
    

    // create MEASUREMENT MODEL ODOM
    ColumnVector measNoiseOdom_Mu(3);  measNoiseOdom_Mu = 0;
    SymmetricMatrix measNoiseOdom_Cov(3);  measNoiseOdom_Cov = 0;
    measNoiseOdom_Cov(1,1) = pow(0.001,2);
    measNoiseOdom_Cov(2,2) = pow(0.001,2);
    measNoiseOdom_Cov(3,3) = pow(0.01,2);
    Gaussian measurement_Uncertainty_Odom(measNoiseOdom_Mu, measNoiseOdom_Cov);
    Matrix Hodom(3,6);  Hodom = 0;
    Hodom(1,1) = 1;    Hodom(2,2) = 1;    Hodom(3,6) = 1;
    _odom_meas_pdf   = new LinearAnalyticConditionalGaussian(Hodom, measurement_Uncertainty_Odom);
    _odom_meas_model = new LinearAnalyticMeasurementModelGaussianUncertainty(_odom_meas_pdf);
    

    // create MEASUREMENT MODEL IMU
    ColumnVector measNoiseImu_Mu(3);  measNoiseImu_Mu = 0;
    SymmetricMatrix measNoiseImu_Cov(3);  measNoiseImu_Cov = 0;
    measNoiseImu_Cov(1,1) = pow(0.001,2);
    measNoiseImu_Cov(2,2) = pow(0.001,2);
    measNoiseImu_Cov(3,3) = pow(0.001,2);
    Gaussian measurement_Uncertainty_Imu(measNoiseImu_Mu, measNoiseImu_Cov);
    Matrix Himu(3,6);  Himu = 0;
    Himu(1,4) = 1;    Himu(2,5) = 1;    Himu(3,6) = 1;
    _imu_meas_pdf   = new LinearAnalyticConditionalGaussian(Himu, measurement_Uncertainty_Imu);
    _imu_meas_model = new LinearAnalyticMeasurementModelGaussianUncertainty(_imu_meas_pdf);


    // create MEASUREMENT MODEL VO
    ColumnVector measNoiseVo_Mu(6);  measNoiseVo_Mu = 0;
    SymmetricMatrix measNoiseVo_Cov(6);  measNoiseVo_Cov = 0;
    measNoiseVo_Cov(1,1) = pow(0.01,2);
    measNoiseVo_Cov(2,2) = pow(0.01,2);
    measNoiseVo_Cov(3,3) = pow(0.01,2);
    measNoiseVo_Cov(4,4) = pow(0.001,2);
    measNoiseVo_Cov(5,5) = pow(0.001,2);
    measNoiseVo_Cov(6,6) = pow(0.001,2);
    Gaussian measurement_Uncertainty_Vo(measNoiseVo_Mu, measNoiseVo_Cov);
    Matrix Hvo(6,6);  Hvo = 0;
    Hvo(1,1) = 1;    Hvo(2,2) = 1;    Hvo(3,3) = 1;    Hvo(4,4) = 1;    Hvo(5,5) = 1;    Hvo(6,6) = 1;
    _vo_meas_pdf   = new LinearAnalyticConditionalGaussian(Hvo, measurement_Uncertainty_Vo);
    _vo_meas_model = new LinearAnalyticMeasurementModelGaussianUncertainty(_vo_meas_pdf);
  };



  // destructor
  odom_estimation::~odom_estimation(){
    if (_filter) delete _filter;
    if (_prior)  delete _prior;
    delete _odom_meas_model;
    delete _odom_meas_pdf;
    delete _imu_meas_model;
    delete _imu_meas_pdf;
    delete _vo_meas_model;
    delete _vo_meas_pdf;
    delete _sys_pdf;
    delete _sys_model;
  };


  // initialize prior density of filter with odom data
  void odom_estimation::Initialize(const Transform& prior, const Time& time)
  {
    // set prior of filter to odom data
    ColumnVector prior_Mu(6); 
    prior_Mu(1) = prior.getOrigin().x();  prior_Mu(2) = prior.getOrigin().y(); prior_Mu(3) = prior.getOrigin().z(); 
    prior.getBasis().getEulerZYX(prior_Mu(6), prior_Mu(5), prior_Mu(4));

#ifdef __EKF_DEBUG_COUT__
    cout << "Prior " << prior_Mu << endl;
#endif
    SymmetricMatrix prior_Cov(6); 
    for (unsigned int i=1; i<=6; i++) {
      for (unsigned int j=1; j<=6; j++){
	if (i==j)  prior_Cov(i,j) = pow(0.001,2);
	else prior_Cov(i,j) = 0;
      }
    }
    _prior  = new Gaussian(prior_Mu,prior_Cov);
    _filter = new ExtendedKalmanFilter(_prior);

    // remember prior
    _filter_estimate_old_vec = prior_Mu;
    _filter_estimate_old = prior;
    _filter_time_old     = time;

    // filter initialized
    _filter_initialized = true;
  }





  // update filter
  void odom_estimation::Update(const Time& odom_time, bool odom_active,
			       const Time& imu_time,  bool imu_active,
			       const Time& vo_time,   bool vo_active, const Time&  filter_time)
  {
    if (_filter_initialized){
      // system update filter
      ColumnVector vel_desi(2); vel_desi = 0;
      _filter->Update(_sys_model, vel_desi);


      // process odom measurement
      if (odom_active){
	_transformer.lookupTransform("base","odom", odom_time, _odom_meas);
	if (_odom_initialized){
#ifdef __EKF_DEBUG_COUT__
	  cout << "init filter estimate old vec  " << _filter_estimate_old_vec << endl;
	  cout << "init odom meas " << _odom_meas.getOrigin().x() << " "  << _odom_meas.getOrigin().y() << endl;
	  cout << "init odom meas old " << _odom_meas_old.getOrigin().x() << " "  << _odom_meas_old.getOrigin().y() << endl;
#endif
	  // convert absolute odom measurements to relative odom measurements in horizontal plane
	  Transform odom_rel_frame =  Transform(Quaternion(_filter_estimate_old_vec(6),0,0),_filter_estimate_old.getOrigin()) * _odom_meas_old.inverse() * _odom_meas;
	  ColumnVector odom_rel(3);
	  odom_rel(1) = odom_rel_frame.getOrigin().x();   odom_rel(2) = odom_rel_frame.getOrigin().y(); 
	  double tmp; odom_rel_frame.getBasis().getEulerZYX(odom_rel(3), tmp, tmp);
	  AngleOverflowCorrect(odom_rel(3), _filter_estimate_old_vec(6));
#ifdef __EKF_DEBUG_COUT__
	  cout << "odom rel  " << odom_rel << endl;
#endif
	  // update filter
	  _filter->Update(_odom_meas_model, odom_rel);
	}
	else _odom_initialized = true;
#ifdef __EKF_DEBUG_COUT__
	cout << "set odom meas " << _odom_meas.getOrigin().x() << " "  << _odom_meas.getOrigin().y() << endl;
	cout << "set odom meas old " << _odom_meas_old.getOrigin().x() << " "  << _odom_meas_old.getOrigin().y() << endl;
#endif
	_odom_meas_old = _odom_meas;
      }


      // process imu measurement
      if (imu_active){
	_transformer.lookupTransform("base","imu", imu_time, _imu_meas);
	if (_imu_initialized){
	  // convert absolute imu yaw measurement to relative imu yaw measurement 
	  Transform imu_rel_frame =  _filter_estimate_old * _imu_meas_old.inverse() * _imu_meas;
	  ColumnVector imu_rel(3); double tmp;
	  imu_rel_frame.getBasis().getEulerZYX(imu_rel(3), tmp, tmp);
	  _imu_meas.getBasis().getEulerZYX(tmp, imu_rel(2), imu_rel(1));
	  AngleOverflowCorrect(imu_rel(3), _filter_estimate_old_vec(6));
#ifdef __EKF_DEBUG_COUT__
	  cout << "imu   " << imu_rel << endl;
#endif
	  // update filter
	  _filter->Update(_imu_meas_model,  imu_rel);
	}
	else _imu_initialized = true;
	_imu_meas_old = _imu_meas; 
      }


      // process vo measurement
      if (vo_active){
	_transformer.lookupTransform("base","vo", vo_time, _vo_meas);
	if (_vo_initialized){
	  // convert absolute vo measurements to relative vo measurements
	  Transform vo_rel_frame =  _filter_estimate_old * _vo_meas_old.inverse() * _vo_meas;
	  ColumnVector vo_rel(6);
	  vo_rel(1) = vo_rel_frame.getOrigin().x();  vo_rel(2) = vo_rel_frame.getOrigin().y();  vo_rel(3) = vo_rel_frame.getOrigin().z();  
	  vo_rel_frame.getBasis().getEulerZYX(vo_rel(6), vo_rel(5), vo_rel(4));
	  AngleOverflowCorrect(vo_rel(6), _filter_estimate_old_vec(6));
	  // update filter
	  _filter->Update(_vo_meas_model,  vo_rel);
	}
	else _vo_initialized = true;
	_vo_meas_old = _vo_meas;
      }


      // remember last estimate
      _filter_estimate_old_vec = _filter->PostGet()->ExpectedValueGet();
      _filter_estimate_old = Transform(Quaternion(_filter_estimate_old_vec(6), _filter_estimate_old_vec(5), _filter_estimate_old_vec(4)),
				       Vector3(_filter_estimate_old_vec(1), _filter_estimate_old_vec(2), _filter_estimate_old_vec(3)));
      _filter_time_old = filter_time;
      
    }
  };


  void odom_estimation::AddMeasurement(const tf::Transform& meas, const string& name, const string& base, const ros::Time& time)
  {
    _transformer.setTransform( Stamped<Transform>(meas, time, name, base) );
  };


  // get filter posterior as vector
  void odom_estimation::GetEstimate(ColumnVector& estimate, Time& time)
  {
    estimate = _filter_estimate_old_vec;
    time = _filter_time_old;
  };

  // get filter posterior as Transform
  void odom_estimation::GetEstimate(Transform& estimate, Time& time)
  {
    estimate = _filter_estimate_old;
    time = _filter_time_old;
  };

  // get filter posterior as PoseStamped
  void odom_estimation::GetEstimate(std_msgs::PoseStamped& estimate)
  {
    PoseTFToMsg(_filter_estimate_old, estimate.pose);
    estimate.header.stamp = _filter_time_old;
  };

  // correct for angle overflow
  void odom_estimation::AngleOverflowCorrect(double& a, double ref)
  {
    while ((a-ref) >  M_PI) a -= 2*M_PI;
    while ((a-ref) < -M_PI) a += 2*M_PI;
  };


}; // namespace