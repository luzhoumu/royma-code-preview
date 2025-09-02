
#include "..\..\..\include\platform\vulkan\royma_vulkan_shader.h"
#include <wrl/client.h>
#undef _MSC_VER
#include <dxcapi.h>
using namespace Microsoft::WRL;

namespace royma
{
#ifdef ROYMA_DEBUG
	INLINE void WINCALL(HRESULT hr)
	{
		if (FAILED(hr))
		{
			string strDesc;
			Log::global()->record(strDesc);
			throw Exception(strDesc);
		}
	}
#else
	INLINE void WINCALL(HRESULT hr)
	{

	}
#endif //ROYMA_DEBUG

	thread_local ComPtr<IDxcCompiler> s_shaderCompiler;
	thread_local ComPtr<IDxcLibrary> s_dxcLibrary;

	struct VulkanIncludeHandler : public IDxcIncludeHandler
	{
		HRESULT STDMETHODCALLTYPE QueryInterface(
				/* [in] */ REFIID riid,
				/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
		{
			if (riid == IID_IUnknown || riid == __uuidof(IDxcIncludeHandler))
			{
				*ppvObject = this;
				AddRef();
				return S_OK;
			}
			else
			{
				*ppvObject = nullptr;
				return E_NOINTERFACE;
			}
		}

		ULONG STDMETHODCALLTYPE AddRef(void) override
		{
			InterlockedIncrement(&m_refCount);
			return m_refCount;
		}

		virtual ULONG STDMETHODCALLTYPE Release(void) override
		{
			ULONG nRefCount = InterlockedDecrement(&m_refCount);
			if (0 == m_refCount)
			{
				delete this;
			}
			return nRefCount;
		}

		virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeBlob) override
		{
			string strShaderName(pFilename);

			if (strShaderName == L"material_input" || strShaderName == L"./material_input")
			{
				strShaderName = L"";
				if (pMapMacros)
				{
					auto it = pMapMacros->find("MATERIAL_INPUT");
					if (it != pMapMacros->end())
					{
						strShaderName.fromAscii(it->second);
					}					
				}

				if(strShaderName == L"")
				{
					throw EFileNotFound(L"未設置有效的材质文件:material_input");
				}
			}

			strong<ShaderSource> spShaderSource = ResourceManager::getResource<ShaderSource>(strShaderName);
			if (spShaderSource != nullptr && spShaderSource->getShaderType() == SHADER_TYPE::INCLUDE_FILE && spShaderSource->getSourceCode().size())
			{
				ComPtr<IDxcBlobEncoding> pIncludeBlob;
				WINCALL(s_dxcLibrary->CreateBlobWithEncodingFromPinned(spShaderSource->getSourceCode().get(), castval<uint>(spShaderSource->getSourceCode().size()), CP_UTF8, &pIncludeBlob));
				*ppIncludeBlob = pIncludeBlob.Get();
				pIncludeBlob->AddRef();
				return S_OK;
			}
			else
			{
				const string strBasePath = Platform::global()->getShaderPath();
				spShaderSource = cast<ShaderSource>(ResourceManager::create(ShaderSource::staticClassId(), strShaderName));
				spShaderSource->loadSourceCode(string() << strBasePath << strShaderName);
				spShaderSource->setShaderType(SHADER_TYPE::INCLUDE_FILE);

				ComPtr<IDxcBlobEncoding> pIncludeBlob;
				WINCALL(s_dxcLibrary->CreateBlobWithEncodingFromPinned(spShaderSource->getSourceCode().get(), castval<uint>(spShaderSource->getSourceCode().size()), CP_UTF8, &pIncludeBlob));
				*ppIncludeBlob = pIncludeBlob.Get();
				pIncludeBlob->AddRef();
				return S_OK;

				/**ppData = nullptr;
				*pBytes = 0;
				return E_FAIL;*/
			}
		}

		const std::map<utf8, utf8>* pMapMacros = nullptr;

	private:

		ULONG m_refCount = 0;
	};

	VulkanShader::VulkanShader(bool isSoftwareShader) : Shader(isSoftwareShader)
	{
		SET_TAG("VlkSdr");
	}

	VulkanShader::~VulkanShader()
    {
        vkDestroyShaderModule(VulkanScreen::s_deviceInfo.device, m_shaderModule, g_pVkAllocationCallbacks);
    }

