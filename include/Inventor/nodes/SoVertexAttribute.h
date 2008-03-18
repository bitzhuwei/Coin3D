#ifndef COIN_SOVERTEXATTRIBUTE_H
#define COIN_SOVERTEXATTRIBUTE_H

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

#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFString.h>
#include <Inventor/fields/SoMField.h>
#include <Inventor/tools/SbPimplPtr.h>

class SoMField;
class SoVertexAttributeP;

class COIN_DLL_API SoVertexAttribute : public SoNode {
  typedef SoNode inherited;
  SO_NODE_HEADER(SoVertexAttribute);

public:
  SoVertexAttribute(void);
  static void initClass(void);

  enum AttributeType {
    NONE, FLOAT, VEC2F, VEC3F, VEC4F, INT16, INT32, MATRIX
  };

  SoSFEnum type;
  SoSFString name;


  virtual void doAction(SoAction * action);
  virtual void GLRender(SoGLRenderAction * action);
  virtual void write(SoWriteAction * action);

  virtual void notify(SoNotList * l);

protected:
  virtual ~SoVertexAttribute(void);

  virtual SbBool readInstance(SoInput * in, unsigned short flags);

private:
  SoVertexAttribute(const SoVertexAttribute & rhs); // N/A
  SoVertexAttribute & operator = (const SoVertexAttribute & rhs); // N/A

  SbPimplPtr<SoVertexAttributeP> pimpl;

}; // SoVertexAttribute

// *************************************************************************

template <int Type>
class SoAnyVertexAttribute : public SoVertexAttribute {
public:

private:
  SoAnyVertexAttribute(void) { }

}; // SoAnyVertexAttribute

typedef class SoAnyVertexAttribute<SoVertexAttribute::FLOAT> SoFloatVertexAttribute;
typedef class SoAnyVertexAttribute<SoVertexAttribute::VEC3F> SoVec3fVertexAttribute;

// *************************************************************************

#endif // !COIN_SOVERTEXATTRIBUTE_H
