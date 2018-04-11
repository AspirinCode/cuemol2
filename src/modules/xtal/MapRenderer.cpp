// -*-Mode: C++;-*-
//
// superclass of density-map renderers
//
// $Id: MapRenderer.cpp,v 1.5 2011/01/08 18:28:29 rishitani Exp $

#include <common.h>

#include "MapRenderer.hpp"
#include "DensityMap.hpp"
// #include <qsys/ScalarObject.hpp>
#include <gfx/SolidColor.hpp>

#include <qsys/Scene.hpp>
#include <modules/molstr/AtomIterator.hpp>

using namespace xtal;
using qsys::ScalarObject;
using molstr::AtomIterator;
using qlib::Matrix3D;
using qlib::Matrix4D;

// default constructor
MapRenderer::MapRenderer()
     : super_t()
{
  //m_pcolor = gfx::SolidColor::createRGB(0.0, 0.0, 1.0);
  //m_dSigLevel = 1.1;
  //m_dMapRange = 15.0;

  m_bUseMolBndry = false;

  m_bPBC = false;
  m_nMaxGrid = 100;

  m_bUseAbsLev = false;
  m_pGrad = qsys::MultiGradientPtr(MB_NEW qsys::MultiGradient());

  super_t::setupParentData("multi_grad");
}

// destructor
MapRenderer::~MapRenderer()
{
}

bool MapRenderer::isCompatibleObj(qsys::ObjectPtr pobj) const
{
  ScalarObject *ptest = dynamic_cast<ScalarObject *>(pobj.get());
  return ptest!=NULL;
}

LString MapRenderer::toString() const
{
  return LString::format("MapRenderer %p", this);
}

double MapRenderer::getMaxLevel() const
{
  MapRenderer *pthis = const_cast<MapRenderer *>(this);
  ScalarObject *pMap = (ScalarObject *) pthis->getClientObj().get();

  double sig = pMap->getRmsdDensity();
  //if (qlib::isNear4(sig, 0.0))
  //return 0.0;
  return pMap->getMaxDensity()/sig;
}

double MapRenderer::getMinLevel() const
{
  MapRenderer *pthis = const_cast<MapRenderer *>(this);
  ScalarObject *pMap = (ScalarObject *) pthis->getClientObj().get();

  double sig = pMap->getRmsdDensity();
  //if (qlib::isNear4(sig, 0.0))
  //return 0.0;
  return pMap->getMinDensity()/sig;
}

double MapRenderer::getLevel() const
{
  MapRenderer *pthis = const_cast<MapRenderer *>(this);
  ScalarObject *pMap = (ScalarObject *) pthis->getClientObj().get();
  double sig = pMap->getRmsdDensity();
  return m_dSigLevel * sig;
}

void MapRenderer::setLevel(double value)
{
  MapRenderer *pthis = const_cast<MapRenderer *>(this);
  ScalarObject *pMap = (ScalarObject *) pthis->getClientObj().get();
  double sig = pMap->getRmsdDensity();
  setSigLevel(value/sig);
}

double MapRenderer::getMaxExtent() const
{
  MapRenderer *pthis = const_cast<MapRenderer *>(this);
  ScalarObject *pMap = (ScalarObject *) pthis->getClientObj().get();

  double grdsz = 1.0;

  if (pMap!=NULL)
    grdsz = qlib::min(pMap->getColGridSize(),
                      qlib::min(pMap->getRowGridSize(),
                                pMap->getSecGridSize()));

  return m_nMaxGrid * pMap->getColGridSize() / 2.0;
}


///////////////////////////////////////////////////
// Common implementations

/// Calculate 8-bit contour level
void MapRenderer::calcContLevel(ScalarObject *pMap)
{
  //
  // calculate the contour level
  //
  const double siglevel = getSigLevel();
  const double level = pMap->getRmsdDensity() * siglevel;
  double lvtmp = floor( (level-pMap->getLevelBase()) / pMap->getLevelStep());
  unsigned int lv = (unsigned int)lvtmp;
  if (lvtmp<0) lv = 0;
  if (lvtmp>0xFF) lv = 0xFF;
  
  MB_DPRINTLN("set isolevel=%d", lv);
  m_nIsoLevel = lv;
}

namespace {
  inline void calcAxisCross(const Vector4D &aax, const Vector4D &bax, const Vector4D &cax, const Vector4D &cent, double extent,
                            double &zmin, double &zmax)
  {
    Vector4D vn = aax.cross(bax);
    const double vncen = vn.dot(cent);
    const double vncax = vn.dot(cax);
    const double evn = extent*vn.length();
    zmin = (vncen - evn)/vncax;
    zmax = (vncen + evn)/vncax;
  }
}

