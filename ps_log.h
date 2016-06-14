#ifndef ps_log_h__
#define ps_log_h__

#pragma once

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SHOW_FILE_LINE 0

#if SHOW_FILE_LINE == 1
#define FILE_LINE "(" __FILE__ ":" LINESTR(__LINE__) ")"
#else
#define FILE_LINE ""
#endif

//------------------------------------------------------------------------------
#define _LINESTR(s) #s
#define LINESTR(s) _LINESTR(s)
#define PU_LOG_DEBUG(cat, ...)    G_PU_LOG_DEBUG_ISOPEN   && PsLogAdd_debug(cat, FILE_LINE,  __VA_ARGS__)
#define PU_LOG_WARNING(cat, ...)  G_PU_LOG_WARNING_ISOPEN && PsLogAdd_warning(cat, FILE_LINE,   __VA_ARGS__)
#define PU_LOG_MESSAGE(cat, ...)  G_PU_LOG_MESSAGE_ISOPEN && PsLogAdd_msg(cat, FILE_LINE,   __VA_ARGS__)
#define PU_LOG_ERROR(cat, ...)   G_PU_LOG_ERROR_ISOPEN   && PsLogAdd_error(cat, FILE_LINE,   __VA_ARGS__)
//------------------------------------------------------------------------------

#define PULOGLV_DEBUG     0
#define PULOGLV_WARNING   1
#define PULOGLV_ERROR     2
#define PULOGLV_MESSAGE   3

#define PULOG_LENGTH 1024
#ifdef _WIN32
#define _COLOR
#endif

#ifdef __cplusplus
extern "C"{
#endif
	typedef void(*ShowlogFunc)(int lv, const char* buffer);
    extern const char LOGTYPESTR[];                                         
    typedef struct _LOG_HANDLE LOG_HANDLE;
    int PsLogAdd_debug(LOG_HANDLE *hHandle, const char *fileln, const char *format, ...);
    int PsLogAdd_warning(LOG_HANDLE *hHandle, const char *fileln, const char *format, ...);
    int PsLogAdd_msg(LOG_HANDLE *hHandle, const char *fileln, const char *format, ...);
    int PsLogAdd_error(LOG_HANDLE *hHandle, const char *fileln, const char *format, ...);
    void PsLogAdd(LOG_HANDLE *hHandle, int lv, char *log_text);
    void PsDefaultShowLogText(int lv, const char* buffer);
	void PsSetShowLogFunc(ShowlogFunc func);
    int PsLogAdd_debug2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... );
    int PsLogAdd_warning2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... );
    int PsLogAdd_msg2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... );
    int PsLogAdd_error2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... );
	void PsSetLogColor(const unsigned short *log_color, unsigned short date_color);
	void PsResetLogColor();
	LOG_HANDLE *PsLogOpen(const char* prefix);
    LOG_HANDLE *SetMainLog(const char* prefix);
    __inline LOG_HANDLE *GetMainLog()
    {
        extern LOG_HANDLE *g_MainpLog;
        return g_MainpLog;
    }
    void PuLogClose(LOG_HANDLE *hHandle);
    extern int G_PU_LOG_DEBUG_ISOPEN;
    extern int G_PU_LOG_MESSAGE_ISOPEN;
    extern int G_PU_LOG_WARNING_ISOPEN;
    extern int G_PU_LOG_ERROR_ISOPEN;
#ifdef __cplusplus
}
#endif

#define FORMAT_LOG(lv, output, fileln, format)                                  \
{                                                                               \
time_t log_time = 0;                                                            \
size_t nlen = 0;                                                                \
struct tm *newtime;                                                             \
va_list ap;                                                                     \
time(&log_time);                                                                \
newtime = localtime(&log_time);                                                 \
memset(output, 0, sizeof(output));                                              \
snprintf(output, sizeof(output)-1, "%d/%d/%d %d:%d:%d %c|%s",                   \
           newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday,      \
           newtime->tm_hour, newtime->tm_min, newtime->tm_sec, LOGTYPESTR[lv], fileln); \
nlen = strlen(output);                                                          \
va_start(ap, format);                                                           \
vsnprintf(output + nlen, sizeof(output) - nlen - 1, format, ap);                \
va_end(ap);                                                                     \
}

#define _FL " %s %d"
#define LOG_FL __FILE__, __LINE__


#endif // ps_log_h__
