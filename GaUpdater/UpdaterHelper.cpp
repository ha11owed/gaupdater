#include "stdafx.h"
#include "UpdaterHelper.h"

// needed for VerQueryValue and GetFileVersionInfo
#pragma comment(lib, "Version.lib")

using namespace rapidxml;
using namespace std;
using namespace galib::util;

#define BUFFER_SIZE 1024*8

CInternetSession internetSession(_T("GaUpdater"), 1,
	PRE_CONFIG_INTERNET_ACCESS, //INTERNET_OPEN_TYPE_DIRECT,
	NULL, NULL,
	INTERNET_FLAG_SECURE | INTERNET_FLAG_DONT_CACHE);

inline const char* ReadNodeContent(xml_node<char>* rootNode, const char* nodeName, const char* defaultValue = "")
{
	xml_node<char> * n = rootNode->first_node(nodeName);
	const char* v = nullptr;
	if (n != nullptr) {
		v = n->value();
	}
	if (v == nullptr) {
		v = defaultValue;
	}
	return v;
}


FileVersion::FileVersion(const char* versionString)
{
	V1 = V2 = V3 = V4 = 0;
	if (versionString != nullptr)
	{
		CString versionStr(versionString);
		int nTokenPos = 0;
		CString vstr = versionStr.Tokenize(_T("."), nTokenPos);
		if (vstr.IsEmpty()) {
			return;
		}
		V1 = _tstoi(vstr);

		vstr = versionStr.Tokenize(_T("."), nTokenPos);
		if (vstr.IsEmpty()) {
			return;
		}
		V2 = _tstoi(vstr);

		vstr = versionStr.Tokenize(_T("."), nTokenPos);
		if (vstr.IsEmpty()) {
			return;
		}
		V3 = _tstoi(vstr);

		vstr = versionStr.Tokenize(_T("."), nTokenPos);
		if (vstr.IsEmpty()) {
			return;
		}
		V4 = _tstoi(vstr);
	}
}

int FileVersion::compareTo(const FileVersion& other) const
{
	int d;
	
	d = V1 - other.V1;
	if (d != 0)
		return d;

	d = V2 - other.V2;
	if (d != 0)
		return d;

	d = V3 - other.V3;
	if (d != 0)
		return d;

	d = V4 - other.V4;
	return d;
}

bool FileVersion::isValid() const {
	return V1 != 0 || V2 != 0 || V3 != 0 || V4 != 0;
}

void FileVersion::readFileVersion(LPCTSTR filePath)
{
	DWORD   dwHandle;
	DWORD   dwFileVersionInfoSize;
	LPVOID  lpData = nullptr;

	VS_FIXEDFILEINFO fileInfo;

	for (;;)
	{
		dwFileVersionInfoSize = GetFileVersionInfoSize(filePath, &dwHandle);
		if (!dwFileVersionInfoSize)
			break;

		lpData = malloc(dwFileVersionInfoSize);
		if (!lpData)
			break;

		if (!GetFileVersionInfo(filePath, dwHandle, dwFileVersionInfoSize, lpData))
			break;

		// VerQueryValue does not allocate memory. lpInfo will point somewhere in the lpData memory.
		LPVOID lpInfo;
		UINT unInfoLen;
		if (!VerQueryValue(lpData, _T("\\"), &lpInfo, &unInfoLen))
			break;
		
		if (unInfoLen != sizeof(VS_FIXEDFILEINFO))
			break;

		memcpy(&fileInfo, lpInfo, unInfoLen);
		V4 = (fileInfo.dwProductVersionLS & 0x0000FFFF);
		V3 = ((fileInfo.dwProductVersionLS & 0xFFFF0000) >> 16);
		V2 = (fileInfo.dwProductVersionMS & 0x0000FFFF);
		V1 = ((fileInfo.dwProductVersionMS & 0xFFFF0000) >> 16);
		break;
	}

	if (lpData != nullptr)
	{
		free(lpData);
		lpData = nullptr;
	}
}




