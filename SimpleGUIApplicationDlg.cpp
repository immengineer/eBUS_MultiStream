// *****************************************************************************
//
//     Copyright (c) 2007, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#include "stdafx.h"

#include "SimpleGUIApplication.h"
#include "SimpleGUIApplicationDlg.h"
#include "Source.h"
#include <PvGenStateStack.h>
#include <string.h>


#define DEFAULT_PAYLOAD_SIZE ( 1920 * 1080 * 2 )

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


BEGIN_MESSAGE_MAP(SimpleGUIApplicationDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_SIZE()
    ON_WM_GETMINMAXINFO()
    ON_BN_CLICKED(IDC_DEVICE_BUTTON, &SimpleGUIApplicationDlg::OnBnClickedDeviceButton)
    ON_BN_CLICKED(IDC_LINK_BUTTON, &SimpleGUIApplicationDlg::OnBnClickedLinkButton)
    ON_BN_CLICKED(IDC_STREAMPARAMS_BUTTON, &SimpleGUIApplicationDlg::OnBnClickedStreamparamsButton)
	ON_BN_CLICKED(IDC_STREAMPARAMS_BUTTON2, &SimpleGUIApplicationDlg::OnBnClickedStreamparamsButton2)
	ON_BN_CLICKED(IDC_STREAMPARAMS_BUTTON3, &SimpleGUIApplicationDlg::OnBnClickedStreamparamsButton3)
    ON_BN_CLICKED(IDC_CONNECT_BUTTON, &SimpleGUIApplicationDlg::OnBnClickedConnectButton)
    ON_BN_CLICKED(IDC_DISCONNECT_BUTTON, &SimpleGUIApplicationDlg::OnBnClickedDisconnectButton)
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_START, &SimpleGUIApplicationDlg::OnBnClickedStart)
    ON_BN_CLICKED(IDC_STOP, &SimpleGUIApplicationDlg::OnBnClickedStop)
    ON_CBN_SELCHANGE(IDC_MODE, &SimpleGUIApplicationDlg::OnCbnSelchangeMode)
    ON_BN_CLICKED(IDC_DEVICEEVENTS_BUTTON, &SimpleGUIApplicationDlg::OnBnClickedDeviceEvents)
    ON_WM_MOVE()
    ON_WM_CTLCOLOR()
	ON_WM_TIMER()
END_MESSAGE_MAP()
const UINT SimpleGUIApplicationDlg::m_nDisplayID[MAX_STREAM_CHANNEL] = { IDC_DISPLAYPOS, IDC_DISPLAYPOS2, IDC_DISPLAYPOS3 };
const UINT SimpleGUIApplicationDlg::m_nGroupID[MAX_STREAM_CHANNEL] = { IDC_DISPLAY_GROUP, IDC_DISPLAY_GROUP2, IDC_DISPLAY_GROUP3 };
const UINT SimpleGUIApplicationDlg::m_nStatusID[MAX_STREAM_CHANNEL] = { IDC_STATUS, IDC_STATUS2, IDC_STATUS3 };
const UINT SimpleGUIApplicationDlg::m_nButtonID[MAX_STREAM_CHANNEL] = { IDC_STREAMPARAMS_BUTTON, IDC_STREAMPARAMS_BUTTON2, IDC_STREAMPARAMS_BUTTON3 };
// =============================================================================
SimpleGUIApplicationDlg::SimpleGUIApplicationDlg( CWnd* pParent /* =NULL */ )
    : CDialog( SimpleGUIApplicationDlg::IDD, pParent )
    , mNeedInit( TRUE )
    , mDeviceWnd( NULL )
    , mCommunicationWnd( NULL )
	, mDevice( NULL )
{
    m_hIcon = AfxGetApp()->LoadIcon( IDR_MAINFRAME );

	// Initialize for all StreamChannel
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		mStreamParametersWnd[nCh] = NULL;
		mDisplayThread[nCh] = NULL;
		mSource[nCh] = NULL;
		m_bIsChEnable[nCh] = false;
	}
	// Create display thread
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		mDisplayThread[nCh] = new DisplayThread(&mDisplay[nCh], nCh);
	}
}
// =============================================================================
SimpleGUIApplicationDlg::~SimpleGUIApplicationDlg()
{
	if (mDevice != NULL) {
		for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
			if (mDisplayThread[nCh] != NULL) {
				delete mDisplayThread[nCh];
				mDisplayThread[nCh] = NULL;
			}
			if (mSource[nCh] != NULL) {
				mSource[nCh]->Close();
				delete mSource[nCh];
				mSource[nCh] = NULL;
			}
		}
		TRACE("~SimpleGUIApplicationDlg() Called!!\n");
		PvDevice::Free(mDevice);
		mDevice = NULL;
	}
}
// =============================================================================
void SimpleGUIApplicationDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_MODE, mModeCombo);
    DDX_Control(pDX, IDC_START, mPlayButton);
    DDX_Control(pDX, IDC_STOP, mStopButton);
    DDX_Control(pDX, IDC_IP_EDIT, mIPEdit);
    DDX_Control(pDX, IDC_MAC_EDIT, mMACEdit);
    DDX_Control(pDX, IDC_GUID_EDIT, mGUIDEdit);
    DDX_Control(pDX, IDC_MODEL_EDIT, mModelEdit);
    DDX_Control(pDX, IDC_MANUFACTURER_EDIT, mManufacturerEdit);
    DDX_Control(pDX, IDC_NAME_EDIT, mNameEdit);

	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		DDX_Control(pDX, m_nStatusID[nCh], mStatus[nCh]);
	}
}


