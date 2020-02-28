#include "VirtualFileDataObject.h"
#include <ObjIdl.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <algorithm>
#include <core/Common.h>
#include <core/devices/Device.h>
#include <core/files/File.h>
#include <core/streams/Stream.h>
#include <iterator>
#include <memory>
#include <strsafe.h>
#include <wx/log.h>

// From include/wx/msw/ole/comimpl.h
class wxAutoULong
{
public:
	wxAutoULong(ULONG value = 0) : m_Value(value) {}

	operator ULONG&() { return m_Value; }
	ULONG& operator=(ULONG value)
	{
		m_Value = value;
		return m_Value;
	}

	wxAutoULong& operator++()
	{
		++m_Value;
		return *this;
	}
	const wxAutoULong operator++(int)
	{
		wxAutoULong temp = *this;
		++m_Value;
		return temp;
	}

	wxAutoULong& operator--()
	{
		--m_Value;
		return *this;
	}
	const wxAutoULong operator--(int)
	{
		wxAutoULong temp = *this;
		--m_Value;
		return temp;
	}

private:
	ULONG m_Value;
};

// From include/wx/msw/ole/comimpl.h
#define DECLARE_IUNKNOWN_METHODS                          \
public:                                                   \
	STDMETHODIMP QueryInterface(REFIID, void**) override; \
	STDMETHODIMP_(ULONG) AddRef() override;               \
	STDMETHODIMP_(ULONG) Release() override;              \
                                                          \
private:                                                  \
	static const IID* ms_aIids[];                         \
	wxAutoULong m_cRef

// From include/wx/msw/ole/comimpl.h
#define BEGIN_IID_TABLE(cname) const IID* cname::ms_aIids[] = {
#define ADD_IID(iid) &IID_I##iid,
#define ADD_RAW_IID(iid) &iid,
#define END_IID_TABLE }

// From src\msw\ole\comimpl.cpp
static bool IsIidFromList(REFIID riid, const IID* aIids[], size_t nCount)
{
	for (size_t i = 0; i < nCount; i++)
	{
		if (riid == *aIids[i])
			return true;
	}

	return false;
}

// From include/wx/msw/ole/comimpl.h
#define IMPLEMENT_IUNKNOWN_METHODS(classname)                       \
	STDMETHODIMP classname::QueryInterface(REFIID riid, void** ppv) \
	{                                                               \
		if (IsIidFromList(riid, ms_aIids, WXSIZEOF(ms_aIids)))      \
		{                                                           \
			*ppv = this;                                            \
			AddRef();                                               \
                                                                    \
			return S_OK;                                            \
		}                                                           \
		else                                                        \
		{                                                           \
			*ppv = NULL;                                            \
                                                                    \
			return (HRESULT)E_NOINTERFACE;                          \
		}                                                           \
	}                                                               \
                                                                    \
	STDMETHODIMP_(ULONG) classname::AddRef() { return ++m_cRef; }   \
                                                                    \
	STDMETHODIMP_(ULONG) classname::Release()                       \
	{                                                               \
		if (--m_cRef == wxAutoULong(0))                             \
		{                                                           \
			delete this;                                            \
			return 0;                                               \
		}                                                           \
		else                                                        \
			return m_cRef;                                          \
	}

// COM IStream that wraps a noire::Stream
class CComFileStream : public IStream
{
public:
	CComFileStream(std::shared_ptr<noire::File> file) : mFile{ file } {}

	DECLARE_IUNKNOWN_METHODS;

	// ISequentialStream
	STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) override;
	STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten) override;

	// IStream
	STDMETHODIMP
	Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override;
	STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) override;
	STDMETHODIMP CopyTo(IStream* pstm,
						ULARGE_INTEGER cb,
						ULARGE_INTEGER* pcbRead,
						ULARGE_INTEGER* pcbWritten) override;
	STDMETHODIMP Commit(DWORD grfCommitFlags) override;
	STDMETHODIMP Revert(void) override;
	STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	STDMETHODIMP
	UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) override;
	STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) override;
	STDMETHODIMP Clone(IStream** ppstm) override;

private:
	std::shared_ptr<noire::File> mFile;

	wxDECLARE_NO_COPY_CLASS(CComFileStream);
};

BEGIN_IID_TABLE(CComFileStream)
ADD_IID(Unknown)
ADD_IID(SequentialStream)
ADD_IID(Stream)
END_IID_TABLE;

IMPLEMENT_IUNKNOWN_METHODS(CComFileStream);

STDMETHODIMP CComFileStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
	wxLogDebug(__FUNCTION__ "(%p, %lu, %p)", pv, cb, pcbRead);
	noire::Stream& s = mFile->Raw();
	const noire::u64 toRead = std::min<noire::u64>(cb, s.Size() - s.Tell());
	noire::u64 read = 0;
	if (toRead != 0)
	{
		read = s.Read(pv, toRead);
	}

	if (pcbRead)
	{
		*pcbRead = read;
	}
	return S_OK;
}

