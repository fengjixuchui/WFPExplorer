#include "pch.h"
#include "FilterGeneralPage.h"
#include "StringHelper.h"
#include <WFPEngine.h>
#include "WFPHelper.h"

CFilterGeneralPage::CFilterGeneralPage(WFPEngine& engine, WFPFilterInfo const& filter) : m_Engine(engine), m_Filter(filter) {
}

LRESULT CFilterGeneralPage::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
    SetDlgItemText(IDC_KEY, StringHelper::GuidToString(m_Filter.FilterKey));
    SetDlgItemText(IDC_ID, std::to_wstring(m_Filter.FilterId).c_str());
    SetDlgItemText(IDC_PROVIDER, WFPHelper::GetProviderName(m_Engine, m_Filter.ProviderKey));
    SetDlgItemText(IDC_NAME, m_Filter.Name.c_str());
    SetDlgItemText(IDC_DESC, m_Filter.Desc.c_str());
    SetDlgItemText(IDC_LAYER, WFPHelper::GetLayerName(m_Engine, m_Filter.LayerKey));
    SetDlgItemText(IDC_SUBLAYER, WFPHelper::GetSublayerName(m_Engine, m_Filter.SubLayerKey));
    SetDlgItemText(IDC_ACTIONTYPE, StringHelper::WFPFilterActionTypeToString(m_Filter.Action.Type));
    if (m_Filter.Action.FilterType != GUID_NULL) {
        SetDlgItemText(IDC_CALLOUT_OR_FILTER, StringHelper::GuidToString(m_Filter.Action.FilterType));
    }
    SetDlgItemText(IDC_WEIGHT, StringHelper::WFPValueToString(m_Filter.Weight, true));
    SetDlgItemText(IDC_EFFECTIVEWEIGHT, StringHelper::WFPValueToString(m_Filter.EffectiveWeight, true));

    auto flags = StringHelper::WFPFilterFlagsToString(m_Filter.Flags);
    if (!flags.IsEmpty())
        flags = std::format(L"0x{:X} ({})", (UINT32)m_Filter.Flags, (PCWSTR)flags).c_str();
    else
        flags = L"(None)";
    SetDlgItemText(IDC_FLAGS, flags);

    AddIconToButton(IDC_PROVIDER_PROP, IDI_PROVIDER);
    AddIconToButton(IDC_LAYER_PROP, IDI_LAYERS);
    AddIconToButton(IDC_SUBLAYER_PROP, IDI_SUBLAYER);

    return 0;
}

LRESULT CFilterGeneralPage::OnShowLayer(WORD, WORD, HWND, BOOL&) {
    auto layer = m_Engine.GetLayerByKey(m_Filter.LayerKey);
    ATLASSERT(layer);
    if (!layer)
        AtlMessageBox(m_hWnd, L"Layer not found", IDS_TITLE, MB_ICONERROR);
    else
        WFPHelper::ShowLayerProperties(m_Engine, *layer);

    return 0;
}

LRESULT CFilterGeneralPage::OnShowProvider(WORD, WORD, HWND, BOOL&) {
    auto provider = m_Engine.GetProviderByKey(m_Filter.ProviderKey);
    if (!provider)
        AtlMessageBox(m_hWnd, L"Provider not found", IDS_TITLE, MB_ICONERROR);
    else
        WFPHelper::ShowProviderProperties(m_Engine, *provider);
    return 0;
}

LRESULT CFilterGeneralPage::OnShowSublayer(WORD, WORD, HWND, BOOL&) {
    return LRESULT();
}