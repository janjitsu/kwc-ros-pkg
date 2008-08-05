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

#ifndef HOKUYO_TESTER_H
#define HOKUYO_TESTER_H

#include "gen_hokuyo_tester.h"
#include "urg_laser.h"
#include <wx/log.h>
#include <wx/thread.h>
#include <wx/glcanvas.h>

class HokuyoTester;

class HokuyoThread : public wxThread
{
public:
    HokuyoThread(HokuyoTester *tester_) : tester(tester_) {}

    virtual void *Entry();

    virtual void OnExit();

public:
  HokuyoTester *tester;
};


class HokuyoTester : public GenHokuyoTester
{
  friend class HokuyoThread;

  wxMutex* urg_mutex;
  wxMutex* log_mutex;

  URG::laser urg;
  wxLogTextCtrl* log;
  wxGLCanvas *gl;

  bool m_init;
  float view_scale, view_x, view_y;

  URG::laser_scan_t  scan;
  HokuyoThread* thread;

  int last_x;
  int last_y;

protected:

  void InitGL();
  void Render();

  virtual void OnEraseBackground(wxEraseEvent& event);
  virtual void OnPaint(wxPaintEvent& event);
  virtual void OnSize(wxSizeEvent& event);
  virtual void OnConnect( wxCommandEvent& event );
  virtual void OnDisconnect( wxCommandEvent& event );
  virtual void OnScan( wxCommandEvent& event );
  virtual void OnMouse( wxMouseEvent& event );

  bool DoStopScan();
  bool DoStartScan();
  bool DoHandleScan();

public:
	/** Constructor */
	HokuyoTester( wxWindow* parent );
};

#endif