/// Setup map rendering information (extent, level, etc)
void MapRenderer::calcMapDispExtent(ScalarObject *pMap)
{
  DensityMap *pXtal = dynamic_cast<DensityMap *>(pMap);

  m_mapSize.x() = pMap->getColNo();
  m_mapSize.y() = pMap->getRowNo();
  m_mapSize.z() = pMap->getSecNo();

  Vector4D cent = getCenter();
  const double extent = getExtent();

  // check the object's xform matrix
  //  & xform the display center
  const Matrix4D &xfm = pMap->getXformMatrix();
  if (!xfm.isIdent()) {
    // apply inv of xformMat
    Matrix3D rmat = xfm.getMatrix3D();
    rmat = rmat.invert();
    Vector4D tr = xfm.getTransPart();
    cent -= tr;
    cent = rmat.mulvec(cent);
  }

  //
  // Calc vmin, vmax
  //
  Vector4D vmin, vmax;

  if (pXtal!=NULL) {
    // non-orthogonal grid (crystal, etc)
    const CrystalInfo &xt = pXtal->getXtalInfo();

    // check PBC
    m_bPBC = false;
    const double dimx = pMap->getColGridSize()*pMap->getColNo();
    const double dimy = pMap->getRowGridSize()*pMap->getRowNo();
    const double dimz = pMap->getSecGridSize()*pMap->getSecNo();
    const double cea = xt.a();
    const double ceb = xt.b();
    const double cec = xt.c();
    if (qlib::isNear4(dimx, cea) &&
        qlib::isNear4(dimy, ceb) &&
        qlib::isNear4(dimz, cec))
      m_bPBC = true;

    Vector4D aax(1.0, 0.0, 0.0, 1.0);
    Vector4D bax(0.0, 1.0, 0.0, 1.0);
    Vector4D cax(0.0, 0.0, 1.0, 1.0);

    xt.fracToOrth(aax);
    xt.fracToOrth(bax);
    xt.fracToOrth(cax);

    calcAxisCross(aax, bax, cax, cent, extent, vmin.z(), vmax.z());
    calcAxisCross(bax, cax, aax, cent, extent, vmin.x(), vmax.x());
    calcAxisCross(cax, aax, bax, cent, extent, vmin.y(), vmax.y());
    
    vmin.x() *= pXtal->getColInterval();
    vmin.y() *= pXtal->getRowInterval();
    vmin.z() *= pXtal->getSecInterval();
    vmax.x() *= pXtal->getColInterval();
    vmax.y() *= pXtal->getRowInterval();
    vmax.z() *= pXtal->getSecInterval();
  }
  else {
    // orthogonal grid (potential map, etc)
    vmin = Vector4D(cent.x()-extent, cent.y()-extent, cent.z()-extent);
    vmax = Vector4D(cent.x()+extent, cent.y()+extent, cent.z()+extent);

    // get map origin / translate the origin to (0,0,0)
    vmin -= pMap->getOrigin();
    vmax -= pMap->getOrigin();

    vmin.x() /= pMap->getColGridSize();
    vmin.y() /= pMap->getRowGridSize();
    vmin.z() /= pMap->getSecGridSize();
    vmax.x() /= pMap->getColGridSize();
    vmax.y() /= pMap->getRowGridSize();
    vmax.z() /= pMap->getSecGridSize();
  }

  if (!isUsePBC() || !m_bPBC) {
    // Not in PBC mode
    // --> limit XYZ in the available region of map
    vmin.x() = floor(qlib::max<double>(vmin.x(), pMap->getStartCol()));
    vmin.y() = floor(qlib::max<double>(vmin.y(), pMap->getStartRow()));
    vmin.z() = floor(qlib::max<double>(vmin.z(), pMap->getStartSec()));
    
    vmax.x() = floor(qlib::min<double>(vmax.x(), pMap->getStartCol()+pMap->getColNo()));
    vmax.y() = floor(qlib::min<double>(vmax.y(), pMap->getStartRow()+pMap->getRowNo()));
    vmax.z() = floor(qlib::min<double>(vmax.z(), pMap->getStartSec()+pMap->getSecNo()));
  }

  m_glbStPos = Vector3I(vmin.xyz());

  // conv to map-base index (from global origin)
  m_mapStPos.x() = m_glbStPos.x() - pMap->getStartCol();
  m_mapStPos.y() = m_glbStPos.y() - pMap->getStartRow();
  m_mapStPos.z() = m_glbStPos.z() - pMap->getStartSec();

  // actual display extent (in grid unit)
  m_dspSize = Vector3I( (vmax-vmin).xyz() );
  
}

