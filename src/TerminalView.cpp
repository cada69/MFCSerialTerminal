#include "stdafx.h"
#include "TerminalView.h"
#include <fstream>

BEGIN_MESSAGE_MAP(CTerminalView, CListBox)
    ON_WM_CONTEXTMENU()
    ON_COMMAND(ID_EDIT_COPY,      &CTerminalView::OnCopy)
    ON_COMMAND(ID_EDIT_SELECT_ALL,&CTerminalView::OnSelectAll)
    ON_COMMAND(ID_EDIT_CLEAR,     &CTerminalView::OnClear)
END_MESSAGE_MAP()

CTerminalView::CTerminalView()
{
    LOGFONT lf{};
    lf.lfHeight  = -14;
    lf.lfWeight  = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
    wcscpy_s(lf.lfFaceName, L"Consolas");
    m_font.CreateFontIndirect(&lf);
}

CTerminalView::~CTerminalView() {}

void CTerminalView::PreSubclassWindow()
{
    CListBox::PreSubclassWindow();
    ModifyStyle(0, LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT |
                   WS_VSCROLL | LBS_EXTENDEDSEL);
    CListBox::SetFont(&m_font);
}

void CTerminalView::AppendData(const std::vector<BYTE>& data, bool isRx)
{
    // Split on newlines for ASCII mode, group into rows for hex
    if (m_mode == DisplayMode::ASCII || m_mode == DisplayMode::MIXED) {
        // Accumulate and split on \n
        static std::vector<BYTE> rxBuf, txBuf;
        auto& buf = isRx ? rxBuf : txBuf;

        for (BYTE b : data) {
            if (b == '\n') {
                TerminalLine line;
                line.raw = buf;
                line.isRx = isRx;
                GetLocalTime(&line.timestamp);
                m_lines.push_back(line);
                buf.clear();

                std::wstring text = FormatLine(m_lines.back());
                AddString(text.c_str());
                if ((int)m_lines.size() > m_maxLines) {
                    m_lines.erase(m_lines.begin());
                    DeleteString(0);
                }
            } else if (b != '\r') {
                buf.push_back(b);
            }
        }
    } else {
        // Hex mode: 16 bytes per line
        for (size_t i = 0; i < data.size(); i += 16) {
            TerminalLine line;
            line.raw   = std::vector<BYTE>(data.begin() + (ptrdiff_t)i,
                         data.begin() + (ptrdiff_t)std::min(i + 16, data.size()));
            line.isRx  = isRx;
            GetLocalTime(&line.timestamp);
            m_lines.push_back(line);

            std::wstring text = FormatLine(m_lines.back());
            AddString(text.c_str());
            if ((int)m_lines.size() > m_maxLines) {
                m_lines.erase(m_lines.begin());
                DeleteString(0);
            }
        }
    }

    // Auto-scroll to bottom
    int count = GetCount();
    if (count > 0) SetTopIndex(count - 1);
}

void CTerminalView::Clear()
{
    m_lines.clear();
    ResetContent();
}

void CTerminalView::SetDisplayMode(DisplayMode mode)
{
    m_mode = mode;
    // Rebuild display
    std::vector<TerminalLine> saved = m_lines;
    ResetContent();
    m_lines.clear();
    for (auto& line : saved) {
        std::wstring text = FormatLine(line);
        m_lines.push_back(line);
        AddString(text.c_str());
    }
}

void CTerminalView::SetFont(const LOGFONT& lf)
{
    m_font.DeleteObject();
    m_font.CreateFontIndirect(&lf);
    CListBox::SetFont(&m_font);
}

std::wstring CTerminalView::FormatLine(const TerminalLine& line) const
{
    std::wstring result;
    if (m_showTimestamp) {
        result += FormatTimestamp(line.timestamp) + L" ";
    }
    result += line.isRx ? L"RX " : L"TX ";

    switch (m_mode) {
    case DisplayMode::ASCII: {
        std::wstring text;
        for (BYTE b : line.raw) {
            if (b >= 0x20 && b < 0x7F) text += (wchar_t)b;
            else { text += L"["; text += std::to_wstring(b); text += L"]"; }
        }
        result += text;
        break;
    }
    case DisplayMode::HEX:
        result += FormatHex(line.raw);
        break;
    case DisplayMode::MIXED:
        result += FormatMixed(line.raw);
        break;
    }
    return result;
}

std::wstring CTerminalView::FormatHex(const std::vector<BYTE>& data) const
{
    std::wostringstream oss;
    for (size_t i = 0; i < data.size(); ++i) {
        oss << std::hex << std::uppercase << std::setw(2) << std::setfill(L'0')
            << (int)data[i];
        if (i + 1 < data.size()) oss << L" ";
    }
    // ASCII sidebar
    oss << L"  |";
    for (BYTE b : data)
        oss << (wchar_t)(b >= 0x20 && b < 0x7F ? b : L'.');
    oss << L"|";
    return oss.str();
}

