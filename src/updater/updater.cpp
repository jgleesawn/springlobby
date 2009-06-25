#include "versionchecker.h"
#include "../settings++/custom_dialogs.h"
#include "../utils.h"
#include "updater.h"
#include "../settings.h"
#include "../globalsmanager.h"
#include "../ui.h"
#include "../mainwindow.h"
#include "../httpdownloader.h"

#include <wx/stdpaths.h>
#include <wx/filefn.h>
#include <wx/filename.h>


BEGIN_EVENT_TABLE(UpdaterClass, wxEvtHandler)
    EVT_COMMAND(wxID_ANY, httpDownloadEvtComplete,  UpdaterClass::OnDownloadEvent)
    EVT_COMMAND(wxID_ANY, httpDownloadEvtFailed,    UpdaterClass::OnDownloadEvent)
END_EVENT_TABLE()

UpdaterClass& Updater()
{
    static GlobalObjectHolder<UpdaterClass> m_upd;
    return m_upd;
}


UpdaterClass::UpdaterClass()
{
}


UpdaterClass::~UpdaterClass()
{
}

void UpdaterClass::CheckForUpdates()
{
    m_cur_mw_title = ui().mw().GetTitle();

  wxString latestVersion = GetLatestVersion();

  if (latestVersion == _T("-1"))
  {
    customMessageBoxNoModal(SL_MAIN_ICON, _("There was an error checking for the latest version.\nPlease try again later.\nIf the problem persists, please use Help->Report Bug to report this bug."), _("Error"));
    return;
  }
  wxString myVersion = GetSpringLobbyVersion() ;

  wxString msg = _("Your Version: ") + myVersion + _T("\n") + _("Latest Version: ") + latestVersion;
  if ( !latestVersion.IsSameAs(myVersion, false) )
  {
      #ifdef __WXMSW__
      int answer = customMessageBox(SL_MAIN_ICON, _("Your SpringLobby version is not up to date.\n\n") + msg + _("\n\nWould you like for me to autodownload the new version? Changes will take effect next you launch the lobby again."), _("Not up to date"), wxYES_NO);
      if (answer == wxYES)
      {
          ui().mw().SetTitle( _("SpringLobby -- downloading update") );
          if ( IsUACenabled() ) {
            WinExecuteAdmin( wxStandardPaths::Get().GetExecutablePath(), _T("-u") );
          }
          else
            StartUpdate( latestVersion );

      }
    #else
    customMessageBox(SL_MAIN_ICON, _("Your SpringLobby version is not up to date.\n\n") + msg, _("Not up to Date") );
    #endif
  }
}

#ifdef __WXMSW__
void UpdaterClass::StartUpdate( const wxString& latestVersion, bool fromCli )
{
    m_fromCli = fromCli;
    wxString sep = wxFileName::GetPathSeparator();
    wxString currentexe = wxStandardPaths::Get().GetExecutablePath();
    if ( !wxFileName::IsDirWritable( currentexe.BeforeLast( wxFileName::GetPathSeparator() ) + wxFileName::GetPathSeparator() ) )
    {
      customMessageBoxNoModal(SL_MAIN_ICON, _("Unable to write to the lobby installation directory.\nPlease update manually or enable write permissions for the current user."), _("Error"));
      return;
    }
    m_newexe = sett().GetLobbyWriteDir() + _T("update") + sep;
    wxMkdir( m_newexe );
    wxString url = _T("springlobby.info/windows/springlobby-") + latestVersion + _T("-win32.zip");
    new HttpDownloaderThread<UpdaterClass>( url, m_newexe + _T("temp.zip"), *this, wxID_HIGHEST + 10000, true, true );
}
#endif

void UpdaterClass::OnDownloadEvent( wxCommandEvent& event )
{
	int code = event.GetInt();
  if ( code != 0) customMessageBox(SL_MAIN_ICON, _("There was an error downloading for the latest version.\nPlease try again later.\nIf the problem persists, please use Help->Report Bug to report this bug."), _("Error"));
  else
  {
    if ( !UpdateExe( m_newexe , false ) ) {
        if ( !m_fromCli ) {
            customMessageBoxNoModal(SL_MAIN_ICON, wxString::Format( _("There was an error while trying to replace the current executable version\n manual copy is necessary from: %s\n to: %s\nPlease use Help->Report Bug to report this bug."), m_newexe.c_str(), wxStandardPaths::Get().GetExecutablePath().c_str() ), _("Error"));
        }
        else {
            customMessageBox(SL_MAIN_ICON, wxString::Format( _("There was an error while trying to replace the current executable version\n manual copy is necessary from: %s\n to: %s\nPlease use Help->Report Bug to report this bug."), m_newexe.c_str(), wxStandardPaths::Get().GetExecutablePath().c_str() ), _("Error"));
        }
    }
    else
    {
        bool locale_ok = UpdateLocale( m_newexe, false );
        if ( locale_ok ) {
            if ( !m_fromCli ) {
                customMessageBoxNoModal(SL_MAIN_ICON, _("Update complete. The changes will be available next lobby start."), _("Success"));
            }
            else {
                customMessageBox(SL_MAIN_ICON, _("Update complete. The changes will be available next lobby start."), _("Success"));
                wxRmdir( m_newexe );
                wxTheApp->ExitMainLoop();
            }
        }
        else {
            if ( !m_fromCli ) {
                customMessageBoxNoModal(SL_MAIN_ICON, _("Binary updated successfully. \nSome translation files could not be updated.\nPlease report this in #springlobby after restarting."), _("Partial success"));
            }
            else {
                customMessageBox(SL_MAIN_ICON, _("Binary updated successfully. \nSome translation files could not be updated.\nPlease report this in #springlobby after restarting."), _("Partial success"));
                wxRmdir( m_newexe );
                wxTheApp->ExitMainLoop();
            }
        }
        wxRmdir( m_newexe );
    }
  }

    if ( !m_fromCli )
        ui().mw().SetTitle( m_cur_mw_title );
}

bool UpdaterClass::UpdateLocale( const wxString& tempdir, bool WaitForReboot )
{
    wxString target = wxPathOnly( wxStandardPaths::Get().GetExecutablePath() ) + wxFileName::GetPathSeparator() + _T("locale");
    wxString origin = tempdir + _T("locale") + wxFileName::GetPathSeparator() ;
    return CopyDir( origin, target );
}

bool UpdaterClass::UpdateExe( const wxString& newexe, bool WaitForReboot )
{
    //this always returns false on msw
//  if ( !wxFileName::IsFileExecutable( newexe + _T("springlobby.exe") ) )
//  {
//      customMessageBoxNoModal(SL_MAIN_ICON, _("not exe."), _("Error"));
//      return false;
//  }
  wxString currentexe = wxStandardPaths::Get().GetExecutablePath();

  wxString backupfile =  currentexe + _T(".bak");
  wxRemoveFile( backupfile );
  if ( !wxRenameFile( currentexe, backupfile ) )
    return false;

  if ( !wxCopyFile( newexe + _T("springlobby.exe"), currentexe ) )
  {
    wxRenameFile(  backupfile, currentexe.AfterLast( wxFileName::GetPathSeparator() )  ); //restore original file from backup on update failure
    return false;
  }

  return true;
}
