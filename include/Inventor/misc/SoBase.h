#ifndef COIN_SOBASE_H
#define COIN_SOBASE_H

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

#include <Inventor/SoType.h>
#include <Inventor/lists/SoAuditorList.h>
#include <Inventor/C/base/rbptree.h>

class SbString;
class SoBaseList;
class SoInput;
class SoOutput;

class COIN_DLL_API SoBase {

public:
  static void initClass(void);

  void ref(void) const;
  void unref(void) const;
  void unrefNoDelete(void) const;
  int32_t getRefCount(void) const;

  void touch(void);

  virtual SoType getTypeId(void) const = 0;
  SbBool isOfType(SoType type) const;
  static SoType getClassTypeId(void);

  virtual SbName getName(void) const;
  virtual void setName(const SbName & newname);

  static void addName(SoBase * const base, const char * const name);
  static void removeName(SoBase * const base, const char * const name);

  virtual void startNotify(void);
  virtual void notify(SoNotList * l);

  void addAuditor(void * const auditor, const SoNotRec::Type type);
  void removeAuditor(void * const auditor, const SoNotRec::Type type);
  const SoAuditorList & getAuditors(void) const;

  virtual void addWriteReference(SoOutput * out, SbBool isfromfield = FALSE);
  SbBool shouldWrite(void);

  static void incrementCurrentWriteCounter(void);
  static void decrementCurrentWriteCounter(void);

  static SoBase * getNamedBase(const SbName & name, SoType type);
  static int getNamedBases(const SbName & name, SoBaseList & baselist,
                           SoType type);

  static SbBool read(SoInput * in, SoBase *& base, SoType expectedtype);
  static void setInstancePrefix(const SbString & c);

  static void setTraceRefs(SbBool trace);
  static SbBool getTraceRefs(void);

  static SbBool connectRoute(SoInput * in,
                             const SbName & fromnodename, const SbName & fromfieldname,
                             const SbName & tonodename, const SbName & tofieldname);

  void assertAlive(void) const;
  static SbBool readRoute(SoInput * in);

protected:
  // Note: these are bitflags.
  enum BaseFlags { IS_ENGINE = 0x01, IS_GROUP = 0x02 };

  SoBase(void);
  virtual ~SoBase();

  virtual void destroy(void);

  SbBool hasMultipleWriteRefs(void) const;
  SbBool writeHeader(SoOutput * out, SbBool isgroup, SbBool isengine) const;
  void writeFooter(SoOutput * out) const;
  virtual const char * getFileFormatName(void) const;

  virtual SbBool readInstance(SoInput * in, unsigned short flags) = 0;

  static uint32_t getCurrentWriteCounter(void);
  static void staticDataLock(void);
  static void staticDataUnlock(void);

private:
  static SbBool readReference(SoInput * in, SoBase *& base);
  static SbBool readBase(SoInput * in, SbName & classname, SoBase *& base);
  static SbBool readBaseInstance(SoInput * in, const SbName & classname,
                                 const SbName & refname, SoBase *& base);

  static SoBase * createInstance(SoInput * in, const SbName & classname);
  static void flushInput(SoInput * in);

  static void cleanClass(void);

  static SoType classTypeId;

  struct {
    int32_t referencecount  : 28;
    unsigned int alive      :  4;
    // The number of bits should sum up to 32, so we don't allocate
    // more than one machine word on a 32-bit platform.
  } objdata;

  void doNotify(SoNotList * l, const void * auditor, const SoNotRec::Type type);
  static void rbptree_notify_cb(void * auditor, void * type, void * closure);
  cc_rbptree auditortree;

  static SbString * refwriteprefix;

  static SbBool tracerefs;
  static uint32_t writecounter;
};

// support for boost::intrusive_ptr<SoBase>
inline void intrusive_ptr_add_ref(SoBase * obj) { obj->ref(); }
inline void intrusive_ptr_release(SoBase * obj) { obj->unref(); }

#endif // !COIN_SOBASE_H
