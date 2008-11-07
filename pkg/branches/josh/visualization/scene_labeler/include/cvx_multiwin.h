//-------------------------------------------------------------------------------
//
//    cvx_multiwin.h
//    OpenCV based code generating a multi-panel image window.
//    Copyright (C) 2006 Adrian Kaehler
//
//----------------------------------------------------------------------------
//
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Stanford University nor the
//       names of its contributors may be used to endorse or promote products
//       derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE AUTHORS AND CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------- 
//   Author: Adrian Kaehler
//
//   Description:
//
// This is the CvxMultiWin Class.  It is intended for helping in debugging activities
// with OpenCV.  The basic idea is that a CvxMultiWin is loosely analogous to a window
// generated by cvNamedWindow, but you can put multiple "panels" into it do display
// mutiple things, instead of having a desktop full of individual windows.
//
//  To create a multi-window, you should first create an instance of the CvxMultiWin
// object.  When using it for debugging, it is best to make it global, so many routines
// can write to it without having to pass pointers to the CvxMultiWin object all over
// the place.
// 
//  Once the object has been craeted, it must be initialized.  Initialization
// requires the size of the image panels to be set (they are all the
// same size!), and the number of images to be set (number of rows and
// number of columns).  The color depth and number of channels are also
// set at initialization time.
//
//   When the CvxMultiWin object has been initialized, it can be written to
// with the command CvxMultiWin::paint().  Paint requires at least an 
// IplImage* pointer for the image to copy into the CvxMultiWin.  Additionally,
// a single integer may be passed, which will be interpreted as a
// pannel number (scanning left to right, top to bottom).  If two numbers are
// provided, they will be interpreted as column, row indices for the panel
// to write to.  Finally, the fourth argument can be used to tell the
// object to only update the memory image and not actually redraw the
// CvxMultiWin.  This is useful if you wish to write to many panels before
// actually updating the window.  The window will be drawn to either when
// a paint request is sent with draw_now=true, or an explicit call to
// CvxMultiWin::redraw() is made.
//
//   If any doubt exists as to whether a CvxMultiWin is initialized, a
// utility membwer CvxMultiWin::initialized() will return a boolean to 
// indicate if it has been or it has not been.
//
//   This code should be understood to be developmental, and may contain bugs.
// Users are invited to make any changes they see fit, and requested to
// make modified or debugged versions generally available to all interested
// parties.  Please maintain a change log in the headder.
//
// CHANGE LOG:
// 5/5/06	Gary Bradski added in setable image origin.
// 5/5/06   Gary Bradski added keeping draw panel within bounds
// 5/7/06	Gary Bradski, plugged memory leaks -- dealocate labels and M_FONT
// 5/9/06   Adrian Kaehler, removed Garys deallocation of static(!) member M_FONT
// 5/10/06	Adrian Kaehler, added visualization method for single channel IPL_DEPH_16S
// 5/24/06  Gary Bradski, added "panelXY" utility to convert mouse click to panel x,y and mouse click relative to that panel
// 10/1/06  Gary Bradski, added STL vector to keep track of each panel's source image width and height. 
// 10/3/06	Gary Bradski, modified panelXY to also return that mouse click's x_image, y_image: x,y relative to source image
// 11/27/06 Gary Bradski, Bug fix when mouse on outer borders
//
//----------------------------------------------------------------------------

#ifndef H_CVX_MULTIWIN
#define H_CVX_MULTIWIN

#include <highgui.h>
#include <cv.h>
#include <stdio.h>
#include "cvx_defs.h"
#include <vector>


using namespace std;

class CvxMultiWin {

private:

	char			m_szWindowName[256];
	int				m_width,		m_height;
	int				m_panel_width,	m_panel_height;
	vector<int>		m_source_img_width;  //Stores the width and height of the source images in the panels (these are set in paint)
	vector<int>		m_source_img_height;
	int				m_rows;
	int				m_columns;
	int				m_origin;  //0 - top-left origin, 1-bottom-left origin
	IplImage*		m_image;
	int				m_implicit_target;
	bool			m_initialized;
	char**			m_labels;

