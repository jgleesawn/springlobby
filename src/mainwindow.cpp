/* Copyright (C) 2007 The SpringLobby Team. All rights reserved. */
//
// Class: MainWindow
//

#include <wx/frame.h>
#include <wx/intl.h>
#include <wx/textdlg.h>
#include <wx/imaglist.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/listbook.h>
#include <wx/menu.h>
#include <wx/dcmemory.h>
#include <wx/tooltip.h>


#include <stdexcept>

#include "mainwindow.h"
#include "settings.h"
#include "ui.h"
#include "server.h"
#include "utils.h"
#include "battlelisttab.h"
#include "mainchattab.h"
#include "mainjoinbattletab.h"
#include "mainsingleplayertab.h"
#include "mainoptionstab.h"
#include "iunitsync.h"
#include "uiutils.h"
#include "replaytab.h"
#ifndef NO_TORRENT_SYSTEM
#include "maintorrenttab.h"
#include "torrentwrapper.h"
#endif


#include "images/springlobby.xpm"
#include "images/chat_icon.png.h"
#include "images/chat_icon_text.png.h"
#include "images/join_icon.png.h"
#include "images/join_icon_text.png.h"
#include "images/single_player_icon.png.h"
#include "images/single_player_icon_text.png.h"
#include "images/options_icon.png.h"
#include "images/options_icon_text.png.h"
#include "images/select_icon.xpm"
#include "images/downloads_icon.png.h"
#include "images/downloads_icon_text.png.h"
#include "images/replay_icon.png.h"
#include "images/replay_icon_text.png.h"

#include "settings++/frame.h"
#include "settings++/custom_dialogs.h"

#include "updater/updater.h"
#include "autojoinchanneldialog.h"

#ifdef HAVE_WX28
    #if defined(__WXMSW__)
        #include <wx/msw/winundef.h>
    #endif
    #include <wx/aboutdlg.h>
#endif

BEGIN_EVENT_TABLE(MainWindow, wxFrame)

  EVT_MENU( MENU_JOIN, MainWindow::OnMenuJoin )
  EVT_MENU( MENU_CHAT, MainWindow::OnMenuChat )
  EVT_MENU( MENU_CONNECT, MainWindow::OnMenuConnect )
  EVT_MENU( MENU_DISCONNECT, MainWindow::OnMenuDisconnect )
  EVT_MENU( MENU_QUIT, MainWindow::OnMenuQuit )
  EVT_MENU( MENU_USYNC, MainWindow::OnUnitSyncReload )
  EVT_MENU( MENU_TRAC, MainWindow::OnReportBug )
  EVT_MENU( MENU_DOC, MainWindow::OnShowDocs )
  EVT_MENU( MENU_SETTINGSPP, MainWindow::OnShowSettingsPP )
  EVT_MENU( MENU_VERSION, MainWindow::OnMenuVersion )
  EVT_MENU( MENU_ABOUT, MainWindow::OnMenuAbout )
  EVT_MENU( MENU_START_TORRENT, MainWindow::OnMenuStartTorrent )
  EVT_MENU( MENU_STOP_TORRENT, MainWindow::OnMenuStopTorrent )
  EVT_MENU( MENU_SHOW_TOOLTIPS, MainWindow::OnShowToolTips )
  EVT_MENU( MENU_AUTOJOIN_CHANNELS, MainWindow::OnMenuAutojoinChannels )
  EVT_MENU_OPEN( MainWindow::OnMenuOpen )
  EVT_LISTBOOK_PAGE_CHANGED( MAIN_TABS, MainWindow::OnTabsChanged )
END_EVENT_TABLE()



