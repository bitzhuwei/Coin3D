/**************************************************************************\
 *
 *  This file is part of the Coin 3D visualization library.
 *  Copyright (C) 1998-2001 by Systems in Motion. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  version 2.1 as published by the Free Software Foundation. See the
 *  file LICENSE.LGPL at the root directory of the distribution for
 *  more details.
 *
 *  If you want to use Coin for applications not compatible with the
 *  LGPL, please contact SIM to acquire a Professional Edition license.
 *
 *  Systems in Motion, Prof Brochs gate 6, 7030 Trondheim, NORWAY
 *  http://www.sim.no support@sim.no Voice: +47 22114160 Fax: +47 22207097
 *
\**************************************************************************/

/*!
  \class SoGLRenderAction SoGLRenderAction.h Inventor/actions/SoGLRenderAction.h
  \brief The SoGLRenderAction class renders the scene graph with OpenGL calls.
  \ingroup actions

  Applying this method at a root node for a scene graph, path or
  pathlist will render all geometry contained within that instance to
  the current OpenGL context.
 */

// Metadon doc:
/*�
  FIXME. This is missing from the implementation:
  <ul>

  <li> the abort callback function is never called -- it _should_ be
       called for every node during traversal.

  <li> the pass callback function is never called.

  <li> the cachecontext setting for sharing OpenGL display lists is
       not used.

  <li> the UpdateArea setting isn't heeded.

  <li> the Smoothing flag does not influence rendering.

  <li> the PassUpdate flag is not used.

  <li> antialiased rendering is not done if number of rendering passes
       is larger than 1.

  </ul>
 */

/*!
  \typedef void SoGLRenderPassCB(void * userdata)

  Callback functions for the setPassCallback() method need to be of
  this type.

  \a userdata is a void pointer to any data the application need to
  know of in the callback function (like for instance a \e this
  pointer).

  \sa setPassCallback()
 */

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbPlane.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoSubActionP.h>
#include <Inventor/elements/SoDecimationPercentageElement.h>
#include <Inventor/elements/SoDecimationTypeElement.h>
#include <Inventor/elements/SoGLLightIdElement.h>
#include <Inventor/elements/SoGLNormalizeElement.h>
#include <Inventor/elements/SoGLRenderPassElement.h>
#include <Inventor/elements/SoGLShadeModelElement.h>
#include <Inventor/elements/SoGLUpdateAreaElement.h>
#include <Inventor/elements/SoGLViewportRegionElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoShapeStyleElement.h>
#include <Inventor/elements/SoTextureOverrideElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoWindowElement.h>
#include <Inventor/lists/SoEnabledElementsList.h>
#include <Inventor/misc/SoGL.h>
#include <Inventor/misc/SoState.h>
#include <Inventor/nodes/SoNode.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H
#include <GL/gl.h>

// *************************************************************************

/*!
  \enum SoGLRenderAction::TransparencyType

  Various settings for how to do rendering of transparent objects in
  the scene. Some of the settings will provide faster rendering, while
  others gives you better quality rendering.
*/

/*!
  \enum SoGLRenderAction::AbortCode

  The return codes which an SoGLRenderAbortCB callback function should
  use.
*/
/*!
  \var SoGLRenderAction::AbortCode SoGLRenderAction::CONTINUE
  Continue rendering as usual.
*/
/*!
  \var SoGLRenderAction::AbortCode SoGLRenderAction::ABORT
  Abort the rendering action immediately.
*/
/*!
  \var SoGLRenderAction::AbortCode SoGLRenderAction::PRUNE
  Do not render the current node or any of its children, but continue
  the rendering traversal.
*/
/*!
  \var SoGLRenderAction::AbortCode SoGLRenderAction::DELAY
  Delay rendering of the current node (and its children) until the
  next rendering pass.
*/

/*!
  \typedef typedef SoGLRenderAction::SoGLRenderAbortCB(void * userdata)
  Abort callbacks should be of this type.
  \sa setAbortCallback()
*/


#ifndef DOXYGEN_SKIP_THIS

class SoGLRenderActionP {
public:
  SoGLRenderActionP(SoGLRenderAction * action) 
    : action(action) { }

