#ifndef SERVICERUNNER_VDEBUG_H_H_
#define SERVICERUNNER_VDEBUG_H_H_
#include <Windows.h>

class ServiceRunner {
private:
    ServiceRunner();
public:
    static ServiceRunner *GetInstance();
    virtual ~ServiceRunner();
    bool InitServiceRunner();

private:
    void OnServWork();
    void RunProcess(LPCWSTR wszKey);
    static DWORD WINAPI ServiveMain(LPVOID param);
    static DWORD WINAPI ServiceHandlerEx(DWORD dwControl, DWORD dwEvent, LPVOID pEventData, LPVOID pContext);
    static VOID WINAPI ServiceMainProc(DWORD dwArgc, LPWSTR *wszArgv);
private:
    //service����
    SERVICE_STATUS_HANDLE m_ServStatus;
    HANDLE m_ServiceThread;
    HANDLE m_ServiceNotify;
    HANDLE m_ServiceLeave;
};
#endif //SERVICERUNNER_VDEBUG_H_H_