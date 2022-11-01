#include "pch.h"
#include "WFPEngine.h"
#include <unordered_map>

bool WFPEngine::Open(DWORD auth) {
	if (m_hEngine)
		return true;
	m_LastError = FwpmEngineOpen(nullptr, auth, nullptr, nullptr, &m_hEngine);
	return m_LastError == ERROR_SUCCESS;
}

WFPEngine::~WFPEngine() {
	Close();
}

void WFPEngine::Close() {
	if (m_hEngine) {
		m_LastError = FwpmEngineClose(m_hEngine);
		m_hEngine = nullptr;
	}
}

DWORD WFPEngine::LastError() const {
	return m_LastError;
}

std::vector<WFPLayerInfo> WFPEngine::EnumLayers(bool includeFields) const {
	HANDLE hEnum;
	std::vector<WFPLayerInfo> info;
	m_LastError = FwpmLayerCreateEnumHandle(m_hEngine, nullptr, &hEnum);
	if (m_LastError)
		return info;
	FWPM_LAYER** layers;
	UINT32 count;
	m_LastError = FwpmLayerEnum(m_hEngine, hEnum, 512, &layers, &count);
	if (m_LastError == ERROR_SUCCESS) {
		info.reserve(count);
		for (UINT32 i = 0; i < count; i++) {
			auto layer = layers[i];
			auto li = InitLayer(layer, includeFields);
			info.emplace_back(std::move(li));
		}
	}
	return info;
}

std::vector<WFPSubLayerInfo> WFPEngine::EnumSubLayers() const {
	HANDLE hEnum;
	std::vector<WFPSubLayerInfo> info;
	m_LastError = FwpmSubLayerCreateEnumHandle(m_hEngine, nullptr, &hEnum);
	if (m_LastError)
		return info;
	FWPM_SUBLAYER** layers;
	UINT32 count;
	m_LastError = FwpmSubLayerEnum(m_hEngine, hEnum, 256, &layers, &count);
	if (m_LastError == ERROR_SUCCESS) {
		info.reserve(count);
		for (UINT32 i = 0; i < count; i++) {
			auto layer = layers[i];
			WFPSubLayerInfo li;
			li.Name = ParseMUIString(layer->displayData.name);
			li.Desc = ParseMUIString(layer->displayData.description);
			li.SubLayerKey = layer->subLayerKey;
			li.Flags = layer->flags;
			li.Weight = layer->weight;
			li.ProviderKey = layer->providerKey ? *layer->providerKey : GUID_NULL;
			li.ProviderData.resize(layer->providerData.size);
			memcpy(li.ProviderData.data(), layer->providerData.data, layer->providerData.size);
			info.emplace_back(std::move(li));
		}
	}
	return info;
}

std::vector<WFPProviderInfo> WFPEngine::EnumProviders(bool includeData) const {
	HANDLE hEnum;
	std::vector<WFPProviderInfo> info;
	m_LastError = FwpmProviderCreateEnumHandle(m_hEngine, nullptr, &hEnum);
	if (m_LastError)
		return info;
	FWPM_PROVIDER** providers;
	UINT32 count;
	m_LastError = FwpmProviderEnum(m_hEngine, hEnum, 128, &providers, &count);
	if (m_LastError == ERROR_SUCCESS) {
		info.reserve(count);
		for (UINT32 i = 0; i < count; i++) {
			auto p = providers[i];
			info.emplace_back(std::move(InitProvider(p, includeData)));
		}
		FwpmFreeMemory((void**)&providers);
		m_LastError = FwpmProviderDestroyEnumHandle(m_hEngine, hEnum);
	}

	return info;
}

std::vector<WFPCalloutInfo> WFPEngine::EnumCallouts() const {
	HANDLE hEnum;
	std::vector<WFPCalloutInfo> info;
	m_LastError = FwpmCalloutCreateEnumHandle(m_hEngine, nullptr, &hEnum);
	if (m_LastError)
		return info;
	FWPM_CALLOUT** callouts;
	UINT32 count;
	m_LastError = FwpmCalloutEnum(m_hEngine, hEnum, 256, &callouts, &count);
	if (m_LastError == ERROR_SUCCESS) {
		info.reserve(count);
		for (UINT32 i = 0; i < count; i++) {
			auto c = callouts[i];
			WFPCalloutInfo ci;
			ci.Name = ParseMUIString(c->displayData.name);
			ci.Desc = ParseMUIString(c->displayData.description);
			ci.ProviderKey = c->providerKey ? *c->providerKey : GUID_NULL;
			ci.Flags = c->flags;
			ci.CalloutKey = c->calloutKey;
			ci.ProviderData.resize(c->providerData.size);
			memcpy(ci.ProviderData.data(), c->providerData.data, c->providerData.size);
			ci.CalloutId = c->calloutId;
			ci.ApplicableLayer = c->applicableLayer;
			info.emplace_back(std::move(ci));
		}
		FwpmFreeMemory((void**)&callouts);
		m_LastError = FwpmCalloutDestroyEnumHandle(m_hEngine, hEnum);
	}

	return info;
}

