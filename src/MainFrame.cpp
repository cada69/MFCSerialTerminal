#include "stdafx.h"
#include "MainFrame.h"
#include "ConnectDialog.h"
#include <shlobj.h>

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    ON_WM_DESTROY()
    ON_WM_TIMER()

    ON_COMMAND(ID_CONN_CONNECT,    &CMainFrame::OnConnect)
    ON_COMMAND(ID_CONN_DISCONNECT, &CMainFrame::OnDisconnect)
    ON_COMMAND(ID_CONN_SETTINGS,   &CMainFrame::OnConnSettings)
    ON_UPDATE_COMMAND_UI(ID_CONN_CONNECT,    &CMainFrame::OnUpdateConnect)
    ON_UPDATE_COMMAND_UI(ID_CONN_DISCONNECT, &CMainFrame::OnUpdateDisconnect)

    ON_COMMAND(ID_VIEW_ASCII,     &CMainFrame::OnViewAscii)
    ON_COMMAND(ID_VIEW_HEX,       &CMainFrame::OnViewHex)
    ON_COMMAND(ID_VIEW_MIXED,     &CMainFrame::OnViewMixed)
    ON_COMMAND(ID_VIEW_TIMESTAMP, &CMainFrame::OnViewTimestamp)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ASCII,     &CMainFrame::OnUpdateViewMode)
    ON_UPDATE_COMMAND_UI(ID_VIEW_HEX,       &CMainFrame::OnUpdateViewMode)
    ON_UPDATE_COMMAND_UI(ID_VIEW_MIXED,     &CMainFrame::OnUpdateViewMode)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TIMESTAMP, &CMainFrame::OnUpdateTimestamp)

    ON_COMMAND(ID_FILE_SAVE_LOG,  &CMainFrame::OnFileSaveLog)
    ON_COMMAND(ID_FILE_CLEAR_LOG, &CMainFrame::OnFileClearLog)
    ON_COMMAND(ID_EDIT_CLEAR,     &CMainFrame::OnFileClearLog)

    ON_COMMAND(ID_TOOLS_SEND_FILE, &CMainFrame::OnSendFile)
    ON_COMMAND(ID_TOOLS_DTR,       &CMainFrame::OnToggleDTR)
    ON_COMMAND(ID_TOOLS_RTS,       &CMainFrame::OnToggleRTS)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_DTR, &CMainFrame::OnUpdateDTR)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_RTS, &CMainFrame::OnUpdateRTS)

    ON_MESSAGE(WM_SERIAL_DATA,  &CMainFrame::OnSerialData)
    ON_MESSAGE(WM_SERIAL_ERROR, &CMainFrame::OnSerialError)
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
{
    m_cfg.baudRate = BaudRate::BR_115200;
}

CMainFrame::~CMainFrame() {}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs)) return FALSE;
    cs.style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    return TRUE;
}

BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName,
    DWORD dwStyle, const RECT& rect,
    CWnd* pParentWnd, LPCTSTR lpszMenuName,
    DWORD dwExStyle, CCreateContext* pContext)
{
    return CFrameWnd::Create(lpszClassName, lpszWindowName,
        dwStyle, rect, pParentWnd, lpszMenuName, dwExStyle, pContext);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
    if (CFrameWnd::OnCreate(lpcs) == -1) return -1;

    BuildMenu();

    // Status bar
    static UINT indicators[] = { ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR, ID_SEPARATOR };
    m_statusBar.Create(this);
    m_statusBar.SetIndicators(indicators, 4);
    m_statusBar.SetPaneInfo(0, ID_SEPARATOR, SBPS_STRETCH, 0);
    m_statusBar.SetPaneInfo(1, ID_SEPARATOR, SBPS_NORMAL, 120);
    m_statusBar.SetPaneInfo(2, ID_SEPARATOR, SBPS_NORMAL, 80);
    m_statusBar.SetPaneInfo(3, ID_SEPARATOR, SBPS_NORMAL, 80);

    // Terminal view (owner-draw listbox)
    m_terminal.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_OWNERDRAWFIXED |
                      LBS_HASSTRINGS | WS_VSCROLL | LBS_EXTENDEDSEL |
                      LBS_NOINTEGRALHEIGHT,
        CRect(0, 0, 10, 10), this, AFX_IDW_PANE_FIRST);

    // Send bar
    BuildSendBar();

    LoadSettings();
    SetTimer(TIMER_STATUS, 500, nullptr);
    UpdateTitle();
    return 0;
}

