/*
Desura is the leading indie game distribution platform
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)

$LicenseInfo:firstyear=2014&license=lgpl$
Copyright (C) 2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <http://www.gnu.org/licenses/>
or write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
*/

#include "Common.h"
#include "gcSpinningBar.h"

#include "gcImage.h"
#include "gcImageHandle.h"

#include "gcManagers.h"

#include "gcFrame.h"
#include "gcDialog.h"
#include "gcScrolledWindow.h"
#include "gcTaskBar.h"

#ifdef NIX
#include <gtk/gtk.h>
#endif

#ifdef WIN32
static std::mutex g_TimerLock;
static std::map<int32, gcSpinningBar*> g_SpinnerMap;

static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (uMsg == WM_TIMER)
	{
		g_TimerLock.lock();
	
		std::map<int32, gcSpinningBar*>::iterator it = g_SpinnerMap.find(idEvent);

		if (it != g_SpinnerMap.end() && it->second)
			it->second->notify();

		g_TimerLock.unlock();
	}
}
#else
static gboolean TimerProc(gcSpinningBar *bar)
{
	if (bar)
		bar->notify();
		
	return TRUE;
}

#endif

gcSpinningBar::gcSpinningBar( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size) : gcPanel(parent, id, pos, size, wxSIMPLE_BORDER)
{
#ifdef WIN32
	g_TimerLock.lock();
	g_SpinnerMap[GetId()] = this;
	g_TimerLock.unlock();
#endif

	SetBackgroundColour( wxColour( 125, 255, 125 ) );
 
	Bind(wxEVT_PAINT, &gcSpinningBar::onPaint, this);
	Bind(wxEVT_ERASE_BACKGROUND, &gcSpinningBar::onEraseBg, this);

	m_imgProg = GetGCThemeManager()->getImageHandle("#spinningbar");
	m_uiOffset = 0;

#ifdef WIN32
	m_tId = ::SetTimer((HWND)GetHWND(), GetId(), 75, TimerProc);
#else
	m_tId = g_timeout_add(75, (GSourceFunc)TimerProc, (gpointer)this);
#endif

	m_bNotifyRedraw = false;
	onNeedRedrawEvent += guiDelegate(this, &gcSpinningBar::onNeedRedraw);
}

gcSpinningBar::~gcSpinningBar()
{
#ifdef WIN32
	g_TimerLock.lock();
	
	std::map<int32, gcSpinningBar*>::iterator it = g_SpinnerMap.find(GetId());

	if (it != g_SpinnerMap.end())
		g_SpinnerMap.erase(it);

	g_TimerLock.unlock();

	::KillTimer((HWND)GetHWND(), m_tId);

#else
	if (m_tId)
		g_source_remove(m_tId);
#endif
}

void gcSpinningBar::notify()
{
	m_uiOffset++;

	if (m_uiOffset >= (uint32)m_imgProg->GetWidth())
		m_uiOffset = 0;

	if (!m_bNotifyRedraw)
	{
		m_bNotifyRedraw = true;
		onNeedRedrawEvent();
	}
}

void gcSpinningBar::onNeedRedraw()
{
	m_bNotifyRedraw = false;

	wxClientDC dc(this);
	doPaint(&dc);
}

void gcSpinningBar::onPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	if (GetSize().GetWidth() == 0 || GetSize().GetHeight() == 0)
		return;

	dc.SetTextBackground(GetBackgroundColour());

	if (m_imgProg.getImg() && m_imgProg->IsOk())
		doPaint(&dc);
}

void gcSpinningBar::doPaint(wxDC* dc)
{
	int h = GetSize().GetHeight();
	int w = GetSize().GetWidth();

	int iw = m_imgProg->GetSize().GetWidth();

	if (w == 0 || h == 0)
		return;

	if (m_Buffer.GetSize() != GetSize())
		m_Buffer = wxBitmap(w,h);

	wxImage scaled = m_imgProg->Scale(iw, h);
	tileImg(wxBitmap(scaled));

	dc->DrawBitmap(m_Buffer, 0,0, true);
}

void gcSpinningBar::tileImg(wxBitmap src)
{
	wxMemoryDC dc(m_Buffer);

	int32 start = m_uiOffset-src.GetWidth();

	for (int32 x=start; x<m_Buffer.GetWidth(); x+=src.GetWidth())
		dc.DrawBitmap(src, x, 0, false);

	dc.SelectObject(wxNullBitmap);
}
