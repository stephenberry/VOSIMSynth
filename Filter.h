#ifndef __FILTER__
#define __FILTER__
#include "Unit.h"
#include <cstring>
#include <cstdlib>
#define AA_FILTER_SIZE 6
const double AA_FILTER_X[1 + AA_FILTER_SIZE] = { 4.760635e-1, 2.856281, 7.140952, 9.521270, 7.140952, 2.856281, 4.760635e-1 };
const double AA_FILTER_Y[AA_FILTER_SIZE] = { -4.522403, -8.676844, -9.007512, -5.328429, -1.702543, -2.303303e-1 };

using namespace std;
namespace syn
{
  template <size_t nX, size_t nY>
  class Filter : public Unit
  {
  protected:
    const double (&XCoefs)[nX];
    const double (&YCoefs)[nY];
    double *YBuf, *XBuf;
    int xBufInd, yBufInd;
    UnitParameter& m_input;
  public:
    Filter(string name, const double(&X)[nX], const double(&Y)[nY]) :
      Unit(name),
      XCoefs(X),
      YCoefs(Y),
      m_input(addParam("input",DOUBLE_TYPE, -1, 1, true))
    {
      xBufInd = 0;
      yBufInd = 0;
      // XCoefs = (double*)malloc(sizeof(double)*numXCoefs);
      // YCoefs = (double*)malloc(sizeof(double)*numYCoefs);
      // memcpy(XCoefs, X, numXCoefs*sizeof(double));
      // memcpy(YCoefs, Y, numYCoefs*sizeof(double));
      XBuf = (double*)malloc(sizeof(double)*nX);
      YBuf = (double*)malloc(sizeof(double)*(nY + 1));
      int i;
      for (i = 0; i < nX; i++)
      {
        XBuf[i] = 0.0;
      }
      for (i = 0; i < nY + 1; i++)
      {
        YBuf[i] = 0.0;
      }
    };
    Filter(const Filter& filt);
    ~Filter()
    {
      delete[] YBuf;
      delete[] XBuf;
    };
    void reset(){ memset(YBuf,0,(nY+1)*sizeof(double)); memset(XBuf,0,(nX)*sizeof(double)); xBufInd=0; yBufInd=0; };
  protected:
    virtual void process();
  private:
    virtual Unit* cloneImpl() const { return new Filter(*this); };
  };

  template <size_t nX, size_t nY>
  Filter<nX, nY>::Filter(const Filter<nX, nY>& filt) :
    Filter<nX,nY>(filt.m_name, filt.XCoefs, filt.YCoefs)
  {

  }

  template <size_t nX, size_t nY>
  void Filter<nX, nY>::process()
  {
    XBuf[xBufInd] = readParam(0);
    YBuf[yBufInd] = 0.0;
    double *output = &YBuf[yBufInd];
    int i, j;
    for (i = 0, j = xBufInd; i < nX; i++, j--)
    {
      if (j < 0)
        j = nX - 1;
      YBuf[yBufInd] += XBuf[j] * XCoefs[i];
    }
    for (i = 0, j = yBufInd - 1; i < nY; i++, j--)
    {
      if (j < 0)
        j = nY;
      YBuf[yBufInd] += YBuf[j] * YCoefs[i];
    }
    xBufInd++;
    if (xBufInd == nX)
      xBufInd = 0;
    yBufInd++;
    if (yBufInd == nY + 1)
      yBufInd = 0;
    m_output = *output;
  }
}
#endif