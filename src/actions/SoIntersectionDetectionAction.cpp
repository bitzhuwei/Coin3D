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
  \class SoIntersectionDetectionAction Inventor/collision/SoIntersectionDetectionAction.h
  \brief The SoIntersectionDetectionAction class is for detecting intersecting
  primitives in a scene.
  \ingroup actions
  \ingroup collision

  Below is a simple usage example for this class. It was written as a
  stand-alone framework set up for profiling and optimization of the
  SoIntersectionDetectionAction. It tests intersection of all shapes
  against each other for the loaded file.

  \code
  #include <stdlib.h>
  #include <Inventor/SbTime.h>
  #include <Inventor/SoDB.h>
  #include <Inventor/SoInteraction.h>
  #include <Inventor/collision/SoIntersectionDetectionAction.h>
  #include <Inventor/errors/SoDebugError.h>
  #include <Inventor/nodekits/SoNodeKit.h>
  #include <Inventor/nodes/SoSeparator.h>
  
  static SoIntersectionDetectionAction::Resp
  intersectionCB(void * closure, 
                 const SoIntersectingPrimitive * pr1,
                 const SoIntersectingPrimitive * pr2)
  {
    (void)fprintf(stdout, "intersection hit!\n");
    return SoIntersectionDetectionAction::NEXT_PRIMITIVE;
  }
  
  int
  main(int argc, char ** argv)
  {
    SoDB::init();
    SoNodeKit::init();
    SoInteraction::init();
  
    if (argc != 2) {
      (void)fprintf(stderr, "\n\tUsage: testapp <filename.iv>\n\n");
      exit(1);
    }
  
    SoInput in;
    SbBool ok = in.openFile(argv[1]);
    assert(ok);
    SoSeparator * root = SoDB::readAll(&in);
    assert(root);
  
    root->ref();
  
    SoIntersectionDetectionAction ida;
    ida.addIntersectionCallback(intersectionCB, NULL);
    ida.setManipsEnabled(FALSE);
    ida.setDraggersEnabled(FALSE);
    ida.setIntersectionDetectionEpsilon(10.0f);
  
    SbTime starttime = SbTime::getTimeOfDay();
    SoDebugError::postInfo("main", "SoIntersectionDetectionAction::apply");
  
    ida.apply(root);
  
    SoDebugError::postInfo("main", "apply() done after %f seconds.",
                           (SbTime::getTimeOfDay() - starttime).getValue());
  
    root->unref();
  
    return 0;
  }
  \endcode

  \since Coin 2.1
  \since TGS Inventor 2.4
*/

// FIXME: intersection with lines not implemented yet. 20030507 mortene.

#include <Inventor/collision/SoIntersectionDetectionAction.h>

#include <Inventor/C/tidbits.h>
#include <Inventor/SbOctTree.h>
#include <Inventor/SbTime.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbXfBox3f.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/caches/SoBoundingBoxCache.h>
#include <Inventor/draggers/SoDragger.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/lists/SbPList.h>
#include <Inventor/manips/SoClipPlaneManip.h>
#include <Inventor/manips/SoDirectionalLightManip.h>
#include <Inventor/manips/SoPointLightManip.h>
#include <Inventor/manips/SoSpotLightManip.h>
#include <Inventor/manips/SoTransformManip.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMatrixTransform.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>

#include "SbTri3f.ih"
#include "SbTri3f.icc"

// *************************************************************************

/*!
  \struct SoIntersectingPrimitive Inventor/collision/SoIntersectionDetectionAction.h
  \brief Struct with collision information.

  This struct is passed to the collision callback registered by the
  application programmer. It contains information about which
  primitives are intersecting each other.
*/

// FIXME: should document all elements in the struct;
//
// struct SoIntersectingPrimitive {
//   SoPath * path;
//   enum PrimitiveType {
//     SEGMENT = 2,
//     LINE_SEGMENT = 2,
//     TRIANGLE = 3
//   } type;
//   SbVec3f vertex[3];
//   SbVec3f xf_vertex[3];
// };
//
// 20030613 mortene.

// *************************************************************************

class ShapeData;
class PrimitiveData;

class SoIntersectionDetectionActionP {
public:
  SoIntersectionDetectionActionP(void);
  ~SoIntersectionDetectionActionP();

  static float staticepsilon;
  float epsilon;
  SbBool epsilonset;
  float getEpsilon(void) const;

  SbBool draggersenabled;
  SbBool manipsenabled;
  SbBool internalsenabled;

  SoIntersectionDetectionAction::SoIntersectionFilterCB * filtercb;
  void * filterclosure;
  SbPList * callbacks;
  SoCallbackAction * traverser;

  SoCallbackAction::Response shape(SoCallbackAction * action, SoShape * shape);
  static SoCallbackAction::Response shapeCB(void * closure, SoCallbackAction * action, const SoNode * node);
  SoCallbackAction::Response traverse(SoCallbackAction * action, const SoNode * node);
  static SoCallbackAction::Response traverseCB(void * closure, SoCallbackAction * action, const SoNode * node);
  SoCallbackAction::Response dragger(SoCallbackAction * action, const SoNode * node);
  static SoCallbackAction::Response draggerCB(void * closure, SoCallbackAction * action, const SoNode * node);
  static SoCallbackAction::Response pruneCB(void * closure, SoCallbackAction * action, const SoNode * node);

  void reset(void);
  void doIntersectionTesting(void);
  void doPrimitiveIntersectionTesting(PrimitiveData * primitives1, PrimitiveData * primitives2, SbBool & cont);
  void doInternalPrimitiveIntersectionTesting(PrimitiveData * primitives, SbBool & cont);

  SoTypeList * prunetypes;

  SoTypeList * traversaltypes;
  SbPList * traversalcallbacks;

  SbList<ShapeData*> shapedata;
  SbXfBox3f fullxfbbox;
};

float SoIntersectionDetectionActionP::staticepsilon = 0.0f;