MainWindow::MainWindow( Ui& ui ) :
  wxFrame( (wxFrame*)0, -1, _("SpringLobby"), wxPoint(50, 50), wxSize(450, 340) ),
  m_ui(ui),m_autojoin_dialog(NULL)
{
  SetIcon( wxIcon(springlobby_xpm) );
  wxMenu *menuFile = new wxMenu;
  menuFile->Append(MENU_CONNECT, _("&Connect..."));
  menuFile->Append(MENU_DISCONNECT, _("&Disconnect"));
  menuFile->AppendSeparator();
  menuFile->Append(MENU_QUIT, _("&Quit"));

  //m_menuEdit = new wxMenu;
  //TODO doesn't work atm


  m_menuTools = new wxMenu;
  m_menuTools->Append(MENU_JOIN, _("&Join channel..."));
  m_menuTools->Append(MENU_CHAT, _("Open private &chat..."));
  m_menuTools->Append(MENU_AUTOJOIN_CHANNELS, _("&Autojoin channels..."));
  m_menuTools->AppendSeparator();
  m_menuTools->Append(MENU_USYNC, _("&Reload maps/mods"));


  m_menuTools->AppendSeparator();
  m_menuTools->AppendCheckItem(MENU_SHOW_TOOLTIPS, _("Show tooltips") );
  m_menuTools->Check( MENU_SHOW_TOOLTIPS, sett().GetShowTooltips() );


  #ifndef NO_TORRENT_SYSTEM
  m_menuTools->AppendSeparator();
  #endif
  m_menuTools->Append(MENU_VERSION, _("Check for new Version"));
  m_settings_menu = new wxMenuItem( m_menuTools, MENU_SETTINGSPP, _("SpringSettings"), wxEmptyString, wxITEM_NORMAL );
  m_menuTools->Append (m_settings_menu);

  wxMenu *menuHelp = new wxMenu;
  menuHelp->Append(MENU_ABOUT, _("&About"));
  menuHelp->Append(MENU_TRAC, _("&Report a bug..."));
  menuHelp->Append(MENU_DOC, _("&Documentation"));

  m_menubar = new wxMenuBar;
  m_menubar->Append(menuFile, _("&File"));
  //m_menubar->Append(m_menuEdit, _("&Edit"));
  m_menubar->Append(m_menuTools, _("&Tools"));
  m_menubar->Append(menuHelp, _("&Help"));
  SetMenuBar(m_menubar);

  m_main_sizer = new wxBoxSizer( wxHORIZONTAL );
  m_func_tabs = new wxListbook( this, MAIN_TABS, wxDefaultPosition, wxDefaultSize, wxLB_LEFT );

  m_chat_icon =  charArr2wxBitmapWithBlending( chat_icon_png , sizeof (chat_icon_png) , chat_icon_text_png, sizeof(chat_icon_text_png), 64 ) ;
  m_battle_icon = charArr2wxBitmapWithBlending( join_icon_png , sizeof (join_icon_png), join_icon_text_png , sizeof (join_icon_text_png), 64 ) ;
  m_sp_icon = charArr2wxBitmapWithBlending( single_player_icon_png , sizeof (single_player_icon_png), single_player_icon_text_png , sizeof (single_player_icon_text_png), 64 ) ;
  m_options_icon =   charArr2wxBitmapWithBlending( options_icon_png , sizeof (options_icon_png), options_icon_text_png , sizeof (options_icon_text_png), 64 ) ;
  m_downloads_icon = charArr2wxBitmapWithBlending( downloads_icon_png , sizeof (downloads_icon_png), downloads_icon_text_png , sizeof (downloads_icon_text_png), 64 ) ;
  m_replay_icon = charArr2wxBitmapWithBlending( replay_icon_png , sizeof (replay_icon_png), replay_icon_text_png , sizeof (replay_icon_text_png), 64 ) ;
  m_select_image = new wxBitmap( select_icon_xpm );

  m_func_tab_images = new wxImageList( 64, 64 );
  MakeImages();

  m_func_tabs->AssignImageList( m_func_tab_images );
  m_chat_tab = new MainChatTab( m_func_tabs, m_ui );
  m_join_tab = new MainJoinBattleTab( m_func_tabs, m_ui );
  m_sp_tab = new MainSinglePlayerTab( m_func_tabs, m_ui );
  m_opts_tab = new MainOptionsTab( m_func_tabs, m_ui );
  m_replay_tab = new ReplayTab ( m_func_tabs, m_ui );
#ifndef NO_TORRENT_SYSTEM
  m_torrent_tab = new MainTorrentTab( m_func_tabs, m_ui);
#endif


  m_func_tabs->AddPage( m_chat_tab, _T(""), true, 0 );
  m_func_tabs->AddPage( m_join_tab, _T(""), false, 1 );
  m_func_tabs->AddPage( m_sp_tab, _T(""), false, 2 );
  m_func_tabs->AddPage( m_opts_tab, _T(""), false, 3 );
  m_func_tabs->AddPage( m_replay_tab, _T(""), false, 4 );

#ifndef NO_TORRENT_SYSTEM
  m_func_tabs->AddPage( m_torrent_tab, _T(""), false, 5 );
#endif

  m_main_sizer->Add( m_func_tabs, 1, wxEXPAND | wxALL, 2 );

  SetSizer( m_main_sizer );

  SetSize( sett().GetMainWindowLeft(), sett().GetMainWindowTop(), sett().GetMainWindowWidth(), sett().GetMainWindowHeight() );
  Layout();

  se_frame_active = false;

  wxToolTip::Enable(sett().GetShowTooltips());

}