void CMainFrame::BuildMenu()
{
    CMenu menu;
    menu.CreateMenu();

    CMenu fileMenu;
    fileMenu.CreatePopupMenu();
    fileMenu.AppendMenu(MF_STRING, ID_FILE_SAVE_LOG, L"&Save Log...\tCtrl+S");
    fileMenu.AppendMenu(MF_STRING, ID_FILE_CLEAR_LOG,L"C&lear Log\tCtrl+L");
    fileMenu.AppendMenu(MF_SEPARATOR);
    fileMenu.AppendMenu(MF_STRING, ID_APP_EXIT, L"E&xit");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)fileMenu.Detach(), L"&File");

    CMenu connMenu;
    connMenu.CreatePopupMenu();
    connMenu.AppendMenu(MF_STRING, ID_CONN_SETTINGS,   L"&Settings...");
    connMenu.AppendMenu(MF_SEPARATOR);
    connMenu.AppendMenu(MF_STRING, ID_CONN_CONNECT,    L"&Connect\tF5");
    connMenu.AppendMenu(MF_STRING, ID_CONN_DISCONNECT, L"&Disconnect\tF6");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)connMenu.Detach(), L"&Connection");

    CMenu viewMenu;
    viewMenu.CreatePopupMenu();
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_ASCII, L"&ASCII Mode");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_HEX,   L"&HEX Mode");
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_MIXED, L"&Mixed Mode");
    viewMenu.AppendMenu(MF_SEPARATOR);
    viewMenu.AppendMenu(MF_STRING, ID_VIEW_TIMESTAMP, L"Show &Timestamps");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)viewMenu.Detach(), L"&View");

    CMenu toolsMenu;
    toolsMenu.CreatePopupMenu();
    toolsMenu.AppendMenu(MF_STRING, ID_TOOLS_SEND_FILE, L"Send &File...");
    toolsMenu.AppendMenu(MF_SEPARATOR);
    toolsMenu.AppendMenu(MF_STRING, ID_TOOLS_DTR, L"Toggle &DTR");
    toolsMenu.AppendMenu(MF_STRING, ID_TOOLS_RTS, L"Toggle &RTS");
    menu.AppendMenu(MF_POPUP, (UINT_PTR)toolsMenu.Detach(), L"&Tools");

    SetMenu(&menu);
    menu.Detach();
}

void CMainFrame::BuildSendBar()
{
    // Edit field for typed input
    m_editSend.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        CRect(0, 0, 10, 10), this, IDC_EDIT_SEND);

    // Send button
    m_btnSend.Create(L"Send", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        CRect(0, 0, 10, 10), this, IDC_BTN_SEND);

    // CR/LF checkboxes
    m_btnCR.Create(L"CR", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(0, 0, 10, 10), this, IDC_CHK_CR);
    m_btnLF.Create(L"LF", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(0, 0, 10, 10), this, IDC_CHK_LF);
    m_btnLF.SetCheck(BST_CHECKED); // default: append LF

    // Hex send toggle
    m_btnSendHex.Create(L"HEX", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        CRect(0, 0, 10, 10), this, IDC_CHK_HEX);
}