SoIntersectionDetectionActionP::SoIntersectionDetectionActionP(void)
{
  this->epsilon = 0.0f;
  this->epsilonset = FALSE;
  this->draggersenabled = TRUE;
  this->manipsenabled = TRUE;
  this->internalsenabled = FALSE;
  this->filtercb = NULL;
  this->filterclosure = NULL;
  this->callbacks = new SbPList;
  this->traverser = NULL;
  this->prunetypes = new SoTypeList;
  this->traversaltypes = new SoTypeList;
  this->traversalcallbacks = new SbPList;
}

SoIntersectionDetectionActionP::~SoIntersectionDetectionActionP(void)
{
  this->reset();
  delete this->callbacks;
  delete this->traverser;
  delete this->prunetypes;
  delete this->traversaltypes;
  delete this->traversalcallbacks;
}

float
SoIntersectionDetectionActionP::getEpsilon(void) const
{
  if ( this->epsilonset ) return this->epsilon;
  return SoIntersectionDetectionActionP::staticepsilon;
}

// *************************************************************************

#define PRIVATE(obj) ((obj)->pimpl)

// *************************************************************************

static SbBool
ida_debug(void)
{
  static int dbg = -1;
  if (dbg == -1) {
    const char * env = coin_getenv("COIN_IDA_DEBUG");
    dbg = env && atoi(env) > 0;
  }
  return dbg == 0 ? FALSE : TRUE;
}

// *************************************************************************

SO_ACTION_SOURCE(SoIntersectionDetectionAction);

void
SoIntersectionDetectionAction::initClass(void)
{
  SO_ACTION_INIT_CLASS(SoIntersectionDetectionAction, SoAction);
}

SoIntersectionDetectionAction::SoIntersectionDetectionAction(void)
{
  PRIVATE(this) = new SoIntersectionDetectionActionP;
}

SoIntersectionDetectionAction::~SoIntersectionDetectionAction(void)
{
  delete PRIVATE(this);
}

// FIXME: is the "epsilon is not supported yet" statement below false?
// 20030326 mortene.

/*!
  Sets the global intersection detection distance epsilon value.
  This will affect all intersection detection action objects in use that
  don't have a locally set value.

  The epsilon value is a worldspace value.

  Be aware that increasing the epsilon value can \e dramatically
  increase the number of primitive intersection tests being done to
  decide intersections. Increasing the epsilon value can therefore
  cause serious slow-downs in the running time of the intersections
  checks.

  Epsilon is not supported yet.
*/

void
SoIntersectionDetectionAction::setIntersectionEpsilon(float epsilon) // static
{
  assert(epsilon >= 0.0f);
  SoIntersectionDetectionActionP::staticepsilon = epsilon;
}

/*!
  Returns the globally set intersection detection distance epsilon value.
*/

float
SoIntersectionDetectionAction::getIntersectionEpsilon(void) // static
{
  return SoIntersectionDetectionActionP::staticepsilon;
}

/*!
  Sets the intersection detection distance epsilon value for the
  action object.  This overrides the global value.

  See also SoIntersectionDetectionAction::setIntersectionEpsilon() for
  important information about how this setting influences performance.
*/
void
SoIntersectionDetectionAction::setIntersectionDetectionEpsilon(float epsilon)
{
  assert(epsilon >= 0.0f);
  PRIVATE(this)->epsilon = epsilon;
  PRIVATE(this)->epsilonset = TRUE;
}

/*!
  Returns the set intersection detection distance epsilon value for the action object.
*/

float
SoIntersectionDetectionAction::getIntersectionDetectionEpsilon(void) const
{
  return PRIVATE(this)->getEpsilon();
}

/*!
  Sets whether nodes of specific types (including derived objects) should be tested
  for intersection or not.

  \sa isTypeEnabled(), setManipsEnabled(), setDraggersEnabled()
*/

void
SoIntersectionDetectionAction::setTypeEnabled(SoType type, SbBool enable)
{
  if ( enable ) {
    int idx = PRIVATE(this)->prunetypes->find(type);
    if ( idx >= 0 ) PRIVATE(this)->prunetypes->remove(idx);
  } else if ( PRIVATE(this)->prunetypes->find(type) ) {
    PRIVATE(this)->prunetypes->append(type);
  }
}

/*!
  Returns whether nodes of specific types are enabled or not.  The \a checkgroups
  argument can be set to TRUE if you wan't the return value to reflect whether
  the node will be implicit enabled/disabled through the settings controlled by the
  setManipsEnabled() and setDraggersEnabled() functions.

  The default is that all node types are enabled.

  Note that derivation checks are not performed - the type needs to be the exact
  same type as has been disabled with setTypeEnabled()

  \sa setTypeEnabled()
*/

SbBool
SoIntersectionDetectionAction::isTypeEnabled(SoType type, SbBool checkgroups) const
{
  if ( PRIVATE(this)->prunetypes->find(type) != -1 ) return FALSE;
  if ( checkgroups ) {
    // is type a dragger?
    if ( !PRIVATE(this)->draggersenabled &&
	 type.isDerivedFrom(SoDragger::getClassTypeId()) ) return FALSE;
    // is type a manip?
    if ( !PRIVATE(this)->manipsenabled ) {
      if ( type.isDerivedFrom(SoTransformManip::getClassTypeId()) ||
	   type.isDerivedFrom(SoClipPlaneManip::getClassTypeId()) ||
	   type.isDerivedFrom(SoDirectionalLightManip::getClassTypeId()) ||
	   type.isDerivedFrom(SoPointLightManip::getClassTypeId()) ||
	   type.isDerivedFrom(SoSpotLightManip::getClassTypeId()) ) return FALSE;
    }
  }
  return TRUE;
}

/*!
  Sets whether manipulators in the scene graph should be tested for intersection
  with other geometry or not.

  Note that when draggers are disabled with setDraggersEnabled(), this setting
  has no effect - manipulators are disabled too.

  \sa isManipsEnabled(), setDraggersEnabled(), setTypeEnabled()
*/

