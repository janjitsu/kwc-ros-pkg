//Software License Agreement (BSD License)

//Copyright (c) 2008, Willow Garage, Inc.
//All rights reserved.

//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:

// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
//   copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided
//   with the distribution.
// * Neither the name of the Willow Garage nor the names of its
//   contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.

//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.

/** \mainpage libTF a transformation library
 *
 * This is a library for keeping track of transforms for 
 * an entire system.
 * 
 * The project page for this can be found <a href="http://pr.willowgarage.com/wiki/Transformations"> here.</a>
 * The most recent manual is in it's source latex in libTF/doc.  It will be 
 * generated by calling make in that directory.  
 *
 * To use libTF on its own the main way to interact with libTF is through the class libTF::TransformReference
 * This class provides both mutators to add data as well as accessors to get data.  
 *
 * For use within a ROS framework using the rosTF wrapper is suggested.  

 *\section overview Overview
 *libTF is designed to provide a simple interface for keeping track 
 *of coordinate transforms within a robotic framework.  
 *A simple case which was one of the driving design considerations was
 *the use of a sensor on an actuated platform.  In this case the sensor
 *will report data in the frame of the moving platform, but the data is much 
 *more useful in the world frame. To provide the transform libTF will be told 
 *the position of the platform periodically as it moves.  And when prompted libTF can 
 *provide the transform from the sensor frame to the world frame at the time when the
 *sensor data was acquired.  Not only will libTF will keep track of the transform between 
 *two coordinate frames, but it will also keep track of a whole tree of coordinate frames, 
 *and automatically chain transformations together between any two connected frames.  

 *\section terms Terminology and Conventions
 *\subsection cf Coodinate Frame
 *In this documentation the default 
 *will be a right handed system with X forward, Y left and Z up. 

 * \subsection dh Denavit-Hartenberg Parameters (DH Parameters)
 *DH Parameters are a way to concicely represent a rigid body tranformation in three dimentions.  
 *It has four parameters: length, twist, offset, and angle.  In addition to using the optimal 
 *amount of data to store the transformation, there are also optimized methods of chaining 
 *transformations together.  And lastly the parameters can directly represent rotary and prismatic
 *joints found on robotic arms.  
 *See http://en.wikipedia.org/wiki/Denavit-Hartenberg_Parameters for more details.  

 \todo Reproduce labeled diagram a good example http://uwf.edu/ria/robotics/robotdraw/DH_parm.htm

\subsection ea Euler Angles
For this library Euler angles are considered to be translations in x, y, z, followed by a rotation around z, y, x.
With the respective angular changes referred to as yaw, pitch and roll. 

\subsection Homogeneous Transformation Matrix
Homogeneous Transformation Matrices are a simple way to manipulate 3D translations and rotations 
with standard matrix multiplication.  It is a composite of a standard 3x3 rotation matrix
(see http://en.wikipedia.org/wiki/Rotation_matrix) and a translation vector.  

Let \f$_1R_0\f$ be the 3x3 rotation matrix defined by the Euler angles \f$(yaw_0,pitch_0,roll_0)\f$ 
and let \f$_1T_0\f$ be the column vector \f$(x_0,y_0,z_0)^T\f$ representing the translation.  The combination 
of these two transformations results in the transformation of reference frame 0 to reference frame 1.

A point P in frame 1, \f$P_1\f$, can be transformed into frame 1, \f$P_0\f$, by the following:

\f[
\left(
\begin{array}{ccc}
P_0 \\
1
\end{array}
\right)
=
\left(\begin{array}{ccc}
_0R_1 & _0T_1 \\
1 & 1
\end{array}
\right)\left(
\begin{array}{ccc}
P_1 \\
1
\end{array}\right)
\f]


The net result is a 4x4 transformation matrix which does both the rotation and translation 
between coordinate frames. The basic approach is to use a 4x1 vector consisting of \f$(x,y,z,1)^T_1\f$ 
and by left multiplying by \f$_0A_1\f$ it will result in \f$(x',y',z',1)^T_1\f$.

The matrix library used within this library is Newmat10.


\subsection newmat Newmat10
Newmat10 is the matrix library used in this library.  Documentation for Newmat can be found at 
http://www.robertnz.net/nm10.htm.  

\subsection popint Point
Within this documentation a point is considered a 3D representation of a position within a 
frame.  It is notated, as x, y, z.  

\subsection time Time Representation
libTF uses an \e unsigned \e long \e long (equivilant to a \e uint64_t to represent nanoseconds since the epoch(1970).  
It is named libTF::ULLtime for clarity.

rosTF converts automatically to the above representation from a time pair of seconds and nanoseconds since the epoch, to 
be consistant with the rest of ROS.  

\subsection vector Vector
Within this library a vector is a representation of a direction.  It is represented with the components in 3 directions, 
x,y,z.  However when subject to transformation, a vector will only be subject to rotations and will remain attached to the 
origin.  For example a point (1,1,1) subject to a translation of 5 in the x direction and yawed by 90 degrees would end
at (5,1,1), while a vector (1,1,1) subject to the same transformation would be (-1,1,1).  

\subsection data Data Types

\section internalmath Summary of Internal Mathematics
This libarary uses quaternion notation as the internal representation
of coordinate transforms.  Transform information is stored in a linked 
list sorted by time.  When a transform is requested the closest two points 
on the linked list are found and then interpolated in time to generate the 
return value.  

\subsection Storage 
The internal storage for the transforms is in quaternion notation.  The data structure is libTF::Pose3D

\subsection Interpolation 
The interpolation method used in this library is Spherical Linear Interpolation
(SLERP). \b SLERP The standard formula for SLERP is defined below.
The inputs are points \f$p_0\f$ and \f$p_1\f$, and \f$t\f$ is the proportion to interpolate 
between \f$p_0\f$ and \f$p_1\f$, and \f$\Omega\f$ is the angle between the axis of the two quaternions. 

\f[
Slerp(p_0,p_1;t) = \frac{sin((1-t)\Omega)}{sin(\Omega)} * p_0 + \frac{sin(t*\Omega)}{sin(\Omega)} * p_1
\f]


\todo add graphic

\subsubsection  Alternatives
The SLERP technique was developed in 1985 by Ken Shoemake. http://graphics.ucmerced.edu/~mkallmann/courses/papers/Shoemake-quattut.pdf
SLERP has been 
largely adopted in the field of computer graphics, however there have been many alternatives 
developed.  The advantages of SLERP are that it provides a constant speed solution 
along the shortest path over the 4D unit sphere.  This is not optimal in all cases however.
A good discussion of various alternatives is here http://number-none.com/product/Understanding%20Slerp,%20Then%20Not%20Using%20It/index.html 
The approaches discussed above are normalized linear interpolation (nlerp), and 
log-quaternion linear interpolation (log-quaternion lerp). 
SLERP can be recursively called to do a cubic interpolation, called SQUAD for short.  
This is discussed at http://www.sjbrown.co.uk/?article=quaternions.


\subsection  conversions Data Representation Conversion
\todo fill this out
\subsubsection mtoq Matrix to Quaternion
\todo fill this out
\subsubsection qtom Quaternion to Matrix
\todo fill this out
\subsubsection dhtom DH Parameters to Matrix
\todo fill this out
\subsubsection etom Euler Angles to Matrix
\todo fill this out
 */


