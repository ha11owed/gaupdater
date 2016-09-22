
// GaUpdaterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GaUpdater.h"
#include "GaUpdaterDlg.h"
#include "afxdialogex.h"
#include "UpdaterHelper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGaUpdaterDlg dialog


CGaUpdaterDlg::CGaUpdaterDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CGaUpdaterDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGaUpdaterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NAME, m_txtName);
	DDX_Control(pDX, IDC_INSTALLED_VERSION, m_txtInstalledVersion);
	DDX_Control(pDX, IDC_LATEST_VERSION, m_txtLatestVersion);
	DDX_Control(pDX, IDC_RELEASE_DATE, m_txtReleaseDate);
	DDX_Control(pDX, IDC_UPDATE_TEXT, m_rtbDescription);
	DDX_Control(pDX, IDOK, m_btnUpdate);
	DDX_Control(pDX, IDC_DOWNLOAD_PROGRESS, m_progressBar);
	DDX_Control(pDX, IDC_STATIC_RELEASE_NOTES, m_lblReleaseNotes);
}

BEGIN_MESSAGE_MAP(CGaUpdaterDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CGaUpdaterDlg::OnBnClickedOk)
	ON_MESSAGE(UWM_DOWNLOADPROGRESS, &CGaUpdaterDlg::OnUwmDownloadprogress)
END_MESSAGE_MAP()


// CGaUpdaterDlg message handlers
#define PROGRESS_BAR_MAX 1000

BOOL CGaUpdaterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// We just want to command line arguments, MFC makes this harder...
	CString name;
	CString url;
	CString filePath;
	CString installArgs;

	int sw = -1;
	for (int i = 1; i < __argc; i++)
	{
		char* val = __argv[i];
		switch (sw) {
		case 1:
			// url
			url = val;
			sw = -1;
			break;
		case 2:
			// file path
			filePath = val;
			sw = -1;
			break;
		case 3:
			installArgs.AppendChar(_T('"'));
			installArgs.Append(CString(val));
			installArgs.AppendChar(_T('"'));
			installArgs.AppendChar(_T(' '));
			// args
			break;
		case 4:
			// name
			name = val;
			sw = -1;
			break;
		default:
			if (strcmp(val, "-url") == 0) {
				sw = 1;
			}
			else if (strcmp(val, "-filePath") == 0) {
				sw = 2;
			}
			else if (strcmp(val, "-installArgs") == 0) {
				sw = 3;
			}
			else if (strcmp(val, "-name") == 0) {
				sw = 4;
			}
		}
	}

#ifdef _DEBUG
	if (url.IsEmpty()) {
		url = _T("http://localhost:8080/alghe/javax.faces.resource/xml/updates.xml");
	}
	if (filePath.IsEmpty()) {
		filePath = _T("C:\\Program Files (x86)\\Mozilla Firefox\\firefox.exe");
	}
	if (name.IsEmpty()) {
		name = _T("<TestName>");
	}
#endif

	auto updateProgress = [](void * ctx, double progress) {
		CGaUpdaterDlg * window = (CGaUpdaterDlg*)ctx;
		int step = (int)ceil(progress * PROGRESS_BAR_MAX);
		window->PostMessage(UWM_DOWNLOADPROGRESS, 0, step);
	};

	auto complete = [](void * ctx, bool ok) {
		CGaUpdaterDlg * window = (CGaUpdaterDlg*)ctx;
		window->PostMessage(UWM_DOWNLOADPROGRESS, 1, ok ? TRUE : FALSE);
	};

	helper.init(name, url, filePath, installArgs, updateProgress, complete, this);

	FileVersion& fileVersion = helper.fileVersion();
	CString str;
	str.Format(_T("%d.%d.%d.%d"), fileVersion.V1, fileVersion.V2, fileVersion.V3, fileVersion.V4);
	m_txtInstalledVersion.SetWindowText(str);


	// Get latest version info
	updateVersionInfo();
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGaUpdaterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGaUpdaterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CGaUpdaterDlg::OnBnClickedOk()
{
	m_btnUpdate.EnableWindow(FALSE);
	m_progressBar.SetRange(0, PROGRESS_BAR_MAX);
	m_progressBar.ShowWindow(SW_SHOW);

	bool ok = false;

	auto asyncFunc = [](LPVOID p) -> UINT
	{
		CGaUpdaterDlg* sender = (CGaUpdaterDlg*)p;
		sender->doDownload();
		return 0;
	};
	AfxBeginThread(asyncFunc, this);
}

void CGaUpdaterDlg::doDownload()
{
	helper.downloadUpdate();
}

void CGaUpdaterDlg::updateVersionInfo()
{
	CString str;
	bool ok = helper.downloadUpdateInfo();
	UpdateInfo& updateInfo = helper.updateInfo();

	CString releaseTxt;
	if (ok)
	{
		m_txtName.SetWindowText(updateInfo.name);

		str.Format(_T("%d.%d.%d.%d"), updateInfo.version.V1, updateInfo.version.V2, updateInfo.version.V3, updateInfo.version.V4);
		m_txtLatestVersion.SetWindowText(str);

		if (updateInfo.releaseDate == INVALID_DATE)
		{
			COleDateTime releaseDate(updateInfo.releaseDate);
			m_txtReleaseDate.SetWindowText(releaseDate.Format(VAR_DATEVALUEONLY));
		}
		else
		{
			m_txtReleaseDate.SetWindowText(_T("-"));
		}

		m_rtbDescription.EnableWindow(TRUE);
		m_rtbDescription.SetWindowText(updateInfo.details);

		FileVersion& oldVersion = helper.fileVersion();
		FileVersion& newVersion = helper.updateInfo().version;
		if (newVersion.isValid() && newVersion.compareTo(oldVersion) > 0)
		{
			releaseTxt.LoadString(IDS_UPDATE_AVAILABLE);
			m_btnUpdate.EnableWindow(TRUE);
		}
		else
		{
			releaseTxt.LoadString(IDS_NO_UPDATE_AVAILABLE);
			m_btnUpdate.EnableWindow(FALSE);
		}

	}
	else
	{
		releaseTxt.LoadString(IDS_CANNOT_CONNECT_TO_SERVER);
		m_txtName.SetWindowText(helper.getName());
		m_txtLatestVersion.SetWindowText(_T("-"));
		m_txtReleaseDate.SetWindowText(_T("-"));
		m_rtbDescription.SetWindowText(_T(""));
		m_rtbDescription.EnableWindow(FALSE);
		m_btnUpdate.EnableWindow(FALSE);
	}

	m_lblReleaseNotes.SetWindowText(releaseTxt);
}

afx_msg LRESULT CGaUpdaterDlg::OnUwmDownloadprogress(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
	{
		// Handle the progress bar
		int step = (int)lParam;
		m_progressBar.OffsetPos(step);

	}
	else
	{
		BOOL ok = (BOOL)lParam;
		if (helper.getDownloads().IsEmpty())
		{
			MessageBox(_T("Could not download the update"), _T("Error"), MB_OK);
		}
		else
		{
			CString filePath = helper.getDownloads().GetTail();
			ok = helper.startInstaller(filePath, helper.getCmdParams());
			if (!ok)
			{
				MessageBox(_T("Could not start the installer"), _T("Error"), MB_OK);
			}
		}

		if (ok)
		{
			CDialogEx::OnOK();
		}

		m_progressBar.ShowWindow(SW_HIDE);
		m_btnUpdate.EnableWindow(TRUE);
	}

	return 0;
}
