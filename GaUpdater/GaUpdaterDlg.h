
// GaUpdaterDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "UpdaterHelper.h"
#include "CustomMessages.h"

// CGaUpdaterDlg dialog
class CGaUpdaterDlg : public CDialogEx
{
// Construction
public:
	CGaUpdaterDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_GAUPDATER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
public:
	void doDownload();

protected:
	void updateVersionInfo();
	CUpdaterHelper helper;
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CStatic m_txtInstalledVersion;
	CStatic m_txtLatestVersion;
	CStatic m_txtReleaseDate;
	CRichEditCtrl m_rtbDescription;
	CButton m_btnUpdate;
	afx_msg void OnBnClickedOk();
	CProgressCtrl m_progressBar;
protected:
	afx_msg LRESULT OnUwmDownloadprogress(WPARAM wParam, LPARAM lParam);
public:
	CStatic m_txtName;
	CStatic m_lblReleaseNotes;
};
