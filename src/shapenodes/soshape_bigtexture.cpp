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

#include "soshape_bigtexture.h"
#include <Inventor/SbClip.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/misc/SoGLBigImage.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/SbBox2f.h>
#include <Inventor/elements/SoCullElement.h>
#include <Inventor/elements/SoTextureMatrixElement.h>
#include <Inventor/elements/SoTextureImageElement.h>
#include <Inventor/bundles/SoMaterialBundle.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/C/tidbits.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <Inventor/system/gl.h>

soshape_bigtexture::soshape_bigtexture(void)
{
  this->clipper = NULL;
  this->pvlist = NULL;
  this->regions = NULL;
  this->numallocregions = 0;
}

soshape_bigtexture::~soshape_bigtexture()
{
  delete[] this->regions;
  delete this->clipper;
  
  if (this->pvlist) {
    int n = this->pvlist->getLength();
    for (int i = 0; i < n; i++) {
      delete (*this->pvlist)[i];
    }
    delete this->pvlist;
  }
}

void
soshape_bigtexture::beginShape(SoGLBigImage * imageptr,
                               const float qualityarg)
{
  this->image = imageptr;
  this->quality = qualityarg;
  this->pvlistcnt = 0;
  this->vertexlist.truncate(0);

  int num = imageptr->initSubImages(SbVec2s(256, 256));
  this->numregions = num;

  if (this->clipper == NULL) {
    this->clipper = new SbClip(clipcb, this);
    this->pvlist = new SbList <SoPrimitiveVertex*>;
    this->regions = new bt_region[num];
    this->numallocregions = num;
  }
  if (num > this->numallocregions) {
    delete[] this->regions;
    this->regions = new bt_region[num];
    this->numallocregions = num;
  }
  for (int i = 0; i < num; i++) {
    bt_region & reg = this->regions[i];
    reg.facelist.truncate(0);
    reg.pvlist.truncate(0);
    this->image->handleSubImage(i,
                                reg.start,
                                reg.end,
                                reg.tcmul);

    // The boundary planes are problematic because the endpositions
    // calculated by handleSubImage are greater than 1 in certain
    // circumstances. This is because SoGLBigImage divides the image
    // into equally sized tiles. Some of the bordertiles might have
    // endpoints outside of the original image. This causes the
    // clipping planes to be positioned wrongly, so we have to check
    // the bordercases, that is clamp the distance to be <= 1. This is
    // only the case for the endpoints, not the startpoints which
    // always are within the original texture.
    reg.planes[0] =
      SbPlane(SbVec3f(1.0f, 0.0f, 0.0f), reg.start[0]);
    reg.planes[1] =
      SbPlane(SbVec3f(0.0f, 1.0f, 0.0f), reg.start[1]);
    reg.planes[2] =
      SbPlane(SbVec3f(-1.0f, 0.0f, 0.0f), -SbMin(reg.end[0], 1.0f));
    reg.planes[3] =
      SbPlane(SbVec3f(0.0f, -1.0f, 0.0f), -SbMin(reg.end[1], 1.0f));
  }
}

