//
// XPCOM timer implementation
//

#include "xpcom.hpp"
#include "XPCTimerImpl.hpp"

#include <qlib/LProcMgr.hpp>
#include <qsys/SceneManager.hpp>

#ifdef WIN32
#  include <windows.h>
#endif

#ifdef XP_MACOSX
#include <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#endif

//#define USE_PERFTIMER 1
#include <qlib/LPerfMeas.hpp>

#ifdef USE_XMLRPC
#  include <xmlrpc_bridge/XmlRpcMgr.hpp>
#endif

using namespace xpcom;

XPCTimerImpl::XPCTimerImpl()
{
  m_timer = do_CreateInstance("@mozilla.org/timer;1");
  start(1);
}

XPCTimerImpl::~XPCTimerImpl()
{
  stop();
}

//static
void XPCTimerImpl::timerCallbackFunc(nsITimer *aTimer, void *aClosure)
{
  //MB_DPRINTLN("Timer: notified");
  try {
    qlib::AutoPerfMeas apm(PM_IDLE_TIMER);

    qlib::LProcMgr *pPM = qlib::LProcMgr::getInstance();
    pPM->checkQueue();

    qlib::EventManager *pEM = qlib::EventManager::getInstance();
    pEM->messageLoop();
    pEM->checkTimerQueue();

#ifdef USE_XMLRPC
    xrbr::XmlRpcMgr *pXRM = xrbr::XmlRpcMgr::getInstance();
    pXRM->processReq(1000);
#endif

    qsys::SceneManager *pSM = qsys::SceneManager::getInstance();
    pSM->checkAndUpdateScenes();

  }
  catch (qlib::LException &e) {
    LOG_DPRINTLN("Exception occured in timerCallback: %s",
		 e.getFmtMsg().c_str());
    throw;
  }
  catch (std::exception &e) {
    LOG_DPRINTLN("Exception occured in timerCallback: %s",
		 e.what());
    throw;
  }
  catch (...) {
    LOG_DPRINTLN("Unknown exception occured in timerCallback");
    throw;
  }

  return;
}

qlib::time_value XPCTimerImpl::getCurrentTime()
{
  qlib::time_value tval;
#ifdef WIN32
  tval = (qlib::time_value) ::GetTickCount();
#endif
#ifdef XP_MACOSX
  uint64_t abstime = mach_absolute_time();
  Nanoseconds nanos = AbsoluteToNanoseconds( *(AbsoluteTime *) &abstime );
  tval = UnsignedWideToUInt64(nanos)/1000000;
#endif

  return tval;
}

void XPCTimerImpl::start(qlib::time_value nperiod)
{
  if (!m_timer)
    return;

  nsresult rv = m_timer->InitWithFuncCallback(timerCallbackFunc, this, nperiod, nsITimer::TYPE_REPEATING_SLACK);
  if (NS_FAILED(rv)) {
    m_timer = nullptr;
    MB_DPRINTLN(">>>>> Setup idle timer ERROR!!");
  }
  else {
    MB_DPRINTLN("Timer> Setup idle timer OK");
  }
}

void XPCTimerImpl::stop()
{
  if (m_timer) {
    m_timer->Cancel();
    m_timer = nullptr;
  }
}