WFPProviderInfo WFPEngine::GetProviderByKey(GUID const& guid) const {
	FWPM_PROVIDER* provider;
	m_LastError = FwpmProviderGetByKey(m_hEngine, &guid, &provider);
	if (ERROR_SUCCESS != m_LastError)
		return {};
	auto p = InitProvider(provider);
	FwpmFreeMemory((void**)&provider);
	return p;
}

WFPFilterInfo WFPEngine::GetFilterByKey(GUID const& key) const {
	FWPM_FILTER* filter;
	m_LastError = FwpmFilterGetByKey(m_hEngine, &key, &filter);
	if (m_LastError != ERROR_SUCCESS)
		return {};

	return InitFilter(filter);
}

WFPFilterInfo WFPEngine::GetFilterById(UINT64 id) const {
	FWPM_FILTER* filter;
	m_LastError = FwpmFilterGetById(m_hEngine, id, &filter);
	if (m_LastError != ERROR_SUCCESS)
		return {};

	return InitFilter(filter, true);
}

WFPLayerInfo WFPEngine::GetLayerByKey(GUID const& key) const {
	FWPM_LAYER* layer;
	m_LastError = FwpmLayerGetByKey(m_hEngine, &key, &layer);
	if (m_LastError != ERROR_SUCCESS)
		return {};

	return InitLayer(layer, true);
}

WFPProviderInfo WFPEngine::InitProvider(FWPM_PROVIDER* p, bool includeData) {
	WFPProviderInfo pi;
	pi.Name = ParseMUIString(p->displayData.name);
	pi.Desc = ParseMUIString(p->displayData.description);
	pi.ProviderKey = p->providerKey;
	pi.Flags = p->flags;
	pi.ProviderDataSize = p->providerData.size;
	if (includeData) {
		pi.ProviderData = std::make_unique<BYTE[]>(p->providerData.size);
		memcpy(pi.ProviderData.get(), p->providerData.data, p->providerData.size);
	}
	return pi;
}

std::wstring WFPEngine::PoorParseMUIString(std::wstring const& path) {
	static std::unordered_map<std::wstring, std::wstring> cache;

	if (path[0] != L'@')
		return path;

	if (auto it = cache.find(path); it != cache.end())
		return it->second;

	auto comma = path.find(L',', 1);
	if (comma == std::wstring::npos)
		return path;

	auto dllname = path.substr(1, comma - 1);
	auto hDll = ::LoadLibraryEx(dllname.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	if (!hDll)
		return path;

	CString result;
	auto id = _wtoi(path.substr(comma + 1).c_str());
	if (id < 0)
		id = -id;
	result.LoadStringW(hDll, id);
	::FreeLibrary(hDll);
	if (!result.IsEmpty()) {
		cache.insert({ path, (PCWSTR)result });
		return (PCWSTR)result;
	}
	if (dllname.substr(dllname.rfind(L'.')) != L".mui")
		return PoorParseMUIString(L"@c:\\Windows\\System32\\en-US\\" + dllname + L".mui" + path.substr(comma));
	return path;
}

std::wstring WFPEngine::ParseMUIString(PCWSTR input) {
	if (input == nullptr)
		return L"";

	if (*input && input[0] == L'@') {
		WCHAR result[256];
		if (::SHLoadIndirectString(input, result, _countof(result), nullptr) == S_OK)
			return result;
	}
	return input;
}

WFPValue WFPValueInit(FWP_VALUE const& value) {
	WFPValue result;
	result.Type = (WFPDataType)value.type;

	switch (result.Type) {
		case WFPDataType::INT64: result.int64 = *value.int64; break;
		case WFPDataType::UINT64: result.uint64 = *value.uint64; break;
		case WFPDataType::DOUBLE: result.double64 = *value.double64; break;
		default:
			memcpy(&result, &value, sizeof(value));
	}
	return result;
}

WFPProviderInfo::operator bool() const {
	return ProviderKey != GUID_NULL;
}