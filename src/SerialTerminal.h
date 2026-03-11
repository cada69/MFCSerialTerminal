#pragma once
#include <afxwin.h>
#include <afxext.h>
#include <afxcmn.h>

class CSerialTerminalApp : public CWinApp
{
public:
    CSerialTerminalApp();
    virtual BOOL InitInstance();
    DECLARE_MESSAGE_MAP()
};

extern CSerialTerminalApp theApp;