void MainWindow::forceSettingsFrameClose()
{
	if (se_frame_active && se_frame != 0)
		se_frame->handleExternExit();
}

MainWindow::~MainWindow()
{
  int x, y, w, h;
  GetSize( &w, &h );
  sett().SetMainWindowHeight( h );
  sett().SetMainWindowWidth( w );
  GetPosition( &x, &y );
  sett().SetMainWindowTop( y );
  sett().SetMainWindowLeft( x );
  sett().SaveSettings();
  m_ui.Quit();
  m_ui.OnMainWindowDestruct();
  freeStaticBox();

  if ( m_autojoin_dialog  != 0 )
  {
    delete m_autojoin_dialog;
    m_autojoin_dialog = 0;
  }
  delete m_chat_icon;
  delete m_battle_icon;
  delete m_options_icon;
  delete m_select_image;

}


void DrawBmpOnBmp( wxBitmap& canvas, wxBitmap& object, int x, int y )
{
  wxMemoryDC dc;
  dc.SelectObject( canvas );
  dc.DrawBitmap( object, x, y, true );
  dc.SelectObject( wxNullBitmap );
}

//void MainWindow::DrawTxtOnBmp( wxBitmap& canvas, wxString text, int x, int y )
//{
//  wxMemoryDC dc;
//  dc.SelectObject( canvas );
//
//  dc.DrawText( text, x, y);
//  dc.SelectObject( wxNullBitmap );
//}

void MainWindow::MakeImages()
{
  m_func_tab_images->RemoveAll();

  //if ( m_func_tabs->GetSelection() == 0 ) {
    /*wxBitmap img( *m_select_image );
    DrawBmpOnBmp( img, *m_chat_icon, 0, 0 );
    m_func_tab_images->Add( img );
  } else {*/
 // DrawTxtOnBmp( *m_battle_icon, _("Test"), 1,1);
    m_func_tab_images->Add( *m_chat_icon );
  //}

  //if ( m_func_tabs->GetSelection() == 1 ) {
    /*wxBitmap img( *m_select_image );
    DrawBmpOnBmp( img, *m_battle_icon, 0, 0 );
    m_func_tab_images->Add( img );
  } else {*/
    m_func_tab_images->Add( *m_battle_icon );
  //}

  //if ( m_func_tabs->GetSelection() == 2 ) {
    /*wxBitmap img( *m_select_image );
    DrawBmpOnBmp( img, *m_sp_icon, 0, 0 );
    m_func_tab_images->Add( img );
  } else {*/
    m_func_tab_images->Add( *m_sp_icon );
  //}

  //if ( m_func_tabs->GetSelection() == 3 ) {
    /*wxBitmap img( *m_select_image );
    DrawBmpOnBmp( img, *m_options_icon, 0, 0 );
    m_func_tab_images->Add( img );
  } else {*/
    m_func_tab_images->Add( *m_options_icon );

    m_func_tab_images->Add( *m_replay_icon );

    m_func_tab_images->Add( *m_downloads_icon );

  //}

}


/*
//! @brief Get the ChatPanel dedicated to server output and input
ChatPanel& servwin()
{
  return m_ui.mw().GetChatTab().ServerChat();
}
*/

//! @brief Returns the curent MainChatTab object
MainChatTab& MainWindow::GetChatTab()
{
  ASSERT_EXCEPTION( m_chat_tab != 0, _T("m_chat_tab = 0") );
  return *m_chat_tab;
}