    static bool		M_CLASS_INITIALIZED;
	static int		M_SPC;
	static CvFont*	M_FONT;

public:

	// Default constructor, basically does nothing.  (Note that a window
	// is NOT created on the screen until CvxMultiWin::initialize() is
	// called.
	//
	CvxMultiWin() {
		m_image = NULL;
		m_initialized=false;
		if( M_FONT==NULL ) {
			M_FONT = new CvFont;
			cvInitFont( 
				M_FONT,				// CvFont* font, 
				CV_FONT_VECTOR0,	// CvFontFace fontFace, 
				1.0f,				// float hscale,
                1.0f,				// float vscale, 
				0.0f,				// float italicScale, 
				1					// int thickness 
			);
		}
	}

	// Get rid of the window and free the memory needed for the
	// multi-panel image.
	//
	~CvxMultiWin() {
		if( m_image != NULL ) cvReleaseImage( &m_image );
		cvDestroyWindow( m_szWindowName );
		if(m_initialized)
		{
			for( int i=0; i<m_rows*m_columns; i++ ) { 
				delete [] m_labels[i];
			}
			delete [] m_labels;
		}
	}

	// The user is required to make an initialize() call before using
	// the multi-window.  This call should indicate the name for the
	// window (as per cvNamedWindow()), as well as the width and height
	// of the panel images.  c,r specify the number of columns and
	// rows (respectively) of panels to create.  depth and channels
	// are passed directly to the multi-panel image.
	// origin can now be user set to flip upside down images
	// IPL_ORIGIN_TL is 0 Just FYI
	//
	bool initialize( 
		const char* name,
		int w,
		int h,
		int c			= 2,
		int r			= 2,
		int depth		= 8,	
		int channels	= 3,
		int flags		= 0,
		int origin		= IPL_ORIGIN_TL
	);
	bool initialize(
		const char* name,
		IplImage*	ref,
		int			c		= 2,
		int			r		= 2,
		int			flags	= 0,
		int			origin  = IPL_ORIGIN_TL
	) {
		return initialize( 
			name, 
			ref->width, 
			ref->height, 
			c, 
			r, 
			ref->depth, 
			ref->nChannels,
			flags,
			origin
		);

	}

	void set_label( int c, int r, const char* txt ) {
		if( ! m_initialized ) return;
		if(c >= m_columns) c = m_columns -1;
		if(c< 0) c = 0;
		if(r >= m_rows) c = m_rows - 1;
		if(r<0) r= 0;
//		if(m_origin == IPL_ORIGIN_BL) r = m_rows - 1 - r;
		strncpy( m_labels[r*m_columns+c], txt, 256 );
	}
	void set_label( const char* txt ) {
		if( ! m_initialized ) return;
		int implicit_max = m_rows*m_columns;
		if(m_implicit_target >= implicit_max) m_implicit_target = 0;
		if(m_implicit_target < 0) m_implicit_target = 0;
		int c = (m_implicit_target) % m_columns;
		int r = (m_implicit_target) / m_columns;
//		if(m_origin == IPL_ORIGIN_BL) r = m_rows - 1 - r;
		strncpy( m_labels[r*m_columns+c], txt, 256 );
	}

	// Here the caller gives an image and we will draw it into multiwindow.
	// If the caller omits the parameter "r", then the argument "rc" is
	// interpreted as the position in raster scan order.  If "r" is supplied,
	// then rc is interpreted as the column, and r is the row.
	// If 'rc' is not provided either, then the image is drawn to m_implicit_target,
	// which is then incremented.  m_implicit_target is cleared on a draw
	// command or if it's out of bounds
	//
	//	To visualize floating point images, optional values allow you to:
	//  min_value_float		- This and anything less will become zero
	//	max_value_float		- This and anything greater will become 255
	//						All else will be scaled proportionately
	//
	void paint( IplImage* in, int rc=-1, int r=-1, bool redraw_now=false, float min_value_float = 0.0, float max_value_float = 1.0 );

