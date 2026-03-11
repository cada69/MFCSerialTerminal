#pragma once
#include "stdafx.h"
#include "SerialPort.h"
#include "TerminalView.h"

// Resource IDs
#define IDR_MAINMENU        100
#define IDR_TOOLBAR         101
#define IDB_TOOLBAR         102

// Control IDs for send bar (manual layout, no RC file)
#define IDC_EDIT_SEND       1000
#define IDC_BTN_SEND        1001
#define IDC_CHK_CR          1002
#define IDC_CHK_LF          1003
#define IDC_CHK_HEX         1004

// Menu IDs
#define ID_FILE_SAVE_LOG    40001
#define ID_FILE_CLEAR_LOG   40002
#define ID_CONN_CONNECT     40010
#define ID_CONN_DISCONNECT  40011
#define ID_CONN_SETTINGS    40012
#define ID_VIEW_ASCII       40020
#define ID_VIEW_HEX         40021
#define ID_VIEW_MIXED       40022
#define ID_VIEW_TIMESTAMP   40023
#define ID_VIEW_AUTOSCROLL  40024
#define ID_TOOLS_SEND_FILE  40030
#define ID_TOOLS_DTR        40031
#define ID_TOOLS_RTS        40032

// Timer
#define TIMER_STATUS        1

// Custom message for data received from read thread
#define WM_SERIAL_DATA      (WM_USER + 100)
#define WM_SERIAL_ERROR     (WM_USER + 101)

class CMainFrame : public CFrameWnd
{
public:
    CMainFrame();
    virtual ~CMainFrame();
    BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
                DWORD dwStyle, const RECT& rect,
                CWnd* pParentWnd = nullptr,
                LPCTSTR lpszMenuName = nullptr,
                DWORD dwExStyle = 0,
                CCreateContext* pContext = nullptr);

protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    virtual int  OnCreate(LPCREATESTRUCT lpcs);

    DECLARE_MESSAGE_MAP()

    // Connection
    afx_msg void OnConnect();
    afx_msg void OnDisconnect();
    afx_msg void OnConnSettings();
    afx_msg void OnUpdateConnect(CCmdUI* pUI);
    afx_msg void OnUpdateDisconnect(CCmdUI* pUI);

    // View
    afx_msg void OnViewAscii();
    afx_msg void OnViewHex();
    afx_msg void OnViewMixed();
    afx_msg void OnViewTimestamp();
    afx_msg void OnUpdateViewMode(CCmdUI* pUI);
    afx_msg void OnUpdateTimestamp(CCmdUI* pUI);

    // File
    afx_msg void OnFileSaveLog();
    afx_msg void OnFileClearLog();

    // Tools
    afx_msg void OnSendFile();
    afx_msg void OnToggleDTR();
    afx_msg void OnToggleRTS();
    afx_msg void OnUpdateDTR(CCmdUI* pUI);
    afx_msg void OnUpdateRTS(CCmdUI* pUI);

    // Send bar
    afx_msg void OnSend();
    afx_msg void OnSendEnter();

    // System
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnDestroy();

    // Serial data (posted from read thread)
    afx_msg LRESULT OnSerialData(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSerialError(WPARAM wParam, LPARAM lParam);

private:
    void BuildMenu();
    void BuildToolbar();
    void BuildSendBar();
    void LayoutControls(int cx, int cy);
    void UpdateStatusBar();
    void UpdateTitle();
    void SaveSettings();
    void LoadSettings();

    // Controls
    CStatusBar   m_statusBar;
    CToolBar     m_toolbar;
    CTerminalView m_terminal;

    // Send bar controls (manual layout — no dialog)
    CEdit        m_editSend;
    CButton      m_btnSend;
    CComboBox    m_cbSendHistory;
    CButton      m_btnSendHex;   // toggle: send as hex bytes
    CButton      m_btnCR;        // append CR
    CButton      m_btnLF;        // append LF

    // Serial
    CSerialPort  m_serial;
    SerialConfig m_cfg;

    bool m_dtr      = false;
    bool m_rts      = false;
    bool m_sendAsHex= false;
    bool m_appendCR = false;
    bool m_appendLF = true;

    // Pending data queue (accessed from UI thread only after posting)
    std::mutex               m_dataMutex;
    std::vector<std::vector<BYTE>> m_pendingRx;
    std::vector<std::wstring>      m_pendingErrors;

    static constexpr int SEND_BAR_H = 36;
    static constexpr int TOOLBAR_H  = 28;
};