SbBool
soshape_bigtexture::endShape(SoState * state,
                             SoShape * shape,
                             SoMaterialBundle & mb)
{
  this->clip_triangles(state);

  // clear texture matrix. We've already calculated the world space
  // texture coordinates.
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);

  // disable texgen functions, we always supply texture coordinates
  glPushAttrib(GL_ENABLE_BIT);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_TEXTURE_GEN_R);
  glDisable(GL_TEXTURE_GEN_Q);
  
  const int numreg = this->numregions;
  for (int i = 0; i < numreg; i++) {
    int numv, j;

    const bt_region & reg = this->regions[i];
    int numface = reg.facelist.getLength();
    if (numface == 0) continue;

    numv = reg.pvlist.getLength();
    SbBox3f bbox;
    for (j = 0; j < numv; j++) {
      bbox.extendBy(reg.pvlist[j]->getPoint());
    }
    SbVec2s rectsize;
    shape->getScreenSize(state, bbox, rectsize);
    this->image->applySubImage(state, i, this->quality, rectsize);
    int vcnt = 0;
    for (j = 0; j < numface; j++) {
      glBegin(GL_TRIANGLE_FAN);
      numv = reg.facelist[j];
      for (int k = 0; k < numv; k++) {
        SoPrimitiveVertex * v = reg.pvlist[vcnt++];
        SbVec4f tc = v->getTextureCoords();
        tc[0] -= reg.start[0];
        tc[1] -= reg.start[1];
        tc[0] /= (reg.end[0]-reg.start[0]);
        tc[1] /= (reg.end[1]-reg.start[1]);
        glTexCoord4fv(tc.getValue());
        glNormal3fv(v->getNormal().getValue());
        mb.send(v->getMaterialIndex(), TRUE);
        glVertex3fv(v->getPoint().getValue());
      }
      glEnd();
    }
  }  

  // enable texgen (if active)
  glPopAttrib();

  // restore texture matrix
  glMatrixMode(GL_TEXTURE);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);

  // return TRUE if all textures were created in the correct resolution
  return ! this->image->exceededChangeLimit();
}

void
soshape_bigtexture::triangle(SoState * state,
                             const SoPrimitiveVertex * v1,
                             const SoPrimitiveVertex * v2,
                             const SoPrimitiveVertex * v3)
{
  const SoPrimitiveVertex * vp[] = {v1, v2, v3};
  
  for (int i = 0; i < 3; i++) {
    SoPrimitiveVertex * pv = this->get_new_pv();
    *pv = *(vp[i]);
    this->vertexlist.append(pv);
  }
}

void 
soshape_bigtexture::clip_triangles(SoState * state)
{
  int n = this->vertexlist.getLength();
  if (n == 0) return;
  
  // need texture matrix to transform the texture coordinates
  SbMatrix texturematrix = SoTextureMatrixElement::get(state);
  int wrap[2];
  SbVec2s dummy;
  int dummync;
  SbColor dummycol;
  int dummymod;

  // need wrapS/T to figure out how to handle the texture coordinates
  (void) SoTextureImageElement::get(state, dummy, dummync,
                                    wrap[0], wrap[1], 
                                    dummymod, dummycol);
  SbVec4f tmp;
  int i, j;

  for (i = 0; i < n; i++) {
    tmp = this->vertexlist[i]->getTextureCoords();
    texturematrix.multVecMatrix(tmp, tmp);
    SbVec3f tmp3;
    tmp.getReal(tmp3);
    this->vertexlist[i]->setTextureCoords(tmp3);
  }

  // Clip the triangles agains the clippingplanes of the windows it
  // passes through. Windows can be seen as the integer components of
  // the texturecoordinate. The number of windows a triangle passes
  // through is indicated by the bounding box of the triangle's
  // texturecoordinates.
  for (i = 0; i < n; i += 3) {
    // Take care of the texturecoords for easy processing
    SbVec4f tc[3], tcf[3];
    tc[0] = this->vertexlist[i]->getTextureCoords();
    tc[1] = this->vertexlist[i+1]->getTextureCoords();
    tc[2] = this->vertexlist[i+2]->getTextureCoords();
    
    // Calculate triangle's texturecoordinate bounding box
    SbBox2f bbox;
    bbox.extendBy(SbVec2f(tc[0][0], tc[0][1]));
    bbox.extendBy(SbVec2f(tc[1][0], tc[1][1]));
    bbox.extendBy(SbVec2f(tc[2][0], tc[2][1]));
      
    // Find the min/max bounds of the bounding box
    SbVec2f bbmin = bbox.getMin();
    SbVec2f bbmax = bbox.getMax();
    
    // Find the intersecting windows
    const int windowstartidxx = (int)floor(bbmin[0]);
    const int windowstartidxy = (int)floor(bbmin[1]);
    const int windowendidxx = (int)ceil(bbmax[0]);
    const int windowendidxy = (int)ceil(bbmax[1]);

    // Do the actual clipping against the windows
    for (int wy = windowstartidxy; wy < windowendidxy; wy++) {
      for (int wx = windowstartidxx; wx < windowendidxx; wx++) {
        // Position the texturecoordinates according to the
        // current window.
        const int transs = -wx;
        const int transt = -wy;

        tcf[0] = tc[0];
        tcf[1] = tc[1];
        tcf[2] = tc[2];

        tcf[0][0] = tcf[0][0] + transs;
        tcf[0][1] = tcf[0][1] + transt;
        
        tcf[1][0] = tcf[1][0] + transs;
        tcf[1][1] = tcf[1][1] + transt;
        
        tcf[2][0] = tcf[2][0] + transs;
        tcf[2][1] = tcf[2][1] + transt;

        this->vertexlist[i]->setTextureCoords(tcf[0]);
        this->vertexlist[i+1]->setTextureCoords(tcf[1]);
        this->vertexlist[i+2]->setTextureCoords(tcf[2]);

        // Clip the triangle against the current window
        this->handle_triangle(state, 
                              this->vertexlist[i], 
                              this->vertexlist[i+1], 
                              this->vertexlist[i+2], 
                              wrap, transs, transt);
      }
    }
  }
}