	// Sopmetimes someone wants to know if *this is initialized.  Usually this
	// is to avoid initializing twice!
	//
	bool initialized() { return( m_initialized ); }

	void redraw() {

		if( ! m_initialized ) return;
		
		if( m_image != NULL ) {
			m_implicit_target=0;
			cvShowImage( m_szWindowName, m_image );
		}
	
	}

	// Sometimes you just want to know what the window was named... this is
	// usually because you are planning to add a slider or something like
	// that.
	//
	const char* name( void ) { 	
		return m_szWindowName; 
	}

	int panel_width()  { return m_panel_width; }
	int panel_height() { return m_panel_height; }

	CvFont* font() { return M_FONT; }
    const IplImage* frame() const { return m_image; }

	//Allow reading and writing of master window origin (deal with upside down video)
	// origin must be either IPL_ORIGIN_TL (0) or IPL_ORIGIN_BL (1)
	void set_origin(int origin)
	{
		if(origin != IPL_ORIGIN_TL) 
			m_origin = IPL_ORIGIN_BL;
		else 
			m_origin = IPL_ORIGIN_TL;
		if( ! m_initialized ) return;
		m_image->origin = m_origin;
	}
	int get_origin() { return m_origin;}

	// ... or, more convieniently, just flip the origin and return it
	int flip()
	{
		if(m_origin == IPL_ORIGIN_TL)
			m_origin = IPL_ORIGIN_BL;
		else
			m_origin = IPL_ORIGIN_TL;
		return m_origin;
	}


	//MULTI-WINDOW MOUSE SUPPORT  
	// You may use cvSetMouseCallback and set up a mouse call back function
	// with a multiwin.  The call back function has the basic prototype:
	//
	//   void mouseCallBack( int event, int x, int y, int flags,void *foo = NULL )
	// 
	// EXAMPLE:
	// CvxMultiWin mw; //Multi-window 
	// ...
	//			mw.initialize("Histo", image->width, image->height, 2,2,8,3,1,IPL_ORIGIN_BL); // set up 2x2 mw
	//			mw.set_label( 0, 0, "c0 r0" ); // label each panel
	//			mw.set_label(1,0,"c1 r0");
	//			mw.set_label(0,1,"c0 r1");
	//			mw.set_label(1,1,"c1 r1");
	//		    cvSetMouseCallback( "Histo", on_mousemw ); //Make "on_mousemw" the mouse call back funtion
	//
	// A mouse event in the multiwindow will now call on_mousemw with the x,y image (not window) coordinate of the mouse
	// The problem is that x and y refer to the whole window.  There is also the 
	// problem of getting the image origin right.  Below are support functions to
	// translate x & y to to coordinates within the scaled window and to draw or sample
	// from the associated panel source image if you pass a pointer to it.

	
	//Given a mouse x,y event in the multiwindow, 
	// calculate the panel Col, Row and x,y offset in that panel's coordinates
	// Return: m_origin of the multiwindow, or < 0 => mouse is in panel border region
	//         -1 => not initialized
	// NOTES:
	//  * You might have to adjust panel's to height - y depending on your associated image's origin.
	//  * x and y are in panel coordinates which are possibly rescaled from the image you passed in.
	//    This x and y are for an image of m_panel_width by m_panel_height you may have to rescale to
	//    in order to get the intended x,y target in the associated image.
    //	* Two versions of this algorithm for backward compatability, one with and one without image_x, image_y
	int panelXY(int &x, int &y, int &panelCol, int &panelRow);
	int panelXY(int &x, int &y, int &panelCol, int &panelRow, int &image_x, int &image_y);


};









#endif