STDMETHODIMP CComFileStream::Write(const void*, ULONG, ULONG*)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP
CComFileStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition)
{
	wxLogDebug(__FUNCTION__ "(%lld, %u, %p)", dlibMove.QuadPart, dwOrigin, plibNewPosition);

	noire::StreamSeekOrigin orig;
	switch (dwOrigin)
	{
	default:
	case STREAM_SEEK_SET: orig = noire::StreamSeekOrigin::Begin; break;
	case STREAM_SEEK_CUR: orig = noire::StreamSeekOrigin::Current; break;
	case STREAM_SEEK_END: orig = noire::StreamSeekOrigin::End; break;
	}

	const noire::u64 newPos = mFile->Raw().Seek(dlibMove.QuadPart, orig);
	if (plibNewPosition)
	{
		plibNewPosition->QuadPart = newPos;
	}
	return S_OK;
}

STDMETHODIMP CComFileStream::SetSize(ULARGE_INTEGER)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP CComFileStream::CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP CComFileStream::Commit(DWORD)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP CComFileStream::Revert(void)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP CComFileStream::LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP CComFileStream::UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	return E_NOTIMPL;
}

STDMETHODIMP CComFileStream::Stat(STATSTG* pstatstg, DWORD grfStatFlag)
{
	wxLogDebug(__FUNCTION__ "(%p, %u)", pstatstg, grfStatFlag);

	const bool needsName = !(grfStatFlag & STATFLAG_NONAME);
	ZeroMemory(pstatstg, sizeof(STATSTG));
	if (needsName)
	{
		wxLogDebug(__FUNCTION__ " : needsName not implemented");
		return E_NOTIMPL;
	}
	else
	{
		pstatstg->pwcsName = nullptr;
	}
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = mFile->Raw().Size();
	GetSystemTimeAsFileTime(&pstatstg->mtime);
	GetSystemTimeAsFileTime(&pstatstg->ctime);
	GetSystemTimeAsFileTime(&pstatstg->atime);
	pstatstg->grfMode = STGM_READ;
	pstatstg->grfLocksSupported = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	return S_OK;
}

STDMETHODIMP CComFileStream::Clone(IStream** ppstm)
{
	wxLogDebug(__FUNCTION__ " not implemented");
	*ppstm = nullptr;
	return E_NOTIMPL;
}

// Must have same layout as wxIDataObject, from src\msw\ole\dataobj.cpp
// Used to replace the wxIDataObject pointer from a wxDataObject with a custom implementation
class wxIDataObjectImposter : public IDataObject
{
public:
	wxIDataObjectImposter(wxDataObject* pDataObject);
	virtual ~wxIDataObjectImposter();

	void SetDeleteFlag() { m_mustDelete = true; }

	// IDataObject
	// STDMETHODIMP GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium) override;
	// STDMETHODIMP GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium) override;
	// STDMETHODIMP QueryGetData(FORMATETC* pformatetc) override;
	// STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* In, FORMATETC* pOut) override;
	// STDMETHODIMP SetData(FORMATETC* pfetc, STGMEDIUM* pmedium, BOOL fRelease) override;
	// STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFEtc) override;
	// STDMETHODIMP DAdvise(FORMATETC* pfetc, DWORD ad, IAdviseSink* p, DWORD* pdw) override;
	// STDMETHODIMP DUnadvise(DWORD dwConnection) override;
	// STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise) override;

	DECLARE_IUNKNOWN_METHODS;

private:
	wxDataObject* m_pDataObject;
	bool m_mustDelete;

	wxDECLARE_NO_COPY_CLASS(wxIDataObjectImposter);

	class SystemDataEntry
	{
	public:
		// Ctor takes ownership of the pointers.
		SystemDataEntry(FORMATETC* pformatetc_, STGMEDIUM* pmedium_)
			: pformatetc(pformatetc_), pmedium(pmedium_)
		{
		}

		~SystemDataEntry()
		{
			delete pformatetc;
			delete pmedium;
		}

		FORMATETC* pformatetc;
		STGMEDIUM* pmedium;
	};
	typedef wxVector<SystemDataEntry*> SystemData;

	// container for system data
	SystemData m_systemData;
};

BEGIN_IID_TABLE(wxIDataObjectImposter)
ADD_IID(Unknown)
ADD_IID(DataObject)
END_IID_TABLE;

IMPLEMENT_IUNKNOWN_METHODS(wxIDataObjectImposter);

wxIDataObjectImposter::wxIDataObjectImposter(wxDataObject* pDataObject)
{
	m_pDataObject = pDataObject;
	m_mustDelete = false;
}