  SoGLRenderAction * action;
  SbViewportRegion viewport;
  int numpasses;
  SoGLRenderAction::TransparencyType transparencytype;
  SbBool smoothing;
  SbBool passupdate;
  SoGLRenderPassCB * passcallback;
  void * passcallbackdata;
  SoGLRenderAction::SoGLRenderAbortCB * abortcallback;
  void * abortcallbackdata;
  uint32_t cachecontext;
  int currentpass;
  SbBool didhavetransparent;
  SbBool isblendenabled;
  SoPathList delayedpaths;
  SbBool delayedrender;
  SbBool delayedpathrender;
  SbBool sortrender;
  SoPathList transpobjpaths;
  SbList<float> transpobjdistances;
  SoGetBoundingBoxAction * bboxaction;
  SbVec2f updateorigin, updatesize;
  SbBool renderingremote;
  SbBool needglinit;
  SbBool isrendering;
public:
  void disableBlend(const SbBool force = FALSE);
  void enableBlend(const SbBool force = FALSE);

  void render(SoNode * node);
  void renderMulti(SoNode * node);
  void renderSingle(SoNode * node);
};

#endif // DOXYGEN_SKIP_THIS

SO_ACTION_SOURCE(SoGLRenderAction);

// Override from parent class.
void
SoGLRenderAction::initClass(void)
{
  SO_ACTION_INIT_CLASS(SoGLRenderAction, SoAction);

  SO_ENABLE(SoGLRenderAction, SoDecimationPercentageElement);
  SO_ENABLE(SoGLRenderAction, SoDecimationTypeElement);
  SO_ENABLE(SoGLRenderAction, SoGLLightIdElement);
  SO_ENABLE(SoGLRenderAction, SoGLNormalizeElement);
  SO_ENABLE(SoGLRenderAction, SoGLRenderPassElement);
  SO_ENABLE(SoGLRenderAction, SoGLShadeModelElement);
  SO_ENABLE(SoGLRenderAction, SoGLUpdateAreaElement);
  SO_ENABLE(SoGLRenderAction, SoLazyElement);
  SO_ENABLE(SoGLRenderAction, SoOverrideElement);
  SO_ENABLE(SoGLRenderAction, SoTextureOverrideElement);
  SO_ENABLE(SoGLRenderAction, SoWindowElement);
  SO_ENABLE(SoGLRenderAction, SoGLViewportRegionElement);
  SO_ENABLE(SoGLRenderAction, SoGLCacheContextElement);
}

// *************************************************************************

#ifdef THIS
#define COIN_THIS_DEFINE_COPY THIS
#undef THIS
#endif // THIS

#define THIS this->pimpl

/*!
  Constructor. Sets up the render action for rendering within the
  given \a viewportregion.
*/
SoGLRenderAction::SoGLRenderAction(const SbViewportRegion & viewportregion)
{
  SO_ACTION_CONSTRUCTOR(SoGLRenderAction);

  SO_ACTION_ADD_METHOD_INTERNAL(SoNode, SoNode::GLRenderS);

  THIS = new SoGLRenderActionP(this);
  // Can't just push this on the SoViewportRegionElement stack, as the
  // state hasn't been made yet.
  THIS->viewport = viewportregion;

  THIS->passcallback = NULL;
  THIS->passcallbackdata = NULL;
  THIS->smoothing = FALSE;
  THIS->numpasses = 1;
  THIS->transparencytype = SoGLRenderAction::SCREEN_DOOR;
  THIS->delayedrender = FALSE;
  THIS->delayedpathrender = FALSE;
  THIS->isrendering = FALSE;
  THIS->sortrender = FALSE;
  THIS->isblendenabled = FALSE;
  THIS->passupdate = FALSE;
  THIS->bboxaction = NULL;
  THIS->updateorigin.setValue(0.0f, 0.0f);
  THIS->updatesize.setValue(1.0f, 1.0f);
  THIS->renderingremote = FALSE;
  THIS->abortcallback = NULL;
  THIS->cachecontext = 0;
  THIS->needglinit = TRUE;
}

/*!
  Destructor, frees up all internal resources for action instance.
*/
SoGLRenderAction::~SoGLRenderAction()
{
  delete THIS->bboxaction;
  delete THIS;
}