void
soshape_bigtexture::handle_triangle(SoState * state,
                                    SoPrimitiveVertex * v1,
                                    SoPrimitiveVertex * v2,
                                    SoPrimitiveVertex * v3, 
                                    const int wrap[2], 
                                    const int transs, 
                                    const int transt)
{
  SbVec4f tc[3];
  tc[0] = v1->getTextureCoords();
  tc[1] = v2->getTextureCoords();
  tc[2] = v3->getTextureCoords();

  SbBox2f bbox;
  bbox.extendBy(SbVec2f(tc[0][0], tc[0][1]));
  bbox.extendBy(SbVec2f(tc[1][0], tc[1][1]));
  bbox.extendBy(SbVec2f(tc[2][0], tc[2][1]));
  SbBox2f regbbox;

  for (int i = 0; i < this->numregions; i++) {
    bt_region * reg = &this->regions[i];
    regbbox.makeEmpty();
    regbbox.extendBy(reg->start);
    regbbox.extendBy(reg->end);

    // check if there is a chance for an intersection
    if (regbbox.intersect(bbox)) {
      this->clipper->reset();

      // need copies
      SoPrimitiveVertex * pv1 = this->get_new_pv();
      *pv1 = *v1;
      SoPrimitiveVertex * pv2 = this->get_new_pv();
      *pv2 = *v2;
      SoPrimitiveVertex * pv3 = this->get_new_pv();
      *pv3 = *v3;

      this->clipper->addVertex(SbVec3f(tc[0][0], tc[0][1], 0.0f), pv1);
      this->clipper->addVertex(SbVec3f(tc[1][0], tc[1][1], 0.0f), pv2);
      this->clipper->addVertex(SbVec3f(tc[2][0], tc[2][1], 0.0f), pv3);

      this->clipper->clip(reg->planes[0]);
      this->clipper->clip(reg->planes[1]);
      this->clipper->clip(reg->planes[2]);
      this->clipper->clip(reg->planes[3]);

      const int numv = this->clipper->getNumVertices();
      if (numv >= 3) {
        int j, k;
        SbBox3f obox;
        for (j = 0; j < numv; j++) {
          SoPrimitiveVertex * v = (SoPrimitiveVertex*) this->clipper->getVertexData(j);
          obox.extendBy(v->getPoint());
        }

        if (!SoCullElement::cullTest(state, obox)) {

          if (wrap[0] == SoTextureImageElement::CLAMP ||
              wrap[1] == SoTextureImageElement::CLAMP) {

            // Clamp the texturecoordinates
            for (j = 0; j < numv; j++) {
              SoPrimitiveVertex * v = (SoPrimitiveVertex *) this->clipper->getVertexData(j);
              SbVec4f texcoord = v->getTextureCoords();
              
              // Clamp the texturecoordinates to be within the
              // clamp-region. Need to translate back to the texture
              // coordinates original position to do this. That is
              // because we are interested in the cases where the
              // texture coordinates go outside the default clipping
              // window.
              if (wrap[0] == SoTextureImageElement::CLAMP) {
                texcoord[0] = SbClamp(texcoord[0] - transs, 0.0f, 1.0f);
              }
              if (wrap[1] == SoTextureImageElement::CLAMP) {
                texcoord[1] = SbClamp(texcoord[1] - transt, 0.0f, 1.0f);
              }
              v->setTextureCoords(texcoord);
            }

            // Find the region the polygon belongs to. This has to be
            // done because the clamping alters the region. The mean
            // texturecoordinates of a polygon are used to find the
            // region the polygon belongs to.
            for (k = 0; k < this->numregions; k++) {
              reg = &this->regions[k];
                            
              // Find the mean texture coordinate for the
              // polygon. This will be used to find the texture
              // region.
              SbVec2f mean(0.0f, 0.0f);
              for (j = 0; j < numv; j++) {
                SoPrimitiveVertex * v = (SoPrimitiveVertex *) this->clipper->getVertexData(j);
                SbVec4f texcoord = v->getTextureCoords();
                
                mean[0] += texcoord[0];
                mean[1] += texcoord[1];
              }
              mean[0] /= (float)numv;
              mean[1] /= (float)numv;

              // Find the bounding box of the current region.
              regbbox.makeEmpty();
              regbbox.extendBy(reg->start);
              regbbox.extendBy(reg->end);
              
              // Test if the mean is inside the current region. If it
              // is, all the texture coordinates for the polygon
              // should be within the same region.
              if (regbbox.intersect(mean)) {
                break;
              }
            }
          }
          
          // Add the polygon to the region
          reg->facelist.append(numv);
          for (j = 0; j < numv; j++) {
            reg->pvlist.append((SoPrimitiveVertex *) this->clipper->getVertexData(j));
          }
        }
      }
    }
  }
}


