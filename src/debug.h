#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG(...)
#endif

#ifdef CDDEBUG
#define CDLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define CDLOG(...)
#endif

#ifdef NETLINK_DEBUG
#define NETLINK_LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define NETLINK_LOG(...)
#endif

#ifdef SCSP_DEBUG
#define SCSPLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define SCSPLOG(...)
#endif

#ifdef SCSPDSP_DEBUG
#define SCSPDSPLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define SCSPDSPLOG(...)
#endif

#ifdef VDP1_DEBUG
#define VDP1LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define VDP1LOG(...)
#endif

#ifdef VDP2_DEBUG
#define VDP2LOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define VDP2LOG(...)
#endif

#ifdef SMPC_DEBUG
#define SMPCLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define SMPCLOG(...)
#endif

#ifdef YGL_DEBUG
#define YGLLOG(...) DebugPrintf(MainLog, __FILE__, __LINE__, __VA_ARGS__)
#else
#define YGLLOG(...)
#endif

#endif