void
SoIntersectionDetectionAction::setManipsEnabled(SbBool enable)
{
  PRIVATE(this)->manipsenabled = enable;
}

/*!
  Returns whether the actions is set up to test intersection on manipulators
  in the scene or not.

  Note that when draggers are disabled with setDraggersEnabled(), this setting
  has no effect - manipulators are disabled too.

  The default is that manipulators are enabled for intersection testing with
  other geometry in the scene.

  \sa setManipsEnabled()
*/

SbBool
SoIntersectionDetectionAction::isManipsEnabled(void) const
{
  return PRIVATE(this)->manipsenabled;
}

/*!
  Sets whether draggers in the scene graph should be tested for intersection
  with other geometry or not.

  Note that when you disable draggers, manipulators are also automatically
  disabled, although the isManipsDisabled() setting might reflect otherwise.

  \sa isDraggersEnabled(), setManipsEnabled(), setTypeEnabled()
*/

void
SoIntersectionDetectionAction::setDraggersEnabled(SbBool enable)
{
  PRIVATE(this)->draggersenabled = enable;
}

/*!
  Returns whether the actions is set up to test intersection on draggers
  in the scene or not.

  The default is that draggers are enabled for intersection testing with
  other geometry in the scene.

  \sa setDraggersEnabled()
*/

SbBool
SoIntersectionDetectionAction::isDraggersEnabled(void) const
{
  return PRIVATE(this)->draggersenabled;
}

/*!
  Sets whether nodes in the scene graph should be checked for intersecting
  primitives within themselves.

  Default is \c FALSE.

  \sa isShapeInternalsEnabled()
*/

void
SoIntersectionDetectionAction::setShapeInternalsEnabled(SbBool enable)
{
  PRIVATE(this)->internalsenabled = enable;
}

/*!
  Returns whether nodes in the scene graph will be checked for
  intersecting primitives within themselves.

  The default value for this setting is \c FALSE.

  \sa setShapeInternalsEnabled()
*/

SbBool
SoIntersectionDetectionAction::isShapeInternalsEnabled(void) const
{
  return PRIVATE(this)->internalsenabled;
}

/*!
  The scene graph traversal can be controlled with callbacks which
  you set with this method.  Use just like you would use
  SoCallbackAction::addPreCallback().

  \sa SoCallbackAction::addPreCallback()
*/

void
SoIntersectionDetectionAction::addVisitationCallback(SoType type, SoIntersectionVisitationCB * cb, void * closure)
{
  PRIVATE(this)->traversaltypes->append(type);
  PRIVATE(this)->traversalcallbacks->append((void *) cb);
  PRIVATE(this)->traversalcallbacks->append(closure);
}

/*!
  The scene graph traversal can be controlled with callbacks which
  you remove with this method.  Use just like you would use
  SoCallbackAction::removePreCallback().

  \sa SoCallbackAction::removePreCallback()
*/

void
SoIntersectionDetectionAction::removeVisitationCallback(SoType type, SoIntersectionVisitationCB * cb, void * closure)
{
  int idx = 0;
  while ( idx < PRIVATE(this)->traversaltypes->getLength() ) {
    if ( (*(PRIVATE(this)->traversaltypes))[idx] == type ) {
      if ( ((*(PRIVATE(this)->traversalcallbacks))[idx*2] == (void *) cb) &&
           ((*(PRIVATE(this)->traversalcallbacks))[idx*2+1] == closure) ) {
        PRIVATE(this)->traversaltypes->remove(idx);
        PRIVATE(this)->traversalcallbacks->remove(idx*2+1);
        PRIVATE(this)->traversalcallbacks->remove(idx*2);
      } else {
        idx += 1;
      }
    } else {
      idx += 1;
    }
  }
}

/*!
  This callback is called when two shapes are found to have intersecting
  bounding boxes, and are about to be checked for real intersection between
  their primitives.

  When intersection epsilon values are in use, bounding box intersection
  testing is done approximately and will trigger the filter callback on
  boxes that are further from each other than the epsilon length.

  If the callback returns TRUE, the intersection test will be performed.
  If the callback returns FALSE, the intersection testing will be skipped.

  The API allows only one filter callback.
*/

void
SoIntersectionDetectionAction::setFilterCallback(SoIntersectionFilterCB * cb, void * closure)
{
  PRIVATE(this)->filtercb = cb;
  PRIVATE(this)->filterclosure = closure;
}

/*!
  Adds a callback to be called when two intersecting primitives are found in
  the scene.

  If the callback returns ABORT, the intersection detection is aborted.  If
  the callback returns NEXT_SHAPE, the intersection detection between these
  two shapes are aborted and the action continues checking other shapes.
  If the callback returns NEXT_PRIMITIVE, the intersection detection testing
  continues checking the other primitives in these two shapes.

  \sa removeIntersectionCallback()
*/

void
SoIntersectionDetectionAction::addIntersectionCallback(SoIntersectionCB * cb, void * closure)
{
  PRIVATE(this)->callbacks->append((void *) cb);
  PRIVATE(this)->callbacks->append(closure);
}

/*!
  Removes a callback set with addIntersectionCallback().

  \sa addIntersectionCallback()
*/

void
SoIntersectionDetectionAction::removeIntersectionCallback(SoIntersectionCB * cb, void * closure)
{
  int i;
  for ( i = 0; i < PRIVATE(this)->callbacks->getLength(); i += 2 ) {
    if ( ((*PRIVATE(this)->callbacks)[i] == (void *) cb) &&
         ((*PRIVATE(this)->callbacks)[i+1] == closure) ) {
      PRIVATE(this)->callbacks->remove(i+1);
      PRIVATE(this)->callbacks->remove(i);
    }
  }
}

// *************************************************************************

