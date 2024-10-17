#pragma once
#include <PvSampleUtils.h>
#include <PvDeviceGEV.h>
#include <PvDeviceU3V.h>
#include <PvPipeline.h>
#include <PvBuffer.h>
#include <PvStream.h>
#include <PvStreamGEV.h>
#include <PvStreamU3V.h>
#include <PvGenStateStack.h>

class Source
{
public:
	Source(PvDevice *aDevice, const PvString &aConnectionID, const int32_t nCh);
	~Source();
	bool Open();
	void Close();
	void StartAcquisition();
	void StopAcquisition();
	CString GetFrameRate();

	PvDevice *mDevice;
	PvStream *mStream;
	PvPipeline *mPipeline;

	PvString mConnectionID;
	int32_t mSourceCh;

protected:
	void SelectSource(PvGenStateStack *aStack);
};