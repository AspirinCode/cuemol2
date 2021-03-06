// -*-Mode: C++;-*-
//
// Xplor/CHARMM/NAMD PSF file reader
//

#ifndef PSF_READER_HPP_
#define PSF_READER_HPP_

#include <qlib/LChar.hpp>
#include <qlib/Array.hpp>
#include <qlib/LineStream.hpp>
#include <modules/molstr/molstr.hpp>

namespace mdtools {

  using qlib::LString;
  using molstr::MolCoordPtr;

  class PsfReader
  {
  private:
    MolCoordPtr m_pMol;

    ///////////////////////////////////////////
  public:
    /// default constructor
    PsfReader();

    /// destructor
    virtual ~PsfReader();

    ///////////////////////////////////////////

    /// Attach MolCoord obj
    virtual void attach(MolCoordPtr pMol);

    /// read from stream
    virtual void read(qlib::InStream &ins);

    ///////////////////////////////////////////

  private:
    int m_natom;
    int m_ntypes;
    int m_nbonh;
    int m_mbona;
    int m_ntheth;
    int m_mtheta;
    int m_nphih;
    int m_mphia;
    int m_nhparm;
    int m_nparm;
    int m_nnb;
    int m_nres;
    int m_nbona;
    int m_ntheta;
    int m_nphia;
    int m_numbnd;
    int m_numang;
    int m_nptra;
    int m_natyp;
    int m_nphb;
    int m_ifpert;
    int m_nbper;
    int m_ngper;
    int m_ndper;
    int m_mbper;
    int m_mgper;
    int m_mdper;
    int m_ifbox;
    int m_nmxrs;
    int m_ifcap;

    qlib::LineStream *m_pls;

  private:
    LString m_line;

    void readLine();

    void removeComment();

    int getInt(int pos, int unit)
    {
      int ret;
      int beg = pos*unit;

      if (beg>=m_line.length() ||
          beg<0) {
        MB_THROW(qlib::FileFormatException, "");
        return -1;
      }

      LString s = m_line.substr(beg, unit);
      if (!s.toInt(&ret)) {
        MB_THROW(qlib::FileFormatException, "");
        return -1;
      }
      return ret;
    }

    float getReal(int pos, int unit)
    {
      float ret;
      int beg = pos*unit;

      if (beg>=m_line.length() ||
          beg<0) {
        MB_THROW(qlib::FileFormatException, "");
        return -1;
      }

      LString s = m_line.substr(beg, unit);
      if (!s.toRealNum<float>(&ret)) {
        MB_THROW(qlib::FileFormatException, "");
        return -1;
      }
      return ret;
    }

    void getRec(int pos, int u, int &ret) {
      ret = getInt(pos, u);
    }
    void getRec(int pos, int u, float &ret) {
      ret = getReal(pos, u);
    }

    void getRec(int pos, int unit, LString &ret)
    {
      int beg = pos*unit;

      if (beg>=m_line.length() ||
          beg<0) {
        MB_THROW(qlib::FileFormatException, "");
        return;
      }

      ret = m_line.substr(beg, unit);
    }

    template <typename _T>
    void getArray(int nrec, int unit, _T *array, int nsize)
    {
      int irec = 0;
      for (; irec<nsize; ++irec) {
        if (irec%nrec==0)
          m_line = m_pls->readLine();
        getRec(irec%nrec, unit, array[irec]);
      }
    }

    void getStrArray(int nrec, int unit, LString *array, int nsize);
    void getIntArray(int nrec, int unit, int *array, int nsize);
  };

}

#endif