/*!
  Sets the viewport region for rendering. This will then override the
  region passed in with the constructor.
*/
void
SoGLRenderAction::setViewportRegion(const SbViewportRegion & newregion)
{
  THIS->viewport = newregion;

  // The SoViewportRegionElement is not set here, as it is always
  // initialized before redraw in beginTraversal().
}

/*!
  Returns the viewport region for the rendering action.
*/
const SbViewportRegion &
SoGLRenderAction::getViewportRegion(void) const
{
  return THIS->viewport;
}

/*!
  Sets the area of the OpenGL context canvas we should render into.

  The coordinates for \a origin and \a size should be normalized to be
  within [0.0, 1.0]. The default settings are <0.0, 0.0> for the \a
  origin and <1.0, 1.0> for the \a size, using the full size of the
  rendering canvas.
*/
void
SoGLRenderAction::setUpdateArea(const SbVec2f & origin, const SbVec2f & size)
{
  THIS->updateorigin = origin;
  THIS->updatesize = size;
}

/*!
  Returns information about the area of the rendering context window
  to be updated.
*/
void
SoGLRenderAction::getUpdateArea(SbVec2f & origin, SbVec2f & size) const
{
  origin = THIS->updateorigin;
  size = THIS->updatesize;
}

/*!
  Sets the abort callback.  The abort callback is called by the action
  for each node during traversal to check for abort conditions.

  \sa SoGLRenderAction::AbortCode
*/
void
SoGLRenderAction::setAbortCallback(SoGLRenderAbortCB * const func,
                                   void * const userdata)
{
  THIS->abortcallback = func;
  THIS->abortcallbackdata = userdata;
}

/*!
  Sets the transparency rendering method for transparent objects in
  the scene graph.

  \sa SoGLRenderAction::TransparencyType
*/
void
SoGLRenderAction::setTransparencyType(const TransparencyType type)
{
  if (THIS->transparencytype != type) {
    THIS->transparencytype = type;
    THIS->needglinit = TRUE;
  }
}

/*!
  Returns the transparency rendering type.
*/
SoGLRenderAction::TransparencyType
SoGLRenderAction::getTransparencyType(void) const
{
  return THIS->transparencytype;
}

/*!
  Sets (or unsets) smoothing. If the smoothing flag is \c on, Coin
  will try to use built-in features from the OpenGL implementation to
  smooth the appearance of otherwise jagged borders.

  This is a simple (and computationally non-intensive) way of doing
  anti-aliasing.

  Default value for this flag is to be \c off.
*/
void
SoGLRenderAction::setSmoothing(const SbBool smooth)
{
  if (smooth != THIS->smoothing) {
    THIS->smoothing = smooth;
    THIS->needglinit = TRUE;
  }
}

/*!
  Returns whether smoothing is set or not.
*/
SbBool
SoGLRenderAction::isSmoothing(void) const
{
  return THIS->smoothing;
}

/*!
  Sets the number of rendering passes.  Default is 1, anything greater
  will enable antialiasing.
*/
void
SoGLRenderAction::setNumPasses(const int num)
{
  THIS->numpasses = num;
}

/*!
  Returns the number of rendering passes done on updates.
*/
int
SoGLRenderAction::getNumPasses(void) const
{
  return THIS->numpasses;
}

/*!
  Sets whether each pass should render to screen or not.
*/
void
SoGLRenderAction::setPassUpdate(const SbBool flag)
{
  THIS->passupdate = flag;
}

/*!
  Returns the value of the "show intermediate updates" flag.

  \sa setPassUpdate()
*/
SbBool
SoGLRenderAction::isPassUpdate(void) const
{
  return THIS->passupdate;
}

/*!
  Sets the pass callback.  The callback is called between each
  rendering pass.
*/
void
SoGLRenderAction::setPassCallback(SoGLRenderPassCB * const func,
                                  void * const userdata)
{
  THIS->passcallback = func;
  THIS->passcallbackdata = userdata;
}