bool CUpdaterHelper::openConnection(LPCTSTR url, CHttpConnection** outConn, CHttpFile** outHttpFile)
{
	lastErrorMsg[0] = _T('\0');
	*outConn = nullptr;
	*outHttpFile = nullptr;

	bool ok = false;
	BOOL bRet;

	for (;;)
	{
		bRet = AfxParseURL(url, dwServiceType, strServer, strObject, nPort);
		if (bRet == FALSE)
			break;

		CHttpConnection * conn = nullptr;
		CHttpFile * httpFile = nullptr;
		try
		{
			conn = internetSession.GetHttpConnection(strServer, nPort);
			if (conn == nullptr)
				break;

			httpFile = conn->OpenRequest(CHttpConnection::HTTP_VERB_GET, strObject);
			if (httpFile == nullptr)
				break;

			bRet = httpFile->SetReadBufferSize(BUFFER_SIZE);
			if (bRet == FALSE)
				break;

			bRet = httpFile->AddRequestHeaders(_T("User-Agent: GaUpdater\r\n"));
			if (bRet == FALSE)
				break;

			bRet = httpFile->SendRequest();
			if (bRet == FALSE)
				break;

			DWORD dwRet = 0;
			bRet = httpFile->QueryInfoStatusCode(dwRet);
			if (bRet == FALSE)
				break;
			if (dwRet != HTTP_STATUS_OK)
				break;

			*outConn = conn;
			*outHttpFile = httpFile;
			ok = true;
		}
		catch (CInternetException* pEx)
		{
			pEx->GetErrorMessage(lastErrorMsg, ARRAYSIZE(lastErrorMsg));
			pEx->Delete();
		}
		break;
	}

	if (!ok)
	{
		cleanConnection(outConn, outHttpFile);
	}
	return ok;
}

void CUpdaterHelper::cleanConnection(CHttpConnection** inOutConn, CHttpFile** inOutHttpFile)
{
	// Cleanup
	if (inOutHttpFile != nullptr && *inOutHttpFile != nullptr)
	{
		(*inOutHttpFile)->Close();
		delete (*inOutHttpFile);
		*inOutHttpFile = nullptr;
	}
	if (inOutConn != nullptr && *inOutConn != nullptr)
	{
		(*inOutConn)->Close();
		delete (*inOutConn);
		*inOutConn = nullptr;
	}
}

bool CUpdaterHelper::download(LPCTSTR url, LPCTSTR targetFilePath)
{
	CHttpConnection * conn = nullptr;
	CHttpFile * httpFile = nullptr;
	BOOL bRet;
	bool ok = false;

	DWORD fileSize = 0;
	DWORD progress = 0;

	for (;;)
	{
		if (!openConnection(url, &conn, &httpFile))
			break;

		bRet = httpFile->QueryInfo(HTTP_QUERY_CONTENT_LENGTH, fileSize);
		if (bRet == FALSE)
			break;

		// Open the target file
		CFile targetFile;
		bRet = targetFile.Open(targetFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary);
		if (bRet == FALSE)
			break;

		// Write the file
		char buffer[BUFFER_SIZE];
		UINT nRead;
		do
		{
			nRead = httpFile->Read(buffer, BUFFER_SIZE);
			targetFile.Write(buffer, nRead);
			progress += nRead;
			if (onUpdateProgress != nullptr)
			{
				onUpdateProgress(callbackContext, (double)progress / fileSize);
				//Sleep(10);
			}
		} while (nRead > 0);
		// Close the file
		targetFile.Close();

		_downloadedFiles.AddTail(targetFilePath);
		ok = true;
		break;
	}

	cleanConnection(&conn, &httpFile);

	if (onComplete != nullptr)
	{
		onComplete(callbackContext, ok);
	}
	return ok;
}