    Buffer<uchar> VulkanShader::buildVertexShader(const Buffer<uchar> bufCode, Buffer<SVertexLayout> vertexLayout, const Map<utf8, utf8>& mapMacros, const utf8& strMainFunc)
    {
		setVertexLayout(vertexLayout);
		return buildShader(SHADER_TYPE::VERTEX_SHADER, bufCode, mapMacros, strMainFunc);
    }

    Buffer<uchar> VulkanShader::buildShader(SHADER_TYPE shaderType, const Buffer<uchar> bufCode, const Map<utf8, utf8>& mapMacros, const utf8& strMainFunc)
    {
		if (!s_shaderCompiler)
		{
			WINCALL(DxcCreateInstance(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (void**)&s_shaderCompiler));
			WINCALL(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &s_dxcLibrary));
		}

		m_shaderType = shaderType;
		m_shaderEntry = strMainFunc;

		ComPtr<IDxcOperationResult> pResult;
		ComPtr<IDxcBlobEncoding> pSource;
		ComPtr<VulkanIncludeHandler> pInclude(new VulkanIncludeHandler);

		/*WINCALL(DxcCreateInstance(CLSID_DxcLibrary, __uuidof(IDxcLibrary), &s_dxcLibrary));*/
		WINCALL(s_dxcLibrary->CreateBlobWithEncodingFromPinned(bufCode.get(), castval<uint>(bufCode.size()), CP_UTF8, &pSource));

		Map<string, string> shaderMacros =
		{
			{ L"CBUFFER_INDEX_CAMERA", string(L"b") << CBUFFER_INDEX_CAMERA },
			{ L"CBUFFER_INDEX_LIGHT", string(L"b") << CBUFFER_INDEX_LIGHT },
			{ L"GBUFFER_READ_INDEX_COLOR", string(L"t") << GBUFFER_INDEX_COLOR },
			{ L"GBUFFER_READ_INDEX_SURFACE", string(L"t") << GBUFFER_INDEX_SURFACE },
			{ L"GBUFFER_READ_INDEX_EMISSION", string(L"t") << GBUFFER_INDEX_EMISSION },
			{ L"GBUFFER_READ_INDEX_NORMAL", string(L"t") << GBUFFER_INDEX_NORMAL },
			{ L"GBUFFER_READ_INDEX_DEPTH", string(L"t") << GBUFFER_INDEX_DEPTH_STENCIL },
			{ L"GBUFFER_READ_INDEX_STENCIL", string(L"t") << GBUFFER_INDEX_STENCIL },
			{ L"GBUFFER_WRITE_INDEX_COLOR", string(L"SV_TARGET") << GBUFFER_INDEX_COLOR },
			{ L"GBUFFER_WRITE_INDEX_SURFACE", string(L"SV_TARGET") << GBUFFER_INDEX_SURFACE },
			{ L"GBUFFER_WRITE_INDEX_EMISSION", string(L"SV_TARGET") << GBUFFER_INDEX_EMISSION },
			{ L"GBUFFER_WRITE_INDEX_NORMAL", string(L"SV_TARGET") << GBUFFER_INDEX_NORMAL },
		};

		for (uint i = SHADER_RESOURCE_READ_SLOT0; i < SHADER_RESOURCE_READ_SLOTS_MAX; ++i)
		{
			string strKey = L"SHADER_RESOURCE_READ_SLOT";
			string strVal = L"t";
			strKey << i - SHADER_RESOURCE_READ_SLOT0;
			strVal << i;
			shaderMacros[strKey] = strVal;
		}

		for (uint i = SHADER_RESOURCE_WRITE_SLOT0; i < SHADER_RESOURCE_WRITE_SLOTS_MAX; ++i)
		{
			string strKey = L"SHADER_RESOURCE_WRITE_SLOT";
			string strVal = L"u";
			strKey << i - SHADER_RESOURCE_READ_SLOT0;
			strVal << i;
			shaderMacros[strKey] = strVal;
		}

		for (auto& item : mapMacros)
		{
			shaderMacros[string().fromAscii(item.first)] = string().fromAscii(item.second);
		}

