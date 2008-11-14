#!/usr/bin/env python
# Software License Agreement (BSD License)
#
# Copyright (c) 2008, Willow Garage, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided
#    with the distribution.
#  * Neither the name of the Willow Garage nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

## Gazebo test cameras validation 

PKG = 'gazebo_plugin'
NAME = 'testscan'

import math
import rostools
rostools.update_path(PKG)
rostools.update_path('rostest')
rostools.update_path('std_msgs')
rostools.update_path('robot_msgs')
rostools.update_path('rostest')
rostools.update_path('rospy')


import sys, unittest
import os, os.path, threading, time
import rospy, rostest
from std_msgs.msg import *


TARGET_RANGES = [
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 2.05170464516, 2.04536008835, 2.03909778595, 2.03291654587, 2.02681565285, 
2.02079415321, 2.01485085487, 2.00898480415, 2.0031952858, 1.99748134613, 1.99184203148, 1.98627638817, 
1.98078382015, 1.97536325455, 1.9700139761, 1.96473526955, 1.95952606201, 1.95438587666, 1.94931387901, 
1.94430923462, 1.93937122822, 1.93449926376, 1.92969250679, 1.92495036125, 1.92027211189, 1.91565704346, 
1.91110467911, 1.90661418438, 1.90218496323, 1.91215276718, 1.93008923531, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 2.64686512947, 2.6173157692, 2.59563803673, 2.57963132858, 2.5657658577, 
2.55445694923, 2.54415798187, 2.53618431091, 2.52831435204, 2.5224199295, 2.51713490486, 2.51194572449, 
2.50885725021, 2.50583004951, 2.50296163559, 2.50192046165, 2.50093340874, 2.5, 2.50093340874, 
2.50192022324, 2.50296139717, 2.50582957268, 2.50885748863, 2.51194500923, 2.51713514328, 2.52241945267, 
2.52831435204, 2.53618454933, 2.54415798187, 2.55445718765, 2.5657658577, 2.57963109016, 2.59563827515, 
2.6173157692, 2.64686512947, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 2.74098372459, 2.72118186951, 2.72313261032, 2.72514390945, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 1.85238564014, 1.80339097977, 1.77707922459, 1.75727534294, 1.74103367329, 
1.72714829445, 1.71498286724, 1.70415234566, 1.69440209866, 1.68555378914, 1.67747652531, 1.67007064819, 
1.66325867176, 1.6569788456, 1.65118086338, 1.64582335949, 1.64087188244, 1.63629746437, 1.63207530975, 
1.62818443775, 1.62460672855, 1.62132656574, 1.61833024025, 1.61560595036, 1.61314368248, 1.61093437672, 
1.60897052288, 1.60724556446, 1.6057536602, 1.60449028015, 1.60345125198, 1.60263371468, 1.60203504562, 
1.60165333748, 1.60148763657, 1.60153746605, 1.60180282593, 1.60228455067, 1.60298418999, 1.60390377045, 
1.60504591465, 1.60641443729, 1.60801339149, 1.60984802246, 1.61192440987, 1.61424958706, 1.61683177948, 
1.61968040466, 1.62280631065, 1.62622225285, 1.62994265556, 1.6339840889, 1.63836622238, 1.64311158657, 
1.64824676514, 1.65380322933, 1.65981829166, 1.66633713245, 1.6734149456, 1.68112039566, 1.68954002857, 
1.69878637791, 1.709010005, 1.72041964531, 1.73332083225, 1.74819290638, 1.76587283611, 1.7881039381, 
1.82019233704, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, 
10.0500001907, 10.0500001907, 10.0500001907, 10.0500001907, ]



class PointCloudTest(unittest.TestCase):
    def __init__(self, *args):
        super(PointCloudTest, self).__init__(*args)
        self.success = False


    def printPointCloud(self, cloud):
        print "["
        i = 0
        for pt in cloud.ranges:
            sys.stdout.write(str(pt) + ", ")
            i = i + 1
            if ((i % 7) == 0):
                print "" #newline
        print "]"


    def pointInput(self, cloud):
        i = 0
        print "Input laser scan received"
        self.printPointCloud(cloud)
        while (i < len(cloud.ranges) and i < len(TARGET_RANGES)):
            d = cloud.ranges[i] - TARGET_RANGES[i]
            if ((d < -0.03) or (d > 0.03)):
                return
            i = i + 1
        self.success = True
    
    def test_pointcloud(self):
        print "LNK\n"
        rospy.Subscriber("base_scan", LaserScan, self.pointInput)
        rospy.init_node(NAME, anonymous=True)
        timeout_t = time.time() + 5.0
        while not rospy.is_shutdown() and not self.success and time.time() < timeout_t:
            time.sleep(0.1)
        self.assert_(self.success)
        
    


if __name__ == '__main__':
    rostest.run(PKG, sys.argv[0], PointCloudTest, sys.argv) #, text_mode=True)