// =============================================================================
BOOL SimpleGUIApplicationDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    SetWindowText( _T( "SimpleGUIApplication for MultiStream" ) );

    GetClientRect( mCrt );
    mNeedInit = FALSE;

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon

	// Init Display
	uint32_t uiVal = 10000;
	CRect lDisplayRect;
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		GetDlgItem(m_nDisplayID[nCh])->GetClientRect(&lDisplayRect);
		GetDlgItem(m_nDisplayID[nCh])->ClientToScreen(&lDisplayRect);
		ScreenToClient(&lDisplayRect);
		mDisplay[nCh].Create(GetSafeHwnd(), 10000 + nCh);
		mDisplay[nCh].SetPosition(lDisplayRect.left, lDisplayRect.top, lDisplayRect.Width(), lDisplayRect.Height());
		mDisplay[nCh].SetBackgroundColor(0x80, 0x80, 0x80);
	}   
    EnableInterface();

	mTimer = 0;

    return TRUE;  // return TRUE  unless you set the focus to a control
}
// =============================================================================
void SimpleGUIApplicationDlg::SetupControl()
{

}

// =============================================================================
void SimpleGUIApplicationDlg::OnPaint()
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
        CDialog::OnPaint();
    }
}


// =============================================================================
HCURSOR SimpleGUIApplicationDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