		auto nMacroCount = shaderMacros.count();
		Buffer<DxcDefine> bufShaderMacros(nMacroCount);
		Buffer<uchar> shaderBinary;
		for (auto [i, it] = std::make_pair(0, shaderMacros.begin()); i < nMacroCount; (++i, ++it))
		{
			bufShaderMacros[i].Name = *it->first;
			bufShaderMacros[i].Value = *it->second;
		}
#ifdef ROYMA_DEBUG
		LPCWSTR paramsDefault[] = { L"-Zi", L"-spirv", L"-fvk-use-dx-position-w", L"-fspv-target-env=vulkan1.1"};
		LPCWSTR paramsVsDsGs[] = { L"-Zi", L"-spirv", L"-fvk-use-dx-position-w", L"-fvk-invert-y" };
#else
		LPCWSTR paramsDefault[] = { L"-spirv", L"-fvk-use-dx-position-w", L"-fspv-target-env=vulkan1.1" };
		LPCWSTR paramsVsDsGs[] = { L"-spirv", L"-fvk-use-dx-position-w", L"-fvk-invert-y" };
#endif //ROYMA_DEBUG
		LPCWSTR* params = paramsDefault;
		uint nParamsCount = array_length(paramsDefault);
		LPCWSTR profile;

		switch (shaderType)
		{
		case royma::SHADER_TYPE::VERTEX_SHADER:
			profile = L"vs_6_0";
			params = paramsVsDsGs;
			nParamsCount = array_length(paramsVsDsGs);
			break;
		case royma::SHADER_TYPE::PIXEL_SHADER:
			profile = L"ps_6_0";
			break;
		case royma::SHADER_TYPE::COMPUTE_SHADER:
			profile = L"cs_6_0";
			break;
		case royma::SHADER_TYPE::HULL_SHADER:
			profile = L"hs_6_0";
			break;
		case royma::SHADER_TYPE::DOMAIN_SHADER:
			profile = L"ds_6_0";
			params = paramsVsDsGs;
			nParamsCount = array_length(paramsVsDsGs);
			break;
		case royma::SHADER_TYPE::GEOMETRY_SHADER:
			profile = L"gs_6_0";
			params = paramsVsDsGs;
			nParamsCount = array_length(paramsVsDsGs);
			break;
		default:
			profile = L"";
		}
		string strShaderEntry;
		strShaderEntry.fromAscii(strMainFunc);

		pInclude->pMapMacros = &mapMacros;
		WINCALL(s_shaderCompiler->Compile(pSource.Get(), L"", *strShaderEntry, profile, params, nParamsCount, bufShaderMacros.get(), castval<uint>(bufShaderMacros.count()), pInclude.Get(), &pResult));
		HRESULT rslt;
		pResult->GetStatus(&rslt);
		if (SUCCEEDED(rslt))
		{
			IDxcBlob* pShaderBlob;
			WINCALL(pResult->GetResult(&pShaderBlob));
			shaderBinary.realloc(pShaderBlob->GetBufferSize());
			memcpy_s(shaderBinary.get(), shaderBinary.size(), pShaderBlob->GetBufferPointer(), pShaderBlob->GetBufferSize());
			pShaderBlob->Release();
		}

		IDxcBlobEncoding* pErrorBlob;
		pResult->GetErrorBuffer(&pErrorBlob);
		auto pErrorDesc = reinterpret_cast<char*>(pErrorBlob->GetBufferPointer());
		if (pErrorDesc)
		{
			Log::global()->record(string().fromUtf8(pErrorDesc));
		}
		WINCALL(rslt);
		pErrorBlob->Release();
		
		VkShaderModuleCreateInfo shaderInfo;
		memset(&shaderInfo, 0, sizeof(VkShaderModuleCreateInfo));
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.pCode = (uint*)shaderBinary.get();
		shaderInfo.codeSize = castval<size_t>(shaderBinary.size());

		if (m_shaderModule != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(VulkanScreen::s_deviceInfo.device, m_shaderModule, g_pVkAllocationCallbacks);
		}
		VKCALL(vkCreateShaderModule(VulkanScreen::s_deviceInfo.device, &shaderInfo, g_pVkAllocationCallbacks, &m_shaderModule));

#ifdef ROYMA_PROFILE
		utf8 strPredefines = (string() << m_profileName << mapMacros.toString()).toUtf8();
		
		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
		nameInfo.objectHandle = (ulong)m_shaderModule;
		nameInfo.pObjectName = strPredefines;
		auto setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(cast<VulkanScreen>(Screen::global())->getInstance(), "vkSetDebugUtilsObjectNameEXT");
		setDebugUtilsObjectNameEXT(VulkanScreen::s_deviceInfo.device, &nameInfo);
#endif // ROYMA_PROFILE
		
		return shaderBinary;
    }
}