void
SoIntersectionDetectionAction::apply(SoNode * node)
{
  // Keep this around for now, for getting a stand-alone scene to work
  // with from an invocation from the ToyotaViewer
  // application. mortene.
#if 0 // disabled
  SoOutput out;
  SbBool ok = out.openFile("/tmp/assembly.wrl");
  assert(ok);
  SoWriteAction wa(&out);
  wa.apply(node);
  out.closeFile();
#endif // disabled

  PRIVATE(this)->reset();

  // Needs a bounding box for the full scene, for later initialization
  // of the SbOctTree of shape bounding boxes.
  SbViewportRegion vp;
  SoGetBoundingBoxAction bboxaction(vp);
  bboxaction.apply(node);
  PRIVATE(this)->fullxfbbox = bboxaction.getXfBoundingBox();

  if (ida_debug()) { // debug
    SoGetPrimitiveCountAction counter;
    counter.apply(node);
    SoDebugError::postInfo("SoIntersectionDetectionAction::apply",
                           "number of triangle primitives in scene: %d",
                           counter.getTriangleCount());
  }

  PRIVATE(this)->traverser->apply(node);

  SbTime starttime;
  if (ida_debug()) { // debug
    starttime = SbTime::getTimeOfDay();
    SoDebugError::postInfo("SoIntersectionDetectionAction::apply",
                           "calling doIntersectionTesting()...");
  }

  PRIVATE(this)->doIntersectionTesting();

  if (ida_debug()) { // debug
    SoDebugError::postInfo("SoIntersectionDetectionAction::apply",
                           "doIntersectionTesting() done after %f seconds.",
                           (SbTime::getTimeOfDay() - starttime).getValue());
  }
}

void
SoIntersectionDetectionAction::apply(SoPath * path)
{
  PRIVATE(this)->reset();

  // Needs a bounding box for the full scene, for later initialization
  // of the SbOctTree of shape bounding boxes.
  SbViewportRegion vp;
  SoGetBoundingBoxAction bboxaction(vp);
  bboxaction.apply(path);
  PRIVATE(this)->fullxfbbox = bboxaction.getXfBoundingBox();

  PRIVATE(this)->traverser->apply(path);
  PRIVATE(this)->doIntersectionTesting();
}

void
SoIntersectionDetectionAction::apply(const SoPathList & paths, SbBool obeysRules)
{
  PRIVATE(this)->reset();

  // Needs a bounding box for the full scene, for later initialization
  // of the SbOctTree of shape bounding boxes.
  SbViewportRegion vp;
  SoGetBoundingBoxAction bboxaction(vp);
  bboxaction.apply(paths, obeysRules);
  PRIVATE(this)->fullxfbbox = bboxaction.getXfBoundingBox();

  PRIVATE(this)->traverser->apply(paths, obeysRules);
  PRIVATE(this)->doIntersectionTesting();
}

// *************************************************************************

class PrimitiveData {
public:
  PrimitiveData(void)
  {
    this->path = NULL;
    this->octtree = NULL;
  }

  ~PrimitiveData()
  {
    if (this->octtree) { delete this->octtree; }
    for (unsigned int i = 0; i < this->numTriangles(); i++) { delete this->getTriangle(i); }
  }


  const SbOctTree * getOctTree(void) {
    if (this->octtree == NULL) {
      SbOctTreeFuncs funcs = {
        NULL /* bboxfunc */, NULL /* ptinsidefunc */,
        PrimitiveData::insideboxfunc,
        NULL /* insidespherefunc */, NULL /* insideplanesfunc */
      };

      this->octtree = new SbOctTree(this->getBoundingBox(), funcs);
      if (ida_debug()) {
        SoDebugError::postInfo("PrimitiveData::getOctTree",
                               "made new octtree for PrimitiveData %p", this);
      }

      for (unsigned int k = 0; k < this->numTriangles(); k++) {
        SbTri3f * t = this->getTriangle(k);
        this->octtree->addItem(t);
      }
    }
    return this->octtree;
  }


  void setPath(SoPath * p) { this->path = p; }
  SoPath * getPath(void) const { return this->path; }

  void addTriangle(SbTri3f * t)
  {
    assert(this->octtree == NULL && "all triangles must be added before making octtree");
    this->triangles.append(t);
    this->bbox.extendBy(t->getBoundingBox());
  }

  unsigned int numTriangles(void) const { return this->triangles.getLength(); }
  SbTri3f * getTriangle(const int idx) const { return this->triangles[idx]; }

  const SbBox3f & getBoundingBox(void) const { return this->bbox; }

  SbMatrix transform;
  SbMatrix invtransform;

private:
  static SbBool insideboxfunc(void * const item, const SbBox3f & box);

  SoPath * path;
  SbList<SbTri3f*> triangles;
  SbBox3f bbox;
  SbOctTree * octtree;
};

SbBool
PrimitiveData::insideboxfunc(void * const item, const SbBox3f & box)
{
  SbTri3f * tri = (SbTri3f *)item;
  return box.intersect(tri->getBoundingBox());
}

// *************************************************************************

class ShapeData {
public:
  ShapeData(void)
  {
    this->primitives = NULL;
  }

  ~ShapeData()
  {
    if (this->primitives) { delete this->primitives; }
  }

  PrimitiveData * getPrimitives(void);

  SoPath * path;
  SbXfBox3f xfbbox;

private:
  static void triangleCB(void * closure, SoCallbackAction *,
                         const SoPrimitiveVertex * v1,
                         const SoPrimitiveVertex * v2,
                         const SoPrimitiveVertex * v3);

  PrimitiveData * primitives;
};

