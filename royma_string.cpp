
#include <utils/royma_string.h>
#include <reflect/royma_value.h>
#include <reflect/royma_utf8.h>
#include <royma_exception.h>
#include <utils/royma_list.h>
#include "reflect/royma_resource_manager.h"
#include "utils/royma_system_buffer.h"

namespace royma
{
	const char16_t string::s_szNull[1] = { 0 };
	const float string::s_exponentTable[20] = { 1e9f, 1e8f, 1e7f, 1e6f, 1e5f, 1e4f, 1e3f, 1e2f, 1e1f, 1e0f, 0.0f, 1e-1f, 1e-2f, 1e-3f, 1e-4f, 1e-5f, 1e-6f, 1e-7f, 1e-8f, 1e-9f };
	const sint string::s_integerTable[11] = { castval<sint>(1e9), castval<sint>(1e8), castval<sint>(1e7),
		castval<sint>(1e6), castval<sint>(1e5), castval<sint>(1e4), 1000, 100, 10, 1, 0 };

	int compare(const string& str1, const string& str2)
	{
		if (str1.length() < str2.length())
		{
			return -1;
		}
		else if (str1.length() > str2.length())
		{
			return 1;
		}
		else
		{
			return memcmp(str1.m_szData, str2.m_szData, str1.length() * sizeof(wchar));
		}
	}

	bool operator==(const string& str1, const string& str2)
	{
		return compare(str1, str2) == 0;
	}

	bool operator!=(const string& str1, const string& str2)
	{
		return !(str1 == str2);
	}

	bool operator<(const string& str1, const string& str2)
	{
		return compare(str1, str2) < 0;
	}

	bool operator>(const string& str1, const string& str2)
	{
		return compare(str1, str2) > 0;
	}

	string& operator<<(string& strDst, const string& strSrc)
	{
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = strSrc.m_nLength;

		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), strSrc.m_szData, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;

		strDst.m_bContentChanged = true;
		return strDst;
	}

	string& operator<<(string& strDst, const utf8& strSrc)
	{
		string strUtf8;
		strUtf8.fromUtf8(strSrc);
		operator<<(strDst, strUtf8);
		return strDst;
	}

	string& operator<<(string& strDst, wchar ch)
	{
		strDst.extend(strDst.m_nLength + 2);
		strDst.m_szData[strDst.m_nLength++] = ch;
		strDst.m_szData[strDst.m_nLength] = 0;
		
		strDst.m_bContentChanged = true;
		return strDst;
	}

	string& operator<<(string& strDst, const wchar* szSrc)
	{
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = wcslen(szSrc);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), (const char16_t*)szSrc, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;
		strDst.m_bContentChanged = true;
#else
		strDst << string().fromUtf32((const char32_t*)szSrc);
#endif //ROYMA_SYSTEM_PLATFORM

		return strDst;
	}

	string& operator<<(string& strDst, sint nValue)
	{
		wchar szSrc[32];
		swprintf(szSrc, 32, L"%d", nValue);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = wcslen(szSrc);

		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), (const char16_t*)szSrc, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;
		strDst.m_bContentChanged = true;
#else
		strDst << string().fromUtf32((const char32_t*)szSrc);
#endif //ROYMA_SYSTEM_PLATFORM

		return strDst;
	}

	string& operator<<(string& strDst, slong lValue)
	{
		wchar szSrc[32];
		swprintf(szSrc, 32, L"%lld", lValue);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = wcslen(szSrc);

		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), (const char16_t*)szSrc, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;
		strDst.m_bContentChanged = true;
#else
		strDst << string().fromUtf32((const char32_t*)szSrc);
#endif //ROYMA_SYSTEM_PLATFORM
		return strDst;
	}

	string& operator<<(string& strDst, uint nValue)
	{
		wchar szSrc[32];
		swprintf(szSrc, 32, L"%d", nValue);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = wcslen(szSrc);

		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), (const char16_t*)szSrc, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;
		strDst.m_bContentChanged = true;
#else
		strDst << string().fromUtf32((const char32_t*)szSrc);
#endif //ROYMA_SYSTEM_PLATFORM

		return strDst;
	}

	string& operator<<(string& strDst, ulong lValue)
	{
		wchar szSrc[32];
		swprintf(szSrc, 32, L"%lld", lValue);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = wcslen(szSrc);

		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), (const char16_t*)szSrc, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;
		strDst.m_bContentChanged = true;
#else
		strDst << string().fromUtf32((const char32_t*)szSrc);
