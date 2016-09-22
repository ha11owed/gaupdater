#pragma once

#include <afxinet.h>

const time_t INVALID_DATE = 0;

class FileVersion
{
public:
	int V1;
	int V2;
	int V3;
	int V4;

	FileVersion() {	V1 = V2 = V3 = V4 = 0; }
	FileVersion(const char* versionString);

	int compareTo(const FileVersion& other) const;
	bool isValid() const;

	void readFileVersion(LPCTSTR filePath);
};

struct UpdateInfo
{
	UpdateInfo()
		: releaseDate(INVALID_DATE)
	{
		
	}

	CString url;
	CString name;
	CString details;
	time_t releaseDate;
	CString urlMD5Hash;
	CString urlSHA1Hash;
	FileVersion version;
};

typedef void(*UpdateProgressCallback)(void* tag, double progress);
typedef void(*DownloadCompletedCallback)(void* tag, bool ok);

class CUpdaterHelper
{
public:
	CUpdaterHelper();
	void init(LPCTSTR name, LPCTSTR url, LPCTSTR filePath, LPCTSTR cmdParams, UpdateProgressCallback onUpdateProgress = nullptr, DownloadCompletedCallback onComplete = nullptr, void* callbackContext = nullptr);
	void clean();

	bool downloadUpdateInfo();
	CString downloadUpdate(LPCTSTR downloadFilePath = nullptr);
	UpdateInfo& updateInfo() { return _updateInfo; }
	FileVersion& fileVersion() { return _fileVersion; }

	const CString& getName() const { return _name; }
	const CString& getUrl() const { return _url; }
	const CString& getFilePath() const { return _filePath; }
	const CString& getCmdParams() const { return _cmdParams; }
	const CStringList& getDownloads() const { return _downloadedFiles; }

public:
	bool downloadUpdateInfo(LPCTSTR url, UpdateInfo& outUpdateInfo);
	bool download(LPCTSTR url, LPCTSTR targetFilePath);
	bool startInstaller(LPCTSTR filePath, LPCTSTR cmdParam);

private:
	CStringList _downloadedFiles;
	CString _name;
	CString _url;
	CString _filePath;
	CString _cmdParams;
	UpdateInfo _updateInfo;
	FileVersion _fileVersion;

	UpdateProgressCallback onUpdateProgress;
	DownloadCompletedCallback onComplete;
	void* callbackContext;

private:
	bool openConnection(LPCTSTR url, CHttpConnection** outConn, CHttpFile** outHttpFile);
	void cleanConnection(CHttpConnection** inOutConn, CHttpFile** inOutHttpFile);

	TCHAR lastErrorMsg[4 * 1024];

	DWORD dwServiceType;
	CString strServer;
	CString strObject;
	INTERNET_PORT nPort;
};