MainJoinBattleTab& MainWindow::GetJoinTab()
{
  ASSERT_EXCEPTION( m_join_tab != 0, _T("m_join_tab = 0") );
  return *m_join_tab;
}


MainSinglePlayerTab& MainWindow::GetSPTab()
{
  ASSERT_EXCEPTION( m_sp_tab != 0, _T("m_sp_tab = 0") );
  return *m_sp_tab;
}
#ifndef NO_TORRENT_SYSTEM
MainTorrentTab& MainWindow::GetTorrentTab()
{
  ASSERT_EXCEPTION( m_torrent_tab  != 0, _T("m_torrent_tab = 0") );
  return *m_torrent_tab ;
}
#endif
ChatPanel* MainWindow::GetActiveChatPanel()
{
  int index = m_func_tabs->GetSelection();
  if ( index == PAGE_CHAT ) return m_chat_tab->GetActiveChatPanel();
  if ( index == PAGE_JOIN ) return m_join_tab->GetActiveChatPanel();
  return 0;
}


ChatPanel* MainWindow::GetChannelChatPanel( const wxString& channel )
{
  return m_chat_tab->GetChannelChatPanel( channel );
}


//! @brief Open a new chat tab with a channel chat
//!
//! @param channel The channel name
//! @note This does NOT join the chatt.
//! @sa Server::JoinChannel OpenPrivateChat
void MainWindow::OpenChannelChat( Channel& channel )
{
  ASSERT_LOGIC( m_chat_tab != 0, _T("m_chat_tab") );
  m_func_tabs->SetSelection( 0 );
  m_chat_tab->AddChatPannel( channel );
}


//! @brief Open a new chat tab with a private chat
//!
//! @param nick The user to whom the chatwindow should be opened to
void MainWindow::OpenPrivateChat( User& user )
{
  ASSERT_LOGIC( m_chat_tab != 0, _T("m_chat_tab") );
  m_func_tabs->SetSelection( 0 );
  m_chat_tab->AddChatPannel( user );
}


//! @brief Displays the lobby configuration.
void MainWindow::ShowConfigure( const unsigned int page )
{
  m_func_tabs->SetSelection( PAGE_OPTOS );
  //possibly out of bounds is captured by m_opts_tab itslef
  m_opts_tab->SetSelection( page );
}


void MainWindow::ReloadSpringPathFromConfig()
{
  m_opts_tab->ReloadSpringPathFromConfig();
}

//! @brief Called when join channel menuitem is clicked
void MainWindow::OnMenuJoin( wxCommandEvent& event )
{

  if ( !m_ui.IsConnected() ) return;
  wxString answer;
  if ( m_ui.AskText( _("Join channel..."), _("Name of channel to join"), answer ) ) {
    m_ui.JoinChannel( answer, _T("") );
  }

}


void MainWindow::OnMenuChat( wxCommandEvent& event )
{

  if ( !m_ui.IsConnected() ) return;
  wxString answer;
  if ( m_ui.AskText( _("Open Private Chat..."), _("Name of user"), answer ) ) {
    if (m_ui.GetServer().UserExists( answer ) ) {
      OpenPrivateChat( m_ui.GetServer().GetUser( answer ) );
    }
  }

}

void MainWindow::OnMenuAbout( wxCommandEvent& event )
{
#ifdef HAVE_WX28
    wxAboutDialogInfo info;
	info.SetName(_T("SpringLobby"));
	info.SetVersion (GetSpringLobbyVersion());
	info.SetDescription(_("SpringLobby is a cross-plattform lobby client for the RTS Spring engine"));
	//info.SetCopyright(_T("");
	info.SetLicence(_T("GPL"));
	info.AddDeveloper(_T("BrainDamage"));
	info.AddDeveloper(_T("dizekat"));
	info.AddDeveloper(_T("insaneinside"));
	info.AddDeveloper(_T("Kaot"));
	info.AddDeveloper(_T("koshi"));
	info.AddDeveloper(_T("semi_"));
	info.AddDeveloper(_T("tc-"));
  info.AddTranslator(_T("chaosch (simplified chinese)"));
	info.AddTranslator(_T("lejocelyn (french)"));
	info.AddTranslator(_T("Suprano (german)"));
  info.AddTranslator(_T("tc- (swedish)"));
	info.SetIcon(wxIcon(springlobby_xpm));
	wxAboutBox(info);

#else
    customMessageBoxNoModal(SL_MAIN_ICON,_T("SpringLobby version: ")+GetSpringLobbyVersion(),_T("About"));
#endif

}

