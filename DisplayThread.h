// *****************************************************************************
//
//     Copyright (c) 2007, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#pragma once

#include "resource.h"

#include <PvDisplayThread.h>
#include <PvDisplayWnd.h>


class DisplayThread : public PvDisplayThread
{
public:

    DisplayThread( PvDisplayWnd *aDisplayWnd , int64_t nCh);
    ~DisplayThread();

protected:

    void OnBufferRetrieved( PvBuffer *aBuffer );
    void OnBufferDisplay( PvBuffer *aBuffer );
    void OnBufferDone( PvBuffer *aBuffer );

private:

    PvDisplayWnd *mDisplayWnd;
	int64_t mCh;
};
