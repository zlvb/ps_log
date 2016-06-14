#include "ps_log.h"
#include "pu_platform.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

int G_PU_LOG_DEBUG_ISOPEN = 1;
int G_PU_LOG_MESSAGE_ISOPEN = 1;
int G_PU_LOG_WARNING_ISOPEN = 1;
int G_PU_LOG_ERROR_ISOPEN = 1;
LOG_HANDLE *g_MainpLog; // 日志
ShowlogFunc gs_default_sh_func = PsDefaultShowLogText;
#ifdef _MSC_VER
#pragma warning(disable:4127 4100)
#endif
typedef struct _LOG_HANDLE
{
    FILE *g_cur_file;
    char g_fname_prefix[MAX_PATH + 1];
    size_t g_bytes;
    PU_MUTEX g_log_mutex;
}LOG_HANDLE;
static const size_t MAX_LOG_FILE_SIZE = 1024*1024*1024;
const char LOGTYPESTR[] = {
    'D',
    'W',
    'E',
    'M'
}; 
#ifdef _MSC_VER
const unsigned short LOGCOLOR[] = {
    FOREGROUND_GREEN,
    FOREGROUND_GREEN | FOREGROUND_RED,    
    FOREGROUND_RED,
    FOREGROUND_RED | FOREGROUND_BLUE,
};

#define LOGCOLOR_DATE (FOREGROUND_GREEN | FOREGROUND_BLUE)

static const unsigned short *cur_log_color = LOGCOLOR;
static unsigned short cur_date_color = LOGCOLOR_DATE;

void PsSetLogColor(const unsigned short *log_color, unsigned short date_color)
{
	cur_log_color = log_color;
	cur_date_color = date_color;
}

void PsResetLogColor()
{
	cur_log_color = LOGCOLOR;
	cur_date_color = LOGCOLOR_DATE;
}

#endif

static void scpy(char *dest, const char *src, int num)
{
    strncpy(dest, src, num);
    dest[num - 1] = 0;
}

// -------------------------------------------------------------------------------------------------
static void switch_file(LOG_HANDLE *hHandle, int isretry)
{
    char filename[512] = {0};
    time_t log_time = 0;
    struct tm *newtime = NULL;
    if (!hHandle)
        return;

    if (hHandle->g_cur_file)
    {
        fclose(hHandle->g_cur_file);
        hHandle->g_cur_file = NULL;
    }
    time(&log_time); 
    newtime = localtime(&log_time);
    if (!isretry)
    {
        snprintf(filename, sizeof(filename), ".\\log\\%s_%d-%d-%d.log",
        hHandle->g_fname_prefix, newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday);
    }
    else
    {
        snprintf(filename, sizeof(filename), ".\\log\\%s_%d-%d-%d_%d-%d-%d.log",
            hHandle->g_fname_prefix, newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday,
            newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
    }
    MkDir("log");
    hHandle->g_cur_file = fopen(filename, "ab");
    if (!hHandle->g_cur_file)
    {
#ifdef _MSC_VER
        Sleep(1000);
#else
        usleep(1000 * 1000);
#endif
        switch_file(hHandle, 1);
    }
    hHandle->g_bytes = 0;
}
// -------------------------------------------------------------------------------------------------
LOG_HANDLE* PsLogOpen(const char* prefix)
{
    LOG_HANDLE *hHandle = (LOG_HANDLE*)malloc(sizeof(LOG_HANDLE));
    InitMutex(&hHandle->g_log_mutex);
    hHandle->g_bytes = 0;
    hHandle->g_cur_file = NULL;

    assert(prefix);
	if (prefix == NULL)
    {
        return NULL;
    }
	else
    {
        const char bad_char[] = "/\\:*?\"<>|"; // 不能用于文件名的字符
        int prefix_len = (int)strlen(prefix);
        int i = 0; 
        memset(hHandle->g_fname_prefix, 0, sizeof(hHandle->g_fname_prefix));
        for (; i < prefix_len && i < (int)sizeof(hHandle->g_fname_prefix) - 1; i++)
        {
            const char c = prefix[i];
            if (strchr(bad_char, c))
                hHandle->g_fname_prefix[i] = '#';
            else
                hHandle->g_fname_prefix[i] = c;
        }
    }

    switch_file(hHandle, 0);
    PsLogAdd_msg(hHandle, "%s.log start", prefix);
    return hHandle;
}
// -------------------------------------------------------------------------------------------------
void PuLogClose(LOG_HANDLE *hHandle)
{
    LockMutex(&hHandle->g_log_mutex);
    if (hHandle->g_cur_file)
    {
        fclose(hHandle->g_cur_file);
        hHandle->g_cur_file = NULL;
    }
    UnlockMute(&hHandle->g_log_mutex);
    DeleteMutex(&hHandle->g_log_mutex);
}
// -------------------------------------------------------------------------------------------------
void MWLogSetFilePrefix(LOG_HANDLE *hHandle, const char *file_prefix)
{
    scpy(hHandle->g_fname_prefix, file_prefix, MAX_PATH);
    switch_file(hHandle, 0);
}

// -------------------------------------------------------------------------------------------------
void PsDefaultShowLogText(int lv, const char* buffer)
{
#if defined(_COLOR) && defined(_WIN32)
	char *Part2 = strstr(buffer, "|") + 1;
	char Part1[PULOG_LENGTH] = {0};
	strncpy(Part1, buffer, Part2 - buffer);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), cur_date_color);
	printf("%s", Part1);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), cur_log_color[lv]);
	printf("%s", Part2);