#endif //ROYMA_SYSTEM_PLATFORM

		return strDst;
	}

	string& operator<<(string& strDst, float fValue)
	{
		wchar szSrc[32];
		swprintf(szSrc, 32, L"%lf", fValue);

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		slong nLen1 = strDst.m_nLength;
		slong nLen2 = wcslen(szSrc);

		strDst.extend(nLen1 + nLen2 + 1);
		copyString(strDst.m_szData + nLen1, strDst.capacity(), (const char16_t*)szSrc, nLen2 + 1);
		strDst.m_nLength = nLen1 + nLen2;
		strDst.m_bContentChanged = true;
#else
		strDst << string().fromUtf32((const char32_t*)szSrc);
#endif //ROYMA_SYSTEM_PLATFORM

		return strDst;
	}

	string& operator<<(string& strDst, double dValue)
	{
		char szSrc[32];
		_gcvt_s(szSrc, 32, dValue, 17);
		strDst << (string().fromAscii(szSrc));

		return strDst;
	}

	string& operator<<(string& strDst, const float2& val)
	{
		return strDst << val.x << L"," << val.y;
	}

	string& operator<<(string& strDst, const float3& val)
	{
		return strDst << val.x << L"," << val.y << L"," << val.z;
	}

	string& operator<<(string& strDst, const float4& val)
	{
		return strDst << val.x << L"," << val.y << L"," << val.z << L"," << val.w;
	}

	string& operator<<(string& strDst, const matrix& val)
	{
		for (int i = 0; i < 4; i++)
		{
			strDst << L"[" << val.r[i].x << L"," << val.r[i].y << L"," << val.r[i].z << L"," << val.r[i].w << L"]\n";
		}

		return strDst;
	}

	string& operator<<(string& strDst, const LinearColor& val)
	{
		return strDst << val.r << L"," << val.g << L"," << val.b << L"," << val.a;
	}

	string& operator<<(string& strDst, const Line& val)
	{
		return strDst << val.Start.x << L"," << val.Start.y << L"," << val.Start.z << L"," << val.Start.w << L"-" << val.Direction.x << L"," << val.Direction.y << L"," << val.Direction.z << L"," << val.Direction.w;
	}

	string& operator<<(string& strDst, const Plane& val)
	{
		return strDst << val.x << L"," << val.y << L"," << val.z << L"," << val.w;
	}

	string& operator<<(string& strDst, const Value& val)
	{
		return strDst << val.toString();
	}

	string& operator<<(string& strDst, hex32 nValue)
	{
		string strVal;
		strVal = nValue;
		return strDst << strVal;
	}

	string& operator<<(string& strDst, hex64 lValue)
	{
		string strVal;
		strVal = lValue;
		return strDst << strVal;
	}

	string& operator<<(string& strDst, num32 nValue)
	{
		string strVal;
		strVal = nValue;
		return strDst << strVal;
	}

	string& operator<<(string& strDst, num64 lValue)
	{
		string strVal;
		strVal = lValue;
		return strDst << strVal;
	}

	///////////////////////////////////////////////////////////

	string& operator<<(string&& strDst, const string& strSrc)
	{
		strDst << strSrc;
		return strDst;
	}

	string& operator<<(string&& strDst, const utf8& strSrc)
	{
		strDst << strSrc;
		return strDst;
	}

	string& operator<<(string&& strDst, wchar ch)
	{
		strDst << ch;
		return strDst;
	}

	string& operator<<(string&& strDst, const wchar* szSrc)
	{
		strDst << szSrc;
		return strDst;
	}

	string& operator<<(string&& strDst, sint nVal)
	{
		strDst << nVal;
		return strDst;
	}

	string& operator<<(string&& strDst, slong lVal)
	{
		strDst << lVal;
		return strDst;
	}

	string& operator<<(string&& strDst, uint nVal)
	{
		strDst << nVal;
		return strDst;
	}

	string& operator<<(string&& strDst, ulong lVal)
	{
		strDst << lVal;
		return strDst;
	}

	string& operator<<(string&& strDst, float fVal)
	{
		strDst << fVal;
		return strDst;
	}

	string& operator<<(string&& strDst, double fVal)
	{
		strDst << fVal;
		return strDst;
	}

	string& operator<<(string&& strDst, const float2& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const float3& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const float4& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const matrix& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const LinearColor& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const Line& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const Plane& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, const Value& val)
	{
		strDst << val;
		return strDst;
	}

	string& operator<<(string&& strDst, hex32 nValue)
	{
		strDst << nValue;
		return strDst;
	}

	string& operator<<(string&& strDst, hex64 lValue)
	{
		strDst << lValue;
		return strDst;
	}

	string& operator<<(string&& strDst, num32 nValue)
	{
		strDst << nValue;
		return strDst;
	}

	string& operator<<(string&& strDst, num64 lValue)
	{
		strDst << lValue;
		return strDst;
	}

	std::ostream& operator<<(std::ostream &os, const string& str)
	{
		os << str.toUtf8();
		return os;
	}

	std::istream& operator>>(std::istream& is, string& str)
	{
		is.seekg(0, std::ios::end);
		size_t nDataLen = is.tellg();
		utf8 szBuffer(nDataLen + 1);

		is.seekg(0);
		is.read(szBuffer.get(), nDataLen);
		szBuffer[nDataLen] = 0;
		str.fromUtf8(szBuffer.get());

		return is;
	}

	void string::init()
	{
		m_szData = const_cast<char16_t*>(s_szNull);
		m_nCapacity = 1;
		m_nLength = 0;
	}

	string::string()
	{
		init();
	}

	string::string(const string& str)
	{
		init();

		if (str.length() > 0)
		{
			realloc(str.capacity());
			copyString(m_szData, capacity(), str.m_szData, str.length() + 1);
			m_nLength = str.length();
		}
	}

	string::string(string&& str)
	{
		if (&str != this)
		{
			m_szData = str.m_szData;
			m_nCapacity = str.m_nCapacity;
			m_nLength = str.m_nLength;

			str.m_szData = const_cast<char16_t*>(s_szNull);
			str.m_nLength = 0;
			str.m_nCapacity = 1;
		}
	}

	string::string(const wchar* szData)
	{
		init();

		slong nStrLen = wcslen(szData);
		if (nStrLen > 0)
		{
#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
			realloc(nStrLen + 1);
			copyString(m_szData, capacity(), (const char16_t*)szData, nStrLen + 1);
			m_nLength = nStrLen;
#else
			fromUtf32((const char32_t*)szData);
#endif //ROYMA_SYSTEM_PLATFORM
		}
	}

	string::~string()
	{
		release();
	}

	ClassId string::getClassId() const
	{
		return staticClassId();
	}

	void string::serialize(Package& pack) const
	{
		Object::serialize(pack);

		utf8 strUtf8 = toUtf8();
		slong nDataLen = strUtf8.length() + 1;

		pack << nDataLen;
		slong nLastPos = pack.tell();
		memcpy_s(pack.data() + nLastPos, nDataLen, strUtf8, nDataLen);
		pack.seek(nLastPos + nDataLen);
	}

	bool string::deserialize(Package& pack, slong& nStartPosInPack)
	{
		if (!Object::deserialize(pack, nStartPosInPack))
		{
			return false;
		}

		slong nDataLen;
		pack >> nDataLen;
		utf8 strBuf(nDataLen);

		slong nLastPos = pack.tell();
		memcpy_s(strBuf.get(), nDataLen, pack.data() + nLastPos, nDataLen);
		fromUtf8(strBuf.get());
		pack.seek(nLastPos + nDataLen);

		return true;
	}

	slong string::serializedDataLength() const
	{
		return titleLength() + sizeof(slong) + toUtf8().length() + 1;
	}

	void string::release()
	{
		if (m_szData != s_szNull)
		{
			MemoryPool::local()->release(m_szData, m_nCapacity * sizeof(char16_t));
		}

		init();
		m_bContentChanged = true;
	}

	void copyString(char16_t* szDst, slong nCapacity, const char16_t* szSrc, slong nLength)
	{
		if (szDst != string::s_szNull)
		{
			int hr = memcpy_s(szDst, nCapacity * sizeof(wchar), szSrc, nLength * sizeof(wchar));
			assert(!hr);
		}
	}

	void string::realloc(slong nCapacity)
	{
		nCapacity = fitCapacity(nCapacity);

		if (nCapacity > capacity())
		{
			release();

			m_szData = (char16_t*)MemoryPool::local()->allocate(nCapacity * sizeof(char16_t));
			if (m_szData)
			{
				m_szData[0] = 0;
				m_nLength = 0;
				m_nCapacity = nCapacity;
			}
			else
			{
				throw(EOutOfMemory());
			}
		}
	}

	slong string::capacity() const
	{
		return m_nCapacity;
	}

	string string::alloc(slong nCapacity)
	{
		string str;
		str.realloc(nCapacity);

		return str;
	}

	ClassId string::staticClassId()
	{
		return ClassId("string");
	}

	string& string::fromAscii(const char* szAscii)
	{
		slong nStrLen = strlen(szAscii);
		if (nStrLen == 0)
		{
			operator=(L"");
			return *this;
		}

		realloc(nStrLen + 1);
		m_nLength = nStrLen;

#if ROYMA_HARDWARE_PLATFORM == ROYMA_HARDWARE_PLATFORM_AVX2
		slong nSegments = m_nCapacity / STRING_SEGMENT_CAPACITY;
		for (slong i = 0; i < nSegments; ++i)
		{
			__m128i seg = _mm_loadu_si128(reinterpret_cast<const __m128i*>(szAscii) + i);
			__m256i dst = _mm256_cvtepu8_epi16(seg);
			_mm256_store_si256(reinterpret_cast<__m256i*>(m_szData) + i, dst);
		}
#else
		for (slong i = 0; i < nStrLen + 1; i++)
		{
			m_szData[i] = szAscii[i];
		}
#endif //ROYMA_HARDWARE_PLATFORM

		m_bContentChanged = true;
		return *this;
	}

	string& string::fromUtf8(const char* szUtf8)
	{
		slong nLength = 0;
		for (slong i = 0; szUtf8[i]; nLength++)
		{
			unsigned char ch = szUtf8[i];
			if (ch < 128)
			{
				i++;
			}
			else if (ch < 0xe0)
			{
				i += 2;
			}
			else
			{
				i += 3;
			}
		}

		if (nLength > 0)
		{
			realloc(nLength + 1);
			m_nLength = nLength;

			slong nCurrentIndex = 0;
			for (slong i = 0; i < nLength; i++)
			{
				unsigned char ch0 = szUtf8[nCurrentIndex];
				if (ch0 < 128)
				{
					m_szData[i] = ch0;
				}
				else if (ch0 < 0xe0)
				{
					unsigned char ch1 = szUtf8[++nCurrentIndex];
					m_szData[i] = ((ch0 ^ 0xc0) << 6) | (ch1 ^ 0x80);
				}
				else
				{
					unsigned char ch1 = szUtf8[++nCurrentIndex];
					unsigned char ch2 = szUtf8[++nCurrentIndex];
					m_szData[i] = ((ch0 ^ 0xe0) << 12) | ((ch1 ^ 0x80) << 6) | (ch2 ^ 0x80);
				}

				nCurrentIndex++;
			}

			m_szData[nLength] = 0;
		}
		else
		{
			operator=(L"");
		}

		m_bContentChanged = true;
		return *this;
	}

	void string::extend(slong nCapacity)
	{
		nCapacity = fitCapacity(nCapacity);

		if (nCapacity > capacity())
		{
			slong nStrLen = length();
			char16_t* szBackup = m_szData;
			slong nBackupSize = m_nCapacity * sizeof(char16_t);
			m_szData = const_cast<char16_t*>(s_szNull);

			realloc(nCapacity);
			copyString(m_szData, m_nCapacity, szBackup, nStrLen + 1);
			m_nLength = nStrLen;
			m_bContentChanged = true;

			if (szBackup != s_szNull)
			{
				MemoryPool::local()->release(szBackup, nBackupSize);
			}
		}
	}

	/*slong wstrlen(const wchar* szData)
	{
		slong nLen = 0;

#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS) && ROYMA_HARDWARE_PLATFORM == ROYMA_HARDWARE_PLATFORM_AVX2
		for (;; (szData += 16, nLen += 16))
		{
			__m256i vNumber = _mm256_loadu_si256((__m256i*)szData);
			__m256i vCmpMask = _mm256_cmpeq_epi16(vNumber, _mm256_setzero_si256());
			int nCmpMask = _mm256_movemask_epi8(vCmpMask);

			if (nCmpMask)
			{
				unsigned long nBytes;
				_BitScanForward(&nBytes, nCmpMask);
				nLen += nBytes / 2;

				break;
			}
		}
#else
		while (szData[nLen])
		{
			++nLen;
		}
#endif

		return nLen;
	}*/
	
	utf8 string::toAscii() const
	{
		utf8 buffer(m_nLength + 1);

#if ROYMA_HARDWARE_PLATFORM == ROYMA_HARDWARE_PLATFORM_AVX2
		slong nSegments = fitCapacity(m_nLength + 1) / STRING_SEGMENT_CAPACITY;
		for (slong i = 0; i < nSegments; ++i)
		{
			__m256i seg = _mm256_load_si256(reinterpret_cast<__m256i*>(m_szData) + i);
			__m256i seghi = _mm256_castsi128_si256(_mm256_extractf128_si256(seg, 1));
			__m128i dst = _mm256_castsi256_si128(_mm256_packus_epi16(seg, seghi));
			_mm_store_si128(reinterpret_cast<__m128i*>(buffer.m_szData) + i, dst);
		}
#else
		for (slong i = 0; i < m_nLength + 1; ++i)
		{
			buffer[i] = (char)m_szData[i];
		}
#endif //ROYMA_HARDWARE_PLATFORM

		buffer.resetLength(m_nLength);
		buffer.m_bContentChanged = true;
		return buffer;
	}

	INLINE slong charLength(wchar ch)
	{
		if (ch < 1 << 7)
		{
			return 1;
		}
		else if (ch < 1 << 11)
		{
			return 2;
		}
		else if (ch < 1 << 16)
		{
			return 3;
		}
		else
		{
			throw EInvalidData(L"不支持非基本平面字符!");
		}
	}

	utf8 string::toUtf8() const
	{
		slong nUtf8Length = 0;
		for (slong i = 0; m_szData[i]; i++)
		{
			nUtf8Length += charLength(m_szData[i]);
		}

		utf8 strDest(nUtf8Length + 1);

		slong nCurrentIndex = 0;
		for (slong i = 0; m_szData[i]; i++)
		{
			wchar ch = m_szData[i];

			if (ch < 1 << 7)
			{
				strDest.m_szData[nCurrentIndex++] = (char)ch;
			}
			else if (ch < 1 << 11)
			{
				strDest.m_szData[nCurrentIndex++] = (ch & 0x7c0) >> 6 | 0xc0;
				strDest.m_szData[nCurrentIndex++] = ch & 0x3f | 0x80;
			}
			else if (ch < 1 << 16)
			{
				strDest.m_szData[nCurrentIndex++] = (ch & 0xf000) >> 12 | 0xe0;
				strDest.m_szData[nCurrentIndex++] = (ch & 0xfc0) >> 6 | 0x80;
				strDest.m_szData[nCurrentIndex++] = ch & 0x3f | 0x80;
			}
			else
			{
				throw EInvalidData(L"不支持非基本平面字符!");
			}
		}

		strDest.m_szData[nUtf8Length] = 0;
		strDest.m_nLength = nUtf8Length;

		strDest.m_bContentChanged = true;
		return strDest;
	}

	bool string::boolValue() const
	{
		if (lowerCase() == L"true")
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	sint string::intValue() const
	{
		if (!isIntNumber())
		{
			throw(ENan(*this));
		}

		if (m_nLength > 0)
		{
			return strtol(toAscii(), nullptr, 10);
		}
		else
		{
			return 0;
		}
	}

	slong string::longValue() const
	{
		if (!isIntNumber())
		{
			throw(ENan(*this));
		}

		if (m_nLength > 0)
		{
			return strtoll(toAscii(), nullptr, 10);
		}
		else
		{
			return 0;
		}
	}

	ulong string::hexValue() const
	{
		if (!isHexNumber())
		{
			throw(ENan(*this));
		}

		if (m_nLength > 0)
		{
			return strtoull(toAscii(), nullptr, 16);
		}
		else
		{
			return 0;
		}
	}

	float string::floatValue() const
	{
		if (!isRealNumber())
		{
			throw(ENan(*this));
		}

		if (this->length() < 1)
		{
			return 0.0f;
		}

		return strtof(toAscii(), nullptr);
	}

	double string::doubleValue() const
	{
		if (!isRealNumber())
		{
			throw(ENan(*this));
		}

		if (this->length() < 1)
		{
			return 0.0;
		}

		return strtod(toAscii(), nullptr);
	}

	sint string::intValue(slong& nStartIndex, wchar rest) const
	{
		if (nStartIndex == m_nLength)
		{
			return 0;
		}

		//寻找小数点
		slong nBaseIndex = m_nLength;
		for (slong i = nStartIndex; i < m_nLength; i++)
		{
			if (m_szData[i] == 0 || m_szData[i] == rest)
			{
				nBaseIndex = i;
				break;
			}
		}

		slong nFirstExponentIndex = 10 - (nBaseIndex - nStartIndex);
		int nResult = 0;

		if (m_szData[nStartIndex] != '-')
		{
			slong i = nStartIndex;
			for (; i < m_nLength && m_szData[i] != rest; i++)
			{
				nResult += (m_szData[i] - L'0') * s_integerTable[nFirstExponentIndex + (i - nStartIndex)];
			}
			nStartIndex = i + 1;
		}
		else
		{
			slong i = nStartIndex + 1;
			nFirstExponentIndex++;
			for (; i < m_nLength && m_szData[i] != rest; i++)
			{
				nResult += (m_szData[i] - L'0') * s_integerTable[nFirstExponentIndex + i - (nStartIndex + 1)];
			}
			nStartIndex = i + 1;
			nResult = -nResult;
		}

		return nResult;
	}

	float string::floatValue(slong& nStartIndex, wchar rest) const
	{
		if (nStartIndex == m_nLength)
		{
			return 0.0f;
		}

		//寻找小数点
		slong nBaseIndex = m_nLength;
		for (slong i = nStartIndex; i < m_nLength; i++)
		{
			if (m_szData[i] == L'.' || m_szData[i] == rest)
			{
				nBaseIndex = i;
				break;
			}
		}

		slong nFirstExponentIndex = 10 - (nBaseIndex - nStartIndex);
		float fResult = 0.0f;
		bool bScientificNotation = false;

		if (m_szData[nStartIndex] != '-')
		{
			slong i = nStartIndex;
			for (; i < m_nLength && m_szData[i] != rest; i++)
			{
				if (m_szData[i] == L'e')
				{
					bScientificNotation = true;
					break;
				}

				fResult += (m_szData[i] - L'0') * s_exponentTable[nFirstExponentIndex + (i - nStartIndex)];
			}
			nStartIndex = i + 1;
		}
		else
		{
			slong i = nStartIndex + 1;
			nFirstExponentIndex++;
			for (; i < m_nLength && m_szData[i] != rest; i++)
			{
				if (m_szData[i] == L'e')
				{
					bScientificNotation = true;
					break;
				}

				fResult += (m_szData[i] - L'0') * s_exponentTable[nFirstExponentIndex + i - (nStartIndex + 1)];
			}
			nStartIndex = i + 1;
			fResult = -fResult;
		}

		//科学计数
		if (bScientificNotation)
		{
			if (m_szData[nStartIndex] != L'-')
			{
				fResult *= powf(10.0f, float(m_szData[nStartIndex] - L'0'));
				nStartIndex += 2;
			}
			else
			{
				fResult *= powf(10.0f, float(L'0' - m_szData[nStartIndex + 1]));
				nStartIndex += 3;
			}
		}

		return fResult;
	}

	string& string::operator=(const wchar* szData)
	{
		slong nStrLen = wcslen(szData);

		if (nStrLen > 0)
		{
#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
			realloc(nStrLen + 1);
			copyString(m_szData, capacity(), (const char16_t*)szData, nStrLen + 1);
			m_nLength = nStrLen;
#else
			fromUtf32((const char32_t*)szData);
#endif //ROYMA_SYSTEM_PLATFORM
		}
		else
		{
			release();
		}

		m_bContentChanged = true;
		return *this;
	}

	string& string::operator=(const string& str)
	{
		if (&str != this)
		{
			if (str.length() > 0)
			{
				realloc(str.length() + 1);
				copyString(m_szData, capacity(), str.m_szData, str.length() + 1);
				m_nLength = str.length();
			}
			else
			{
				release();
			}
		}
		
		m_bContentChanged = true;
		return *this;
	}

	string& string::operator=(string&& str)
	{
		if (&str != this)
		{
			release();

			m_szData = str.m_szData;
			m_nCapacity = str.m_nCapacity;
			m_nLength = str.m_nLength;

			str.m_szData = const_cast<char16_t*>(s_szNull);
			str.m_nLength = 0;
			str.m_nCapacity = 1;

			m_bContentChanged = true;
		}

		return *this;
	}

	string& string::operator=(bool bValue)
	{
		if (bValue)
		{
			operator=(L"true");
		}
		else
		{
			operator=(L"false");
		}

		return *this;
	}

	string& string::operator=(sint nValue)
	{
		wchar buffer[32];
		swprintf(buffer, 32, L"%d", nValue);
		operator=(buffer);

		return *this;
	}

	string& string::operator=(slong lValue)
	{
		wchar buffer[32];
		swprintf(buffer, 32, L"%lld", lValue);
		operator=(buffer);

		return *this;
	}

	string& string::operator=(uint nValue)
	{
		wchar buffer[32];
		swprintf(buffer, 32, L"%d", nValue);
		operator=(buffer);

		return *this;
	}

	string& string::operator=(ulong lValue)
	{
		wchar buffer[32];
		swprintf(buffer, 32, L"%lld", lValue);
		operator=(buffer);

		return *this;
	}

	string& string::operator=(float fValue)
	{
		wchar buffer[32];
		swprintf(buffer, 32, L"%f", fValue);
		operator=(buffer);

		return *this;
	}

	string& string::operator=(double dValue)
	{
		char buffer[32];
		_gcvt_s(buffer, 32, dValue, 17);
		fromAscii(buffer);

		return *this;
	}

	string& string::operator=(const Value& val)
	{
		return *this = val.toString();
	}

	string& string::operator=(hex32 nValue)
	{
		wchar buffer[32];
		swprintf(buffer, L"%x", nValue);
		operator=(buffer);
		return *this;
	}

	string& string::operator=(hex64 lValue)
	{
		wchar buffer[32];
		swprintf(buffer, L"%llx", lValue);
		operator=(buffer);
		return *this;
	}

	string& string::operator=(num32 nValue)
	{
		operator=((uint)nValue);
		*this = separateByComma();
		return *this;
	}

	string& string::operator=(num64 lValue)
	{
		operator=((ulong)lValue);
		*this = separateByComma();
		return *this;
	}

	string string::substr(slong nStart, slong nLength) const
	{
		assert(nLength >= 0);

		if (nStart < 0 || nStart + nLength > m_nLength)
		{
			return L"";
		}

		string str = alloc(nLength + 1);
		if (nLength > 0)
		{
			copyString(str.m_szData, str.capacity(), m_szData + nStart, nLength);
			str.m_szData[nLength] = 0;
			str.m_nLength = nLength;
		}

		return str;
	}

	List<string> string::split(const wchar chSeparator) const
	{
		List<string> dstString;
		string strBuffer;

		slong nStart = 0;
		slong i = 0;
		for (i = 0; i < length(); i++)
		{
			if (m_szData[i] == chSeparator)
			{
				strBuffer = substr(nStart, i - nStart);
				if (strBuffer.length() > 0)
				{
					dstString.insert(strBuffer);
				}

				nStart = i + 1;
			}
		}

		strBuffer = substr(nStart, i - nStart);
		if (strBuffer.length() > 0)
		{
			dstString.insert(strBuffer);
		}

		return std::move(dstString);
	}

	List<string> string::lines() const
	{
		List<string> dstString;
		string strBuffer;

		slong nStart = 0;
		slong i = 0;
		for (i = 0; i < length(); i++)
		{
			if (m_szData[i] == L'\n')
			{
				strBuffer = substr(nStart, i - nStart);
				dstString.insert(strBuffer);

				nStart = i + 1;
			}
		}

		strBuffer = substr(nStart, i - nStart);
		dstString.insert(strBuffer);

		return std::move(dstString);
	}

	List<string> string::split(const string& strSeparator) const
	{
		List<string> dstString;
		string strBuffer;

		slong nStart = 0;
		slong i = 0;
		for (i = 0; i < length() - strSeparator.length() + 1; i++)
		{
			if (substr(i, strSeparator.length()) == strSeparator)
			{
				strBuffer = substr(nStart, i - nStart);
				dstString.insert(strBuffer);

				nStart = i + strSeparator.length();
			}
		}

		strBuffer = substr(nStart, length() - nStart);
		dstString.insert(strBuffer);

		return std::move(dstString);
	}

	string& string::replace(slong nStart, slong nLength, const string& str)
	{
		if (nStart >= 0 && nStart + nLength <= m_nLength)
		{
			slong nDstLen = str.length();
			if (nDstLen == nLength)
			{
				copyString(m_szData + nStart, capacity(), str.m_szData, nDstLen);
				return *this;
			}

			string strBackup = *this;
			slong nNewLen = length() + nDstLen - nLength;
			if (nDstLen > nLength)
			{
				realloc(nNewLen + 1);
			}
			m_nLength = nNewLen;

			slong nDstStart = 0;
			slong nSrcStart = 0;

			copyString(m_szData, capacity(), strBackup.m_szData, nStart);
			nDstStart += nStart;
			nSrcStart += nStart;

			copyString(m_szData + nDstStart, capacity(), str.m_szData, str.length());
			nDstStart += nDstLen;
			nSrcStart += nLength;

			copyString(m_szData + nDstStart, capacity(), strBackup.m_szData + nSrcStart, strBackup.m_nLength - nSrcStart + 1);

			m_bContentChanged = true;
		}

		return *this;
	}

	string& string::replace(const string& strOrigin, const string& strInstead)
	{
		List<slong> dstItemList;
		slong i = 0;
		while (i < m_nLength)
		{
			if (substr(i, strOrigin.length()) == strOrigin)
			{
				dstItemList.insert(i);
				i += strOrigin.length();
			}
			else
			{
				i++;
			}
		}

		if (strInstead.length() == strOrigin.length())
		{
			for (slong i : dstItemList)
			{
				copyString(m_szData + i, capacity(), strInstead.m_szData, strInstead.m_nLength);
			}
			m_bContentChanged = true;
			return *this;
		}

		string strBackup = *this;
		slong nNewLen = length() + dstItemList.count() * (strInstead.length() - strOrigin.length());
		if (strInstead.length() > strOrigin.length())
		{
			realloc(nNewLen + 1);
		}
		m_nLength = nNewLen;

		slong nDstStart = 0;
		slong nSrcStart = 0;
		slong nLength;

		for (slong i : dstItemList)
		{
			nLength = i - nSrcStart;

			copyString(m_szData + nDstStart, capacity(), strBackup.m_szData + nSrcStart, nLength);
			nDstStart += nLength;
			nSrcStart += nLength;

			copyString(m_szData + nDstStart, capacity(), strInstead.m_szData, strInstead.m_nLength);
			nDstStart += strInstead.m_nLength;
			nSrcStart += strOrigin.m_nLength;
		}
		copyString(m_szData + nDstStart, capacity(), strBackup.m_szData + nSrcStart, strBackup.m_nLength - nSrcStart + 1);

		m_bContentChanged = true;
		return *this;
	}

	string& string::replace(wchar chOrigin, wchar chInstead)
	{
		for (slong i = 0; i < m_nLength; i++)
		{
			if (m_szData[i] == chOrigin)
			{
				m_szData[i] = chInstead;
			}
		}

		m_bContentChanged = true;
		return *this;
	}

	string& string::trim()
	{
		replace(L"\t", L"");
		replace(L"\r", L"");
		trim(L'\n');
		trim(L' ');

		return *this;
	}

	string& string::trim(wchar ch)
	{
		if (ch)
		{
			slong nFirst = -1;
			for (slong i = 0; i < m_nLength; i++)
			{
				if (m_szData[i] != ch)
				{
					nFirst = i;
					break;
				}
			}

			slong nLast = -1;
			for (slong i = m_nLength - 1; i > -1; i--)
			{
				if (m_szData[i] != ch)
				{
					nLast = i;
					break;
				}
			}

			if (nFirst == -1 || nLast == -1)
			{
				*this = L"";
			}
			else
			{
				*this = substr(nFirst, nLast + 1 - nFirst);
			}

			m_bContentChanged = true;
		}

		return *this;
	}

	string string::upperCase() const
	{
		string str = *this;
		char16_t* szString = str.m_szData;

		for (slong i = 0; i < m_nLength; i++)
		{
			if (szString[i] >= L'a' && szString[i] <= L'z')
			{
				szString[i] = szString[i] + L'A' - L'a';
			}
			else
			{
				szString[i] = szString[i];
			}
		}

		return str;
	}

	string string::lowerCase() const
	{
		string str = *this;
		char16_t* szString = str.m_szData;

		for (slong i = 0; i < length(); i++)
		{
			if (szString[i] >= L'A' && szString[i] <= L'Z')
			{
				szString[i] = szString[i] + L'a' - L'A';
			}
			else
			{
				szString[i] = szString[i];
			}
		}

		return str;
	}

	string string::separateByComma() const
	{
		string str;
		slong srcLen;

		if (m_szData[0] != L'-')
		{
			srcLen = length();
		}
		else
		{
			srcLen = length() - 1;
		}

		slong nNumComma = (srcLen - 1) / 3;
		slong nDstLen = srcLen + nNumComma;

		str.realloc(nDstLen + 1);
		str.m_nLength = nDstLen;
		str.m_szData[nDstLen] = 0;

		slong i = length() - 1;
		for (slong k = 1; k <= nDstLen; k++)
		{
			if (nNumComma && k % 4 == 0)
			{
				str.m_szData[nDstLen - k] = L',';
				nNumComma--;
			}
			else
			{
				str.m_szData[nDstLen - k] = m_szData[i--];
			}
		}

		return str;
	}

	bool string::isRealNumber() const
	{
		slong nCountDot = count(L'.');
		slong nCountE = count(L'e');
		slong nCountAdd = count(L'+');
		slong nCountSub = count(L'-');

		if (nCountDot > 1)
		{
			return false;
		}

		if (nCountE > 1)
		{
			return false;
		}

		if (nCountAdd > 2 || nCountSub > 2)
		{
			return false;
		}

		if (first(L'+') != 0 && first(L'+') != first(L'e') + 1)
		{
			return false;
		}

		if (last(L'+') != 0 && last(L'+') != first(L'e') + 1)
		{
			return false;
		}

		if (first(L'-') != 0 && first(L'-') != first(L'e') + 1)
		{
			return false;
		}

		if (last(L'-') != 0 && last(L'-') != first(L'e') + 1)
		{
			return false;
		}

		if (nCountDot + nCountE + nCountAdd + nCountSub + count(L'0', L'9') != m_nLength)
		{
			return false;
		}

		return true;
	}

	bool string::isIntNumber() const
	{
		slong nCountAdd = count(L'+');
		slong nCountSub = count(L'-');

		if (nCountAdd > 1 || nCountSub > 1)
		{
			return false;
		}

		if (first(L'+') > 0 || first(L'-') > 0)
		{
			return false;
		}

		if (count(L'0', L'9') + nCountAdd + nCountSub != m_nLength)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	bool string::isHexNumber() const
	{
		string str = upperCase();

		if (str.count(L'0', L'9') + str.count(L'A') + str.count(L'B') + str.count(L'C') + str.count(L'D') + str.count(L'E') + str.count('F') != m_nLength)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	const wchar* string::operator*() const
	{
#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		return (wchar*)m_szData;
#else
#endif //ROYMA_SYSTEM_PLATFORM
	}

	slong string::first(const string& strKeyword, slong nIdx) const
	{
		slong nLength = m_nLength - strKeyword.length() + 1;
		if (nIdx < 0 || nIdx >= nLength)
		{
			return -1;
		}

		for (slong i = nIdx; i < nLength; i++)
		{
			if (substr(i, strKeyword.length()) == strKeyword)
			{
				return i;
			}
		}

		return -1;
	}

	slong string::last(const string& strKeyword, slong nIdx) const
	{
		slong nLength = m_nLength - strKeyword.length() + 1;
		if (nIdx == -1)
		{
			nIdx = nLength - 1;
		}
		else if (nIdx < 0 || nIdx >= nLength)
		{
			return -1;
		}
		
		for (slong i = nIdx; i >= 0; i--)
		{
			if (substr(i, strKeyword.length()) == strKeyword)
			{
				return i;
			}
		}

		return -1;
	}

	slong string::first(wchar ch, slong nIdx) const
	{
		for (slong i = nIdx; i < m_nLength; i++)
		{
			if (m_szData[i] == ch)
			{
				return i;
			}
		}

		return -1;
	}

	slong string::last(wchar ch, slong nIdx) const
	{
		if (nIdx == -1)
		{
			nIdx = m_nLength - 1;
		}
		else if (nIdx < 0 || nIdx >= m_nLength)
		{
			return -1;
		}

		for (slong i = nIdx; i >= 0; i--)
		{
			if (m_szData[i] == ch)
			{
				return i;
			}
		}

		return -1;
	}
	
	slong string::first(wchar chStart, wchar chEnd, slong nIdx) const
	{
		for (slong i = nIdx; i < m_nLength; i++)
		{
			if (chStart <= m_szData[i] && m_szData[i] <= chEnd)
			{
				return i;
			}
		}

		return -1;
	}

	slong string::last(wchar chStart, wchar chEnd, slong nIdx) const
	{
		if (nIdx == -1)
		{
			nIdx = m_nLength - 1;
		}
		else if (nIdx < 0 || nIdx >= m_nLength)
		{
			return -1;
		}

		for (slong i = nIdx; i >= 0; i--)
		{
			if (chStart <= m_szData[i] && m_szData[i] <= chEnd)
			{
				return i;
			}
		}

		return -1;
	}

	string string::truncate(slong nOffset, wchar ch) const
	{
		slong nEndIdx = first(ch);

		return substr(0, nEndIdx + nOffset + 1);
	}

	string string::repeat(wchar ch, slong nNum)
	{
		string strRslt = alloc(nNum + 1);

		for (slong i = 0; i < nNum; i++)
		{
			strRslt.m_szData[i] = ch;
		}

		if (strRslt.m_szData != s_szNull)
		{
			strRslt.m_szData[nNum] = 0;
		}
		strRslt.m_nLength = nNum;

		strRslt.m_bContentChanged = true;
		return strRslt;
	}

	royma::slong string::count(wchar ch) const
	{
		slong nCount = 0;

		for (slong i = 0; i < m_nLength; ++i)
		{
			if (m_szData[i] == ch)
			{
				++nCount;
			}
		}

		return nCount;
	}

	royma::slong string::count(wchar chStart, wchar chEnd) const
	{
		slong nCount = 0;

		for (slong i = 0; i < m_nLength; ++i)
		{
			if (chStart <= m_szData[i] && m_szData[i] <= chEnd)
			{
				++nCount;
			}
		}

		return nCount;
	}

	void string::toUtf32(char32_t* szUtf32)
	{
		for (int i = 0; i < m_nLength + 1; ++i)
		{
			szUtf32[i] = m_szData[i];
		}
	}

	void string::fromUtf32(const char32_t* szUtf32)
	{
#if ROYMA_SYSTEM_PLATFORM_IS(ROYMA_SYSTEM_PLATFORM_WINDOWS)
		const wchar* szString = (const wchar*)szUtf32;
		slong nStrLen = wcslen(szString);
		if (nStrLen == 0)
		{
			operator=(L"");
			return;
		}

		realloc(nStrLen + 1);
		m_nLength = nStrLen;

		for (int i = 0; i < m_nLength + 1; ++i)
		{
			m_szData[i] = castval<char16_t>(szUtf32[i]);
		}
				
#endif //ROYMA_SYSTEM_PLATFORM

		m_bContentChanged = true;
	}
}