wxIDataObjectImposter::~wxIDataObjectImposter()
{
	// delete system data
	for (SystemData::iterator it = m_systemData.begin(); it != m_systemData.end(); ++it)
	{
		delete (*it);
	}

	if (m_mustDelete)
	{
		delete m_pDataObject;
	}
}

// Based on https://devblogs.microsoft.com/oldnewthing/tag/what-a-drag
class VirtualFileComDataObject : public wxIDataObjectImposter
{
public:
	VirtualFileComDataObject(wxDataObject* obj,
							 noire::Device& device,
							 const std::vector<noire::PathView>& paths);

	// IDataObject
	STDMETHODIMP GetData(FORMATETC* pformatetcIn, STGMEDIUM* pmedium) override;
	STDMETHODIMP GetDataHere(FORMATETC* pformatetc, STGMEDIUM* pmedium) override;
	STDMETHODIMP QueryGetData(FORMATETC* pformatetc) override;
	STDMETHODIMP GetCanonicalFormatEtc(FORMATETC* In, FORMATETC* pOut) override;
	STDMETHODIMP SetData(FORMATETC* pfetc, STGMEDIUM* pmedium, BOOL fRelease) override;
	STDMETHODIMP EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFEtc) override;
	STDMETHODIMP DAdvise(FORMATETC* pfetc, DWORD ad, IAdviseSink* p, DWORD* pdw) override;
	STDMETHODIMP DUnadvise(DWORD dwConnection) override;
	STDMETHODIMP EnumDAdvise(IEnumSTATDATA** ppenumAdvise) override;

	static constexpr std::size_t InvalidIndex{ static_cast<std::size_t>(-1) };

	std::size_t GetDataCount();
	std::size_t GetDataIndex(const FORMATETC* pfetc);

	HRESULT CreateFileGroupDescriptor(HGLOBAL* outGlobal);

	std::vector<FORMATETC> mFormats;
	noire::Device& mDevice;
	std::vector<noire::Path> mPaths;
};

static void SetFORMATETC(FORMATETC& pfe,
						 UINT cf,
						 TYMED tymed = TYMED_HGLOBAL,
						 LONG lindex = -1,
						 DWORD dwAspect = DVASPECT_CONTENT,
						 DVTARGETDEVICE* ptd = NULL)
{
	pfe.cfFormat = (CLIPFORMAT)cf;
	pfe.tymed = tymed;
	pfe.lindex = lindex;
	pfe.dwAspect = dwAspect;
	pfe.ptd = ptd;
}

VirtualFileComDataObject::VirtualFileComDataObject(wxDataObject* obj,
												   noire::Device& device,
												   const std::vector<noire::PathView>& paths)
	: wxIDataObjectImposter(obj), mDevice{ device }, mPaths{}
{
	Expects(std::all_of(paths.begin(), paths.end(), [&device](noire::PathView p) {
		return p.IsFile() && device.Exists(p);
	}));

	mPaths.reserve(paths.size());
	std::transform(paths.begin(), paths.end(), std::back_inserter(mPaths), [](auto p) {
		return noire::Path{ p };
	});

	mFormats.resize(GetDataCount());
	SetFORMATETC(mFormats[0], RegisterClipboardFormat(CFSTR_FILEDESCRIPTOR));
	for (std::size_t i = 1; i < mFormats.size(); i++)
	{
		SetFORMATETC(mFormats[i],
					 RegisterClipboardFormat(CFSTR_FILECONTENTS),
					 TYMED_HGLOBAL,
					 i - 1);
	}
}

std::size_t VirtualFileComDataObject::GetDataCount()
{
	return 1 + mPaths.size(); // one CFSTR_FILEDESCRIPTOR + (one CFSTR_FILECONTENTS for each file);
}

std::size_t VirtualFileComDataObject::GetDataIndex(const FORMATETC* pfetc)
{
	for (std::size_t i = 0; i < mFormats.size(); i++)
	{
		if (pfetc->cfFormat == mFormats[i].cfFormat && (pfetc->tymed & mFormats[i].tymed) &&
			pfetc->dwAspect == mFormats[i].dwAspect && pfetc->lindex == mFormats[i].lindex)
		{
			return i;
		}
	}

	return InvalidIndex;
}