#else
	printf("%s", buffer);
#endif

#if defined(_COLOR) && defined(_WIN32)
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
#endif
}
void PsSetShowLogFunc(ShowlogFunc func)
{
	gs_default_sh_func = func;
}
// -------------------------------------------------------------------------------------------------
void PsLogAdd(LOG_HANDLE *hHandle, int lv, char *buffer)
{
    if (!hHandle)
        hHandle = GetMainLog();

#if defined(_DEBUG) && defined(_WIN32)
    OutputDebugStringA("[");
    OutputDebugStringA(hHandle->g_fname_prefix);
    OutputDebugStringA("] ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\r\n");
#endif

    LockMutex(&hHandle->g_log_mutex);
    if (hHandle->g_cur_file)
    {
        size_t len = strlen(buffer);
        if (len >= PULOG_LENGTH-1){
            buffer[len-1] = '\n';
            fwrite(buffer, len-1, 1, hHandle->g_cur_file);
        }else{
            buffer[len] = '\n';
            buffer[len+1] = '\0';
            fwrite(buffer, len+1, 1, hHandle->g_cur_file);
        }        
        fflush(hHandle->g_cur_file);
        
        hHandle->g_bytes += len;
        if (hHandle->g_bytes >= MAX_LOG_FILE_SIZE)
            switch_file(hHandle, 0);
        
		gs_default_sh_func(lv, buffer);
    }
    UnlockMute(&hHandle->g_log_mutex);
    return;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_debug2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... )
{
    FORMAT_LOG(PULOGLV_DEBUG, buffer, fileln, format);
    PsLogAdd(hHandle, PULOGLV_DEBUG, buffer);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_warning2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... )
{
    FORMAT_LOG(PULOGLV_WARNING, buffer, fileln, format);
    PsLogAdd(hHandle, PULOGLV_WARNING, buffer);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_msg2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... )
{
    FORMAT_LOG(PULOGLV_MESSAGE, buffer, fileln, format);
    PsLogAdd(hHandle, PULOGLV_MESSAGE, buffer);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_error2(LOG_HANDLE *hHandle, const char *fileln, char *buffer, const char *format, ... )
{
    FORMAT_LOG(PULOGLV_ERROR, buffer, fileln, format);
    PsLogAdd(hHandle, PULOGLV_ERROR, buffer);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_debug(LOG_HANDLE *hHandle, const char *fileln, const char *format, ... )
{
    char output[PULOG_LENGTH];
    FORMAT_LOG(PULOGLV_DEBUG, output, fileln, format);
    PsLogAdd(hHandle, PULOGLV_DEBUG, output);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_warning(LOG_HANDLE *hHandle, const char *fileln, const char *format, ... )
{
    char output[PULOG_LENGTH];
    FORMAT_LOG(PULOGLV_WARNING, output, fileln, format);
    PsLogAdd(hHandle, PULOGLV_WARNING, output);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_msg(LOG_HANDLE *hHandle, const char *fileln, const char *format, ... )
{
    char output[PULOG_LENGTH];
    FORMAT_LOG(PULOGLV_MESSAGE, output, fileln, format);
    PsLogAdd(hHandle, PULOGLV_MESSAGE, output);
    return 0;
}
// -------------------------------------------------------------------------------------------------
int PsLogAdd_error(LOG_HANDLE *hHandle, const char *fileln, const char *format, ... )
{
    char output[PULOG_LENGTH];
    FORMAT_LOG(PULOGLV_ERROR, output, fileln, format);
    PsLogAdd(hHandle, PULOGLV_ERROR, output);
    return 0;
}
// -------------------------------------------------------------------------------------------------
LOG_HANDLE * SetMainLog( const char* prefix )
{
    if (!g_MainpLog)
        g_MainpLog = PsLogOpen(prefix);
    return g_MainpLog;
}