SoPrimitiveVertex *
soshape_bigtexture::get_new_pv(void)
{
  if (this->pvlistcnt < this->pvlist->getLength())
    return (*this->pvlist)[this->pvlistcnt++];
  else {
    SoPrimitiveVertex * pv = new SoPrimitiveVertex;
    this->pvlistcnt++;
    this->pvlist->append(pv);
    return pv;
  }
}

void *
soshape_bigtexture::clipcb(const SbVec3f & v0, void * vdata0,
                           const SbVec3f & v1, void * vdata1,
                           const SbVec3f & newvertex,
                           void * userdata)
{
  soshape_bigtexture * thisp = (soshape_bigtexture*) userdata;

  SoPrimitiveVertex * pv0 = (SoPrimitiveVertex*) vdata0;
  SoPrimitiveVertex * pv1 = (SoPrimitiveVertex*) vdata1;

  float dist = (v1-v0).length();
  float newdist = (newvertex-v0).length();
  if (dist == 0.0f) newdist = 0.0f;
  else newdist /= dist;

  SoPrimitiveVertex * pv = thisp->get_new_pv();
  pv->setPoint(pv0->getPoint() + (pv1->getPoint()-pv0->getPoint()) * newdist);
  pv->setTextureCoords(SbVec2f(newvertex[0], newvertex[1]));
  pv->setNormal(pv0->getNormal() + (pv1->getNormal()-pv0->getNormal()) * newdist);
  pv->setMaterialIndex(pv0->getMaterialIndex());
  return pv;
}