HRESULT VirtualFileComDataObject::CreateFileGroupDescriptor(HGLOBAL* outGlobal)
{
	const std::size_t bufferSize =
		sizeof(FILEGROUPDESCRIPTOR) + sizeof(FILEDESCRIPTOR) * (mPaths.size() - 1);

	HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, bufferSize);
	if (hglob)
	{
		FILEGROUPDESCRIPTOR* fgd = reinterpret_cast<FILEGROUPDESCRIPTOR*>(GlobalLock(hglob));
		if (fgd)
		{
			std::memset(fgd, 0, bufferSize);
			fgd->cItems = gsl::narrow_cast<UINT>(mPaths.size());
			for (std::size_t i = 0; i < mPaths.size(); i++)
			{
				noire::Path& path = mPaths[i];
				FILEDESCRIPTOR& f = fgd->fgd[i];

				std::string_view name = path.Name();
				MultiByteToWideChar(CP_UTF8,
									0,
									name.data(),
									name.size(),
									f.cFileName,
									ARRAYSIZE(f.cFileName));
			}

			GlobalUnlock(hglob);
		}
		else
		{
			GlobalFree(hglob);
			hglob = NULL;
		}
	}

	*outGlobal = hglob;
	return hglob ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP VirtualFileComDataObject::GetData(FORMATETC* pfe, STGMEDIUM* pmed)
{
	ZeroMemory(pmed, sizeof(*pmed));
	switch (std::size_t i = GetDataIndex(pfe); i)
	{
	case 0: // CFSTR_FILEDESCRIPTOR
	{
		pmed->tymed = TYMED_HGLOBAL;
		return CreateFileGroupDescriptor(&pmed->hGlobal);
	}
	default:
	{
		if (i < 1 || i > mPaths.size())
		{
			return DV_E_FORMATETC;
		}

		pmed->tymed = TYMED_ISTREAM;
		pmed->pstm = new CComFileStream(mDevice.Open(mPaths[i - 1]));
		if (pmed->pstm)
		{
			pmed->pstm->AddRef();

			// set the stream position properly
			LARGE_INTEGER liZero = { 0, 0 };
			pmed->pstm->Seek(liZero, STREAM_SEEK_END, NULL);
		}
		return pmed->pstm ? S_OK : E_FAIL;
	}
	}
}

STDMETHODIMP VirtualFileComDataObject::GetDataHere(FORMATETC*, STGMEDIUM*)
{
	return E_NOTIMPL;
}

STDMETHODIMP VirtualFileComDataObject::QueryGetData(FORMATETC* pfe)
{
	return GetDataIndex(pfe) == InvalidIndex ? S_FALSE : S_OK;
}

STDMETHODIMP VirtualFileComDataObject::GetCanonicalFormatEtc(FORMATETC* In, FORMATETC* Out)
{
	*Out = *In;
	Out->ptd = NULL;
	return DATA_S_SAMEFORMATETC;
}

STDMETHODIMP VirtualFileComDataObject::SetData(FORMATETC*, STGMEDIUM*, BOOL)
{
	return E_NOTIMPL;
}

STDMETHODIMP VirtualFileComDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFEtc)
{
	if (dwDirection == DATADIR_GET)
	{
		return SHCreateStdEnumFmtEtc(mFormats.size(), mFormats.data(), ppenumFEtc);
	}
	*ppenumFEtc = NULL;
	return E_NOTIMPL;
}

STDMETHODIMP VirtualFileComDataObject::DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP VirtualFileComDataObject::DUnadvise(DWORD)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP VirtualFileComDataObject::EnumDAdvise(IEnumSTATDATA**)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

namespace noire::explorer
{
	VirtualFileDataObject::VirtualFileDataObject(Device& device, const std::vector<PathView>& paths)
		: wxDataObject()
	{
		static_assert(
			sizeof(wxDataObject) == sizeof(void*) * 2,
			"Size of wxDataObject changed, expected sizeof(vtbl*) + sizeof(IDataObject*)");

		IDataObject* origDataObj = GetInterface();

		VirtualFileComDataObject* newDataObj = new VirtualFileComDataObject(this, device, paths);
		newDataObj->AddRef();

		// Really hacky workaround:
		// We replace the internal IDataObject with our own implementation, since the implementation
		// provided by wxWidgets doesn't seem to support TYMED_ISTREAM or a CFSTR_FILEDESCRIPTOR
		// with multiple associated CFSTR_FILECONTENTS
		const std::uintptr_t thisAddr = reinterpret_cast<std::uintptr_t>(this);
		*reinterpret_cast<IDataObject**>(thisAddr + sizeof(void*)) = newDataObj;

		// delete the original
		Ensures(origDataObj->Release() == 0);
	}

	void VirtualFileDataObject::GetAllFormats(wxDataFormat*, Direction) const {}

	bool VirtualFileDataObject::GetDataHere(const wxDataFormat&, void*) const { return false; }

	std::size_t VirtualFileDataObject::GetDataSize(const wxDataFormat&) const { return 0; }

	std::size_t VirtualFileDataObject::GetFormatCount(Direction) const { return 0; }

	wxDataFormat VirtualFileDataObject::GetPreferredFormat(Direction) const
	{
		return wxDataFormat();
	}

	bool VirtualFileDataObject::SetData(const wxDataFormat&, std::size_t, const void*)
	{
		return false;
	}
}
