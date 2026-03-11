#include "stdafx.h"
#include "SerialTerminal.h"
#include "MainFrame.h"

BEGIN_MESSAGE_MAP(CSerialTerminalApp, CWinApp)
END_MESSAGE_MAP()

CSerialTerminalApp theApp;

CSerialTerminalApp::CSerialTerminalApp() {}

BOOL CSerialTerminalApp::InitInstance()
{
    CWinApp::InitInstance();
    SetRegistryKey(_T("MFCSerialTerminal"));

    CMainFrame* pFrame = new CMainFrame();
    m_pMainWnd = pFrame;

    pFrame->Create(NULL, _T("Serial Terminal"),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CRect(100, 100, 1000, 700));

    pFrame->ShowWindow(SW_SHOW);
    pFrame->UpdateWindow();
    return TRUE;
}
