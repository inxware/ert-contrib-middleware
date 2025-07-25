/*****************************************************************************
 * simple_preferences.cpp : "Simple preferences"
 ****************************************************************************
 * Copyright (C) 2006-2008 the VideoLAN team
 * $Id: ecbc26ab07ef98ac4d37688d092ba3f00021dc5c $
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Antoine Cellerier <dionoea@videolan.org>
 *          Jean-Baptiste Kempf <jb@videolan.org>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "components/simple_preferences.hpp"
#include "components/preferences_widgets.hpp"

#include <vlc_config_cat.h>
#include <vlc_configuration.h>

#include <QString>
#include <QFont>
#include <QToolButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QScrollArea>

#include <QStyleFactory>
#include <QSettings>
#include <QtAlgorithms>

#define ICON_HEIGHT 64

#ifdef WIN32
# include <vlc_windows_interfaces.h>
#endif

/*********************************************************************
 * The List of categories
 *********************************************************************/
SPrefsCatList::SPrefsCatList( intf_thread_t *_p_intf, QWidget *_parent, bool small ) :
                                  QWidget( _parent ), p_intf( _p_intf )
{
    QVBoxLayout *layout = new QVBoxLayout();

    QButtonGroup *buttonGroup = new QButtonGroup( this );
    buttonGroup->setExclusive ( true );
    CONNECT( buttonGroup, buttonClicked ( int ),
            this, switchPanel( int ) );

    short icon_height = small ? ICON_HEIGHT /2 : ICON_HEIGHT;

#define ADD_CATEGORY( button, label, icon, numb )                           \
    QToolButton * button = new QToolButton( this );                         \
    button->setIcon( QIcon( ":/prefsmenu/" #icon ) );                   \
    button->setText( label );                                               \
    button->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );              \
    button->setIconSize( QSize( icon_height, icon_height ) );               \
    button->resize( icon_height + 6 , icon_height + 6 );                    \
    button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding) ;  \
    button->setAutoRaise( true );                                           \
    button->setCheckable( true );                                           \
    buttonGroup->addButton( button, numb );                                 \
    layout->addWidget( button );

    ADD_CATEGORY( SPrefsInterface, qtr("Interface"),
                  cone_interface_64, 0 );
    ADD_CATEGORY( SPrefsAudio, qtr("Audio"),
                  cone_audio_64, 1 );
    ADD_CATEGORY( SPrefsVideo, qtr("Video"),
                  cone_video_64, 2 );
    ADD_CATEGORY( SPrefsSubtitles, qtr("Subtitles && OSD"),
                  cone_subtitles_64, 3 );
    ADD_CATEGORY( SPrefsInputAndCodecs, qtr("Input && Codecs"),
                  cone_input_64, 4 );
    ADD_CATEGORY( SPrefsHotkeys, qtr("Hotkeys"),
                  cone_hotkeys_64, 5 );

#undef ADD_CATEGORY

    SPrefsInterface->setChecked( true );
    layout->setMargin( 0 );
    layout->setSpacing( 1 );

    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setLayout( layout );

}

void SPrefsCatList::switchPanel( int i )
{
    emit currentItemChanged( i );
}

/*********************************************************************
 * The Panels
 *********************************************************************/
SPrefsPanel::SPrefsPanel( intf_thread_t *_p_intf, QWidget *_parent,
                          int _number, bool small ) : QWidget( _parent ), p_intf( _p_intf )
{
    module_config_t *p_config;
    ConfigControl *control;
    number = _number;

#define CONFIG_GENERIC( option, type, label, qcontrol )                   \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, ui.qcontrol, false );         \
                controls.append( control );                               \
            }                                                             \
            else {                                                        \
                ui.qcontrol->setEnabled( false );                         \
                if( label ) label->setEnabled( false );                   \
            }

#define CONFIG_BOOL( option, qcontrol )                           \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new BoolConfigControl( VLC_OBJECT(p_intf),     \
                           p_config, NULL, ui.qcontrol, false );          \
                controls.append( control );                               \
            }                                                             \
            else { ui.qcontrol->setEnabled( false ); }


#define CONFIG_GENERIC_NO_UI( option, type, label, qcontrol )             \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, qcontrol, false );            \
                controls.append( control );                               \
            }                                                             \
            else {                                                        \
                QWidget *widget = label;                                  \
                qcontrol->setVisible( false );                            \
                if( widget ) widget->setEnabled( false );                 \
            }


#define CONFIG_GENERIC_NO_BOOL( option, type, label, qcontrol )           \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, ui.qcontrol );                \
                controls.append( control );                               \
            }

#define CONFIG_GENERIC_FILE( option, type, label, qcontrol, qbutton )     \
            p_config =  config_FindConfig( VLC_OBJECT(p_intf), option );  \
            if( p_config )                                                \
            {                                                             \
                control =  new type ## ConfigControl( VLC_OBJECT(p_intf), \
                           p_config, label, qcontrol, qbutton );          \
                controls.append( control );                               \
            }

