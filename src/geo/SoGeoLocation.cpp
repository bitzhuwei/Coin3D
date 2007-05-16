/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2007 by Systems in Motion.  All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  ("GPL") version 2 as published by the Free Software Foundation.
 *  See the file LICENSE.GPL at the root directory of this source
 *  distribution for additional information about the GNU GPL.
 *
 *  For using Coin with software that can not be combined with the GNU
 *  GPL, and for taking advantage of the additional benefits of our
 *  support services, please contact Systems in Motion about acquiring
 *  a Coin Professional Edition License.
 *
 *  See http://www.coin3d.org/ for more information.
 *
 *  Systems in Motion, Postboks 1283, Pirsenteret, 7462 Trondheim, NORWAY.
 *  http://www.sim.no/  sales@sim.no  coin-support@coin3d.org
 *
\**************************************************************************/

/*!
  \class SoGeoLocation SoGeoLocation.h Inventor/nodes/SoGeoLocation.h
  \brief The SoGeoLocation class is used to georeference the following nodes.
  \ingroup nodes

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    GeoLocation {
      geoSystem ["GD", "WE"] 
      geoCoords ""
    }
  \endcode
*/

// *************************************************************************

#include <Inventor/nodes/SoGeoLocation.h>
#include <Inventor/nodes/SoSubNodeP.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

// *************************************************************************

/*!
  \var SoSFString SoGeoLocation::geoCoords
*/

/*!
  \var SoMFString SoGeoLocation::geoSystem
*/


// *************************************************************************

SO_NODE_SOURCE(SoGeoLocation);

/*!
  Constructor.
*/
SoGeoLocation::SoGeoLocation(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoGeoLocation);

  SO_NODE_ADD_FIELD(geoCoords, (""));
  SO_NODE_ADD_FIELD(geoSystem, (""));

  this->geoSystem.setNum(2);
  this->geoSystem.set1Value(0, "GD");
  this->geoSystem.set1Value(1, "WE");
  this->geoSystem.setDefault(TRUE);
}

/*!
  Destructor.
*/
SoGeoLocation::~SoGeoLocation()
{
}

// Doc from superclass.
void
SoGeoLocation::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoGeoLocation, SO_FROM_INVENTOR_1|SoNode::VRML1);
}

// Doc from superclass.
void
SoGeoLocation::doAction(SoAction * action)
{
  SoState * state = action->getState();
  SbMatrix m = this->getTransform(state);

  SoModelMatrixElement::set(state, this, m);
}

// Doc from superclass.
void
SoGeoLocation::GLRender(SoGLRenderAction * action)
{
  SoGeoLocation::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoLocation::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoGeoLocation::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoLocation::getMatrix(SoGetMatrixAction * action)
{
  SbMatrix m = this->getTransform(action->getState());

  action->getMatrix().multLeft(m);
  action->getInverse().multRight(m.inverse());
}

// Doc from superclass.
void
SoGeoLocation::callback(SoCallbackAction * action)
{
  SoGeoLocation::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoLocation::pick(SoPickAction * action)
{
  SoGeoLocation::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoGeoLocation::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoGeoLocation::doAction((SoAction *)action);
}

// *************************************************************************

SbMatrix
SoGeoLocation::getTransform(SoState * state) const
{
  // FIXME: calculate based on GeoOrigin
  return SbMatrix::identity();
}