void
ShapeData::triangleCB(void * closure, SoCallbackAction *,
                      const SoPrimitiveVertex * v1,
                      const SoPrimitiveVertex * v2,
                      const SoPrimitiveVertex * v3)
{
  PrimitiveData * primitives = (PrimitiveData *) closure;
  const SbVec3f & oa = v1->getPoint();
  const SbVec3f & ob = v2->getPoint();
  const SbVec3f & oc = v3->getPoint();
  SbVec3f wa, wb, wc;
  primitives->transform.multVecMatrix(oa, wa);
  primitives->transform.multVecMatrix(ob, wb);
  primitives->transform.multVecMatrix(oc, wc);

  // Only add valid triangles.
  const SbVec3f normal = (wa - wb).cross(wa - wc);
  if (normal.length() > 0.0f) {
    SbTri3f * triangle = new SbTri3f(wa, wb, wc);
    primitives->addTriangle(triangle);
  }
  else {
    static SbBool warn = TRUE;
    if (warn) {
      warn = FALSE;
      SoDebugError::postWarning("ShapeData::triangleCB",
                                "Found an invalid triangle while souping up "
                                "triangle primitives from a shape for "
                                "intersection testing. Transformed=="
                                "<<%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>>. "
                                "Untransformed=="
                                "<<%f, %f, %f>, <%f, %f, %f>, <%f, %f, %f>>. "
                                "Will only warn once, there could be more "
                                "cases.",
                                wa[0], wa[1], wa[2],
                                wb[0], wb[1], wb[2],
                                wc[0], wc[1], wc[2],
                                oa[0], oa[1], oa[2],
                                ob[0], ob[1], ob[2],
                                oc[0], oc[1], oc[2]);
    }
  }
}

PrimitiveData *
ShapeData::getPrimitives(void)
{
  if (this->primitives) { return this->primitives; }

  this->primitives = new PrimitiveData;
  this->primitives->setPath(this->path);
  this->primitives->transform = this->xfbbox.getTransform();
  this->primitives->invtransform = primitives->transform.inverse();
  SoCallbackAction generator;
  generator.addTriangleCallback(SoShape::getClassTypeId(),
                                ShapeData::triangleCB,
                                this->primitives);
  generator.apply(this->path);
  return this->primitives;
}

// *************************************************************************

