#include "stdafx.h"
#include "ConnectDialog.h"

BEGIN_MESSAGE_MAP(CConnectDialog, CDialog)
    ON_BN_CLICKED(IDC_BTN_REFRESH, &CConnectDialog::OnRefreshPorts)
    ON_CBN_SELCHANGE(IDC_COMBO_PORT,     &CConnectDialog::OnSettingsChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_BAUD,     &CConnectDialog::OnSettingsChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_DATABITS, &CConnectDialog::OnSettingsChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_STOPBITS, &CConnectDialog::OnSettingsChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_PARITY,   &CConnectDialog::OnSettingsChanged)
    ON_CBN_SELCHANGE(IDC_COMBO_FLOW,     &CConnectDialog::OnSettingsChanged)
END_MESSAGE_MAP()

CConnectDialog::CConnectDialog(CWnd* pParent)
    : CDialog(IDD, pParent)
{}

void CConnectDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_PORT,     m_cbPort);
    DDX_Control(pDX, IDC_COMBO_BAUD,     m_cbBaud);
    DDX_Control(pDX, IDC_COMBO_DATABITS, m_cbData);
    DDX_Control(pDX, IDC_COMBO_STOPBITS, m_cbStop);
    DDX_Control(pDX, IDC_COMBO_PARITY,   m_cbParity);
    DDX_Control(pDX, IDC_COMBO_FLOW,     m_cbFlow);
    DDX_Control(pDX, IDC_STATIC_PREVIEW, m_stPreview);
}

BOOL CConnectDialog::OnInitDialog()
{
    CDialog::OnInitDialog();
    SetWindowText(L"Connect to Serial Port");

    PopulatePorts();
    PopulateBaudRates();

    // Data bits
    m_cbData.AddString(L"5"); m_cbData.AddString(L"6");
    m_cbData.AddString(L"7"); m_cbData.AddString(L"8");
    m_cbData.SetCurSel(3); // default 8

    // Stop bits
    m_cbStop.AddString(L"1"); m_cbStop.AddString(L"1.5"); m_cbStop.AddString(L"2");
    m_cbStop.SetCurSel(0);

    // Parity
    m_cbParity.AddString(L"None"); m_cbParity.AddString(L"Odd");
    m_cbParity.AddString(L"Even"); m_cbParity.AddString(L"Mark");
    m_cbParity.AddString(L"Space");
    m_cbParity.SetCurSel(0);

    // Flow control
    m_cbFlow.AddString(L"None"); m_cbFlow.AddString(L"Hardware (RTS/CTS)");
    m_cbFlow.AddString(L"Software (XON/XOFF)");
    m_cbFlow.SetCurSel(0);

    // Apply saved config
    CString portStr(m_cfg.port.c_str());
    int idx = m_cbPort.FindStringExact(-1, portStr);
    if (idx != CB_ERR) m_cbPort.SetCurSel(idx);

    UpdatePreview();
    return TRUE;
}

void CConnectDialog::PopulatePorts()
{
    m_cbPort.ResetContent();
    auto ports = CSerialPort::EnumeratePorts();
    for (auto& p : ports)
        m_cbPort.AddString(p.c_str());
    if (m_cbPort.GetCount() > 0)
        m_cbPort.SetCurSel(0);
    else
        m_cbPort.AddString(L"No ports found");
}

void CConnectDialog::PopulateBaudRates()
{
    static const wchar_t* rates[] = {
        L"110",L"300",L"600",L"1200",L"2400",L"4800",
        L"9600",L"14400",L"19200",L"38400",L"57600",
        L"115200",L"230400",L"460800",L"921600"
    };
    m_cbBaud.ResetContent();
    for (auto r : rates) m_cbBaud.AddString(r);
    // Default 115200
    int idx = m_cbBaud.FindStringExact(-1, L"115200");
    m_cbBaud.SetCurSel(idx != CB_ERR ? idx : 11);
}

void CConnectDialog::OnRefreshPorts()
{
    CString cur;
    m_cbPort.GetWindowText(cur);
    PopulatePorts();
    int idx = m_cbPort.FindStringExact(-1, cur);
    if (idx != CB_ERR) m_cbPort.SetCurSel(idx);
    UpdatePreview();
}

void CConnectDialog::OnSettingsChanged()
{
    UpdatePreview();
}

void CConnectDialog::UpdatePreview()
{
    CString port, baud, data, stop, parity, flow;
    m_cbPort.GetWindowText(port);
    m_cbBaud.GetWindowText(baud);
    m_cbData.GetWindowText(data);
    m_cbStop.GetWindowText(stop);
    m_cbParity.GetWindowText(parity);
    m_cbFlow.GetWindowText(flow);

    CString preview;
    preview.Format(L"%s  %s,%s,%s,%s  Flow:%s",
        (LPCWSTR)port, (LPCWSTR)baud,
        (LPCWSTR)data, (LPCWSTR)parity, (LPCWSTR)stop,
        (LPCWSTR)flow);
    m_stPreview.SetWindowText(preview);
}

void CConnectDialog::OnOK()
{
    CString port, baud;
    m_cbPort.GetWindowText(port);
    m_cbBaud.GetWindowText(baud);

    m_cfg.port = port.GetString();

    // Parse baud rate
    try { m_cfg.baudRate = static_cast<BaudRate>(_wtoi(baud)); }
    catch (...) { m_cfg.baudRate = BaudRate::BR_115200; }

    // Helper: clamp negative CB_ERR to 0
    auto sel = [](int v) -> int { return v < 0 ? 0 : v; };

    // Data bits
    static const DataBits db[] = { DataBits::DB_5, DataBits::DB_6, DataBits::DB_7, DataBits::DB_8 };
    m_cfg.dataBits = db[sel(m_cbData.GetCurSel())];

    // Stop bits
    static const StopBits sb[] = { StopBits::SB_1, StopBits::SB_15, StopBits::SB_2 };
    m_cfg.stopBits = sb[sel(m_cbStop.GetCurSel())];

    // Parity
    static const Parity par[] = { Parity::NONE, Parity::ODD, Parity::EVEN, Parity::MARK, Parity::SPACE };
    m_cfg.parity = par[sel(m_cbParity.GetCurSel())];

    // Flow
    static const FlowCtrl fc[] = { FlowCtrl::NONE, FlowCtrl::HARDWARE, FlowCtrl::SOFTWARE };
    m_cfg.flowCtrl = fc[sel(m_cbFlow.GetCurSel())];

    CDialog::OnOK();
}
