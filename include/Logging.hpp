#ifndef __LOGGING_HPP__
#define __LOGGING_HPP__

#include <sstream>


#ifdef MICROS_INSTALL_SOFTLINK_PREFIX

#include "micros_bt/loggers/bt_hierarchical_logger.h"

#define ISN_INFO_LOG(logContext)  BT_INFO_COUT << logContext
#define ISN_WARN_LOG(logContext)  BT_WARN_COUT << logContext
#define ISN_ERROR_LOG(logContext)  BT_ERROR_COUT << logContext
#define ISN_DEBUG_LOG(logContext)  BT_DEBUG_COUT << logContext
#define INIT_LOGGER()
#define FINI_LOGGER()

#else //MICROS_INSTALL_SOFTLINK_PREFIX
#include <glog/logging.h>

#define INIT_LOGGER()                      \
    {                                      \
        FLAGS_logtostderr = 1;             \
        google::InitGoogleLogging("isn");\
        google::SetLogDestination(google::GLOG_INFO, "./logs/info.log");    \
        google::SetLogDestination(google::GLOG_WARNING, "./logs/warn.log"); \
        google::SetLogDestination(google::GLOG_ERROR, "./logs/error.log");  \
        google::SetStderrLogging(google::GLOG_ERROR);                     \
    }
    //gflags::ParseCommandLineFlags(&argc, &argv, true);    \

#define FINI_LOGGER() \
    google::ShutdownGoogleLogging();

#define ISN_DEBUG_LOG(logContent) \
{                                \
     LOG(INFO) << logContent;    \
}

#define ISN_INFO_LOG(logContent) \
{                                \
     LOG(INFO) << logContent;    \
}

#define ISN_WARN_LOG(logContent) \
    {                            \
        LOG(WARNING) << logContent; \
    }

#define ISN_ERROR_LOG(logContent)\
    {                            \
        LOG(ERROR) << logContent; \
    }

#endif //MICROS_INSTALL_SOFTLINK_PREFIX

#endif //__LOGGING_HPP__
