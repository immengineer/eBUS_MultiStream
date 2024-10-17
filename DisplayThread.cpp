// *****************************************************************************
//
//     Copyright (c) 2007, Pleora Technologies Inc., All rights reserved.
//
// *****************************************************************************

#include "stdafx.h"

#include "DisplayThread.h"


///
/// \brief Constructor
///

DisplayThread::DisplayThread( PvDisplayWnd *aDisplayWnd, int64_t nCh )
    : mDisplayWnd( aDisplayWnd ), mCh(nCh)
{
}


///
/// \brief Destructor
///

DisplayThread::~DisplayThread()
{
}


///
/// \brief Callback from PvDisplayThread
///

void DisplayThread::OnBufferDisplay( PvBuffer *aBuffer )
{
    mDisplayWnd->Display( *aBuffer );
}


///
/// \brief Callback from PvDisplayThread
///

void DisplayThread::OnBufferRetrieved( PvBuffer *aBuffer )
{
    PVUNREFPARAM( aBuffer );
}


///
/// \brief Callback from PvDisplayThread
///

void DisplayThread::OnBufferDone( PvBuffer *aBuffer )
{
    PVUNREFPARAM( aBuffer );
}