void CMainFrame::LayoutControls(int cx, int cy)
{
    CRect statusRc;
    m_statusBar.GetWindowRect(statusRc);
    int statusH = statusRc.Height();

    int termBottom = cy - statusH - SEND_BAR_H;
    m_terminal.MoveWindow(0, 0, cx, termBottom);

    // Send bar layout
    int y = termBottom + 4;
    int h = SEND_BAR_H - 8;

    int btnW   = 70;
    int chkW   = 45;
    int hexW   = 50;
    int editW  = cx - btnW - chkW * 2 - hexW - 20;

    int x = 4;
    m_editSend.MoveWindow(x, y, editW, h);
    x += editW + 4;
    m_btnCR.MoveWindow(x, y, chkW, h);  x += chkW;
    m_btnLF.MoveWindow(x, y, chkW, h);  x += chkW;
    m_btnSendHex.MoveWindow(x, y, hexW, h); x += hexW + 4;
    m_btnSend.MoveWindow(x, y, btnW, h);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWnd::OnSize(nType, cx, cy);
    if (m_terminal.GetSafeHwnd())
        LayoutControls(cx, cy);
}

void CMainFrame::OnDestroy()
{
    KillTimer(TIMER_STATUS);
    m_serial.Close();
    SaveSettings();
    CFrameWnd::OnDestroy();
}

// ─── Connection ─────────────────────────────────────────────────────────────

void CMainFrame::OnConnSettings()
{
    CConnectDialog dlg(this);
    dlg.SetConfig(m_cfg);
    if (dlg.DoModal() == IDOK) {
        m_cfg = dlg.GetConfig();
    }
}

void CMainFrame::OnConnect()
{
    if (m_serial.IsOpen()) return;

    if (m_cfg.port.empty()) {
        OnConnSettings();
        if (m_cfg.port.empty()) return;
    }

    // Register callbacks (called from worker thread — post to UI thread)
    m_serial.SetDataCallback([this](const std::vector<BYTE>& data) {
        auto* heap = new std::vector<BYTE>(data);
        PostMessage(WM_SERIAL_DATA, 0, reinterpret_cast<LPARAM>(heap));
    });
    m_serial.SetErrorCallback([this](const std::wstring& msg) {
        auto* heap = new std::wstring(msg);
        PostMessage(WM_SERIAL_ERROR, 0, reinterpret_cast<LPARAM>(heap));
    });

    if (!m_serial.Open(m_cfg)) {
        MessageBox(L"Failed to open port. Check settings and availability.",
                   L"Connection Error", MB_ICONERROR);
        return;
    }

    UpdateTitle();
    UpdateStatusBar();
}

void CMainFrame::OnDisconnect()
{
    m_serial.Close();
    UpdateTitle();
    UpdateStatusBar();
}

void CMainFrame::OnUpdateConnect(CCmdUI* pUI)
{
    pUI->Enable(!m_serial.IsOpen());
}

void CMainFrame::OnUpdateDisconnect(CCmdUI* pUI)
{
    pUI->Enable(m_serial.IsOpen());
}

// ─── View ────────────────────────────────────────────────────────────────────

void CMainFrame::OnViewAscii()  { m_terminal.SetDisplayMode(DisplayMode::ASCII); }
void CMainFrame::OnViewHex()    { m_terminal.SetDisplayMode(DisplayMode::HEX);   }
void CMainFrame::OnViewMixed()  { m_terminal.SetDisplayMode(DisplayMode::MIXED); }

void CMainFrame::OnViewTimestamp()
{
    static bool show = true;
    show = !show;
    m_terminal.SetShowTimestamp(show);
}

void CMainFrame::OnUpdateViewMode(CCmdUI* pUI)
{
    DisplayMode cur = m_terminal.GetDisplayMode();
    bool checked = false;
    if (pUI->m_nID == ID_VIEW_ASCII && cur == DisplayMode::ASCII) checked = true;
    if (pUI->m_nID == ID_VIEW_HEX   && cur == DisplayMode::HEX)   checked = true;
    if (pUI->m_nID == ID_VIEW_MIXED && cur == DisplayMode::MIXED)  checked = true;
    pUI->SetRadio(checked);
}