/*!
  Sets the OpenGL cache context key, which is used for deciding when
  to share OpenGL display lists.
*/
void
SoGLRenderAction::setCacheContext(const uint32_t context)
{
  if (context != THIS->cachecontext) {
    THIS->cachecontext = context;
    this->invalidateState();
  }
}

/*!
  Returns the cache context key for this rendering action instance.
*/
uint32_t
SoGLRenderAction::getCacheContext(void) const
{
  return THIS->cachecontext;
}

/*!
  Overloaded from parent class to initialize the OpenGL state.
*/
void
SoGLRenderAction::beginTraversal(SoNode * node)
{
  if (THIS->isrendering) {
    inherited::beginTraversal(node);
    return;
  }
  if (THIS->needglinit) {
    THIS->needglinit = FALSE;

    // we are always using GL_COLOR_MATERIAL in Coin
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);

    // force blending to off in case GL state is invalid
    THIS->disableBlend(TRUE);

    switch(THIS->transparencytype) {
    case BLEND:
    case DELAYED_BLEND:
    case SORTED_OBJECT_BLEND:
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      break;
    case ADD:
    case DELAYED_ADD:
    case SORTED_OBJECT_ADD:
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      break;
    case SCREEN_DOOR:
      // needed for line smoothing to work properly
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      break;
    }
    if (THIS->smoothing) {
      glEnable(GL_POINT_SMOOTH);
      glEnable(GL_LINE_SMOOTH);
    }
    else {
      glDisable(GL_POINT_SMOOTH);
      glDisable(GL_LINE_SMOOTH);
    }
  }
  THIS->render(node);
}

/*!
  Overloaded from parent class to clean up the lists of objects which
  were included in the delayed rendering.
 */
void
SoGLRenderAction::endTraversal(SoNode * node)
{
  inherited::endTraversal(node);
}

/*!
  Used by shape nodes or others which need to know whether or not they
  should immediately render themselves or if they should wait until
  the next pass.
*/
SbBool
SoGLRenderAction::handleTransparency(SbBool istransparent)
{
  if (THIS->delayedpathrender) { // special case when rendering delayed paths
    if (!istransparent) {
      THIS->disableBlend();
    }
    else {
      if (THIS->transparencytype != SCREEN_DOOR) THIS->enableBlend();
      else THIS->disableBlend();
    }
    return FALSE; // always render
  }

  // normal case
  if (istransparent) THIS->didhavetransparent = TRUE;
  if (THIS->transparencytype == DELAYED_ADD ||
      THIS->transparencytype == DELAYED_BLEND) {
    if (THIS->delayedrender) return !istransparent;
    return istransparent;
  }
  else if (THIS->transparencytype == SORTED_OBJECT_ADD ||
           THIS->transparencytype == SORTED_OBJECT_BLEND) {
    if (THIS->sortrender || !istransparent) return FALSE;
    this->addTransPath(this->getCurPath()->copy());
    return TRUE;
  }
  else if (THIS->transparencytype == ADD || THIS->transparencytype == BLEND) {
    if (istransparent) THIS->enableBlend();
    else THIS->disableBlend();
    return FALSE; // node should always render
  }
  else
    return FALSE; // polygon stipple used to render
}

/*!
  Returns the number of the current rendering pass.
*/

int
SoGLRenderAction::getCurPass(void) const
{
  return THIS->currentpass;
}

/*!
  Returns \c TRUE if the render action should abort now based on user
  callback.

  \sa setAbortCallback()
*/
SbBool
SoGLRenderAction::abortNow(void)
{
  if (this->hasTerminated()) return TRUE;
  SbBool abort = FALSE;
  if (THIS->abortcallback) {
    switch (THIS->abortcallback(THIS->abortcallbackdata)) {
    case CONTINUE:
      break;
    case ABORT:
      this->setTerminated(TRUE);
      abort = TRUE;
      break;
    case PRUNE:
      // abort this node, but do not abort rendering
      abort = TRUE;
      break;
    case DELAY:
      this->addDelayedPath((SoPath*)this->getCurPath());
      // prune this node
      abort = TRUE;
      break;
    }
  }
  return abort;
}

