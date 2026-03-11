#pragma once
#include "stdafx.h"
#include "SerialPort.h"

// IDD_CONNECT_DIALOG = 200
#define IDD_CONNECT_DIALOG 200
#define IDC_COMBO_PORT      201
#define IDC_COMBO_BAUD      202
#define IDC_COMBO_DATABITS  203
#define IDC_COMBO_STOPBITS  204
#define IDC_COMBO_PARITY    205
#define IDC_COMBO_FLOW      206
#define IDC_BTN_REFRESH     207
#define IDC_STATIC_PREVIEW  208

class CConnectDialog : public CDialog
{
public:
    explicit CConnectDialog(CWnd* pParent = nullptr);

    SerialConfig GetConfig() const { return m_cfg; }
    void         SetConfig(const SerialConfig& cfg) { m_cfg = cfg; }

    enum { IDD = IDD_CONNECT_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    virtual void OnOK() override;

    DECLARE_MESSAGE_MAP()
    afx_msg void OnRefreshPorts();
    afx_msg void OnSettingsChanged();

private:
    void PopulatePorts();
    void PopulateBaudRates();
    void UpdatePreview();

    CComboBox m_cbPort, m_cbBaud, m_cbData, m_cbStop, m_cbParity, m_cbFlow;
    CStatic   m_stPreview;
    SerialConfig m_cfg;
};