SoCallbackAction::Response
SoIntersectionDetectionActionP::shape(SoCallbackAction * action, SoShape * shape)
{
  SbBox3f bbox;
  SbVec3f center;

  const SoBoundingBoxCache * bboxcache = shape->getBoundingBoxCache();
  if (bboxcache && bboxcache->isValid(action->getState())) {
    bbox = bboxcache->getProjectedBox();
    if (bboxcache->isCenterSet()) center = bboxcache->getCenter();
    else center = bbox.getCenter();
  }
  else {
    shape->computeBBox(action, bbox, center);
  }
  ShapeData * data = new ShapeData;
  data->path = new SoPath(*(action->getCurPath()));
  data->path->ref();
  data->xfbbox = bbox;
  data->xfbbox.setTransform(action->getModelMatrix());
  this->shapedata.append(data);
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoIntersectionDetectionActionP::shapeCB(void * closure, SoCallbackAction * action, const SoNode * node)
{
  assert(node && node->isOfType(SoShape::getClassTypeId()));
  return ((SoIntersectionDetectionActionP *) closure)->shape(action, (SoShape *) node);
}

SoCallbackAction::Response
SoIntersectionDetectionActionP::traverse(SoCallbackAction * action, const SoNode * node)
{
  const SoPath * curpath = action->getCurPath();
  int i;
  for ( i = 0; i < this->traversaltypes->getLength(); i++ ) {
    if ( node->getTypeId().isDerivedFrom((*(this->traversaltypes))[i]) ) {
      SoIntersectionDetectionAction::SoIntersectionVisitationCB * cb =
        (SoIntersectionDetectionAction::SoIntersectionVisitationCB *)
        (*(this->traversalcallbacks))[i*2];
      SoCallbackAction::Response response = cb((*(this->traversalcallbacks))[i*2+1], curpath);
      if ( response != SoCallbackAction::CONTINUE ) return response;
    }
  }
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoIntersectionDetectionActionP::traverseCB(void * closure, SoCallbackAction * action, const SoNode * node)
{
  return ((SoIntersectionDetectionActionP *) closure)->traverse(action, node);
}

SoCallbackAction::Response
SoIntersectionDetectionActionP::dragger(SoCallbackAction * action, const SoNode *)
{
  if ( !this->draggersenabled ) // dragger setting overrides setting for manipulators
    return SoCallbackAction::PRUNE;
  if ( !this->manipsenabled ) {
    const SoPath * path = action->getCurPath();
    SoNode * tail = path->getTail();
    SoType type = tail->getTypeId();
    if ( type.isDerivedFrom(SoTransformManip::getClassTypeId()) ||
         type.isDerivedFrom(SoClipPlaneManip::getClassTypeId()) ||
         type.isDerivedFrom(SoDirectionalLightManip::getClassTypeId()) ||
         type.isDerivedFrom(SoPointLightManip::getClassTypeId()) ||
         type.isDerivedFrom(SoSpotLightManip::getClassTypeId()) )
      return SoCallbackAction::PRUNE;
  }
  return SoCallbackAction::CONTINUE;
}

SoCallbackAction::Response
SoIntersectionDetectionActionP::draggerCB(void * closure, SoCallbackAction * action, const SoNode * node)
{
  return ((SoIntersectionDetectionActionP *) closure)->dragger(action, node);
}

SoCallbackAction::Response
SoIntersectionDetectionActionP::pruneCB(void *, SoCallbackAction *, const SoNode *)
{
  return SoCallbackAction::PRUNE;
}

void
SoIntersectionDetectionActionP::reset(void)
{
  int i;
  for (i = 0; i < this->shapedata.getLength(); i++) {
    ShapeData * data = this->shapedata[i];
    data->path->unref();
    delete data;
  }
  this->shapedata.truncate(0);
  this->traverser = new SoCallbackAction;
  this->traverser->addPreCallback(SoDragger::getClassTypeId(),
                                  draggerCB, this);
  this->traverser->addPreCallback(SoNode::getClassTypeId(),
                                  traverseCB, this);
  for (i = 0; i < this->prunetypes->getLength(); i++)
    this->traverser->addPreCallback((*(this->prunetypes))[i],
                                    pruneCB, NULL);
  this->traverser->addPreCallback(SoShape::getClassTypeId(),
                                  shapeCB, this);
}

// This is a helper function for debugging purposes: it sets up an
// SoCoordinate3 + SoIndexedLineSet pair of nodes exposing the
// geometry of the SbBox3f input argument.
static void
make_scene_graph(const SbBox3f & box, SoCoordinate3 *& coord3, SoIndexedLineSet *& ils)
{
  const SbVec3f & vmin = box.getMin();
  const SbVec3f & vmax = box.getMax();

  const SbVec3f corners[] = {
    // back face
    SbVec3f(vmin[0], vmin[1], vmin[2]),
    SbVec3f(vmax[0], vmin[1], vmin[2]),
    SbVec3f(vmax[0], vmax[1], vmin[2]),
    SbVec3f(vmin[0], vmax[1], vmin[2]),

    // front face
    SbVec3f(vmin[0], vmin[1], vmax[2]),
    SbVec3f(vmax[0], vmin[1], vmax[2]),
    SbVec3f(vmax[0], vmax[1], vmax[2]),
    SbVec3f(vmin[0], vmax[1], vmax[2])
  };

  const int32_t indices[] = {
    0, 1, 2, 3, 0, -1, // back face
    4, 5, 6, 7, 4, -1, // front face
    0, 4, -1, 1, 5, -1, 2, 6, -1, 3, 7, -1 // "crossover" lines
  };

  coord3 = new SoCoordinate3;
  coord3->point.setValues(0, sizeof(corners) / sizeof(corners[0]), corners);

  ils = new SoIndexedLineSet;
  ils->coordIndex.setValues(0, sizeof(indices) / sizeof(indices[0]), indices);
}

// This is a helper function for debugging purposes: it sets up a
// small scene graph that shows the geometry of the SbXfBox3f input
// argument, with the identity tag at it's corner.
static SoSeparator *
make_scene_graph(const SbXfBox3f & xfbox, const char * tag)
{
  SoSeparator * root = new SoSeparator;
  root->setName(tag);

  // Add the geometry for the projected SbXfBox3f.

  const SbBox3f projbox = xfbox.project();
  SoCoordinate3 * coord3;
  SoIndexedLineSet * ils;
  make_scene_graph(projbox, coord3, ils);

  root->addChild(coord3);
  root->addChild(ils);

  // Add the geometry for the non-projected SbXfBox3f.

  SoBaseColor * basecol = new SoBaseColor;
  basecol->rgb.setValue(SbColor(1, 1, 0));
  root->addChild(basecol);

  SoMatrixTransform * transform = new SoMatrixTransform;
  transform->matrix = xfbox.getTransform();
  root->addChild(transform);

  const SbBox3f & box3f = (const SbBox3f &)xfbox;
  SoCoordinate3 * xfcoord3;
  SoIndexedLineSet * xfils;
  make_scene_graph(box3f, xfcoord3, xfils);

  root->addChild(xfcoord3);
  root->addChild(xfils);

  SoSeparator * tagsep = new SoSeparator;
  root->addChild(tagsep);

  SoBaseColor * textcol = new SoBaseColor;
  textcol->rgb.setValue(SbColor(1, 0, 0));
  tagsep->addChild(textcol);

  SoTranslation * translation = new SoTranslation;
  translation->translation = xfcoord3->point[0];
  tagsep->addChild(translation);

  SoText2 * tagtext = new SoText2;
  tagtext->string = tag;
  tagsep->addChild(tagtext);

  return root;
}

// Expand SbXfBox3f in all directions with an epsilon value.
static SbXfBox3f
expand_SbXfBox3f(const SbXfBox3f & box, float epsilon)
{
  assert(epsilon > 0.0f);

  // FIXME: quality check the calculation for the epsilon-extended
  // bbox. It needs to be correct _and_ not adding on too much
  // fat. 20030331 mortene.

  // This invokes the copy constructor (and not the SbXfBox3f(SbBox3f)
  // constructor), so the transformation matrix is also copied.
  SbXfBox3f extbox(box);

  SbVec3f epsilonvec(epsilon, epsilon, epsilon);
  // Move epsilon to object space.
  box.getTransform().multDirMatrix(epsilonvec, epsilonvec);
  const float localepsilon = epsilonvec.length(); // yes, it's a bit large...
  epsilonvec = SbVec3f(localepsilon, localepsilon, localepsilon);

  // Get superclass-pointer, so we can modify the box corners
  // directly.
  SbBox3f * extboxp = (SbBox3f *)&extbox;

  extboxp->getMin() -= epsilonvec;
  extboxp->getMax() += epsilonvec;

  return extbox;
}

// The callback test function for the SbOctTree.
static SbBool
shapeinsideboxfunc(void * const item, const SbBox3f & box)
{
  ShapeData * shape = (ShapeData *)item;
  return shape->xfbbox.intersect(box);
}

// Execute full set of intersection detection operations on all the
// primitives that has been souped up from the scene graph.
void
SoIntersectionDetectionActionP::doIntersectionTesting(void)
{
  if (this->callbacks->getLength() == 0) {
    SoDebugError::postWarning("SoIntersectionDetectionActionP::doIntersectionTesting",
                              "intersection testing invoked, but no callbacks set up");
    return;
  }

  delete this->traverser;
  this->traverser = NULL;

  if (ida_debug()) {
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doIntersectionTesting",
                           "total number of shapedata items == %d",
                           this->shapedata.getLength());
    
  }

  SbOctTreeFuncs funcs = {
    NULL /* bboxfunc */, NULL /* ptinsidefunc */,
    shapeinsideboxfunc,
    NULL /* insidespherefunc */, NULL /* insideplanesfunc */
  };

  SbOctTree shapetree(this->fullxfbbox.project(), funcs);
  for (int k = 0; k < this->shapedata.getLength(); k++) {
    ShapeData * shape = this->shapedata[k];
    if (shape->xfbbox.isEmpty()) { continue; }
    shapetree.addItem(shape);
  }

  if (ida_debug()) { shapetree.debugTree(stderr); }

  // For debugging.
  unsigned int nrshapeshapeisects = 0;
  unsigned int nrselfisects = 0;

  const float epsilon = this->getEpsilon();

  for (int i = 0; i < this->shapedata.getLength(); i++) {
    ShapeData * shape1 = this->shapedata[i];

    // If the shape has no geometry, immediately skip to next
    // iteration of for-loop.
    if (shape1->xfbbox.isEmpty()) { continue; }

    // Remove shapes from octtree as we iterate over the full set, to
    // avoid self-intersection and to avoid checks against other
    // shapes happening both ways.
    shapetree.removeItem(shape1);

    // FIXME: shouldn't we also invoke the filter-callback here? 20030403 mortene.
    if (this->internalsenabled) {
      nrselfisects++;
      SbBool cont;
      this->doInternalPrimitiveIntersectionTesting(shape1->getPrimitives(), cont);
      if (!cont) { goto done; }
    }

    SbBox3f shapebbox = shape1->xfbbox.project();
    if (epsilon > 0.0f) {
      const SbVec3f e(epsilon, epsilon, epsilon);
      // Extend bbox in all 6 directions with the epsilon value.
      shapebbox.getMin() -= e;
      shapebbox.getMax() += e;
    }
    SbList<void*> candidateshapes;
    shapetree.findItems(shapebbox, candidateshapes);

    if (ida_debug()) {
      SoDebugError::postInfo("SoIntersectionDetectionActionP::doIntersectionTesting",
                             "shape %d intersects %d other shapes",
                             i, candidateshapes.getLength());

      // debug, dump to .iv-file the "master" shape bbox given by i,
      // plus ditto for all intersected shapes
#if 0
      if (i == 4) {
        SoSeparator * root = new SoSeparator;
        root->ref();

        root->addChild(make_scene_graph(shape1->xfbbox, "mastershape"));

        for (int j = 0; j < candidateshapes.getLength(); j++) {
          ShapeData * s = (ShapeData * )candidateshapes[j];
          SbString str;
          str.sprintf("%d", j);
          root->addChild(make_scene_graph(s->xfbbox, str.getString()));
        }

        SoOutput out;
        SbBool ok = out.openFile("/tmp/shapechk.iv");
        assert(ok);
        SoWriteAction wa(&out);
        wa.apply(root);

        root->unref();
      }
#endif // debug
    }

    SbXfBox3f xfboxchk;
    if (epsilon > 0.0f) { xfboxchk = expand_SbXfBox3f(shape1->xfbbox, epsilon); }
    else { xfboxchk = shape1->xfbbox; }

    for (int j = 0; j < candidateshapes.getLength(); j++) {
      ShapeData * shape2 = (ShapeData * )candidateshapes[j];

      if (!xfboxchk.intersect(shape2->xfbbox)) {
        if (ida_debug()) {
          SoDebugError::postInfo("SoIntersectionDetectionActionP::doIntersectionTesting",
                                 "shape %d intersecting %d is a miss when tried with SbXfBox3f::intersect(SbXfBox3f)",
                                 i, j);
        }
        continue;
      }

      if (!this->filtercb ||
          this->filtercb(this->filterclosure, shape1->path, shape2->path)) {
        nrshapeshapeisects++;
        SbBool cont;
        this->doPrimitiveIntersectionTesting(shape1->getPrimitives(), shape2->getPrimitives(), cont);
        if (!cont) { goto done; }
      }
    }
  }

 done:
  if (ida_debug()) {
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doIntersectionTesting",
                           "shape-shape intersections: %d, shape self-intersections: %d",
                           nrshapeshapeisects, nrselfisects);
  }
}