/*!
  Let SoGLRenderAction instance know if application is running on the
  local machine or if the rendering instructions are sent over the
  network.

  This call has no effect in Coin for now, but eventually it will be
  used to optimize rendering (the displaylist caching strategy should
  be influenced by this flag to be more aggressive with the caching
  when rendering instructions are passed over the network).

  \sa getRenderingIsRemote()
 */
void
SoGLRenderAction::setRenderingIsRemote(SbBool isremote)
{
  THIS->renderingremote = isremote;
}

/*!
  Returns whether or not the application is running remotely.

  \sa setRenderingIsRemote()
 */
SbBool
SoGLRenderAction::getRenderingIsRemote(void) const
{
  return THIS->renderingremote;
}

/*!
  Adds a path to the list of paths to render after the current pass.
 */
void
SoGLRenderAction::addDelayedPath(SoPath * path)
{
  assert(!THIS->delayedpathrender);
  SoPath * copy = path->copy();
  THIS->delayedpaths.append(copy);
}

/*!
  Returns a flag indicating whether or not we are currently rendering
  from the list of delayed paths of the scene graph.
 */
SbBool
SoGLRenderAction::isRenderingDelayedPaths(void) const
{
  return THIS->delayedpathrender;
}

// Remember a path containing a transparent object for later
// rendering.
void
SoGLRenderAction::addTransPath(SoPath * path)
{
  // Do this first to increase the reference count (we want to avoid a
  // zero refcount for the bboxaction apply()).
  THIS->transpobjpaths.append(path);

  if (THIS->bboxaction == NULL) {
    THIS->bboxaction =
      new SoGetBoundingBoxAction(SoViewportRegionElement::get(this->state));
  }
  THIS->bboxaction->setViewportRegion(SoViewportRegionElement::get(this->state));
  THIS->bboxaction->apply(path);
  SbVec3f center = THIS->bboxaction->getBoundingBox().getCenter();
  SoModelMatrixElement::get(this->state).multVecMatrix(center, center);
  float dist = SoViewVolumeElement::get(this->state).getPlane(0.0f).getDistance(center);
  THIS->transpobjdistances.append(dist);
}

/*!
  Overloaded to reinitialize GL state on next apply.
*/
void
SoGLRenderAction::invalidateState(void)
{
  inherited::invalidateState();
  THIS->needglinit = TRUE;
}

// Sort paths with transparent objects before rendering.
void
SoGLRenderAction::doPathSort(void)
{
  // need to cast to SbPList to avoid ref/unref problems
  SbPList * plist = (SbPList *)&THIS->transpobjpaths;
  float * darray = (float *)THIS->transpobjdistances.getArrayPtr();

  int i, j, distance, n = THIS->transpobjdistances.getLength();
  void * ptmp;
  float dtmp;

  // shell sort algorithm (O(nlog(n))
  for (distance = 1; distance <= n/9; distance = 3*distance + 1);
  for (; distance > 0; distance /= 3) {
    for (i = distance; i < n; i++) {
      dtmp = darray[i];
      ptmp = plist->get(i);
      j = i;
      while (j >= distance && darray[j-distance] > dtmp) {
        darray[j] = darray[j-distance];
        plist->set(j, plist->get(j-distance));
        j -= distance;
      }
      darray[j] = dtmp;
      plist->set(j, ptmp);
    }
  }
}

#undef THIS

#ifdef COIN_THIS_DEFINE_COPY
#define THIS COIN_THIS_DEFINE_COPY
#endif // COIN_THIS_DEFINE_COPY


// methods in SoGLRenderActionP

// Enable OpenGL blending.
void
SoGLRenderActionP::enableBlend(const SbBool force)
{
  if (force || !this->isblendenabled) {
    glEnable(GL_BLEND);
    if (!this->delayedpathrender && 
        this->transparencytype != SoGLRenderAction::SCREEN_DOOR) {
      glDepthMask(GL_FALSE);
    }
    this->isblendenabled = TRUE;
  }
}

// Disable OpenGL blending.
void
SoGLRenderActionP::disableBlend(const SbBool force)
{
  if (force || this->isblendenabled) {
    glDisable(GL_BLEND);
    if (!this->delayedpathrender && 
        this->transparencytype != SoGLRenderAction::SCREEN_DOOR) {
      glDepthMask(GL_TRUE);
    }
    this->isblendenabled = FALSE;
  }
}