std::wstring CTerminalView::FormatMixed(const std::vector<BYTE>& data) const
{
    std::wstring result;
    for (BYTE b : data) {
        if (b >= 0x20 && b < 0x7F) {
            result += (wchar_t)b;
        } else {
            std::wostringstream oss;
            oss << L"<" << std::hex << std::uppercase << std::setw(2)
                << std::setfill(L'0') << (int)b << L">";
            result += oss.str();
        }
    }
    return result;
}

std::wstring CTerminalView::FormatTimestamp(const SYSTEMTIME& st) const
{
    wchar_t buf[32];
    swprintf_s(buf, L"%02d:%02d:%02d.%03d",
        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buf;
}

void CTerminalView::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
    if (lpDIS->itemID == (UINT)-1) return;

    CDC dc;
    dc.Attach(lpDIS->hDC);
    CRect rc(lpDIS->rcItem);

    bool selected = (lpDIS->itemState & ODS_SELECTED) != 0;

    // Background
    if (selected)
        dc.FillSolidRect(rc, m_colSel);
    else
        dc.FillSolidRect(rc, m_colBg);

    if (lpDIS->itemID < (UINT)m_lines.size()) {
        const TerminalLine& line = m_lines[lpDIS->itemID];
        dc.SetBkMode(TRANSPARENT);

        CFont* pOld = dc.SelectObject(&m_font);
        CRect textRc = rc;
        textRc.left += 4;

        // Timestamp in gray
        if (m_showTimestamp) {
            std::wstring ts = FormatTimestamp(line.timestamp) + L" ";
            dc.SetTextColor(m_colTimestamp);
            dc.DrawText(ts.c_str(), (int)ts.size(), textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SIZE sz;
            GetTextExtentPoint32W(dc.m_hDC, ts.c_str(), (int)ts.size(), &sz);
            textRc.left += sz.cx;
        }

        // Direction tag
        std::wstring dir = line.isRx ? L"RX " : L"TX ";
        dc.SetTextColor(line.isRx ? RGB(100, 200, 100) : RGB(100, 150, 255));
        dc.DrawText(dir.c_str(), (int)dir.size(), textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SIZE szDir;
        GetTextExtentPoint32W(dc.m_hDC, dir.c_str(), (int)dir.size(), &szDir);
        textRc.left += szDir.cx;

        // Data
        std::wstring data;
        switch (m_mode) {
        case DisplayMode::ASCII: {
            for (BYTE b : line.raw) {
                if (b >= 0x20 && b < 0x7F) data += (wchar_t)b;
                else { data += L"["; data += std::to_wstring(b); data += L"]"; }
            }
            break;
        }
        case DisplayMode::HEX:   data = FormatHex(line.raw);   break;
        case DisplayMode::MIXED: data = FormatMixed(line.raw); break;
        }

        dc.SetTextColor(line.isRx ? m_colRxText : m_colTxText);
        dc.DrawText(data.c_str(), (int)data.size(), textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        dc.SelectObject(pOld);
    }
    dc.Detach();
}

void CTerminalView::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
    lpMIS->itemHeight = m_lineHeight;
}

void CTerminalView::OnContextMenu(CWnd*, CPoint pt)
{
    CMenu menu;
    menu.CreatePopupMenu();
    menu.AppendMenu(MF_STRING, ID_EDIT_COPY,       L"Copy\tCtrl+C");
    menu.AppendMenu(MF_STRING, ID_EDIT_SELECT_ALL, L"Select All\tCtrl+A");
    menu.AppendMenu(MF_SEPARATOR);
    menu.AppendMenu(MF_STRING, ID_EDIT_CLEAR,      L"Clear");
    menu.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, this);
}

void CTerminalView::CopySelectedToClipboard()
{
    OnCopy();
}

void CTerminalView::OnCopy()
{
    int count = GetCount();
    if (count <= 0) return;

    std::vector<int> selItems(count);
    int selCount = GetSelItems(count, selItems.data());
    if (selCount <= 0) return;

    std::wstring text;
    for (int i = 0; i < selCount; ++i) {
        int idx = selItems[i];
        if (idx < (int)m_lines.size()) {
            text += FormatLine(m_lines[idx]) + L"\r\n";
        }
    }
    if (text.empty()) return;

    if (OpenClipboard()) {
        EmptyClipboard();
        size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            memcpy(GlobalLock(hMem), text.c_str(), bytes);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }
}

void CTerminalView::OnSelectAll()
{
    SelItemRange(TRUE, 0, GetCount() - 1);
}

void CTerminalView::OnClear()
{
    Clear();
}

void CTerminalView::SaveToFile(const CString& path)
{
    std::wofstream file(path.GetString());
    if (!file) return;
    for (auto& line : m_lines)
        file << FormatLine(line) << L"\r\n";
}
