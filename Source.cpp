#include "stdafx.h"
#include "Source.h"
#include "SimpleGUIApplicationDlg.h"

using namespace std;

PV_INIT_SIGNAL_HANDLER();
#define BUFFER_COUNT ( 16 )

//
// Simple class used to control a source and get images from it
//  >> modifiyed from MultiSource.cpp (MultiSource sample)
//

// Constructor
Source::Source(PvDevice *aDevice, const PvString &aConnectionID, const int32_t nCh)
	: mDevice(aDevice)
	, mStream(NULL)
	, mPipeline(NULL)
	, mConnectionID(aConnectionID)
	, mSourceCh(nCh)
{

}

// Destructor
Source::~Source()
{
	PVDELETE(mPipeline);
	PVDELETE(mStream);
}
// Open source: opens the stream, set destination, starts pipeline
bool Source::Open()
{
	// Select source (if applicable)
	PvGenStateStack lStack(mDevice->GetParameters());
	SelectSource(&lStack);

	// Create and open stream, use same protocol as PvDevice. PvStream::CreateAndOpen is not used as we
	// need to specify additional parameters like streaming channel
	PvDeviceGEV *lDeviceGEV = dynamic_cast<PvDeviceGEV *>(mDevice);
	PvResult lResult;
	if (lDeviceGEV != NULL)
	{
		// Create and open stream
		PvStreamGEV *lStreamGEV = new PvStreamGEV;
		lResult = lStreamGEV->Open(mConnectionID, 0, mSourceCh);
		if (!lResult.IsOK())
		{
			TRACE("**** Error opening stream to GigE Vision device\n");
			return false;
		}
		// Save pointer to stream object
		mStream = lStreamGEV;

		// Get stream local information
		PvString lLocalIP = lStreamGEV->GetLocalIPAddress();
		uint16_t lLocalPort = lStreamGEV->GetLocalPort();

		// Set source destination on GigE Vision device
		lDeviceGEV->SetStreamDestination(lLocalIP, lLocalPort, mSourceCh);

		// Reading payload size from device
		int64_t lSize = mDevice->GetPayloadSize();

		// Dynamically allocate pipeline (required PvStream pointer)
		mPipeline = new PvPipeline(mStream);

		// Set the Buffer size and the Buffer count
		mPipeline->SetBufferSize(static_cast<uint32_t>(lSize));
		mPipeline->SetBufferCount(BUFFER_COUNT); // Increase for high frame rate without missing block IDs

		// Start pipeline thread
		//mPipeline->Start();		‚Ü‚¾Start‚³‚¹‚È‚¢
		return true;
	}
	else
		return false;
}
// Close source: close the stream, pipeline
void Source::Close()
{
	if (mPipeline != NULL) {
		if (mPipeline->IsStarted())
			mPipeline->Stop();
		delete mPipeline;
		mPipeline = NULL;
	}
	if (mStream != NULL) {
		if (mStream->IsOpen())
			mStream->Close();
		delete mStream;
		mStream = NULL;
	}
}
void Source::StartAcquisition()
{
	// Select source (if applicable)
	PvGenStateStack lStack(mDevice->GetParameters());
	SelectSource(&lStack);

	// Enables stream before sending the AcquisitionStart command.
	mDevice->StreamEnable();
	mDevice->GetParameters()->ExecuteCommand("AcquisitionStart");
}
void Source::StopAcquisition()
{
	// Select source (if applicable)
	PvGenStateStack lStack(mDevice->GetParameters());
	SelectSource(&lStack);

	// The pipeline is already "armed", we just have to tell the device
	// to start sending us images
	mDevice->GetParameters()->ExecuteCommand("AcquisitionStop");

	// Disable stream after sending the AcquisitionStop command.
	mDevice->StreamDisable();
}
void Source::SelectSource(PvGenStateStack *aStack)
{
	// If no source defined, there is likely no source selector, nothing to select
	if (mSourceCh < 0 || mSourceCh >= MAX_STREAM_CHANNEL)
		return;	

	// Select source. When stack goes out of scope, the previous value will be restored
	aStack->SetEnumValue("SourceSelector", (int64_t)mSourceCh);
}

CString Source::GetFrameRate()
{
	// Get frame rate
	double lFPS = 0.0;
	mStream->GetParameters()->GetFloatValue("AcquisitionRate", lFPS);

	// Get bandwidth, convert in Mb/s
	double lBandwidth = 0.0;
	mStream->GetParameters()->GetFloatValue("Bandwidth", lBandwidth);
	lBandwidth /= 1000000.0;

	CString cstrValue;
	cstrValue.Format(_T("%.2f FPS   %.2f Mb/s "), lFPS, lBandwidth);
	return cstrValue;
}