// =============================================================================
void SimpleGUIApplicationDlg::OnClose()
{
    // Make sure we cleanup before we leave
    Disconnect();
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		if (mDisplayThread[nCh] != NULL) {
			delete mDisplayThread[nCh];
			mDisplayThread[nCh] = NULL;
		}
	}

    CDialog::OnClose();
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedConnectButton()
{
    // create a device finder wnd and open the select device dialog
    PvDeviceFinderWnd lFinder;
    PvResult lResult = lFinder.ShowModal();

    if ( lResult.GetCode() == PvResult::Code::ABORTED )
    {
        MessageBox( _T( "Invalid selection. Please select a device." ), _T("SimpleGUIApplication") );
        return;
    }

    if ( !lResult.IsOK() || ( lFinder.GetSelected() == NULL ) )
    {
        return;
    }

    CWaitCursor lCursor;

    UpdateWindow();

    const PvDeviceInfo* lDeviceInfo = lFinder.GetSelected();

    if ( lDeviceInfo != NULL )
    {
        Connect( lDeviceInfo );
    }
    else
    {
        MessageBox( _T( "No device selected." ), _T( "Error" ), 
            MB_OK | MB_ICONINFORMATION );
        return;
    }

}
// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedDisconnectButton()
{
    CWaitCursor lCursor;

    Disconnect();
}
// =============================================================================
void SimpleGUIApplicationDlg::EnableInterface()
{
    // This method can be called really early or late when the window is not created
    if ( GetSafeHwnd() == 0 )
    {
        return;
    }

    bool lConnected = ( mDevice != NULL ) && mDevice->IsConnected();

    GetDlgItem( IDC_CONNECT_BUTTON )->EnableWindow( !lConnected );
    GetDlgItem( IDC_DISCONNECT_BUTTON )->EnableWindow( lConnected );

    GetDlgItem( IDC_COMMUNICATION_BUTTON )->EnableWindow( lConnected ); 
    GetDlgItem( IDC_DEVICE_BUTTON )->EnableWindow( lConnected );
    GetDlgItem( IDC_STREAMPARAMS_BUTTON )->EnableWindow( lConnected );

	mPlayButton.EnableWindow(lConnected);
	mStopButton.EnableWindow(lConnected);

    // If not connected, disable the acquisition mode control. If enabled,
    // it will be managed automatically by events from the GenICam parameters
    if ( !lConnected )
    {
        mModeCombo.EnableWindow( FALSE );
    }
	
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		BOOL bEnable = m_bIsChEnable[nCh] ? TRUE : FALSE;
		GetDlgItem(m_nGroupID[nCh])->EnableWindow(bEnable);
		GetDlgItem(m_nDisplayID[nCh])->EnableWindow(bEnable);
		GetDlgItem(m_nStatusID[nCh])->EnableWindow(bEnable);
		GetDlgItem(m_nButtonID[nCh])->EnableWindow(bEnable);

		CString cstrTitle;
		cstrTitle.Format(_T("Display Stream%d"), nCh);
		if (bEnable)
			GetDlgItem(m_nGroupID[nCh])->SetWindowText(cstrTitle);
	}
}