// Intersection testing between primitives of different shapes.
void
SoIntersectionDetectionActionP::doPrimitiveIntersectionTesting(PrimitiveData * primitives1,
                                                             PrimitiveData * primitives2,
                                                             SbBool & cont)
{
  cont = TRUE;

  // for debugging
  if (ida_debug()) {
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doPrimitiveIntersectionTesting",
                           "primitives1 (%p) = %d tris, primitives2 (%p) = %d tris",
                           primitives1, primitives1->numTriangles(),
                           primitives2, primitives2->numTriangles());
  }
  unsigned int nrisectchks = 0;
  unsigned int nrhits = 0;

  // Use the majority size shape from an octtree.
  //
  // (Some initial investigation indicates that this isn't a clear-cut
  // choice, by the way -- should investigate further. mortene.)
  PrimitiveData * octtreeprims = primitives1;
  PrimitiveData * iterationprims = primitives2;
  if (primitives1->numTriangles() < primitives2->numTriangles()) {
    octtreeprims = primitives2;
    iterationprims = primitives1;
  }

  const SbOctTree * octtree = octtreeprims->getOctTree();

  const float epsilon = this->getEpsilon();
  const SbVec3f e(epsilon, epsilon, epsilon);
 
  for (unsigned int i = 0; i < iterationprims->numTriangles(); i++) {
    SbTri3f * t1 = (SbTri3f *) iterationprims->getTriangle(i);

    SbBox3f tribbox = t1->getBoundingBox();
    if (epsilon > 0.0f) {
      // Extend bbox in all 6 directions with the epsilon value.
      tribbox.getMin() -= e;
      tribbox.getMax() += e;
    }
    
    SbList<void*> candidatetris;
    octtree->findItems(tribbox, candidatetris);

    for (int j = 0; j < candidatetris.getLength(); j++) {
      SbTri3f * t2 = (SbTri3f *) candidatetris[j];

      nrisectchks++;

      if (t1->intersect(*t2, epsilon)) {
        nrhits++;

        SoIntersectingPrimitive p1;
        p1.path = iterationprims->getPath();
        p1.type = SoIntersectingPrimitive::TRIANGLE;
        t1->getValue(p1.xf_vertex[0], p1.xf_vertex[1], p1.xf_vertex[2]);
        iterationprims->invtransform.multVecMatrix(p1.xf_vertex[0], p1.vertex[0]);
        iterationprims->invtransform.multVecMatrix(p1.xf_vertex[1], p1.vertex[1]);
        iterationprims->invtransform.multVecMatrix(p1.xf_vertex[2], p1.vertex[2]);

        SoIntersectingPrimitive p2;
        p2.path = octtreeprims->getPath();
        p2.type = SoIntersectingPrimitive::TRIANGLE;
        t2->getValue(p2.xf_vertex[0], p2.xf_vertex[1], p2.xf_vertex[2]);
        octtreeprims->invtransform.multVecMatrix(p2.xf_vertex[0], p2.vertex[0]);
        octtreeprims->invtransform.multVecMatrix(p2.xf_vertex[1], p2.vertex[1]);
        octtreeprims->invtransform.multVecMatrix(p2.xf_vertex[2], p2.vertex[2]);

        int c;
        for ( c = 0; c < this->callbacks->getLength(); c += 2 ) {
          SoIntersectionDetectionAction::SoIntersectionCB * cb =
            (SoIntersectionDetectionAction::SoIntersectionCB *) (*(this->callbacks))[c];
          switch ( cb((*(this->callbacks))[c+1], &p1, &p2) ) {
          case SoIntersectionDetectionAction::NEXT_PRIMITIVE:
            // Break out of the switch, invoke next callback.
            break;
          case SoIntersectionDetectionAction::NEXT_SHAPE:
            // FIXME: remaining callbacks won't be invoked -- should they? 20030328 mortene.
            cont = TRUE;
            goto done;
          case SoIntersectionDetectionAction::ABORT:
            // FIXME: remaining callbacks won't be invoked -- should they? 20030328 mortene.
            cont = FALSE;
            goto done;
          default:
            assert(0);
          }
        }
      }
    }
  }

