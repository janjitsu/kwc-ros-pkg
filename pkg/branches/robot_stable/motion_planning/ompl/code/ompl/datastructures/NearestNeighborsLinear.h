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

/* \author Ioan Sucan */

#ifndef OMPL_DATASTRUCTURES_NEAREST_NEIGHBORS_LINEAR_
#define OMPL_DATASTRUCTURES_NEAREST_NEIGHBORS_LINEAR_

#include "ompl/datastructures/NearestNeighbors.h"

namespace ompl
{

    template<typename _T>
    class NearestNeighborsLinear : public NearestNeighbors<_T>
    {
    public:
        NearestNeighborsLinear(void) : NearestNeighbors<_T()
	{
	}
	
	virtual ~NearestNeighborsLinear(void)
	{
	}
	
	virtual void clear(void)
	{
	    m_data.clear();
	    m_active.clear();
	}

	virtual void add(_T &data)
	{
	    m_data.push_back(data);
	    m_active.push_back(true);
	}

	virtual bool remove(_T &data)
	{
	    for (int i = m_data.size() - 1 ; i >= 0 ; --i)
		if (m_data[i] == data)
		{
		    m_active[i] = false;
		    return true;
		}
	    return false;
	}
	
	virtual _T nearest(_T &data) const
	{
	    int pos = -1;
	    double dmin = 0.0;
	    for (unsigned int i = 0 ; i < m_data.size() ; ++i)
	    {
		if (m_active[i])
		{
		    double distance = (*NearestNeighbors<_T>::m_distFun)(m_data[i], data);
		    if (pos < 0 || dmin > distance)
		    {
			pos = i;
			dmin = distance;
		    }
		}
	    }
	    return pos >= 0 ? m_data[pos] : data;
	}
	
	virtual unsigned int size(void) const
	{
	    return m_data.size();
	}
	
	virtual void list(std::vector<_T> &data) const
	{
	    data = m_data;
	}
	
    protected:
	
	std::vector<_T>   m_data;
	std::vector<bool> m_active;
	
    };
    
    
}

#endif