// =============================================================================
void SimpleGUIApplicationDlg::Connect( const PvDeviceInfo *aDI )
{
    ASSERT( aDI != NULL );
    if ( aDI == NULL )  
    {
        return;
    }

    const PvDeviceInfoGEV *lDIGEV = dynamic_cast<const PvDeviceInfoGEV *>( aDI );
    const PvDeviceInfoU3V *lDIU3V = dynamic_cast<const PvDeviceInfoU3V *>( aDI );

    // Just in case we came here still connected...
    Disconnect();

    // Device connection, packet size negotiation and stream opening
    PvResult lResult = PvResult::Code::NOT_CONNECTED;

    // Connect device
    mDevice = PvDevice::CreateAndConnect( aDI, &lResult );
    if ( !lResult.IsOK() )
    {
        Disconnect();
        return;
    }
	// This sample only supports GigE Vision devices
	if (aDI->GetType() == PvDeviceInfoTypeU3V) {
		MessageBox(_T("The selected device is not currently supported by this sample."), _T("Info"),
			MB_OK | MB_ICONINFORMATION);
		Disconnect();
		return;
	}
	// GigE Vision 
	int64_t lCount = 0;
	if (aDI->GetType() == PvDeviceInfoTypeGEV) {
		PvGenEnum *lSourceSelector = mDevice->GetParameters()->GetEnum("SourceSelector");
		if (lSourceSelector != NULL)
			lSourceSelector->GetEntriesCount(lCount);
		else
			lCount = 1;

		PvDeviceGEV *lDeviceGEV = dynamic_cast<PvDeviceGEV *>(mDevice);
		PvString lConnectionID = aDI->GetConnectionID();
		for (int32_t nCh = 0; nCh < lCount; nCh++) {
			mSource[nCh] = new Source(mDevice, lConnectionID, nCh);
			if (mSource[nCh]->Open()) {
				m_bIsChEnable[nCh] = true;
			}
			else {
				delete mSource[nCh];
				mSource[nCh] = NULL;
			}
		}
		// Register to all events of the parameters in the device's node map
		PvGenParameterArray *lGenDevice = mDevice->GetParameters();
		for (uint32_t i = 0; i < lGenDevice->GetCount(); i++)
		{
			lGenDevice->Get(i)->RegisterEventSink(this);
		}

		PvString lManufacturerStr = aDI->GetVendorName();
		PvString lModelNameStr = aDI->GetModelName();
		PvString lDeviceVersionStr = aDI->GetVersion();

		// GigE Vision only parameters
		PvString lIPStr = "N/A";
		PvString lMACStr = "N/A";
		if (lDIGEV != NULL)
		{
			// IP (GigE Vision only)
			lIPStr = lDIGEV->GetIPAddress();

			// MAC address (GigE Vision only)
			lMACStr = lDIGEV->GetMACAddress();
		}

		// USB3 Vision only parameters
		PvString lDeviceGUIDStr = "N/A";
		if (lDIU3V != NULL)
		{
			// Device GUID (USB3 Vision only)
			lDeviceGUIDStr = lDIU3V->GetDeviceGUID();
		}

		// Device name (User ID)
		PvString lNameStr = aDI->GetUserDefinedName();

		mManufacturerEdit.SetWindowText(lManufacturerStr);
		mModelEdit.SetWindowText(lModelNameStr);
		mIPEdit.SetWindowText(lIPStr);
		mMACEdit.SetWindowText(lMACStr);
		mGUIDEdit.SetWindowText(lDeviceGUIDStr);
		mNameEdit.SetWindowText(lNameStr);

		// Get acquisition mode GenICam parameter
		PvGenEnum *lMode = lGenDevice->GetEnum("AcquisitionMode");
		int64_t lEntriesCount = 0;
		lMode->GetEntriesCount(lEntriesCount);

		// Fill acquisition mode combo box
		mModeCombo.ResetContent();
		for (uint32_t i = 0; i < lEntriesCount; i++)
		{
			const PvGenEnumEntry *lEntry = NULL;
			lMode->GetEntryByIndex(i, &lEntry);

			if (lEntry->IsAvailable())
			{
				PvString lEEName;
				lEntry->GetName(lEEName);

				int64_t lEEValue;
				lEntry->GetValue(lEEValue);

				int lIndex = mModeCombo.AddString(lEEName.GetUnicode());
				mModeCombo.SetItemData(lIndex, static_cast<DWORD_PTR>(lEEValue));
			}
		}

		// Set mode combo box to value currently used by the device
		int64_t lValue = 0;
		lMode->GetValue(lValue);
		for (int i = 0; i < mModeCombo.GetCount(); i++)
		{
			if (lValue == static_cast<int64_t>(mModeCombo.GetItemData(i)))
			{
				mModeCombo.SetCurSel(i);
				break;
			}
		}

		// Force an update on all the parameters on acquisition mode
		OnParameterUpdate(lMode);

		// Ready image reception
		StartStreaming();
	}
    // Sync up UI
    EnableInterface();
}