done:
  // for debugging
  if (ida_debug()) {
    const unsigned int total = primitives1->numTriangles() + primitives2->numTriangles();
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doPrimitiveIntersectionTesting",
                           "intersection checks = %d (pr primitive: %f)",
                           nrisectchks, float(nrisectchks) / total);
    SbString chksprhit;
    if (nrhits == 0) { chksprhit = "-"; }
    else { chksprhit.sprintf("%f", float(nrisectchks) / nrhits); }
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doPrimitiveIntersectionTesting",
                           "hits = %d (chks pr hit: %s)", nrhits, chksprhit.getString());
  }
}

// Does intersection testing internally within the same
// shape. Triangles are not tested against themselves.
//
// Can ignore epsilon setting, as that only indicates a distance
// between distinct shapes.
void
SoIntersectionDetectionActionP::doInternalPrimitiveIntersectionTesting(PrimitiveData * primitives,
                                                                     SbBool & cont)
{
  // for debugging
  if (ida_debug()) {
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doInternalPrimitiveIntersectionTesting",
                           "triangles shape = %d", primitives->numTriangles());
  }
  unsigned int nrisectchks = 0;

  // FIXME: use the SbOctTree optimization, as above. Should refactor
  // doPrimitiveIntersectionTesting() and
  // doInternalPrimitiveIntersectionTesting() into common
  // code. 20030328 mortene.

  cont = TRUE;
  const int numprimitives = primitives->numTriangles();
  for (int i = 0; i < numprimitives; i++ ) {
    SbTri3f * t1 = (SbTri3f *) primitives->getTriangle(i);
    for (int j = i + 1; j < numprimitives; j++ ) {
      SbTri3f * t2 = (SbTri3f *) primitives->getTriangle(j);
      nrisectchks++;
      if ( t1->intersect(*t2) ) {
        SoIntersectingPrimitive p1;
        p1.path = primitives->getPath();
        p1.type = SoIntersectingPrimitive::TRIANGLE;
        t1->getValue(p1.xf_vertex[0], p1.xf_vertex[1], p1.xf_vertex[2]);
        primitives->invtransform.multVecMatrix(p1.xf_vertex[0], p1.vertex[0]);
        primitives->invtransform.multVecMatrix(p1.xf_vertex[1], p1.vertex[1]);
        primitives->invtransform.multVecMatrix(p1.xf_vertex[2], p1.vertex[2]);

        SoIntersectingPrimitive p2;
        p2.path = primitives->getPath();
        p2.type = SoIntersectingPrimitive::TRIANGLE;
        t2->getValue(p2.xf_vertex[0], p2.xf_vertex[1], p2.xf_vertex[2]);
        primitives->invtransform.multVecMatrix(p2.xf_vertex[0], p2.vertex[0]);
        primitives->invtransform.multVecMatrix(p2.xf_vertex[1], p2.vertex[1]);
        primitives->invtransform.multVecMatrix(p2.xf_vertex[2], p2.vertex[2]);
	int c;
        for ( c = 0; c < this->callbacks->getLength(); c += 2 ) {
          SoIntersectionDetectionAction::SoIntersectionCB * cb =
            (SoIntersectionDetectionAction::SoIntersectionCB *) (*(this->callbacks))[c];
          switch ( cb((*(this->callbacks))[c+1], &p1, &p2) ) {
          case SoIntersectionDetectionAction::NEXT_PRIMITIVE:
            break;
          case SoIntersectionDetectionAction::NEXT_SHAPE:
            cont = TRUE;
            goto done;
          case SoIntersectionDetectionAction::ABORT:
            cont = FALSE;
            goto done;
	  default:
	    assert(0);
          }
        }
      }
    }
  }
 done:
  // for debugging
  if (ida_debug()) {
    SoDebugError::postInfo("SoIntersectionDetectionActionP::doInternalPrimitiveIntersectionTesting",
                           "intersection checks = %d", nrisectchks);
  }
}
