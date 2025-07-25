/*****************************************************************************
 * main_interface.cpp : Main interface
 ****************************************************************************
 * Copyright (C) 2006-2010 VideoLAN and AUTHORS
 * $Id: ddc6c1c1dc53b354068844419d18268422eff497 $
 *
 * Authors: Jean-Baptiste Kempf <jb@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/


#include "main_interface.hpp"

#include "input_manager.hpp"
#include "actions_manager.hpp"

#ifdef WIN32
 #include <QBitmap>
 #include <vlc_windows_interfaces.h>

#define WM_APPCOMMAND 0x0319

#define APPCOMMAND_VOLUME_MUTE            8
#define APPCOMMAND_VOLUME_DOWN            9
#define APPCOMMAND_VOLUME_UP              10
#define APPCOMMAND_MEDIA_NEXTTRACK        11
#define APPCOMMAND_MEDIA_PREVIOUSTRACK    12
#define APPCOMMAND_MEDIA_STOP             13
#define APPCOMMAND_MEDIA_PLAY_PAUSE       14
#define APPCOMMAND_LAUNCH_MEDIA_SELECT    16
#define APPCOMMAND_BASS_DOWN              19
#define APPCOMMAND_BASS_BOOST             20
#define APPCOMMAND_BASS_UP                21
#define APPCOMMAND_TREBLE_DOWN            22
#define APPCOMMAND_TREBLE_UP              23
#define APPCOMMAND_MICROPHONE_VOLUME_MUTE 24
#define APPCOMMAND_MICROPHONE_VOLUME_DOWN 25
#define APPCOMMAND_MICROPHONE_VOLUME_UP   26
#define APPCOMMAND_DICTATE_OR_COMMAND_CONTROL_TOGGLE    43
#define APPCOMMAND_MIC_ON_OFF_TOGGLE      44
#define APPCOMMAND_MEDIA_PLAY             46
#define APPCOMMAND_MEDIA_PAUSE            47
#define APPCOMMAND_MEDIA_RECORD           48
#define APPCOMMAND_MEDIA_FAST_FORWARD     49
#define APPCOMMAND_MEDIA_REWIND           50
#define APPCOMMAND_MEDIA_CHANNEL_UP       51
#define APPCOMMAND_MEDIA_CHANNEL_DOWN     52

#define FAPPCOMMAND_MOUSE 0x8000
#define FAPPCOMMAND_KEY   0
#define FAPPCOMMAND_OEM   0x1000
#define FAPPCOMMAND_MASK  0xF000

#define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#define GET_DEVICE_LPARAM(lParam)     ((WORD)(HIWORD(lParam) & FAPPCOMMAND_MASK))
#define GET_MOUSEORKEY_LPARAM         GET_DEVICE_LPARAM
#define GET_FLAGS_LPARAM(lParam)      (LOWORD(lParam))
#define GET_KEYSTATE_LPARAM(lParam)   GET_FLAGS_LPARAM(lParam)

void MainInterface::createTaskBarButtons()
{
    taskbar_wmsg = WM_NULL;
    /*Here is the code for the taskbar thumb buttons
    FIXME:We need pretty buttons in 16x16 px that are handled correctly by masks in Qt
    FIXME:the play button's picture doesn't changed to pause when clicked
    */

    CoInitialize( 0 );

    if( S_OK == CoCreateInstance( &clsid_ITaskbarList,
                NULL, CLSCTX_INPROC_SERVER,
                &IID_ITaskbarList3,
                (void **)&p_taskbl) )
    {
        p_taskbl->vt->HrInit(p_taskbl);

        if(himl = ImageList_Create( 20, //cx
                        20, //cy
                        ILC_COLOR32,//flags
                        4,//initial nb of images
                        0//nb of images that can be added
                        ))
        {
            QPixmap img   = QPixmap(":/win7/prev");
            QPixmap img2  = QPixmap(":/win7/pause");
            QPixmap img3  = QPixmap(":/win7/play");
            QPixmap img4  = QPixmap(":/win7/next");
            QBitmap mask  = img.createMaskFromColor(Qt::transparent);
            QBitmap mask2 = img2.createMaskFromColor(Qt::transparent);
            QBitmap mask3 = img3.createMaskFromColor(Qt::transparent);
            QBitmap mask4 = img4.createMaskFromColor(Qt::transparent);

            if(-1 == ImageList_Add(himl, img.toWinHBITMAP(QPixmap::PremultipliedAlpha),mask.toWinHBITMAP()))
                msg_Err( p_intf, "ImageList_Add failed" );
            if(-1 == ImageList_Add(himl, img2.toWinHBITMAP(QPixmap::PremultipliedAlpha),mask2.toWinHBITMAP()))
                msg_Err( p_intf, "ImageList_Add failed" );
            if(-1 == ImageList_Add(himl, img3.toWinHBITMAP(QPixmap::PremultipliedAlpha),mask3.toWinHBITMAP()))
                msg_Err( p_intf, "ImageList_Add failed" );
            if(-1 == ImageList_Add(himl, img4.toWinHBITMAP(QPixmap::PremultipliedAlpha),mask4.toWinHBITMAP()))
                msg_Err( p_intf, "ImageList_Add failed" );
        }

        // Define an array of two buttons. These buttons provide images through an
        // image list and also provide tooltips.
        DWORD dwMask = THB_BITMAP | THB_FLAGS;

        THUMBBUTTON thbButtons[3];
        thbButtons[0].dwMask = dwMask;
        thbButtons[0].iId = 0;
        thbButtons[0].iBitmap = 0;
        thbButtons[0].dwFlags = THBF_HIDDEN;

        thbButtons[1].dwMask = dwMask;
        thbButtons[1].iId = 1;
        thbButtons[1].iBitmap = 2;
        thbButtons[1].dwFlags = THBF_HIDDEN;

        thbButtons[2].dwMask = dwMask;
        thbButtons[2].iId = 2;
        thbButtons[2].iBitmap = 3;
        thbButtons[2].dwFlags = THBF_HIDDEN;

        HRESULT hr = p_taskbl->vt->ThumbBarSetImageList(p_taskbl, winId(), himl );
        if(S_OK != hr)
            msg_Err( p_intf, "ThumbBarSetImageList failed with error %08x", hr );
        else
        {
            hr = p_taskbl->vt->ThumbBarAddButtons(p_taskbl, winId(), 3, thbButtons);
            if(S_OK != hr)
                msg_Err( p_intf, "ThumbBarAddButtons failed with error %08x", hr );
        }
        CONNECT( THEMIM->getIM(), statusChanged( int ), this, changeThumbbarButtons( int ) );
    }
    else
    {
        himl = NULL;
        p_taskbl = NULL;
    }

}