void MainWindow::OnMenuConnect( wxCommandEvent& event )
{
  m_ui.ShowConnectWindow();
}


void MainWindow::OnMenuDisconnect( wxCommandEvent& event )
{
  m_ui.Disconnect();
}


void MainWindow::OnMenuQuit( wxCommandEvent& event )
{
  m_ui.Quit();
}


void MainWindow::OnMenuVersion( wxCommandEvent& event )
{
  Updater().CheckForUpdates();
}

void MainWindow::OnUnitSyncReload( wxCommandEvent& event )
{
  m_ui.ReloadUnitSync();
}


void MainWindow::OnMenuStartTorrent( wxCommandEvent& event )
{
  #ifndef NO_TORRENT_SYSTEM
  sett().SetTorrentSystemAutoStartMode( 2 ); /// switch operation to manual mode
  torrent().ConnectToP2PSystem();
  #endif
}


void MainWindow::OnMenuStopTorrent( wxCommandEvent& event )
{
  #ifndef NO_TORRENT_SYSTEM
  sett().SetTorrentSystemAutoStartMode( 2 ); /// switch operation to manual mode
  torrent().DisconnectToP2PSystem();
  #endif
}


void MainWindow::OnMenuOpen( wxMenuEvent& event )
{
  #ifndef NO_TORRENT_SYSTEM
  m_menuTools->Delete(MENU_STOP_TORRENT);
  m_menuTools->Delete(MENU_START_TORRENT);
  if ( !torrent().IsConnectedToP2PSystem() )
  {
    m_menuTools->Insert( 5, MENU_START_TORRENT, _("Manually &Start Torrent System") );
  }
  else
  {
    m_menuTools->Insert( 5, MENU_STOP_TORRENT, _("Manually &Stop Torrent System") );
  }
  #endif
}


void MainWindow::OnReportBug( wxCommandEvent& event )
{
    wxString reporter = wxEmptyString;
    if (m_ui.IsConnected() )
        reporter = _T("?reporter=") + m_ui.GetServer().GetMe().GetNick();
  m_ui.OpenWebBrowser( _T("http://trac.springlobby.info/newticket") + reporter);
}


void MainWindow::OnShowDocs( wxCommandEvent& event )
{
  m_ui.OpenWebBrowser( _T("http://springlobby.info") );
}

void MainWindow::OnTabsChanged( wxListbookEvent& event )
{
  MakeImages();

  int newsel = event.GetSelection();

  if ( newsel == 0 || newsel == 1 )
  {
    if ( !m_ui.IsConnected() && m_ui.IsMainWindowCreated() ) m_ui.Connect();
  }
}

void MainWindow::OnUnitSyncReloaded()
{
  wxLogDebugFunc( _T("") );
  wxLogMessage( _T("Reloading join tab") );
  GetJoinTab().OnUnitSyncReloaded();
  wxLogMessage( _T("Join tab updated") );
  wxLogMessage( _T("Reloading Singleplayer tab") );
  GetSPTab().OnUnitSyncReloaded();
  wxLogMessage( _T("Singleplayer tab updated") );
}

void MainWindow::OnShowSettingsPP( wxCommandEvent& event )
{
	se_frame = new settings_frame(this,wxID_ANY,wxT("Settings++"),wxDefaultPosition,
	  	    		wxDefaultSize);
	se_frame_active = true;
	se_frame->Show();
}

void MainWindow::OnShowToolTips( wxCommandEvent& event )
{
    bool show = m_menuTools->IsChecked(MENU_SHOW_TOOLTIPS);
    wxToolTip::Enable(show);
    sett().SetShowTooltips(show);
}

void MainWindow::OnMenuAutojoinChannels( wxCommandEvent& event )
{
    m_autojoin_dialog = new AutojoinChannelDialog (this);
    m_autojoin_dialog->Show();
}