// =============================================================================
void SimpleGUIApplicationDlg::Disconnect()
{   
    if ( mDevice != NULL )
    {
        // Unregister all events of the parameters in the device's node map
        PvGenParameterArray *lGenDevice = mDevice->GetParameters();
        for ( uint32_t i = 0; i < lGenDevice->GetCount(); i++ )
        {
            lGenDevice->Get( i )->UnregisterEventSink( this );
        }
    }   
    
    // Close all configuration child windows
    CloseGenWindow( &mDeviceWnd );
    CloseGenWindow( &mCommunicationWnd );

	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		CloseGenWindow(&mStreamParametersWnd[nCh]);
	}

    // If streaming, stop streaming
    StopStreaming();

	if (mDevice != NULL) {
		for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
			if (mSource[nCh] != NULL) {
				mSource[nCh]->Close();
				delete mSource[nCh];
				mSource[nCh] = NULL;
			}
			m_bIsChEnable[nCh] = false;
		}
		PvDevice::Free(mDevice);
		mDevice = NULL;
	}

    // Reset device ID - can be called by the destructor when the window
    // no longer exists, be careful...
    if ( GetSafeHwnd() != 0 )
    {
        mManufacturerEdit.SetWindowText( _T( "" ) );
        mModelEdit.SetWindowText( _T( "" ) );
        mIPEdit.SetWindowText( _T( "" ) );
        mMACEdit.SetWindowText( _T( "" ) );
        mGUIDEdit.SetWindowText( _T( "" ) );
        mNameEdit.SetWindowText( _T( "" ) );
    }
    EnableInterface();
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedDeviceButton()
{
    if ( !mDevice->IsConnected() )
    {
        return;
    }

    ShowGenWindow( 
        &mDeviceWnd, 
        mDevice->GetParameters(), 
        _T( "Device Control" ) );
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedDeviceEvents()
{
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedLinkButton()
{
    ShowGenWindow( 
        &mCommunicationWnd, 
        mDevice->GetCommunicationParameters(), 
        _T( "Communication Control" ) );
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedStreamparamsButton()
{
    if ( !mDevice->IsConnected() )
    {
        return;
    }
    
	if (mSource[0]->mStream != NULL) {
		if (mSource[0]->mStream->IsOpen()) {
			ShowGenWindow(
				&mStreamParametersWnd[0],
				mSource[0]->mStream->GetParameters(),
				_T("Image Stream Control 0"));
		}
	}
}
// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedStreamparamsButton2()
{
	if (!mDevice->IsConnected())
	{
		return;
	}

	if (mSource[1]->mStream != NULL) {
		if (mSource[1]->mStream->IsOpen()) {
			ShowGenWindow(
				&mStreamParametersWnd[1],
				mSource[1]->mStream->GetParameters(),
				_T("Image Stream Control 1"));
		}
	}
}
// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedStreamparamsButton3()
{
	if (!mDevice->IsConnected())
	{
		return;
	}

	if (mSource[2]->mStream != NULL) {
		if (mSource[2]->mStream->IsOpen()) {
			ShowGenWindow(
				&mStreamParametersWnd[2],
				mSource[2]->mStream->GetParameters(),
				_T("Image Stream Control 2"));
		}
	}
}

// =============================================================================
void SimpleGUIApplicationDlg::ShowGenWindow( PvGenBrowserWnd **aWnd, PvGenParameterArray *aParams, const CString &aTitle )
{
    if ( ( *aWnd ) != NULL )
    {
        if ( ( *aWnd )->GetHandle() != 0 )
        {
            CWnd lWnd;
            lWnd.Attach( ( *aWnd )->GetHandle() );

            // Window already visible, give it focus and bring it on top
            lWnd.BringWindowToTop();
            lWnd.SetFocus();

            lWnd.Detach();
            return;
        }

        // Window object exists but was closed/destroyed. Free it before re-creating
        CloseGenWindow( aWnd );
    }

    // Create, assign parameters, set title and show modeless
    ( *aWnd ) = new PvGenBrowserWnd;
    ( *aWnd )->SetTitle( PvString( aTitle ) );
    ( *aWnd )->SetGenParameterArray( aParams );
    ( *aWnd )->ShowModeless( GetSafeHwnd() );
}


// =============================================================================
void SimpleGUIApplicationDlg::CloseGenWindow( PvGenBrowserWnd **aWnd )
{
    // If the window object does not even exist, do nothing
    if ( ( *aWnd ) == NULL )
    {
        return;
    }

    // If the window object exists and is currently created (visible), close/destroy it
    if ( ( *aWnd )->GetHandle() != 0 )
    {
        ( *aWnd )->Close();
    }

    // Finally, release the window object
    delete ( *aWnd );
    ( *aWnd ) = NULL;
}
// =============================================================================
void SimpleGUIApplicationDlg::StartStreaming()
{
	//// Start threads
	//mDisplayThread->Start(mPipeline, mDevice->GetParameters());
	//mDisplayThread->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);

	//// Start pipeline
	//mPipeline->Start();
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		if (m_bIsChEnable[nCh] && mSource[nCh] != NULL) {
			mDisplayThread[nCh]->Start(mSource[nCh]->mPipeline, mDevice->GetParameters());
			mDisplayThread[nCh]->SetPriority(THREAD_PRIORITY_ABOVE_NORMAL);
			mSource[nCh]->mPipeline->Start();
		}
	}
}


// =============================================================================
void SimpleGUIApplicationDlg::StopStreaming()
{
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		if (mDisplayThread[nCh] != NULL)
			mDisplayThread[nCh]->Stop(false);

		if (mSource[nCh] != NULL && mSource[nCh]->mPipeline != NULL && mSource[nCh]->mPipeline->IsStarted())
			mSource[nCh]->mPipeline->Stop();
						
		if (mDisplayThread[nCh] != NULL)
			mDisplayThread[nCh]->WaitComplete();	
	}
}
// =============================================================================
void SimpleGUIApplicationDlg::StartAcquisition()
{
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		if (m_bIsChEnable[nCh] && mSource[nCh] != NULL)
			// Start Acquisition
			mSource[nCh]->StartAcquisition();
	}
	// Start Timer
	mTimer = SetTimer(1, 200, NULL);
}
// =============================================================================
void SimpleGUIApplicationDlg::StopAcquisition()
{
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		if (m_bIsChEnable[nCh] && mSource[nCh] != NULL)
			mSource[nCh]->StopAcquisition();
	}
	// Stop Timer
	if (mTimer != 0) {
		KillTimer(1);
		mTimer = 0;
	}
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedStart()
{
    if ( !mDevice->IsConnected() )
    {
        return;
    }
    StartAcquisition();
    EnableInterface();
	for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
		mStatus[nCh].SetWindowText(_T(""));
	}
}