bool MainInterface::winEvent ( MSG * msg, long * result )
{
    if (msg->message == taskbar_wmsg)
    {
        //We received the "taskbarbuttoncreated" message, now we can really create the buttons
        createTaskBarButtons();
    }

    short cmd;
    switch( msg->message )
    {
        case WM_COMMAND:
            if (HIWORD(msg->wParam) == THBN_CLICKED)
            {
                switch(LOWORD(msg->wParam))
                {
                    case 0:
                        THEMIM->prev();
                        break;
                    case 1:
                        THEMIM->togglePlayPause();
                        break;
                    case 2:
                        THEMIM->next();
                        break;
                }
            }
            break;
        case WM_APPCOMMAND:
            cmd = GET_APPCOMMAND_LPARAM(msg->lParam);
            switch(cmd)
            {
                case APPCOMMAND_MEDIA_PLAY_PAUSE:
                    THEMIM->togglePlayPause();
                    break;
                case APPCOMMAND_MEDIA_PLAY:
                    THEMIM->play();
                    break;
                case APPCOMMAND_MEDIA_PAUSE:
                    THEMIM->pause();
                    break;
                case APPCOMMAND_MEDIA_PREVIOUSTRACK:
                    THEMIM->prev();
                    break;
                case APPCOMMAND_MEDIA_NEXTTRACK:
                    THEMIM->next();
                    break;
                case APPCOMMAND_MEDIA_STOP:
                    THEMIM->stop();
                    break;
                case APPCOMMAND_VOLUME_DOWN:
                    THEAM->AudioDown();
                    break;
                case APPCOMMAND_VOLUME_UP:
                    THEAM->AudioUp();
                    break;
                case APPCOMMAND_VOLUME_MUTE:
                    THEAM->toggleMuteAudio();
                    break;
                default:
                     msg_Dbg( p_intf, "unknown APPCOMMAND = %d", cmd);
                     break;
            }
            break;
    }
    return false;
}
#endif

//moc doesn't know about #ifdef, so we have to build this method for every platform
void MainInterface::changeThumbbarButtons( int i_status)
{
#ifdef WIN32
    // Define an array of three buttons. These buttons provide images through an
    // image list and also provide tooltips.
    DWORD dwMask = THB_BITMAP | THB_FLAGS;

    THUMBBUTTON thbButtons[3];
    //prev
    thbButtons[0].dwMask = dwMask;
    thbButtons[0].iId = 0;
    thbButtons[0].iBitmap = 0;

    //play/pause
    thbButtons[1].dwMask = dwMask;
    thbButtons[1].iId = 1;

    //next
    thbButtons[2].dwMask = dwMask;
    thbButtons[2].iId = 2;
    thbButtons[2].iBitmap = 3;

    switch( i_status )
    {
        case PLAYING_S:
            {
                thbButtons[0].dwFlags = THBF_ENABLED;
                thbButtons[1].dwFlags = THBF_ENABLED;
                thbButtons[2].dwFlags = THBF_ENABLED;
                thbButtons[1].iBitmap = 1;
                break;
            }
        case PAUSE_S:
            {
                thbButtons[0].dwFlags = THBF_ENABLED;
                thbButtons[1].dwFlags = THBF_ENABLED;
                thbButtons[2].dwFlags = THBF_ENABLED;
                thbButtons[1].iBitmap = 2;
                break;
            }
        default:
            return;
    }
    HRESULT hr =  p_taskbl->vt->ThumbBarUpdateButtons(p_taskbl, this->winId(), 3, thbButtons);
    if(S_OK != hr)
        msg_Err( p_intf, "ThumbBarUpdateButtons failed with error %08x", hr );
#endif
}