bool CUpdaterHelper::downloadUpdateInfo(LPCTSTR url, UpdateInfo& outUpdateInfo)
{
	CHttpConnection * conn = nullptr;
	CHttpFile * httpFile = nullptr;
	bool ok = false;

	for (;;) {
		if (!openConnection(url, &conn, &httpFile))
			break;

		// Read the XML
		stringstream ss;
		char buffer[BUFFER_SIZE + 2];
		UINT nRead;
		do
		{
			nRead = httpFile->Read(buffer, BUFFER_SIZE);
			buffer[nRead] = '\0';
			ss << buffer;
		} while (nRead > 0);

		// Parse the xml
		xml_document<char> doc;		
		string strSS = ss.str();
		doc.parse<0>((char*) (strSS.c_str()));

		xml_node<char> * rootNode = doc.first_node("update");
		if (rootNode == nullptr)
			break;

		// url
		outUpdateInfo.url = ReadNodeContent(rootNode, "url");
		// details
		outUpdateInfo.details = ReadNodeContent(rootNode, "description");
		// name
		outUpdateInfo.name = ReadNodeContent(rootNode, "name");
		// releaseDate
		string releaseDate(ReadNodeContent(rootNode, "date"));
		outUpdateInfo.releaseDate = DateTime::parseStrUTC(releaseDate);
		// md5
		outUpdateInfo.urlMD5Hash = ReadNodeContent(rootNode, "urlMD5Hash");
		// sha1
		outUpdateInfo.urlSHA1Hash = ReadNodeContent(rootNode, "urlSHA1Hash");
		// version
		outUpdateInfo.version = FileVersion(ReadNodeContent(rootNode, "version"));
		ok = true;
		break;
	}

	cleanConnection(&conn, &httpFile);
	return ok;
}

bool CUpdaterHelper::startInstaller(LPCTSTR filePath, LPCTSTR cmdParam)
{
	lastErrorMsg[0] = _T('\0');

	STARTUPINFO sinfo;
	PROCESS_INFORMATION  pinfo;
	ZeroMemory(&sinfo, sizeof(sinfo));
	sinfo.cb = sizeof(sinfo);
	sinfo.lpDesktop = _T("WinSta0\\Default");
	sinfo.dwFlags = STARTF_USESHOWWINDOW;
	sinfo.wShowWindow = SW_SHOW;

	CString cmd(filePath);
	if (cmdParam != nullptr && cmdParam[0] != _T('\0'))
	{
		cmd += _T(" ");
		cmd += cmdParam;
	}
	LPCTSTR pCmd = (LPCTSTR)cmd;

	if (!CreateProcess(NULL, (LPTSTR) pCmd, 
		NULL, NULL, 
		FALSE, 
		NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE, 
		NULL, NULL, &sinfo, &pinfo))
	{
		_stprintf_s(lastErrorMsg, ARRAYSIZE(lastErrorMsg), _T("Can't execute program: %s params %s"), filePath, cmdParam);
		return false;
	}
	else
	{
		return true;
	}
}

CUpdaterHelper::CUpdaterHelper()
{
	onUpdateProgress = nullptr;
	onComplete = nullptr;
	callbackContext = nullptr;
}

void CUpdaterHelper::init(LPCTSTR name, LPCTSTR url, LPCTSTR filePath, LPCTSTR cmdParams, 
	UpdateProgressCallback onUpdateProgress, DownloadCompletedCallback onComplete, void* callbackContext)
{
	_name = name;
	_url = url;
	_filePath = filePath;
	_cmdParams = cmdParams;
	_updateInfo = UpdateInfo();

	this->onUpdateProgress = onUpdateProgress;
	this->onComplete = onComplete;
	this->callbackContext = callbackContext;

	// Get current version info
	_fileVersion.readFileVersion(_filePath);
}

bool CUpdaterHelper::downloadUpdateInfo()
{
	_updateInfo = UpdateInfo();
	return downloadUpdateInfo(_url, _updateInfo);
}

CString CUpdaterHelper::downloadUpdate(LPCTSTR downloadFilePath)
{
	CString url = updateInfo().url;
	CString filePath, retFilePath;

	TCHAR buffer[MAX_PATH + 2];
	for (;;)
	{
		if (url.IsEmpty())
			break;

		if (0 == GetTempPath(MAX_PATH + 1, buffer))
			break;

		filePath.Append(buffer);
		CString fileName = url.Mid(url.ReverseFind('/') + 1);
		if (filePath.Right(1) != _T('\\'))
		{
			filePath.Append(_T("\\"));
		}
		filePath.Append(fileName);

		bool ok = download(url, filePath);
		if (!ok)
			break;

		if (updateInfo().urlMD5Hash)

		retFilePath = filePath;
		break;
	}

	return retFilePath;
}

void CUpdaterHelper::clean()
{
	for (int i = 0; i < _downloadedFiles.GetCount(); i++)
	{
		auto pos = _downloadedFiles.FindIndex(i);
		CString path = _downloadedFiles.GetAt(pos);

		if (DeleteFile(path))
		{
			_downloadedFiles.RemoveAt(pos);
			i--;
		}
	}
}
