#pragma once
#include "stdafx.h"

// Baud rate options
enum class BaudRate : DWORD {
    BR_110    = 110,
    BR_300    = 300,
    BR_600    = 600,
    BR_1200   = 1200,
    BR_2400   = 2400,
    BR_4800   = 4800,
    BR_9600   = 9600,
    BR_14400  = 14400,
    BR_19200  = 19200,
    BR_38400  = 38400,
    BR_57600  = 57600,
    BR_115200 = 115200,
    BR_230400 = 230400,
    BR_460800 = 460800,
    BR_921600 = 921600
};

enum class DataBits  { DB_5 = 5, DB_6 = 6, DB_7 = 7, DB_8 = 8 };
enum class StopBits  { SB_1 = ONESTOPBIT, SB_15 = ONE5STOPBITS, SB_2 = TWOSTOPBITS };
enum class Parity    { NONE = NOPARITY, ODD = ODDPARITY, EVEN = EVENPARITY, MARK = MARKPARITY, SPACE = SPACEPARITY };
enum class FlowCtrl  { NONE, HARDWARE, SOFTWARE };

struct SerialConfig {
    std::wstring port      = L"COM1";
    BaudRate     baudRate  = BaudRate::BR_115200;
    DataBits     dataBits  = DataBits::DB_8;
    StopBits     stopBits  = StopBits::SB_1;
    Parity       parity    = Parity::NONE;
    FlowCtrl     flowCtrl  = FlowCtrl::NONE;
    DWORD        readTimeout  = 50;   // ms
    DWORD        writeTimeout = 500;  // ms
};

// Callback: fired on every received chunk
using DataCallback  = std::function<void(const std::vector<BYTE>&)>;
using ErrorCallback = std::function<void(const std::wstring&)>;

class CSerialPort
{
public:
    CSerialPort();
    ~CSerialPort();

    bool Open(const SerialConfig& cfg);
    void Close();
    bool IsOpen() const { return m_hPort != INVALID_HANDLE_VALUE; }

    bool Write(const std::vector<BYTE>& data);
    bool Write(const std::string& text);

    void SetDataCallback(DataCallback cb)  { m_dataCb  = cb; }
    void SetErrorCallback(ErrorCallback cb){ m_errorCb = cb; }

    const SerialConfig& GetConfig() const { return m_cfg; }

    // Enumerate available COM ports
    static std::vector<std::wstring> EnumeratePorts();

    // Line status
    bool GetCTS() const;
    bool GetDSR() const;
    bool GetRING() const;
    bool GetRLSD() const;
    void SetDTR(bool on);
    void SetRTS(bool on);

private:
    void ReadThread();
    void ReportError(const std::wstring& msg);

    HANDLE          m_hPort    = INVALID_HANDLE_VALUE;
    SerialConfig    m_cfg;
    DataCallback    m_dataCb;
    ErrorCallback   m_errorCb;
    std::thread     m_readThread;
    std::atomic<bool> m_running{ false };
    mutable std::mutex m_writeMutex;

    static constexpr size_t READ_BUF = 4096;
};
