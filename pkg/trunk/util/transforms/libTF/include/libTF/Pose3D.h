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

#ifndef POSE3D_HH
#define POSE3D_HH

#include <iostream>
#include <vector>
#include <newmat10/newmat.h>
#include <newmat10/newmatio.h>
#include <pthread.h>


namespace libTF
{
  /** \brief A class used to store and do basic minipulations of 3D transformations
   * 
   */
  class Pose3D 
  {
      friend class Pose3DCache;
      
  public:
      /** \brief A struct to return the translational component */
      struct Position
      {
        double x,y,z;
      };
  
      /** \brief A struct to return the quaternion component */
      struct Quaternion
      {
        double x,y,z,w;
      };
      /** \brief A struct to return the Euler angles */
      struct Euler
      {
        double yaw, pitch, roll;
      };
  
      /* Constructors */
      /** \brief Empty Constructor initialize to zero */
      Pose3D();
      /** \brief Translation only constructor */
      Pose3D(double xt, double yt, double zt); 
      /** \brief  Quaternion only constructor */
      Pose3D(double xr, double yt, double zt, double w);
      /** \brief Translation and Quaturnion constructor */
      Pose3D(Position &pos, Quaternion &quat);
      /** \brief Translation and Quaturnion constructor */
      Pose3D(double xt, double yt, double zt, 
             double xr, double yt, double zt, double w);
  
      // Utility functions to normalize and get magnitude.
      /** \brief Normalize the quaternion */
      void normalize(void);
      /** \brief Get the magnitude of the Quaternion 
       * used for normalization */
      double getMagnitude(void);
      /** \brief Assignment operator overload*/
      Pose3D & operator=(const Pose3D & input);
  
    
      /* Accessors */
      /** \brief Return the transform as a matrix */
      NEWMAT::Matrix asMatrix();
      /** \brief Return the inverse of the transform as a matrix */
      NEWMAT::Matrix getInverseMatrix(void);
      /** \brief Return the rotation as a quaternion */
      Quaternion getQuaternion(void) const;
      /** \brief Return the rotation as a quaternion */
      void getQuaternion(Quaternion &quat) const;
      /** \brief Return the translation as a position */
      void getPosition(Position &pos) const;
      /** \brief Return the translation as a position */
      Position   getPosition(void) const;
      /** \brief Return the rotation as an axis angle pair */
      void getAxisAngle(double axis[3], double *angle) const;
      
      /** Mutators **/
      /** \brief Set the values to the identity transform */
      void setIdentity(void);      
      /** \brief Set the values from a matrix */
      void setFromMatrix(const NEWMAT::Matrix& matIn);
      /** \brief Set the values using Euler angles */
      void setFromEuler(double _x, double _y, double _z, double _yaw, double _pitch, double _roll);
      /** \brief Set the values using Euler angles */
      void setFromEuler(Position &pos, Euler &euler);
      /** \brief Set the values using DH Parameters */
      void setFromDH(double length, double alpha, double offset, double theta);

      /** \brief Set the translational components */
      void setPosition(double x, double y, double z);
      /** \brief Set the translational components */
      void setPosition(Position &pos);
      /** \brief Set the rotational components */
      void setQuaternion(double x, double y, double z, double w);
      /** \brief Set the rotational components */
      void setQuaternion(Quaternion &quat);
      /** \brief Set the quaterion from an axis-angle representation */
      void setAxisAngle(double ax, double ay, double az, double angle);
      /** \brief Set the quaterion from an axis-angle representation */
      void setAxisAngle(double axis[3], double angle);
      
      /** \brief Set the translational components */
      void addPosition(double x, double y, double z);
      /** \brief Set the translational components */
      void addPosition(Position &pos);
      /** \brief Set the rotational components */
      void multiplyQuaternion(double x, double y, double z, double w);
      /** \brief Set the rotational components */
      void multiplyQuaternion(Quaternion &quat);
      
      /** \brief Apply another pose to the transform contained in the current pose (transform multiplication) */
      void multiplyPose(Pose3D &pose);
      

      /** Application of the transform **/
      /** \brief Apply the stored transform to a point */
      void applyToPosition(Position &pos) const;
      /** \brief Apply the stored transform to a vector of points */
      void applyToPositions(std::vector<Position*> &posv) const;
      
      /**************** Static Helper Functions ***********************/
      /** \brief Convert DH Parameters to a Homogeneous Transformation Matrix */
      static NEWMAT::Matrix matrixFromDH(double length, double alpha, double offset, double theta);
      /** \brief Convert Euler Angles to a Homogeneous Transformation Matrix */
      static NEWMAT::Matrix matrixFromEuler(double ax, double ay, double az,
					    double yaw, double pitch, double roll);
      /** \brief isolate Euler Angles from a homogenous transform matrix */
      static Euler    eulerFromMatrix(const NEWMAT::Matrix & matrix_in, unsigned int solution_number=1);
      /** \brief isolate translational change from a homogeneous transform matrix */
      static Position positionFromMatrix(const NEWMAT::Matrix & matrix_in);
      
  protected:
      
      /** Internal Data Storage*/    
      double xt, yt, zt, xr, yr, zr, w;

  };


  /** \brief A namespace ostream overload for displaying poses */
  std::ostream & operator<<(std::ostream& mystream, const Pose3D &pose);

}


#endif //POSE3D_HH