void 
SoGLRenderActionP::render(SoNode * node)
{
  this->isrendering = TRUE;

  this->currentpass = 0;

  if (this->transparencytype == SoGLRenderAction::SCREEN_DOOR && 
      this->smoothing) {
    this->enableBlend(); // needed for line smoothing
  }
  else {
    this->disableBlend();
  }

  SoState * state = this->action->getState();
  state->push();

  SoShapeStyleElement::setTransparencyType(state,
                                           this->transparencytype);

  SoViewportRegionElement::set(state, this->viewport);
  SoLazyElement::setTransparencyType(state,
                                     (int32_t)this->transparencytype);
  SoLazyElement::setBlending(state, FALSE);
  SoLazyElement::setColorMaterial(state, TRUE);

  // FIXME: is this the correct place to set these elements? 19990314 mortene.
  SoDecimationPercentageElement::set(state, 1.0f);
  SoDecimationTypeElement::set(state,
                               SoDecimationTypeElement::AUTOMATIC);

  // FIXME: use these as they're supposed to be used. 19990314 mortene.
  SoGLUpdateAreaElement::set(state,
                             SbVec2f(0.0f, 0.0f), SbVec2f(1.0f, 1.0f));

  SoGLCacheContextElement::set(state, this->cachecontext,
                               FALSE, this->renderingremote);

  if (this->action->getNumPasses() > 1) {
    this->renderMulti(node);
  }
  else {
    this->renderSingle(node);
  }

  state->pop();
  this->isrendering = FALSE;
}

void
SoGLRenderActionP::renderMulti(SoNode * node)
{
  assert(this->numpasses > 1);
  float fraction = 1.0f / float(this->numpasses);

  this->renderSingle(node);
  glAccum(GL_LOAD, fraction);
  
  for (int i = 1; i < this->numpasses; i++) {
    if (this->passupdate) {
      glAccum(GL_RETURN, float(this->numpasses) / float(i));
    }
    if (this->passcallback) this->passcallback(this->passcallbackdata);
    this->currentpass = i;
    this->renderSingle(node);
    
    if (this->action->hasTerminated()) return;
    glAccum(GL_ACCUM, fraction);
  }
  glAccum(GL_RETURN, 1.0f);
}

void
SoGLRenderActionP::renderSingle(SoNode * node)
{
  SoState * state = this->action->getState();

  SoGLRenderPassElement::set(state, this->currentpass);
  SoGLCacheContextElement::set(state, this->cachecontext,
                               FALSE, this->renderingremote);

  assert(this->sortrender == FALSE);
  assert(this->delayedrender == FALSE);
  assert(this->delayedpathrender == FALSE);

  this->didhavetransparent = FALSE;
  this->action->beginTraversal(node);

  if (this->didhavetransparent) {
    assert(!this->sortrender);
    if (this->transparencytype == SoGLRenderAction::DELAYED_BLEND ||
        this->transparencytype == SoGLRenderAction::DELAYED_ADD) {
      SoGLCacheContextElement::set(state, this->cachecontext,
                                   TRUE, this->renderingremote);
      this->enableBlend();
      this->delayedrender = TRUE;
      this->action->beginTraversal(node);
      this->delayedrender = FALSE;
      this->disableBlend();
    }
    else if (this->transparencytype == SoGLRenderAction::SORTED_OBJECT_BLEND ||
             this->transparencytype == SoGLRenderAction::SORTED_OBJECT_ADD) {
      SoGLCacheContextElement::set(state, this->cachecontext,
                                   TRUE, this->renderingremote);
      this->sortrender = TRUE;
      this->action->doPathSort();
      this->enableBlend();
      this->action->apply(this->transpobjpaths, TRUE);
      this->disableBlend();
      this->sortrender = FALSE;
      this->transpobjpaths.truncate(0);
      this->transpobjdistances.truncate(0);
    }
  }

  this->disableBlend();

  if (this->delayedpaths.getLength()) {
    this->delayedpathrender = TRUE;
    this->action->apply(this->delayedpaths, TRUE);
    this->delayedpathrender = FALSE;
    this->delayedpaths.truncate(0);
  }  
}