void CMainFrame::OnUpdateTimestamp(CCmdUI* pUI) { pUI->Enable(TRUE); }

// ─── File ────────────────────────────────────────────────────────────────────

void CMainFrame::OnFileSaveLog()
{
    CFileDialog dlg(FALSE, L"log", L"serial_log",
        OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
        L"Log Files (*.log)|*.log|Text Files (*.txt)|*.txt||");
    if (dlg.DoModal() == IDOK) {
        m_terminal.SaveToFile(dlg.GetPathName());
    }
}

void CMainFrame::OnFileClearLog()
{
    m_terminal.Clear();
}

// ─── Tools ───────────────────────────────────────────────────────────────────

void CMainFrame::OnSendFile()
{
    CFileDialog dlg(TRUE, nullptr, nullptr,
        OFN_FILEMUSTEXIST, L"All Files (*.*)|*.*||");
    if (dlg.DoModal() != IDOK) return;

    std::ifstream file(dlg.GetPathName().GetString(), std::ios::binary);
    if (!file) { MessageBox(L"Cannot open file.", L"Error", MB_ICONERROR); return; }

    file.seekg(0, std::ios::end);
    std::streamsize sz = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<BYTE> data(static_cast<size_t>(sz));
    file.read(reinterpret_cast<char*>(data.data()), sz);
    if (m_serial.Write(data)) {
        m_terminal.AppendData(data, false);
    }
}

void CMainFrame::OnToggleDTR()
{
    if (!m_serial.IsOpen()) return;
    m_dtr = !m_dtr;
    m_serial.SetDTR(m_dtr);
}

void CMainFrame::OnToggleRTS()
{
    if (!m_serial.IsOpen()) return;
    m_rts = !m_rts;
    m_serial.SetRTS(m_rts);
}

void CMainFrame::OnUpdateDTR(CCmdUI* pUI)
{
    pUI->Enable(m_serial.IsOpen());
    pUI->SetCheck(m_dtr ? 1 : 0);
}

void CMainFrame::OnUpdateRTS(CCmdUI* pUI)
{
    pUI->Enable(m_serial.IsOpen());
    pUI->SetCheck(m_rts ? 1 : 0);
}

// ─── Send bar ────────────────────────────────────────────────────────────────

void CMainFrame::OnSend()
{
    CString text;
    m_editSend.GetWindowText(text);
    if (text.IsEmpty()) return;

    std::vector<BYTE> data;
    bool asHex = (m_btnSendHex.GetCheck() == BST_CHECKED);

    if (asHex) {
        // Parse space-separated hex bytes: "0D 0A 41 42"
        std::wstring s = text.GetString();
        std::wistringstream iss(s);
        std::wstring token;
        while (iss >> token) {
            try {
                BYTE b = static_cast<BYTE>(std::stoul(token, nullptr, 16));
                data.push_back(b);
            } catch (...) {}
        }
    } else {
        // Convert CString (wide) to UTF-8 bytes
        int len = WideCharToMultiByte(CP_UTF8, 0, text.GetString(), -1, nullptr, 0, nullptr, nullptr);
        if (len > 1) {
            std::string narrow(len - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, text.GetString(), -1, &narrow[0], len, nullptr, nullptr);
            data.assign(narrow.begin(), narrow.end());
        }
        if (m_btnCR.GetCheck() == BST_CHECKED) data.push_back('\r');
        if (m_btnLF.GetCheck() == BST_CHECKED) data.push_back('\n');
    }

    if (data.empty()) return;

    if (m_serial.IsOpen() && m_serial.Write(data)) {
        m_terminal.AppendData(data, false);
        m_editSend.SetWindowText(L"");
    } else if (!m_serial.IsOpen()) {
        MessageBox(L"Not connected.", L"Error", MB_ICONWARNING);
    }
}