Matrix4D MapRenderer::calcXformMat(ScalarObject *pMap, DensityMap *pXtal)
{
  Matrix4D rval;

  // check the object's xform matrix
  const Matrix4D &xfm = pMap->getXformMatrix();
  if (!xfm.isIdent()) {
    rval = xfm;
  }

  //  setup frac-->orth matrix
  if (pXtal==NULL) {
    rval.translate(pMap->getOrigin());
  }
  else {
    Matrix3D orthmat = pXtal->getXtalInfo().getOrthMat();
    rval.matprod(Matrix4D(orthmat));
  }

  Vector4D vtmp;
  if (pXtal!=NULL)
    vtmp = Vector4D(1.0/double(pXtal->getColInterval()),
                    1.0/double(pXtal->getRowInterval()),
                    1.0/double(pXtal->getSecInterval()));
  else
    vtmp = Vector4D(pMap->getColGridSize(),
                    pMap->getRowGridSize(),
                    pMap->getSecGridSize());
  
  //pdc->scale(vtmp);
  rval.matprod(Matrix4D::makeScaleMat(vtmp));
  
  vtmp = Vector4D(getGlbStPos());
  //pdc->translate(vtmp);
  rval.translate(vtmp);

  return rval;
}

Matrix4D MapRenderer::calcNormMat(ScalarObject *pMap, DensityMap *pXtal)
{
  Matrix4D rval;

  // check the object's xform matrix
  const Matrix4D &xfm = pMap->getXformMatrix();
  if (!xfm.isIdent()) {
    rval = xfm;
  }

  return rval;
}

void MapRenderer::setupXform(gfx::DisplayContext *pdc, ScalarObject *pMap, DensityMap *pXtal)
{
  pdc->multMatrix( calcXformMat(pMap, pXtal) );
}

///////////////////////////////////////////////////
// Mol boundary mode routines

void MapRenderer::setBndryMolName(const LString &s)
{
  if (s.equals(m_strBndryMol))
    return;
  m_strBndryMol = s;

  /// target mol is changed-->redraw map
  invalidateDisplayCache();
}

void MapRenderer::setBndrySel(const SelectionPtr &pSel)
{
  ensureNotNull(pSel);
  
  if (!m_pSelBndry.isnull())
    if (m_pSelBndry->equals(pSel.get()))
      return;

  m_pSelBndry = pSel;
  //setupMolBndry();

  /// selection is changed-->redraw map
  invalidateDisplayCache();
}

void MapRenderer::setBndryRng(double d)
{
  if (qlib::isNear4(d, m_dBndryRng))
    return;
  m_dBndryRng = d;
  if (m_dBndryRng<0.0)
    m_dBndryRng = 0.0;
  // setupMolBndry();

  //if (m_bUseMolBndry)
  invalidateDisplayCache();
}

void MapRenderer::setupMolBndry()
{
  m_boundary.clear();
  m_bUseMolBndry = false;

  if (m_strBndryMol.isEmpty())
    return;

  qsys::ObjectPtr pobj = ensureNotNull(getScene())->getObjectByName(m_strBndryMol);
  MolCoordPtr pMol = MolCoordPtr(pobj, qlib::no_throw_tag());

  if (pMol.isnull()) {
    m_strBndryMol = LString();
    return;
  }

  AtomIterator aiter(pMol, m_pSelBndry);
  int i, natoms=0;
  for (aiter.first();
       aiter.hasMore();
       aiter.next()) {
    ++natoms;
  }

  m_boundary.alloc(natoms);

  for (aiter.first(), i=0;
       aiter.hasMore() && i<natoms ;
       aiter.next(), ++i) {
    m_boundary.setAt(i, aiter.get()->getPos(), aiter.getID());
  }

  m_boundary.build();
  m_bUseMolBndry = true;
}

qsys::ObjectPtr MapRenderer::getColorMapObj() const
{
  qsys::ObjectPtr pobj = ensureNotNull(getScene())->getObjectByName(getColorMapName());
  return pobj;
}

void MapRenderer::propChanged(qlib::LPropEvent &ev)
{
  if (ev.getParentName().equals("multi_grad") &&
      m_nMode==MAPREND_MULTIGRAD) {
    invalidateDisplayCache();
  }

  super_t::propChanged(ev);
}


