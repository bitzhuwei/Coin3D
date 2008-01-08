/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2008 by Kongsberg SIM.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Kongsberg SIM about acquiring
 *  a Coin Professional Edition License.
 *
 *  See http://www.coin3d.org/ for more information.
 *
 *  Kongsberg SIM, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  http://www.sim.no/  sales@sim.no  coin-support@coin3d.org
 *
\**************************************************************************/

#include "profiler/SbProfilingData.h"

/*!
  Constructor
*/

SbProfilingData::SbProfilingData()
{
}

/*!
  \brief Add profiling data from another data set.

  Add the data from \a rhs by applying the following rules:
  For each NodeProfile in \a rhs:
    If we allready have data for the NodeProfile:
      Update the has_glcache field from \a rhs as it's considered to 
      be the most recent.
      For each child timing in the NodeProfile in \a rhs:
         If we allready have timing data for the child:
           Accumulate the time
         Otherwise:
           Copy the child timing from \a rhs
    Otherwise:
      Copy the NodeProfile from \a rhs.
 */
SbProfilingData &
SbProfilingData::operator+=(const SbProfilingData & rhs)
{
  assert(this != &rhs);

  NodeProfileMap::const_iterator it, end;
  for (it = rhs.profile_map.begin(), end = rhs.profile_map.end();
       it != end; ++it) {
    NodeProfileMap::iterator thisit = this->profile_map.find(it->first);
    if (thisit != this->profile_map.end()) {
      // merge NodeProfiles
      thisit->second->has_glcache = it->second->has_glcache;
      
      ChildTimingMap::const_iterator rhs_child_it, rhs_child_end;
      for (rhs_child_it = it->second->child_map.begin(),
             rhs_child_end = it->second->child_map.end();
           rhs_child_it != rhs_child_end; ++rhs_child_it) {
        ChildTimingMap::iterator this_child_it = 
          thisit->second->child_map.find(rhs_child_it->first);
        if (this_child_it != thisit->second->child_map.end()) {
          // accumulate child timings
          this_child_it->second += rhs_child_it->second;
        } else {
          // copy child timings
          std::pair<SoNode *, SbTime> entry(rhs_child_it->first,
                                            rhs_child_it->second);
          thisit->second->child_map.insert(entry);
        }
      }
    } else {
      // copy rhs NodeProfile
      NodeProfile * profile = new NodeProfile(*(it->second));
      std::pair<SoNode *, NodeProfile *> entry(it->first, profile);
      this->profile_map.insert(entry);
    }
  }
  return *this;
}

/*!
  \brief Desctructor.
*/
SbProfilingData::~SbProfilingData()
{
  this->clear();
}

/*!
  \brief Remove all stored data.
*/
void 
SbProfilingData::clear()
{
  NodeProfileMap::iterator it, end;
  for (it = this->profile_map.begin(), end = this->profile_map.end();
       it != end; ++it) {
    it->second->child_map.clear();
    delete it->second; it->second = NULL;
  }
  this->profile_map.clear();
}

/*!
  \brief Get time spent traversing a child node of \a parent.
  
  If the action traversed \a parent several times, because the node appears at 
  different places in the scene graph, the returned time is the accumulated
  time.

  The function will return SbTim::zero() if there's not any recorded timing data
  for the parent and child pair.
 */
SbTime
SbProfilingData::getChildTiming(SoNode * parent, SoNode * child) const
{
  NodeProfileMap::const_iterator it = this->profile_map.find(parent);
  if (it != this->profile_map.end()) {
    ChildTimingMap::iterator child_it = it->second->child_map.find(child);
    if (child_it != it->second->child_map.end())
      return child_it->second;
  }
  return SbTime::zero();
}

/*!
  \brief Store time spent traversing a child node of \a parent.
 */
void
SbProfilingData::setChildTiming(SoNode * parent, SoNode * child, 
                                const SbTime & time)
{
  NodeProfileMap::iterator it = this->profile_map.find(parent);
  if (it != this->profile_map.end()) {
    // update the timing data for the existing NodeProfile
    ChildTimingMap::iterator child_it = it->second->child_map.find(child);
    if (child_it != it->second->child_map.end()) {
      // accumulate time in the existing child entry
      child_it->second += time;
    } else {
      // create new child entry
      std::pair<SoNode *, SbTime> entry(child, time);
      it->second->child_map.insert(entry);
    }
  } else {
    // create new NodeProfile and add the timing data for the child
    std::pair<SoNode *, SbTime> child_entry(child, time);
    NodeProfile * profile = new NodeProfile;
    profile->child_map.insert(child_entry);
    profile->has_glcache = FALSE;
    std::pair<SoNode *, NodeProfile *> profile_entry(parent, profile);
    this->profile_map.insert(profile_entry);
  }
}

/*!
  \brief Check if the node had GL chache in last render.
 */
SbBool
SbProfilingData::hasGLCache(SoNode * node) const
{
  NodeProfileMap::const_iterator it = this->profile_map.find(node);
  if (it != this->profile_map.end())
    return it->second->has_glcache;
  return FALSE;
}

/*!
  \brief Store flag that tells if the node had GL cache in last render.
*/
void
SbProfilingData::setHasGLCache(SoNode * node, SbBool hascache)
{
  NodeProfileMap::iterator it = this->profile_map.find(node);
  if (it != this->profile_map.end()) {
    // just update the existing profile
    it->second->has_glcache = hascache;
  } else {
    // create new profile and add it to the map
    NodeProfile * profile = new NodeProfile;
    profile->has_glcache = hascache;
    std::pair<SoNode *, NodeProfile *> entry(node, profile);
    this->profile_map.insert(entry);
  }
}