#define START_SPREFS_CAT( name , label )    \
        case SPrefs ## name:                \
        {                                   \
            Ui::SPrefs ## name ui;      \
            ui.setupUi( panel );            \
            panel_label->setText( label );

#define END_SPREFS_CAT      \
            break;          \
        }

    QVBoxLayout *panel_layout = new QVBoxLayout();
    QWidget *panel = new QWidget();
    panel_layout->setMargin( 3 );

    // Title Label
    QLabel *panel_label = new QLabel;
    QFont labelFont = QApplication::font( static_cast<QWidget*>(0) );
    labelFont.setPointSize( labelFont.pointSize() + 6 );
    labelFont.setFamily( "Verdana" );
    panel_label->setFont( labelFont );

    // Title <hr>
    QFrame *title_line = new QFrame;
    title_line->setFrameShape(QFrame::HLine);
    title_line->setFrameShadow(QFrame::Sunken);

    QFont italicFont = QApplication::font( static_cast<QWidget*>(0) );
    italicFont.setItalic( true );

    switch( number )
    {
        /******************************
         * VIDEO Panel Implementation *
         ******************************/
        START_SPREFS_CAT( Video , qtr("Video Settings") );
            CONFIG_BOOL( "video", enableVideo );

            CONFIG_BOOL( "fullscreen", fullscreen );
            CONFIG_BOOL( "overlay", overlay );
            CONFIG_BOOL( "video-on-top", alwaysOnTop );
            CONFIG_BOOL( "video-deco", windowDecorations );
            CONFIG_BOOL( "skip-frames", skipFrames );
            CONFIG_GENERIC( "vout", Module, ui.voutLabel, outputModule );

            CONFIG_BOOL( "video-wallpaper", wallpaperMode );
#ifdef WIN32
            CONFIG_GENERIC( "directx-device", StringList, ui.dxDeviceLabel,
                            dXdisplayDevice );
            CONFIG_BOOL( "directx-hw-yuv", hwYUVBox );
#else
            ui.directXBox->setVisible( false );
            ui.hwYUVBox->setVisible( false );
#endif

            CONFIG_GENERIC( "deinterlace", IntegerList, ui.deinterLabel, deinterlaceBox );
            CONFIG_GENERIC( "deinterlace-mode", StringList, ui.deinterModeLabel, deinterlaceModeBox );
            CONFIG_GENERIC( "aspect-ratio", String, ui.arLabel, arLine );

            CONFIG_GENERIC_FILE( "snapshot-path", Directory, ui.dirLabel,
                                 ui.snapshotsDirectory, ui.snapshotsDirectoryBrowse );
            CONFIG_GENERIC( "snapshot-prefix", String, ui.prefixLabel, snapshotsPrefix );
            CONFIG_BOOL( "snapshot-sequential",
                            snapshotsSequentialNumbering );
            CONFIG_GENERIC( "snapshot-format", StringList, ui.arLabel,
                            snapshotsFormat );
         END_SPREFS_CAT;

        /******************************
         * AUDIO Panel Implementation *
         ******************************/
        START_SPREFS_CAT( Audio, qtr("Audio Settings") );

            CONFIG_BOOL( "audio", enableAudio );
            ui.SPrefsAudio_zone->setEnabled( ui.enableAudio->isChecked() );
            CONNECT( ui.enableAudio, toggled( bool ),
                     ui.SPrefsAudio_zone, setEnabled( bool ) );

#define audioCommon( name ) \
            QWidget * name ## Control = new QWidget( ui.outputAudioBox ); \
            QHBoxLayout * name ## Layout = new QHBoxLayout( name ## Control); \
            name ## Layout->setMargin( 0 ); \
            name ## Layout->setSpacing( 0 ); \
            QLabel * name ## Label = new QLabel( qtr( "Device:" ), name ## Control ); \
            name ## Label->setMinimumSize(QSize(250, 0)); \
            name ## Layout->addWidget( name ## Label ); \

#define audioControl( name) \
            audioCommon( name ) \
            QComboBox * name ## Device = new QComboBox( name ## Control ); \
            name ## Layout->addWidget( name ## Device ); \
            name ## Label->setBuddy( name ## Device ); \
            outputAudioLayout->addWidget( name ## Control, outputAudioLayout->rowCount(), 0, 1, -1 );

#define audioControl2( name) \
            audioCommon( name ) \
            QLineEdit * name ## Device = new QLineEdit( name ## Control ); \
            name ## Layout->addWidget( name ## Device ); \
            name ## Label->setBuddy( name ## Device ); \
            QPushButton * name ## Browse = new QPushButton( qtr( "Browse..." ), name ## Control); \
            name ## Layout->addWidget( name ## Browse ); \
            outputAudioLayout->addWidget( name ## Control, outputAudioLayout->rowCount(), 0, 1, -1 );

            /* Build if necessary */
            QGridLayout * outputAudioLayout = qobject_cast<QGridLayout *>(ui.outputAudioBox->layout());
#ifdef WIN32
            audioControl( DirectX );
            optionWidgets.append( DirectXControl );
            CONFIG_GENERIC_NO_UI( "directx-audio-device-name", StringList,
                    DirectXLabel, DirectXDevice );
#else
            if( module_exists( "alsa" ) )
            {
                audioControl( alsa );
                optionWidgets.append( alsaControl );

                CONFIG_GENERIC_NO_UI( "alsa-audio-device" , StringList, alsaLabel,
                                alsaDevice );
            }
            else
                optionWidgets.append( NULL );
            if( module_exists( "oss" ) )
            {
                audioControl2( OSS );
                optionWidgets.append( OSSControl );
                CONFIG_GENERIC_FILE( "oss-audio-device" , File, NULL, OSSDevice,
                                 OSSBrowse );
            }
            else
                optionWidgets.append( NULL );
#endif

#undef audioControl2
#undef audioControl
#undef audioCommon

            /* Audio Options */
            CONFIG_GENERIC_NO_BOOL( "volume" , IntegerRangeSlider, NULL,
                                     defaultVolume );
            CONNECT( ui.defaultVolume, valueChanged( int ),
                     this, updateAudioVolume( int ) );

            CONFIG_BOOL( "qt-autosave-volume", keepVolumeRadio );
            ui.defaultVolume_zone->setEnabled( ui.resetVolumeRadio->isChecked() );
            CONNECT( ui.resetVolumeRadio, toggled( bool ),
                     ui.defaultVolume_zone, setEnabled( bool ) );

            CONFIG_GENERIC( "audio-language" , String , ui.langLabel,
                            preferredAudioLanguage );

            CONFIG_BOOL( "spdif", spdifBox );
            CONFIG_GENERIC( "force-dolby-surround", IntegerList, ui.dolbyLabel,
                            detectionDolby );

            CONFIG_GENERIC_NO_BOOL( "norm-max-level" , Float, NULL,
                                    volNormSpin );
            CONFIG_GENERIC( "audio-replay-gain-mode", StringList, ui.replayLabel,
                            replayCombo );
            CONFIG_GENERIC( "audio-visual" , Module , ui.visuLabel,
                            visualisation);
            CONFIG_BOOL( "audio-time-stretch", autoscaleBox );

            /* Audio Output Specifics */
            CONFIG_GENERIC( "aout", Module, ui.outputLabel, outputModule );

            CONNECT( ui.outputModule, currentIndexChanged( int ),
                     this, updateAudioOptions( int ) );

            /* File output exists on all platforms */
            CONFIG_GENERIC_FILE( "audiofile-file", File, ui.fileLabel,
                                 ui.fileName, ui.fileBrowseButton );

            optionWidgets.append( ui.fileControl );
            optionWidgets.append( ui.outputModule );
            optionWidgets.append( ui.volNormBox );
            /*Little mofification of ui.volumeValue to compile with Qt < 4.3 */
            ui.volumeValue->setButtonSymbols(QAbstractSpinBox::NoButtons);
            optionWidgets.append( ui.volumeValue );
            optionWidgets.append( ui.headphoneEffect );
            optionWidgets.append( ui.spdifBox );
            updateAudioOptions( ui.outputModule->currentIndex() );

            /* LastFM */
            if( module_exists( "audioscrobbler" ) )
            {
                CONFIG_GENERIC( "lastfm-username", String, ui.lastfm_user_label,
                        lastfm_user_edit );
                CONFIG_GENERIC( "lastfm-password", String, ui.lastfm_pass_label,
                        lastfm_pass_edit );

                if( config_ExistIntf( VLC_OBJECT( p_intf ), "audioscrobbler" ) )
                    ui.lastfm->setChecked( true );
                else
                    ui.lastfm->setChecked( false );

                ui.lastfm_zone->setVisible( ui.lastfm->isChecked() );

                CONNECT( ui.lastfm, toggled( bool ),
                         ui.lastfm_zone, setVisible( bool ) );
                CONNECT( ui.lastfm, stateChanged( int ),
                         this, lastfm_Changed( int ) );
            }
            else
            {
                ui.lastfm->hide();
                ui.lastfm_zone->hide();
            }

            /* Normalizer */
            CONNECT( ui.volNormBox, toggled( bool ), ui.volNormSpin,
                     setEnabled( bool ) );

            char* psz = config_GetPsz( p_intf, "audio-filter" );
            qs_filter = qfu( psz ).split( ':', QString::SkipEmptyParts );
            free( psz );

            bool b_enabled = ( qs_filter.contains( "volnorm" ) );
            ui.volNormBox->setChecked( b_enabled );
            ui.volNormSpin->setEnabled( b_enabled );

            b_enabled = ( qs_filter.contains( "headphone" ) );
            ui.headphoneEffect->setChecked( b_enabled );

            /* Volume Label */
            updateAudioVolume( ui.defaultVolume->value() ); // First time init

        END_SPREFS_CAT;

        /* Input and Codecs Panel Implementation */
        START_SPREFS_CAT( InputAndCodecs, qtr("Input & Codecs Settings") );

            /* Disk Devices */
            {
                ui.DVDDevice->setToolTip(
                    qtr( "If this property is blank, different values\n"
                         "for DVD, VCD, and CDDA are set.\n"
                         "You can define a unique one or configure them \n"
                         "individually in the advanced preferences." ) );
                char *psz_dvddiscpath = config_GetPsz( p_intf, "dvd" );
                char *psz_vcddiscpath = config_GetPsz( p_intf, "vcd" );
                char *psz_cddadiscpath = config_GetPsz( p_intf, "cd-audio" );
                if( psz_dvddiscpath && psz_vcddiscpath && psz_cddadiscpath )
                if( !strcmp( psz_cddadiscpath, psz_dvddiscpath ) &&
                    !strcmp( psz_dvddiscpath, psz_vcddiscpath ) )
                {
                    ui.DVDDevice->setText( qfu( psz_dvddiscpath ) );
                }
                free( psz_cddadiscpath );
                free( psz_dvddiscpath );
                free( psz_vcddiscpath );
            }
            CONFIG_GENERIC_FILE( "dvd", File, ui.DVDLabel,
                                 ui.DVDDevice, ui.DVDBrowse );
            CONFIG_GENERIC_FILE( "input-record-path", Directory, ui.recordLabel,
                                 ui.recordPath, ui.recordBrowse );

            CONFIG_GENERIC_NO_BOOL( "server-port", Integer, ui.portLabel,
                                    UDPPort );
            CONFIG_GENERIC( "http-proxy", String , ui.httpProxyLabel, proxy );
            CONFIG_GENERIC_NO_BOOL( "ffmpeg-pp-q", Integer, ui.ppLabel,
                                    PostProcLevel );
            CONFIG_GENERIC( "avi-index", IntegerList, ui.aviLabel, AviRepair );

            /* live555 module prefs */
            CONFIG_BOOL( "rtsp-tcp",
                                live555TransportRTSP_TCPRadio );
            if ( !module_exists( "live555" ) )
            {
                ui.live555TransportRTSP_TCPRadio->hide();
                ui.live555TransportHTTPRadio->hide();
                ui.live555TransportLabel->hide();
            }
#ifdef WIN32
            CONFIG_BOOL( "prefer-system-codecs", systemCodecBox );
#else
            ui.systemCodecBox->hide();
#endif
            CONFIG_BOOL( "ffmpeg-hw", hwAccelBox );
            optionWidgets.append( ui.DVDDevice );
            optionWidgets.append( ui.cachingCombo );
            CONFIG_GENERIC( "ffmpeg-skiploopfilter", IntegerList, ui.filterLabel, loopFilterBox );

            /* Caching */
            /* Add the things to the ComboBox */
            #define addToCachingBox( str, cachingNumber ) \
                ui.cachingCombo->addItem( qtr(str), QVariant( cachingNumber ) );
            addToCachingBox( N_("Custom"), CachingCustom );
            addToCachingBox( N_("Lowest latency"), CachingLowest );
            addToCachingBox( N_("Low latency"), CachingLow );
            addToCachingBox( N_("Normal"), CachingNormal );
            addToCachingBox( N_("High latency"), CachingHigh );
            addToCachingBox( N_("Higher latency"), CachingHigher );
            #undef addToCachingBox

#define TestCaC( name ) \
    b_cache_equal =  b_cache_equal && \
     ( i_cache == config_GetInt( p_intf, name ) )

#define TestCaCi( name, int ) \
    b_cache_equal = b_cache_equal &&  \
    ( ( i_cache * int ) == config_GetInt( p_intf, name ) )
            /* Select the accurate value of the ComboBox */
            bool b_cache_equal = true;
            int i_cache = config_GetInt( p_intf, "file-caching");

            TestCaC( "udp-caching" );
            if (module_exists ("dvdread"))
                TestCaC( "dvdread-caching" );
            if (module_exists ("dvdnav"))
                TestCaC( "dvdnav-caching" );
            TestCaC( "tcp-caching" );
            TestCaC( "fake-caching" ); TestCaC( "cdda-caching" );
            TestCaC( "screen-caching" ); TestCaC( "vcd-caching" );
            #ifdef WIN32
            TestCaC( "dshow-caching" );
            #else
            if (module_exists ("v4l"))
                TestCaC( "v4l-caching" );
            if (module_exists ("access_jack"))
                TestCaC( "jack-input-caching" );
            if (module_exists ("v4l2"))
                TestCaC( "v4l2-caching" );
            if (module_exists ("pvr"))
                TestCaC( "pvr-caching" );
            #endif
            if (module_exists ("livedotcom"))
                TestCaCi( "rtsp-caching", 4 );
            TestCaCi( "ftp-caching", 2 );
            TestCaCi( "http-caching", 2 );
            if (module_exists ("access_realrtsp"))
                TestCaCi( "realrtsp-caching", 10 );
            TestCaCi( "mms-caching", 10 );
            if( b_cache_equal == 1 )
                ui.cachingCombo->setCurrentIndex(
                ui.cachingCombo->findData( QVariant( i_cache ) ) );
#undef TestCaCi
#undef TestCaC

        END_SPREFS_CAT;
        /*******************
         * Interface Panel *
         *******************/
        START_SPREFS_CAT( Interface, qtr("Interface Settings") );
//            ui.defaultLabel->setFont( italicFont );
            ui.skinsLabel->setText(
                    qtr( "This is VLC's skinnable interface. You can download other skins at" )
                    + QString( " <a href=\"http://www.videolan.org/vlc/skins.php\">" )
                    + qtr( "VLC skins website" )+ QString( "</a>." ) );
            ui.skinsLabel->setFont( italicFont );

#if defined( WIN32 )
            CONFIG_GENERIC( "language", StringList, ui.languageLabel, language );
            BUTTONACT( ui.assoButton, assoDialog() );
#else
            ui.languageBox->hide();
            ui.assoBox->hide();
#endif
            /* interface */
            char *psz_intf = config_GetPsz( p_intf, "intf" );
            if( psz_intf )
            {
                if( strstr( psz_intf, "skin" ) )
                    ui.skins->setChecked( true );
            } else {
                /* defaults to qt */
                ui.qt4->setChecked( true );
            }
            free( psz_intf );

            optionWidgets.append( ui.skins );
            optionWidgets.append( ui.qt4 );
#if !defined(NDEBUG) || !defined( WIN32)
            ui.stylesCombo->addItem( qtr("System's default") );
            ui.stylesCombo->addItems( QStyleFactory::keys() );
            ui.stylesCombo->setCurrentIndex( ui.stylesCombo->findText(
                        getSettings()->value( "MainWindow/QtStyle", "" ).toString() ) );
            ui.stylesCombo->insertSeparator( 1 );

            CONNECT( ui.stylesCombo, currentIndexChanged( QString ), this, changeStyle( QString ) );
            optionWidgets.append( ui.stylesCombo );
#else
            ui.stylesCombo->hide();
            optionWidgets.append( NULL );
#endif

            ui.skins_zone->setEnabled( ui.skins->isChecked() );
            CONNECT( ui.skins, toggled( bool ), ui.skins_zone, setEnabled( bool ) );

            ui.native_zone->setEnabled( ui.qt4->isChecked() );
            CONNECT( ui.qt4, toggled( bool ), ui.native_zone, setEnabled( bool ) );

            CONNECT( ui.minimalviewBox, toggled( bool ),
                     ui.mainPreview, setNormalPreview( bool ) );
            CONFIG_BOOL( "qt-minimal-view", minimalviewBox );
            ui.mainPreview->setNormalPreview( ui.minimalviewBox->isChecked() );
            ui.skinsPreview->setPreview( InterfacePreviewWidget::SKINS );

            CONFIG_BOOL( "embedded-video", embedVideo );
            CONFIG_BOOL( "qt-fs-controller", fsController );
            CONFIG_BOOL( "qt-system-tray", systrayBox );
            CONFIG_BOOL( "qt-notification", sysPop );
            CONFIG_GENERIC_FILE( "skins2-last", File, ui.skinFileLabel,
                                 ui.fileSkin, ui.skinBrowse );
            CONFIG_BOOL( "qt-video-autoresize", resizingBox );

            CONFIG_GENERIC( "album-art", IntegerList, ui.artFetchLabel,
                                                      artFetcher );

            /* UPDATE options */
#ifdef UPDATE_CHECK
            CONFIG_BOOL( "qt-updates-notif", updatesBox );
            CONFIG_GENERIC_NO_BOOL( "qt-updates-days", Integer, NULL,
                    updatesDays );
            ui.updatesDays->setEnabled( ui.updatesBox->isChecked() );
            CONNECT( ui.updatesBox, toggled( bool ),
                     ui.updatesDays, setEnabled( bool ) );
#else
            ui.updatesBox->hide();
            ui.updatesDays->hide();
#endif
            /* ONE INSTANCE options */
#if defined( WIN32 ) || defined( HAVE_DBUS ) || defined(__APPLE__)
            CONFIG_BOOL( "one-instance", OneInterfaceMode );
            CONFIG_BOOL( "playlist-enqueue",
                    EnqueueOneInterfaceMode );
            ui.EnqueueOneInterfaceMode->setEnabled( ui.OneInterfaceMode->isChecked() );
            CONNECT( ui.OneInterfaceMode, toggled( bool ),
                     ui.EnqueueOneInterfaceMode, setEnabled( bool ) );
#else
            ui.OneInterfaceBox->hide();
#endif
            /* RECENTLY PLAYED options */
            CONNECT( ui.saveRecentlyPlayed, toggled( bool ),
                     ui.recentlyPlayedFilters, setEnabled( bool ) );
            ui.recentlyPlayedFilters->setEnabled( false );
            CONFIG_BOOL( "qt-recentplay", saveRecentlyPlayed );
            CONFIG_GENERIC( "qt-recentplay-filter", String, ui.filterLabel,
                    recentlyPlayedFilters );

        END_SPREFS_CAT;

        START_SPREFS_CAT( Subtitles,
                            qtr("Subtitles & On Screen Display Settings") );
            CONFIG_BOOL( "osd", OSDBox);
            CONFIG_BOOL( "video-title-show", OSDTitleBox);


            CONFIG_GENERIC( "subsdec-encoding", StringList, ui.encodLabel,
                            encoding );
            CONFIG_GENERIC( "sub-language", String, ui.subLangLabel,
                            preferredLanguage );
            CONFIG_GENERIC_NO_BOOL( "freetype-font", Font, ui.fontLabel, font );
            CONFIG_GENERIC( "freetype-color", IntegerList, ui.fontColorLabel,
                            fontColor );
            CONFIG_GENERIC( "freetype-rel-fontsize", IntegerList,
                            ui.fontSizeLabel, fontSize );
            CONFIG_GENERIC( "freetype-effect", IntegerList, ui.fontEffectLabel,
                            effect );
            CONFIG_GENERIC_NO_BOOL( "sub-margin", Integer, ui.subsPosLabel, subsPosition );

        END_SPREFS_CAT;

        case SPrefsHotkeys:
        {
            p_config = config_FindConfig( VLC_OBJECT(p_intf), "key-play" );

            QGridLayout *gLayout = new QGridLayout;
            panel->setLayout( gLayout );
            int line = 0;

            panel_label->setText( qtr( "Configure Hotkeys" ) );
            control = new KeySelectorControl( VLC_OBJECT(p_intf), p_config ,
                                                this, gLayout, line );
            controls.append( control );

            line++;

            QFrame *sepline = new QFrame;
            sepline->setFrameStyle(QFrame::HLine | QFrame::Sunken);
            gLayout->addWidget( sepline, line, 0, 1, -1 );

            line++;

            p_config = config_FindConfig( VLC_OBJECT(p_intf), "hotkeys-mousewheel-mode" );
            control = new IntegerListConfigControl( VLC_OBJECT(p_intf),
                    p_config, false, this, gLayout, line );
            controls.append( control );

            break;
        }
    }

    panel_layout->addWidget( panel_label );
    panel_layout->addWidget( title_line );

    if( small )
    {
        QScrollArea *scroller= new QScrollArea;
        scroller->setWidget( panel );
        scroller->setWidgetResizable( true );
        scroller->setFrameStyle( QFrame::NoFrame );
        panel_layout->addWidget( scroller );
    }
    else
    {
        panel_layout->addWidget( panel );
        if( number != SPrefsHotkeys ) panel_layout->addStretch( 2 );
    }

    setLayout( panel_layout );

#undef END_SPREFS_CAT
#undef START_SPREFS_CAT
#undef CONFIG_GENERIC_FILE
#undef CONFIG_GENERIC_NO_BOOL
#undef CONFIG_GENERIC_NO_UI
#undef CONFIG_GENERIC
#undef CONFIG_BOOL
}


void SPrefsPanel::updateAudioOptions( int number)
{
    QString value = qobject_cast<QComboBox *>(optionWidgets[audioOutCoB])
                                            ->itemData( number ).toString();
#ifdef WIN32
    optionWidgets[directxW]->setVisible( ( value == "aout_directx" ) );
#else
    /* optionWidgets[ossW] can be NULL */
    if( optionWidgets[ossW] )
        optionWidgets[ossW]->setVisible( ( value == "oss" ) );
    /* optionWidgets[alsaW] can be NULL */
    if( optionWidgets[alsaW] )
        optionWidgets[alsaW]->setVisible( ( value == "alsa" ) );
#endif
    optionWidgets[fileW]->setVisible( ( value == "aout_file" ) );
    optionWidgets[spdifChB]->setVisible( ( value == "alsa" || value == "oss" || value == "auhal" ||
                                           value == "aout_directx" || value == "waveout" ) );
}


SPrefsPanel::~SPrefsPanel()
{
    qDeleteAll( controls ); controls.clear();
}

void SPrefsPanel::updateAudioVolume( int volume )
{
    qobject_cast<QSpinBox *>(optionWidgets[volLW])
        ->setValue( volume * 100 / 256 );
}


/* Function called from the main Preferences dialog on each SPrefs Panel */
void SPrefsPanel::apply()
{
    /* Generic save for ever panel */
    QList<ConfigControl *>::Iterator i;
    for( i = controls.begin() ; i != controls.end() ; i++ )
    {
        ConfigControl *c = qobject_cast<ConfigControl *>(*i);
        c->doApply( p_intf );
    }

    switch( number )
    {
    case SPrefsInputAndCodecs:
    {
        /* Device default selection */
        char *psz_devicepath =
            strdup( qtu( qobject_cast<QLineEdit *>(optionWidgets[inputLE] )->text() ) );
        if( !EMPTY_STR( psz_devicepath ) )
        {
            config_PutPsz( p_intf, "dvd", psz_devicepath );
            config_PutPsz( p_intf, "vcd", psz_devicepath );
            config_PutPsz( p_intf, "cd-audio", psz_devicepath );
            free( psz_devicepath );
        }

#define CaCi( name, int ) config_PutInt( p_intf, name, int * i_comboValue )
#define CaC( name ) CaCi( name, 1 )
        /* Caching */
        QComboBox *cachingCombo = qobject_cast<QComboBox *>(optionWidgets[cachingCoB]);
        int i_comboValue = cachingCombo->itemData( cachingCombo->currentIndex() ).toInt();
        if( i_comboValue )
        {
            CaC( "udp-caching" );
            if (module_exists ("dvdread" ))
                CaC( "dvdread-caching" );
            if (module_exists ("dvdnav" ))
                CaC( "dvdnav-caching" );
            CaC( "tcp-caching" ); CaC( "vcd-caching" );
            CaC( "fake-caching" ); CaC( "cdda-caching" ); CaC( "file-caching" );
            CaC( "screen-caching" ); CaC( "bd-caching" );
            CaCi( "rtsp-caching", 2 ); CaCi( "ftp-caching", 2 );
            CaCi( "http-caching", 2 );
            if (module_exists ("access_realrtsp" ))
                CaCi( "realrtsp-caching", 10 );
            CaCi( "mms-caching", 10 );
            #ifdef WIN32
            CaC( "dshow-caching" );
            #else
            if (module_exists ( "v4l" ))
                CaC( "v4l-caching" );
            if (module_exists ( "access_jack" ))
            CaC( "jack-input-caching" );
            if (module_exists ( "v4l2" ))
                CaC( "v4l2-caching" );
            if (module_exists ( "pvr" ))
                CaC( "pvr-caching" );
            #endif
            //CaCi( "dv-caching" ) too short...
        }
        break;
#undef CaC
#undef CaCi
    }

    /* Interfaces */
    case SPrefsInterface:
    {
        if( qobject_cast<QRadioButton *>(optionWidgets[skinRB])->isChecked() )
            config_PutPsz( p_intf, "intf", "skins2" );
        if( qobject_cast<QRadioButton *>(optionWidgets[qtRB])->isChecked() )
            config_PutPsz( p_intf, "intf", "qt" );
        if( qobject_cast<QComboBox *>(optionWidgets[styleCB]) )
            getSettings()->setValue( "MainWindow/QtStyle",
                qobject_cast<QComboBox *>(optionWidgets[styleCB])->currentText() );

        break;
    }

    case SPrefsAudio:
    {
        bool b_checked =
            qobject_cast<QCheckBox *>(optionWidgets[normalizerChB])->isChecked();
        if( b_checked && !qs_filter.contains( "volnorm" ) )
            qs_filter.append( "volnorm" );
        if( !b_checked && qs_filter.contains( "volnorm" ) )
            qs_filter.removeAll( "volnorm" );

        b_checked =
            qobject_cast<QCheckBox *>(optionWidgets[headphoneB])->isChecked();

        if( b_checked && !qs_filter.contains( "headphone" ) )
            qs_filter.append( "headphone" );
        if( !b_checked && qs_filter.contains( "headphone" ) )
            qs_filter.removeAll( "headphone" );

        config_PutPsz( p_intf, "audio-filter", qtu( qs_filter.join( ":" ) ) );
        break;
    }
    }
}

void SPrefsPanel::clean()
{}

void SPrefsPanel::lastfm_Changed( int i_state )
{
    if( i_state == Qt::Checked )
        config_AddIntf( VLC_OBJECT( p_intf ), "audioscrobbler" );
    else if( i_state == Qt::Unchecked )
        config_RemoveIntf( VLC_OBJECT( p_intf ), "audioscrobbler" );
}

void SPrefsPanel::changeStyle( QString s_style )
{
    QApplication::setStyle( s_style );

    /* force refresh on all widgets */
    QWidgetList widgets = QApplication::allWidgets();
    QWidgetList::iterator it = widgets.begin();
    while( it != widgets.end() ) {
        (*it)->update();
        it++;
    };
}

#ifdef WIN32
#include <QDialogButtonBox>
#include <QHeaderView>
#include "util/registry.hpp"
#include <string>

bool SPrefsPanel::addType( const char * psz_ext, QTreeWidgetItem* current,
                           QTreeWidgetItem* parent, QVLCRegistry *qvReg )
{
    bool b_temp;
    const char* psz_VLC = "VLC";
    current = new QTreeWidgetItem( parent, QStringList( psz_ext ) );

    if( strstr( qvReg->ReadRegistryString( psz_ext, "", ""  ), psz_VLC ) )
    {
        current->setCheckState( 0, Qt::Checked );
        b_temp = false;
    }
    else
    {
        current->setCheckState( 0, Qt::Unchecked );
        b_temp = true;
    }
    listAsso.append( current );
    return b_temp;
}

void SPrefsPanel::assoDialog()
{
    LPAPPASSOCREGUI p_appassoc;
    CoInitialize( 0 );

    if( S_OK == CoCreateInstance( &clsid_IApplication2,
                NULL, CLSCTX_INPROC_SERVER,
                &IID_IApplicationAssociationRegistrationUI,
                (void **)&p_appassoc) )
    {
        if(S_OK == p_appassoc->vt->LaunchAdvancedAssociationUI(p_appassoc, L"VLC" ) )
        {
            CoUninitialize();
            return;
        }
    }

    CoUninitialize();

    QDialog *d = new QDialog( this );
    QGridLayout *assoLayout = new QGridLayout( d );

    QTreeWidget *filetypeList = new QTreeWidget;
    assoLayout->addWidget( filetypeList, 0, 0, 1, 4 );
    filetypeList->header()->hide();

    QVLCRegistry * qvReg = new QVLCRegistry( HKEY_CLASSES_ROOT );

    QTreeWidgetItem *audioType = new QTreeWidgetItem( QStringList( qtr( "Audio Files" ) ) );
    QTreeWidgetItem *videoType = new QTreeWidgetItem( QStringList( qtr( "Video Files" ) ) );
    QTreeWidgetItem *otherType = new QTreeWidgetItem( QStringList( qtr( "Playlist Files" ) ) );

    filetypeList->addTopLevelItem( audioType );
    filetypeList->addTopLevelItem( videoType );
    filetypeList->addTopLevelItem( otherType );

    audioType->setExpanded( true ); audioType->setCheckState( 0, Qt::Unchecked );
    videoType->setExpanded( true ); videoType->setCheckState( 0, Qt::Unchecked );
    otherType->setExpanded( true ); otherType->setCheckState( 0, Qt::Unchecked );

    QTreeWidgetItem *currentItem;

    int i_temp = 0;
#define aTa( name ) i_temp += addType( name, currentItem, audioType, qvReg )
#define aTv( name ) i_temp += addType( name, currentItem, videoType, qvReg )
#define aTo( name ) i_temp += addType( name, currentItem, otherType, qvReg )

    aTa( ".a52" ); aTa( ".aac" ); aTa( ".ac3" ); aTa( ".dts" ); aTa( ".flac" );
    aTa( ".m4a" ); aTa( ".m4p" ); aTa( ".mka" ); aTa( ".mod" ); aTa( ".mp1" );
    aTa( ".mp2" ); aTa( ".mp3" ); aTa( ".oma" ); aTa( ".oga" ); aTa( ".spx" );
    aTa( ".tta" ); aTa( ".wav" ); aTa( ".wma" ); aTa( ".xm" );
    audioType->setCheckState( 0, ( i_temp > 0 ) ?
                              ( ( i_temp == audioType->childCount() ) ?
                               Qt::Checked : Qt::PartiallyChecked )
                            : Qt::Unchecked );

    i_temp = 0;
    aTv( ".asf" ); aTv( ".avi" ); aTv( ".divx" ); aTv( ".dv" ); aTv( ".flv" );
    aTv( ".gxf" ); aTv( ".m1v" ); aTv( ".m2v" ); aTv( ".m2ts" ); aTv( ".m4v" );
    aTv( ".mkv" ); aTv( ".mov" ); aTv( ".mp2" ); aTv( ".mp4" ); aTv( ".mpeg" );
    aTv( ".mpeg1" ); aTv( ".mpeg2" ); aTv( ".mpeg4" ); aTv( ".mpg" );
    aTv( ".mts" ); aTv( ".mxf" );
    aTv( ".ogg" ); aTv( ".ogm" ); aTv( ".ogx" ); aTv( ".ogv" );  aTv( ".ts" );
    aTv( ".vob" ); aTv( ".vro" ); aTv( ".wmv" );
    videoType->setCheckState( 0, ( i_temp > 0 ) ?
                              ( ( i_temp == audioType->childCount() ) ?
                               Qt::Checked : Qt::PartiallyChecked )
                            : Qt::Unchecked );

    i_temp = 0;
    aTo( ".asx" ); aTo( ".b4s" ); aTo( ".ifo" ); aTo( ".m3u" ); aTo( ".pls" );
    aTo( ".sdp" ); aTo( ".vlc" ); aTo( ".xspf" );
    otherType->setCheckState( 0, ( i_temp > 0 ) ?
                              ( ( i_temp == audioType->childCount() ) ?
                               Qt::Checked : Qt::PartiallyChecked )
                            : Qt::Unchecked );

#undef aTo
#undef aTv
#undef aTa

    QDialogButtonBox *buttonBox = new QDialogButtonBox( d );
    QPushButton *closeButton = new QPushButton( qtr( "&Apply" ) );
    QPushButton *clearButton = new QPushButton( qtr( "&Cancel" ) );
    buttonBox->addButton( closeButton, QDialogButtonBox::AcceptRole );
    buttonBox->addButton( clearButton, QDialogButtonBox::ActionRole );

    assoLayout->addWidget( buttonBox, 1, 2, 1, 2 );

    CONNECT( closeButton, clicked(), this, saveAsso() );
    CONNECT( clearButton, clicked(), d, reject() );
    d->resize( 300, 400 );
    d->exec();
    delete d;
    delete qvReg;
    listAsso.clear();
}

void addAsso( QVLCRegistry *qvReg, const char *psz_ext )
{
    std::string s_path( "VLC" ); s_path += psz_ext;
    std::string s_path2 = s_path;

    /* Save a backup if already assigned */
    char *psz_value = qvReg->ReadRegistryString( psz_ext, "", ""  );

    if( psz_value && strlen( psz_value ) > 0 )
        qvReg->WriteRegistryString( psz_ext, "VLC.backup", psz_value );
    delete psz_value;

    /* Put a "link" to VLC.EXT as default */
    qvReg->WriteRegistryString( psz_ext, "", s_path.c_str() );

    /* Create the needed Key if they weren't done in the installer */
    if( !qvReg->RegistryKeyExists( s_path.c_str() ) )
    {
        qvReg->WriteRegistryString( psz_ext, "", s_path.c_str() );
        qvReg->WriteRegistryString( s_path.c_str(), "", "Media file" );
        qvReg->WriteRegistryString( s_path.append( "\\shell" ).c_str() , "", "Play" );

        /* Get the installer path */
        QVLCRegistry *qvReg2 = new QVLCRegistry( HKEY_LOCAL_MACHINE );
        std::string str_temp; str_temp.assign(
            qvReg2->ReadRegistryString( "Software\\VideoLAN\\VLC", "", "" ) );

        if( str_temp.size() )
        {
            qvReg->WriteRegistryString( s_path.append( "\\Play\\command" ).c_str(),
                "", str_temp.append(" --started-from-file \"%1\"" ).c_str() );

            qvReg->WriteRegistryString( s_path2.append( "\\DefaultIcon" ).c_str(),
                        "", str_temp.append(",0").c_str() );
        }
        delete qvReg2;
    }
}

void delAsso( QVLCRegistry *qvReg, const char *psz_ext )
{
    char psz_VLC[] = "VLC";
    char *psz_value = qvReg->ReadRegistryString( psz_ext, "", ""  );

    if( psz_value && !strcmp( strcat( psz_VLC, psz_ext ), psz_value ) )
    {
        free( psz_value );
        psz_value = qvReg->ReadRegistryString( psz_ext, "VLC.backup", "" );
        if( psz_value )
            qvReg->WriteRegistryString( psz_ext, "", psz_value );

        qvReg->DeleteKey( psz_ext, "VLC.backup" );
    }
    delete( psz_value );
}
void SPrefsPanel::saveAsso()
{
    QVLCRegistry * qvReg;
    for( int i = 0; i < listAsso.size(); i ++ )
    {
        qvReg  = new QVLCRegistry( HKEY_CLASSES_ROOT );
        if( listAsso[i]->checkState( 0 ) > 0 )
        {
            addAsso( qvReg, qtu( listAsso[i]->text( 0 ) ) );
        }
        else
        {
            delAsso( qvReg, qtu( listAsso[i]->text( 0 ) ) );
        }
    }
    /* Gruik ? Naaah */
    qobject_cast<QDialog *>(listAsso[0]->treeWidget()->parent())->accept();
    delete qvReg;
}

#endif /* WIN32 */

