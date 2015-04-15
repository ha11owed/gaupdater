#include "stdafx.h"
#include "Hasher.h"
#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>
#include <tchar.h>

#include <stdio.h>
#include <windows.h>
#include <Wincrypt.h>

#define BUFSIZE (1024 * 4)
#define MD5LEN  16
#define SHA1LEN 20


template<DWORD THashLen, ALG_ID TAlgo>
CString HashGeneric(const CString& filePath)
{
	CString hash;

	DWORD dwStatus = 0;
	BOOL bResult = FALSE;
	// Logic to check usage goes here.

	HANDLE hFile = NULL;
	hFile = CreateFile(filePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_SEQUENTIAL_SCAN,
		NULL);

	if (INVALID_HANDLE_VALUE == hFile)
	{
		//dwStatus = GetLastError();
		return hash;
	}

	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;

	// Get handle to the crypto provider
	if (!CryptAcquireContext(&hProv,
		NULL,
		NULL,
		PROV_RSA_FULL,
		CRYPT_VERIFYCONTEXT))
	{
		//dwStatus = GetLastError();
		CloseHandle(hFile);
		return hash;
	}

	if (!CryptCreateHash(hProv, TAlgo, 0, 0, &hHash))
	{
		//dwStatus = GetLastError();
		CloseHandle(hFile);
		CryptReleaseContext(hProv, 0);
		return hash;
	}

	BYTE rgbFile[BUFSIZE];
	DWORD cbRead = 0;
	while (bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL))
	{
		if (0 == cbRead)
		{
			break;
		}

		if (!CryptHashData(hHash, rgbFile, cbRead, 0))
		{
			//dwStatus = GetLastError();
			CryptReleaseContext(hProv, 0);
			CryptDestroyHash(hHash);
			CloseHandle(hFile);
			return hash;
		}
	}

	if (!bResult)
	{
		//dwStatus = GetLastError();
		CryptReleaseContext(hProv, 0);
		CryptDestroyHash(hHash);
		CloseHandle(hFile);
		return hash;
	}

	DWORD cbHash = THashLen;
	BYTE rgbHash[THashLen];
	CHAR rgbDigits[] = "0123456789abcdef";
	if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
	{
		for (DWORD i = 0; i < cbHash; i++)
		{
			hash.AppendChar(rgbDigits[rgbHash[i] >> 4]);
			hash.AppendChar(rgbDigits[rgbHash[i] & 0xf]);
		}
	}
	else
	{
		//dwStatus = GetLastError();
	}

	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	CloseHandle(hFile);

	return hash;
}

CString HashMD5(const CString& filePath)
{
	return HashGeneric<MD5LEN, CALG_MD5>(filePath);
}

CString HashSHA1(const CString& filePath)
{
	return HashGeneric<SHA1LEN, CALG_SHA1>(filePath);
}