// ─── Serial data received ────────────────────────────────────────────────────

LRESULT CMainFrame::OnSerialData(WPARAM, LPARAM lParam)
{
    auto* data = reinterpret_cast<std::vector<BYTE>*>(lParam);
    if (data) {
        m_terminal.AppendData(*data, true);
        delete data;
    }
    return 0;
}

LRESULT CMainFrame::OnSerialError(WPARAM, LPARAM lParam)
{
    auto* msg = reinterpret_cast<std::wstring*>(lParam);
    if (msg) {
        m_terminal.AppendData(
            std::vector<BYTE>(msg->begin(), msg->end()), false);
        m_statusBar.SetPaneText(0, (L"Error: " + *msg).c_str());
        delete msg;
    }
    return 0;
}

// ─── Status / Title ──────────────────────────────────────────────────────────

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_STATUS) UpdateStatusBar();
    CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::UpdateStatusBar()
{
    if (m_serial.IsOpen()) {
        CString port(m_cfg.port.c_str());
        CString baud; baud.Format(L"%lu", (DWORD)m_cfg.baudRate);
        m_statusBar.SetPaneText(0, L"Connected");
        m_statusBar.SetPaneText(1, port + L"  " + baud);
        m_statusBar.SetPaneText(2, m_serial.GetCTS() ? L"CTS ON" : L"CTS OFF");
        m_statusBar.SetPaneText(3, m_serial.GetDSR() ? L"DSR ON" : L"DSR OFF");
    } else {
        m_statusBar.SetPaneText(0, L"Disconnected");
        m_statusBar.SetPaneText(1, L"---");
        m_statusBar.SetPaneText(2, L"");
        m_statusBar.SetPaneText(3, L"");
    }
}

void CMainFrame::UpdateTitle()
{
    if (m_serial.IsOpen()) {
        CString title;
        title.Format(L"Serial Terminal — %s @ %lu",
            m_cfg.port.c_str(), (DWORD)m_cfg.baudRate);
        SetWindowText(title);
    } else {
        SetWindowText(L"Serial Terminal — Disconnected");
    }
}

// ─── Persistence ─────────────────────────────────────────────────────────────

void CMainFrame::SaveSettings()
{
    AfxGetApp()->WriteProfileString(L"Port",   L"Name",     m_cfg.port.c_str());
    AfxGetApp()->WriteProfileInt(   L"Port",   L"BaudRate", (int)m_cfg.baudRate);
    AfxGetApp()->WriteProfileInt(   L"Port",   L"DataBits", (int)m_cfg.dataBits);
    AfxGetApp()->WriteProfileInt(   L"Port",   L"StopBits", (int)m_cfg.stopBits);
    AfxGetApp()->WriteProfileInt(   L"Port",   L"Parity",   (int)m_cfg.parity);
    AfxGetApp()->WriteProfileInt(   L"Port",   L"Flow",     (int)m_cfg.flowCtrl);
}

void CMainFrame::LoadSettings()
{
    CString port = AfxGetApp()->GetProfileString(L"Port", L"Name", L"COM1");
    m_cfg.port     = port.GetString();
    m_cfg.baudRate = (BaudRate)AfxGetApp()->GetProfileInt(L"Port", L"BaudRate", 115200);
    m_cfg.dataBits = (DataBits)AfxGetApp()->GetProfileInt(L"Port", L"DataBits", 8);
    m_cfg.stopBits = (StopBits)AfxGetApp()->GetProfileInt(L"Port", L"StopBits", ONESTOPBIT);
    m_cfg.parity   = (Parity)  AfxGetApp()->GetProfileInt(L"Port", L"Parity",   NOPARITY);
    m_cfg.flowCtrl = (FlowCtrl)AfxGetApp()->GetProfileInt(L"Port", L"Flow",     0);
}
