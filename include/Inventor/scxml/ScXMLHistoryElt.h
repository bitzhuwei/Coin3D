#ifndef COIN_SCXMLHISTORYELT_H
#define COIN_SCXMLHISTORYELT_H

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

#include <Inventor/scxml/ScXMLElt.h>

#include <Inventor/tools/SbPimplPtr.h>

class ScXMLTransitionElt;

class ScXMLHistoryElt : public ScXMLElt {
  typedef ScXMLElt inherited;
  SCXML_ELEMENT_HEADER(ScXMLHistoryElt)

public:
  static void initClass(void);
  static void cleanClass(void);

  ScXMLHistoryElt(void);
  virtual ~ScXMLHistoryElt(void);

  // XML attributes
  virtual void setIdAttribute(const char * id);
  const char * getIdAttribute(void) const { return this->id; }
  virtual void setTypeAttribute(const char * type);
  const char * getTypeAttribute(void) const { return this->type; }

  virtual SbBool handleXMLAttributes(void);

  virtual void copyContents(const ScXMLElt * rhs);

  virtual const ScXMLElt * search(const char * attrname, const char * attrvalue) const;

  // transition
  virtual void setTransition(ScXMLTransitionElt * transition);
  virtual ScXMLTransitionElt * getTransition(void) const;

protected:
  char * id;
  char * type;

private:
  ScXMLHistoryElt(const ScXMLHistoryElt & rhs); // N/A
  ScXMLHistoryElt & operator = (const ScXMLHistoryElt & rhs); // N/A

  class PImpl;
  SbPimplPtr<PImpl> pimpl;

}; // ScXMLHistoryElt

#endif // !COIN_SCXMLHISTORYELT_H
