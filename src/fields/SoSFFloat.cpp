/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2003 by Systems in Motion.  All rights reserved.
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
 *  See <URL:http://www.coin3d.org> for  more information.
 *
 *  Systems in Motion, Teknobyen, Abels Gate 5, 7030 Trondheim, NORWAY.
 *  <URL:http://www.sim.no>.
 *
\**************************************************************************/

/*!
  \class SoSFFloat SoSFFloat.h Inventor/fields/SoSFFloat.h
  \brief The SoSFFloat class is a container for a floating point value.
  \ingroup fields

  This field is used where nodes, engines or other field containers
  needs to store a single floating point value.

  \sa SoMFFloat
*/

#include <Inventor/fields/SoSFFloat.h>

#include <Inventor/C/tidbitsp.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoSubFieldP.h>


SO_SFIELD_SOURCE(SoSFFloat, float, float);


// Override from parent.
void
SoSFFloat::initClass(void)
{
  SO_SFIELD_INTERNAL_INIT_CLASS(SoSFFloat);
}

// No need to document readValue() and writeValue() here, as the
// necessary information is provided by the documentation of the
// parent classes.
#ifndef DOXYGEN_SKIP_THIS

SbBool
sofield_read_float_values(SoInput * in, float * val, int numvals)
{
  for (int i=0; i < numvals; i++) {

    if (!in->read(val[i])) {
      SoReadError::post(in, "unable to read floating point value");
      return FALSE;
    }

    if (!coin_finite(val[i])) {
      SoReadError::post(in,
                        "Detected non-valid floating point number, replacing "
                        "with 0.0f");
      val[i] = 0.0f;
      // We don't return FALSE, thereby allowing the read process to
      // continue, as a convenience for the application programmer.
    }
  }

  return TRUE;
}

// Read floating point value from input stream, return TRUE if
// successful. Also used from SoMFFloat class.
SbBool
sosffloat_read_value(SoInput * in, float & val)
{
  return sofield_read_float_values(in, &val, 1);
}

SbBool
SoSFFloat::readValue(SoInput * in)
{
  float val;
  if (!sosffloat_read_value(in, val)) return FALSE;
  this->setValue(val);
  return TRUE;
}

// Write floating point value to output stream. Also used from
// SoMFFloat class.
void
sosffloat_write_value(SoOutput * out, float val)
{
  out->write(val);
}

void
SoSFFloat::writeValue(SoOutput * out) const
{
  sosffloat_write_value(out, this->getValue());
}

#endif // DOXYGEN_SKIP_THIS