// =============================================================================
void SimpleGUIApplicationDlg::OnBnClickedStop()
{
    if ( !mDevice->IsConnected() )
    {
        return;
    }   
    StopAcquisition();
    EnableInterface();
}


// =============================================================================
void SimpleGUIApplicationDlg::OnCbnSelchangeMode()
{
    if ( !mDevice->IsConnected() )
    {
        return;
    }

    if ( mModeCombo.GetCurSel() < 0 )
    {
        return;
    }

    PvGenParameterArray *lDeviceParams = mDevice->GetParameters();

    uint64_t lValue = mModeCombo.GetItemData( mModeCombo.GetCurSel() );
    PvResult lResult = lDeviceParams->SetEnumValue( "AcquisitionMode", lValue );
    if ( !lResult.IsOK() )
    {
        MessageBox( _T( "Unable to set AcquisitionMode value." ), _T( "Error" ), 
            MB_OK | MB_ICONINFORMATION );
    }
}


// =============================================================================
void SimpleGUIApplicationDlg::OnParameterUpdate( PvGenParameter *aParameter )
{
    PvString lName;
    aParameter->GetName( lName );

    if ( ( lName == "AcquisitionMode" ) &&
         ( mModeCombo.GetSafeHwnd() != 0 ) )
    {
        bool lAvailable = false, lWritable = false;
        aParameter->IsAvailable( lAvailable );
        if ( lAvailable )
        {
            aParameter->IsWritable( lWritable );
        }

        mModeCombo.EnableWindow( lAvailable && lWritable );

        PvGenEnum *lEnum = dynamic_cast<PvGenEnum *>( aParameter );
        if ( lEnum != NULL )
        {
            int64_t lEEValue = 0;
            lEnum->GetValue( lEEValue );

            for ( int i = 0; i < mModeCombo.GetCount(); i++ )
            {
                DWORD_PTR lData = mModeCombo.GetItemData( i );
                if ( static_cast<int64_t>( lData ) == lEEValue )
                {
                    mModeCombo.SetCurSel( i );
                    break;
                }
            }
        }
    }
}


// =============================================================================
void SimpleGUIApplicationDlg::OnMove(int x, int y)
{
    CDialog::OnMove(x, y);
}
// ==============================================================================
void SimpleGUIApplicationDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) {
		if (mDevice->IsConnected()) {
			for (int nCh = 0; nCh < MAX_STREAM_CHANNEL; nCh++) {
				if (m_bIsChEnable[nCh] && mSource[nCh] != NULL) {
					CString cstrText = mSource[nCh]->GetFrameRate();
					mStatus[nCh].SetWindowText(cstrText);
				}
			}
		}
	}

	CDialog::OnTimer(nIDEvent);
}