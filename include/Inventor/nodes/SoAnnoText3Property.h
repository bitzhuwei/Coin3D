#ifndef COIN_SOANNOTEXT3PROPERTY_H
#define COIN_SOANNOTEXT3PROPERTY_H

/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2004 by Systems in Motion.  All rights reserved.
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
 *  See <URL:http://www.coin3d.org/> for more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no/>.
 *
\**************************************************************************/

#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFBool.h>

// *************************************************************************

class COIN_DLL_API SoAnnoText3Property : public SoNode {
  typedef SoNode inherited;

  SO_NODE_HEADER(SoAnnoText3Property);

public:
  static void initClass(void);
  SoAnnoText3Property(void);

  enum RenderPrintType {
    RENDER3D_PRINT_VECTOR,
    RENDER3D_PRINT_RASTER,
    RENDER2D_PRINT_RASTER
  };

  enum FontSizeHint {
    ANNOTATION,
    FIT_VECTOR_TEXT
  };

  SoSFBool isCharOrientedRasterPrint;
  SoSFEnum fontSizeHint;
  SoSFEnum renderPrintType;

protected:
  virtual ~SoAnnoText3Property();

  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void callback(SoCallbackAction * action);
  virtual void getBoundingBox(SoGetBoundingBoxAction * action);
  virtual void getPrimitiveCount(SoGetPrimitiveCountAction * action);
  virtual void pick(SoPickAction * action);

private:
  class SoAnnoText3PropertyP * pimpl;
};

// *************************************************************************

#endif // ! COIN_SOANNOTEXT3PROPERTY_H