#include "stdafx.h"
#include "SerialPort.h"
#include <SetupAPI.h>
#pragma comment(lib, "setupapi.lib")

CSerialPort::CSerialPort() = default;

CSerialPort::~CSerialPort()
{
    Close();
}

bool CSerialPort::Open(const SerialConfig& cfg)
{
    Close();
    m_cfg = cfg;

    std::wstring portPath = L"\\\\.\\" + cfg.port;
    m_hPort = CreateFileW(portPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        nullptr);

    if (m_hPort == INVALID_HANDLE_VALUE) {
        ReportError(L"Failed to open " + cfg.port);
        return false;
    }

    // Set buffer sizes
    SetupComm(m_hPort, READ_BUF, READ_BUF);

    // Timeouts
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout         = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier  = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant    = cfg.readTimeout;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant   = cfg.writeTimeout;
    SetCommTimeouts(m_hPort, &timeouts);

    // DCB
    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(m_hPort, &dcb)) {
        ReportError(L"GetCommState failed");
        Close(); return false;
    }

    dcb.BaudRate = static_cast<DWORD>(cfg.baudRate);
    dcb.ByteSize = static_cast<BYTE>(cfg.dataBits);
    dcb.StopBits = static_cast<BYTE>(cfg.stopBits);
    dcb.Parity   = static_cast<BYTE>(cfg.parity);

    switch (cfg.flowCtrl) {
    case FlowCtrl::HARDWARE:
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl  = RTS_CONTROL_HANDSHAKE;
        break;
    case FlowCtrl::SOFTWARE:
        dcb.fOutX = dcb.fInX = TRUE;
        dcb.XonChar  = 0x11;
        dcb.XoffChar = 0x13;
        break;
    default:
        dcb.fOutxCtsFlow = FALSE;
        dcb.fRtsControl  = RTS_CONTROL_DISABLE;
        dcb.fOutX = dcb.fInX = FALSE;
        break;
    }

    if (!SetCommState(m_hPort, &dcb)) {
        ReportError(L"SetCommState failed — check port settings");
        Close(); return false;
    }

    PurgeComm(m_hPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    // Start read thread
    m_running = true;
    m_readThread = std::thread(&CSerialPort::ReadThread, this);
    return true;
}

void CSerialPort::Close()
{
    m_running = false;
    if (m_hPort != INVALID_HANDLE_VALUE) {
        // Unblock overlapped read
        CancelIoEx(m_hPort, nullptr);
        if (m_readThread.joinable())
            m_readThread.join();
        CloseHandle(m_hPort);
        m_hPort = INVALID_HANDLE_VALUE;
    } else {
        if (m_readThread.joinable())
            m_readThread.join();
    }
}

bool CSerialPort::Write(const std::vector<BYTE>& data)
{
    if (!IsOpen() || data.empty()) return false;
    std::lock_guard<std::mutex> lock(m_writeMutex);

    OVERLAPPED ov{};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    DWORD written = 0;
    bool ok = true;
    if (!WriteFile(m_hPort, data.data(), static_cast<DWORD>(data.size()), &written, &ov)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(ov.hEvent, m_cfg.writeTimeout + 100);
            GetOverlappedResult(m_hPort, &ov, &written, FALSE);
        } else {
            ok = false;
        }
    }
    CloseHandle(ov.hEvent);
    return ok;
}

bool CSerialPort::Write(const std::string& text)
{
    return Write(std::vector<BYTE>(text.begin(), text.end()));
}

void CSerialPort::ReadThread()
{
    std::vector<BYTE> buf(READ_BUF);
    OVERLAPPED ov{};
    ov.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    while (m_running) {
        DWORD bytesRead = 0;
        ResetEvent(ov.hEvent);

        if (!ReadFile(m_hPort, buf.data(), READ_BUF, &bytesRead, &ov)) {
            DWORD err = GetLastError();
            if (err == ERROR_IO_PENDING) {
                DWORD wait = WaitForSingleObject(ov.hEvent, m_cfg.readTimeout + 100);
                if (wait == WAIT_OBJECT_0) {
                    GetOverlappedResult(m_hPort, &ov, &bytesRead, FALSE);
                }
            } else if (err != ERROR_OPERATION_ABORTED) {
                ReportError(L"Read error: " + std::to_wstring(err));
                break;
            }
        }

        if (bytesRead > 0 && m_dataCb) {
            m_dataCb(std::vector<BYTE>(buf.begin(), buf.begin() + bytesRead));
        }
    }
    CloseHandle(ov.hEvent);
}

void CSerialPort::ReportError(const std::wstring& msg)
{
    if (m_errorCb) m_errorCb(msg);
}

bool CSerialPort::GetCTS()  const { DWORD s=0; GetCommModemStatus(m_hPort,&s); return (s & MS_CTS_ON)  != 0; }
bool CSerialPort::GetDSR()  const { DWORD s=0; GetCommModemStatus(m_hPort,&s); return (s & MS_DSR_ON)  != 0; }
bool CSerialPort::GetRING() const { DWORD s=0; GetCommModemStatus(m_hPort,&s); return (s & MS_RING_ON) != 0; }
bool CSerialPort::GetRLSD() const { DWORD s=0; GetCommModemStatus(m_hPort,&s); return (s & MS_RLSD_ON) != 0; }
void CSerialPort::SetDTR(bool on) { EscapeCommFunction(m_hPort, on ? SETDTR  : CLRDTR);  }
void CSerialPort::SetRTS(bool on) { EscapeCommFunction(m_hPort, on ? SETRTS  : CLRRTS);  }

std::vector<std::wstring> CSerialPort::EnumeratePorts()
{
    std::vector<std::wstring> ports;
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_PORTS, nullptr, nullptr,
        DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) return ports;

    SP_DEVINFO_DATA devData{};
    devData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &devData); ++i) {
        HKEY hKey = SetupDiOpenDevRegKey(hDevInfo, &devData, DICS_FLAG_GLOBAL,
            0, DIREG_DEV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE) continue;

        WCHAR portName[32]{};
        DWORD size = sizeof(portName);
        DWORD type = 0;
        if (RegQueryValueExW(hKey, L"PortName", nullptr, &type,
            reinterpret_cast<LPBYTE>(portName), &size) == ERROR_SUCCESS) {
            std::wstring name(portName);
            if (name.find(L"COM") != std::wstring::npos)
                ports.push_back(name);
        }
        RegCloseKey(hKey);
    }
    SetupDiDestroyDeviceInfoList(hDevInfo);

    // Sort: COM1, COM2, ... COM10, COM11
    std::sort(ports.begin(), ports.end(), [](const std::wstring& a, const std::wstring& b){
        auto num = [](const std::wstring& s){
            return std::stoi(s.substr(3));
        };
        try { return num(a) < num(b); } catch(...) { return a < b; }
    });
    return ports;
}
