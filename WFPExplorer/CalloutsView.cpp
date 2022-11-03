#include "pch.h"
#include "CalloutsView.h"
#include "StringHelper.h"
#include <SortHelper.h>

CCalloutsView::CCalloutsView(IMainFrame* frame, WFPEngine& engine) : CFrameView(frame), m_Engine(engine) {
}

LRESULT CCalloutsView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_hWndClient = m_List.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | LVS_OWNERDATA | LVS_REPORT | LVS_SHOWSELALWAYS);
	m_List.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	auto cm = GetColumnManager(m_List);
	cm->AddColumn(L"Callout Key", 0, 250, ColumnType::Key);
	cm->AddColumn(L"Layer ID", LVCFMT_RIGHT, 90, ColumnType::Id);
	cm->AddColumn(L"Name", 0, 250, ColumnType::Name);
	cm->AddColumn(L"Flags", LVCFMT_RIGHT, 80, ColumnType::Flags);
	cm->AddColumn(L"Provider", 0, 250, ColumnType::Provider);
	cm->AddColumn(L"Description", 0, 300, ColumnType::Desc);

	Refresh();

	return 0;
}

LRESULT CCalloutsView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	Refresh();
	return 0;
}

void CCalloutsView::Refresh() {
	m_Callouts = m_Engine.EnumCallouts<CalloutInfo>();
	m_List.SetItemCountEx((int)m_Callouts.size(), LVSICF_NOSCROLL);
}

CString CCalloutsView::GetColumnText(HWND, int row, int col) {
	auto& info = m_Callouts[row];
	switch (GetColumnManager(m_List)->GetColumnTag<ColumnType>(col)) {
		case ColumnType::Key: return StringHelper::GuidToString(info.CalloutKey);
		case ColumnType::Provider:
			if (info.Provider.IsEmpty()) {
				if (info.ProviderKey != GUID_NULL) {
					auto sl = m_Engine.GetProviderByKey(info.ProviderKey);
					if (sl && !sl.value().Name.empty() && sl.value().Name[0] != L'@')
						info.Provider = sl.value().Name.c_str();
					else
						info.Provider = StringHelper::GuidToString(info.ProviderKey);
				}
			}
			return info.Provider;

		case ColumnType::Name: return info.Name.c_str();
		case ColumnType::Desc: return info.Desc.c_str();
		case ColumnType::Flags: return std::format(L"0x{:X}", info.Flags).c_str();
		case ColumnType::Id: return std::to_wstring(info.CalloutId).c_str();
	}
	return CString();
}

void CCalloutsView::DoSort(SortInfo const* si) {
	auto col = GetColumnManager(m_List)->GetColumnTag<ColumnType>(si->SortColumn);
	auto asc = si->SortAscending;

	auto compare = [&](auto& c1, auto& c2) {
		switch (col) {
			case ColumnType::Key: return SortHelper::Sort(StringHelper::GuidToString(c1.CalloutKey), StringHelper::GuidToString(c2.CalloutKey), asc);
			case ColumnType::Name: return SortHelper::Sort(c1.Name, c2.Name, asc);
			case ColumnType::Desc: return SortHelper::Sort(c1.Desc, c2.Desc, asc);
			case ColumnType::Flags: return SortHelper::Sort(c1.Flags, c2.Flags, asc);
			case ColumnType::Provider: return SortHelper::Sort(c1.Provider, c2.Provider, asc);
			case ColumnType::Id: return SortHelper::Sort(c1.CalloutId, c2.CalloutId, asc);
		}
		return false;
	};
	std::ranges::sort(m_Callouts, compare);
}

int CCalloutsView::GetSaveColumnRange(HWND, int&) const {
	return 1;
}