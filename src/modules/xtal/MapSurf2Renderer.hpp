// -*-Mode: C++;-*-
//
// Generate/Render the contour surface of ScalarObject (ver.2)
//

#ifndef XTAL_MAP_SURF2_RENDERER_HPP_INCLUDED
#define XTAL_MAP_SURF2_RENDERER_HPP_INCLUDED

#include "xtal.hpp"
#include "MapRenderer.hpp"

#include <qlib/ByteMap.hpp>
#include <qsys/ScalarObject.hpp>
#include <qsys/ViewEvent.hpp>
#include <modules/molstr/molstr.hpp>
#include <modules/molstr/BSPTree.hpp>

#include <modules/surface/MolSurfObj.hpp>
#include <gfx/DrawElem.hpp>

class MapSurf2Renderer_wrap;

namespace sysdep {
  class OglProgramObject;
}

namespace xtal {

  using gfx::DisplayContext;
  using qsys::ScalarObject;
  using molstr::SelectionPtr;
  using molstr::MolCoordPtr;
  using molstr::BSPTree;
  class DensityMap;

  /// Map surface renderer class (ver.2)
  /// This class uses ver2 interface to perform file,
  /// display-list and VBO (with Omp) rendering
  class MapSurf2Renderer : public MapRenderer,
                          public qsys::ViewEventListener
  {
    MC_SCRIPTABLE;
    MC_CLONEABLE;

  private:
    typedef MapRenderer super_t;
    friend class ::MapSurf2Renderer_wrap;

    ///////////////////////////////////////////
    // properties

  public:
    enum {
      MSRDRAW_FILL = 0,
      MSRDRAW_LINE = 1,
      MSRDRAW_POINT = 2,
    };
    
  private:
    /// Mesh-drawing mode
    int m_nDrawMode;

    /// Line width (used in LINE/POINT mode)
    double m_lw;

  public:
    int getDrawMode() const { return m_nDrawMode; }
    void setDrawMode(int n) {
      m_nDrawMode = n;
      invalidateDisplayCache();
    }
    
    void setLineWidth(double f) {
      m_lw = f;
      invalidateDisplayCache();
    }
    double getLineWidth() const { return m_lw; }
    

  private:
    /// cull face
    bool m_bCullFace;

  public:
    bool isCullFace() const { return m_bCullFace; }
    void setCullFace(bool b) {
      m_bCullFace = b;
      invalidateDisplayCache();
    }
    
  //private:
  protected:
    /// binning
    int m_nBinFac;

  public:
    int getBinFac() const { return m_nBinFac; }
    void setBinFac(int n) {
      m_nBinFac = n;
      invalidateDisplayCache();
    }
    
/*
  private:
    /// Max grid size (default=100x100x100 grid)
    int m_nMaxGrid;

  public:
    int getMaxGrids() const { return m_nMaxGrid; }
    void setMaxGrids(int n);

    /// Get max extent (in angstrom unit; calculated from m_nMaxGrid)
    double getMaxExtent() const;
*/
    
  private:
    /// OpenMP Thread number(-1: use all system cores)
    int m_nOmpThr;

  public:
    int getOmpThr() const { return m_nOmpThr; }
    void setOmpThr(int val) {
      m_nOmpThr = val;
      invalidateDisplayCache();
    }

  protected:

    ///////////////////////////////////////////
    // work area

    /// Periodic boundary flag. This value is determined by the map size and usePBC flag
    // bool m_bPBC;

    /// size of map (copy from m_pMap)
    // int m_nMapColNo, m_nMapRowNo, m_nMapSecNo;

    /// size of section array
    // int m_nActCol, m_nActRow, m_nActSec;
    // int m_nStCol, m_nStRow, m_nStSec;

    /// contour level (not a property)
    double m_dLevel;

    /// for debug
    std::deque<Vector4D> m_tmpv;
    
    // cached ptr of target obj
    ScalarObject *m_pCMap;

  public:

    ///////////////////////////////////////////
    // constructors / destructor

    /// default constructor
    MapSurf2Renderer();

    /// destructor
    virtual ~MapSurf2Renderer();

    ///////////////////////////////////////////

    virtual const char *getTypeName() const;

    //virtual void attachObj(qlib::uid_t obj_uid);
    virtual void setSceneID(qlib::uid_t nid);

    virtual qlib::uid_t detachObj();

    /// For updating the center by the mouse events
    virtual void viewChanged(qsys::ViewEvent &);

    ///////////////////////////////////////////
    // Old/common rendering interface
    //   (for file/display-list rendering)

    virtual void render(DisplayContext *pdl);
    virtual void preRender(DisplayContext *pdc);
    virtual void postRender(DisplayContext *pdc);

    // virtual bool isTransp() const { return true; }

  private:

    // void setupXformMat(DisplayContext *pdl);

    // void makerange();

    void renderImpl(DisplayContext *pdl);

    void marchCube(DisplayContext *pdl, int fx, int fy, int fz);

    //double getOffset(double fValue1, double fValue2, double fValueDesired);
    void getVertexColor(Vector4D &rfColor, Vector4D &rfPosition, Vector4D &rfNormal);
    Vector4D getNormal(const Vector4D &rfNormal,bool,bool,bool);

    inline float getDen(int x, int y, int z) const
    {
      // TO DO: support symop

      if (m_bPBC) {
        const int xx = (x+10000*m_mapSize.x())%m_mapSize.x();
        const int yy = (y+10000*m_mapSize.y())%m_mapSize.y();
        const int zz = (z+10000*m_mapSize.z())%m_mapSize.z();
        // return pMap->atByte(xx,yy,zz);
        return m_pCMap->atFloat(xx, yy, zz);
      }
      else {
        if (x<0||y<0||z<0)
          return 0.0;
        if (x>=m_mapSize.x()||
            y>=m_mapSize.y()||
            z>=m_mapSize.z())
          return 0.0;
        return m_pCMap->atFloat(x, y, z);
      }
      
    }

    inline Vector4D getGrdNorm2(int ix, int iy, int iz) {
      const int n = 1;
      return Vector4D(getDen(ix-n, iy,   iz  ) - getDen(ix+n, iy,   iz ),
                      getDen(ix,   iy-n, iz  ) - getDen(ix,   iy+n, iz  ),
                      getDen(ix,   iy,   iz-n) - getDen(ix,   iy,   iz+n));
    }


    float m_values[8];
    bool m_bary[8];
    Vector4D m_norms[8];

    ////////////////////////////////////////////////
    // New rendering interface (for VBO/GLSL)

  public:

    /// Use Ver2 interface (returns true)
    virtual bool isUseVer2Iface() const;

    virtual void invalidateDisplayCache();
    
    virtual void createDisplayCache();

    virtual bool isCacheAvail() const;

    virtual void renderVBO(DisplayContext *pdc);

    // virtual void renderGLSL(DisplayContext *pdc);

  protected:

    /// Workarea data OK/NG (invalid)
    bool m_bWorkOK;

    // qbyte m_bIsoLev;

    int m_nbcol;
    int m_nbrow;
    int m_nbsec;

    qbyte m_col_r, m_col_g, m_col_b, m_col_a;

    gfx::DrawElemVNC *m_pVBO;

    void createVBO1(DisplayContext *pdl);
    void displayVBO1(DisplayContext *pdl);
    
    void marchCube2(int fx, int fy, int fz,
		    const qbyte *values,
		    const bool *bary,
		    int *pvind);
      
    inline qbyte getByteDen(int x, int y, int z) const
    {
      // TO DO: support symop

      if (m_bPBC) {
        const int xx = (x+10000*m_mapSize.x())%m_mapSize.x();
        const int yy = (y+10000*m_mapSize.y())%m_mapSize.y();
        const int zz = (z+10000*m_mapSize.z())%m_mapSize.z();
        return m_pCMap->atByte(xx,yy,zz);
      }
      else {
        if (x<0||y<0||z<0)
          return 0;
        if (x>=m_mapSize.x()||
            y>=m_mapSize.y()||
            z>=m_mapSize.z())
          return 0;
        return m_pCMap->atByte(x, y, z);
      }
    }

    inline void getGrdNormByte(int ix, int iy, int iz, float *norm)
    {
      const int del = 1;
      norm[0] = float(getByteDen(ix-del, iy,   iz  )) - float(getByteDen(ix+del, iy,   iz  ));
      norm[1] = float(getByteDen(ix,   iy-del, iz  )) - float(getByteDen(ix,   iy+del, iz  ));
      norm[2] = float(getByteDen(ix,   iy,   iz-del)) - float(getByteDen(ix,   iy,   iz+del));
      norm[3] = 0.0f;
    }
    
    ///////////////////////////////////////////////
    // Surface object generation

  private:
    std::deque<surface::MSVert> m_msverts;
    Matrix4D m_xform;

    int addMSVert(const Vector4D &v, const Vector4D &n)
    {
      Vector4D vv(v);
      vv.w() = 1.0;
      m_xform.xform4D(vv);

      Vector4D nn(n);
      nn.w() = 0.0;
      m_xform.xform4D(nn);

      int nid = m_msverts.size();
      m_msverts.push_back( surface::MSVert(vv, nn) );

      return nid;
    }

  public:    
    qsys::ObjectPtr generateSurfObj();

  };

}

#endif