/* Copyright (C) 2007 The SpringLobby Team. All rights reserved. */
#include <stdexcept>
#include <iterator>
#include <wx/file.h>
#include <wx/tokenzr.h>
#include <wx/intl.h>
#include <wx/filefn.h>

#include "replaylist.h"
#include <sstream>
#include "../iunitsync.h"
#include "../utils.h"
#include "../settings++/custom_dialogs.h"
#include "../tdfcontainer.h"
#include "replaytab.h"

const unsigned int replay_bulk_limit = 200;
const unsigned int replay_chunk_size = 200;
const unsigned int timer_interval = 500; //miliseconds

BEGIN_EVENT_TABLE(ReplayList,wxEvtHandler)
    	EVT_TIMER(wxID_ANY, ReplayList::OnTimer)
END_EVENT_TABLE()

ReplayList::ReplayList(ReplayTab& replay_tab)
    : m_timer(this,wxID_ANY),m_replay_tab(replay_tab)
{
}

void ReplayList::LoadReplays()
{
    m_filenames = usync().GetReplayList();

    int temp = m_filenames.GetCount();

    if ( m_filenames.GetCount() < replay_bulk_limit )
        LoadReplays( 0, m_filenames.GetCount() );
    else {
        LoadReplays( 0, replay_chunk_size );
        m_current_parse_pos = replay_chunk_size;
        m_timer.Start( timer_interval, wxTIMER_CONTINUOUS );
    }
}

void ReplayList::LoadReplays( const unsigned int from, const unsigned int to)
{
    for (unsigned int i = from; i < to - 1; ++i)
    {
        Replay rep;
        if ( GetReplayInfos( m_filenames[i] , rep ) ){
            AddReplay( rep );
            m_replay_tab.AddReplay( m_replays[rep.id] );
        }
    }
}

void ReplayList::OnTimer(wxTimerEvent& event)
{
    if ( replay_chunk_size + m_current_parse_pos >  m_filenames.GetCount() ){
        //final parse run
        m_timer.Stop();
        LoadReplays( m_current_parse_pos, m_filenames.GetCount() );
    }
    else {
        LoadReplays( m_current_parse_pos, m_current_parse_pos + replay_chunk_size );
        m_current_parse_pos += replay_chunk_size;
    }
}

void ReplayList::AddReplay( Replay replay )
{
  m_replays[replay.id] = replay;
}


void ReplayList::RemoveReplay( replay_id_t const& id ) {
  m_replays.erase(id);
}

replay_map_t::size_type ReplayList::GetNumReplays()
{
  return m_replays.size();
}

Replay ReplayList::GetReplayById( replay_id_t const& id ) {
//TODO catch
  replay_iter_t b = m_replays.find(id);
  if (b == m_replays.end())
    throw std::runtime_error("ReplayList_Iter::GetReplay(): no such replay");
  return b->second;
}

Replay& ReplayList::GetReplay( int const index ) {
//TODO secure index
  replay_iter_t b = m_replays.begin();
  std::advance(b,index);
  if (b == m_replays.end())
    throw std::runtime_error("ReplayList_Iter::GetReplay(): no such replay");
  return b->second;
}

bool ReplayList::ReplayExists( replay_id_t const& id )
{
  return m_replays.find(id) != m_replays.end();
}

bool ReplayList::GetReplayInfos ( const wxString& ReplayPath, Replay& ret )
{
    //wxLOG_Info  ( STD_STRING( ReplayPath ) );
    //TODO extract moar info
    static long r_id = 0;
    ret.Filename = ReplayPath;

    wxString FileName = ReplayPath.AfterLast( '/' ); // strips file path
    FileName = FileName.Left( FileName.Find( _T(".sdf") ) ); //strips the file extension
    FileName = FileName.AfterFirst( _T('-') );

    ret.ReplayName = FileName.AfterLast(_T('-')); // void string if multiple replays wich share previous paramteres aren't present
    FileName = FileName.BeforeLast(_T('-'));
    if ( ret.ReplayName.Contains(_T(".")) ) /// what we just parsed is not a multiple replay but spring version
    {
      ret.SpringVersion = ret.ReplayName;
      ret.ReplayName = _T("");
    }
    else
    {
       ret.SpringVersion = FileName.AfterLast(_T('-'));
       FileName = FileName.BeforeLast(_T('-'));
    }
    ret.MapName = FileName;
    ret.id = r_id;
    wxString script = GetScriptFromReplay( ReplayPath );
    if ( script.IsEmpty() )
        return false;

    GetHeaderInfo( ret, ReplayPath );
    ret.battle = GetBattleFromScript( script );
    ret.ModName = ret.battle.GetHostModName();

    r_id++; //sucessful parsing assumed --> increment id(index)
    return true;
}

wxString ReplayList::GetScriptFromReplay ( const wxString& ReplayPath )
{
    wxString script;

    try
    {
        wxFile replay( ReplayPath, wxFile::read );
        replay.Seek( 20 );
        int headerSize ;
        replay.Read( &headerSize, 4);
        replay.Seek( 64 );
        int scriptSize;
        replay.Read( &scriptSize, 4);
        replay.Seek( headerSize );
        char* script_a = new char[scriptSize];
        replay.Read( script_a, scriptSize );
        wxString script = WX_STRINGC( script_a ) ;//(script_a,scriptSize);

        return script;
    }
    catch (...)
    {
        return wxEmptyString;
    }

}

//BattleOptions GetBattleOptsFromScript( const wxString& script_ )
OfflineBattle ReplayList::GetBattleFromScript( const wxString& script_ )
{
    OfflineBattle battle;
    BattleOptions opts;
    std::stringstream ss ( (const char *)script_.mb_str(wxConvUTF8) );// no need to convert wxstring-->std::string-->std::stringstream, convert directly.
    PDataList script( ParseTDF(ss) );

    PDataList replayNode ( script->Find(_T("GAME") ) );
    if ( replayNode.ok() ){

        wxString modname = replayNode->GetString( _T("GameType") );
        wxString modhash    = replayNode->GetString( _T("ModHash") );
        battle.SetHostMod( modname, modhash );
//        battle.LoadMod();

        //don't have the maphash, what to do?
        //ui download function works with mapname if hash is empty, so works for now
        opts.mapname    = replayNode->GetString( _T("Mapname") );
        battle.SetHostMap( opts.mapname, wxEmptyString );
//        battle.LoadMap();

        opts.ip         = replayNode->GetString( _T("HostIP") );
        opts.port       = replayNode->GetInt  ( _T("HostPort"), DEFAULT_EXTERNAL_UDP_SOURCE_PORT );
        opts.spectators = 0;

        int playernum = replayNode->GetInt  ( _T("NumPlayers"), 0);
        int allynum = replayNode->GetInt  ( _T("NumAllyTeams"), 1);
        int teamnum = replayNode->GetInt  ( _T("NumTeams"), 1);

        //[PLAYERX] sections
        for ( int i = 0; i < playernum ; ++i ){
            PDataList player ( replayNode->Find( _T("PLAYER") + i2s(i) ) );
            if ( player.ok() ) {
                OfflineUser user ( player->GetString( _T("name") ), (player->GetString( _T("countryCode")).Upper() ), 0);
                UserBattleStatus status;
                //how to convert back?
                user.SetSideName( player->GetString( _T("side") ) );
                status.spectator = player->GetInt( _T("Spectator"), 0 );
                opts.spectators += status.spectator;
                status.team = player->GetInt( _T("team") );

                user.UpdateBattleStatus( status );
                battle.AddUser( user );
            }
        }

        //MMoptions, this'll fail unless loading map/mod into wrapper first
//        LoadMMOpts( _T("mapoptions"), battle, replayNode );
//        LoadMMOpts( _T("modoptions"), battle, replayNode );

        opts.maxplayers = playernum ;

    }
    battle.SetBattleOptions( opts );
    return battle;
}

void ReplayList::LoadMMOpts( const wxString& sectionname, OfflineBattle& battle, const PDataList& node )
{
    PDataList section ( node->Find(sectionname) );
    mmOptionsWrapper& opts = battle.CustomBattleOptions();
    for ( PNode n = section->First(); n != section->Last(); n = section->Next( n ) )
        opts.setSingleOption( n->Name(), section->GetString( n->Name() ) );
}

void ReplayList::LoadMMOpts( OfflineBattle& battle, const PDataList& node )
{
    mmOptionsWrapper& opts = battle.CustomBattleOptions();
    typedef std::map<wxString,wxString> optMap;
    optMap options = opts.getOptionsMap(EngineOption);
    for ( optMap::const_iterator i = options.begin(); i != options.end(); ++i)
        opts.setSingleOption( i->first, node->GetString( i->first, i->second ) );
}

void ReplayList::GetHeaderInfo( Replay& rep, const wxString& ReplayPath )
{
    try
    {
        wxFile replay( ReplayPath, wxFile::read );
        replay.Seek( 72 );
        int gametime = 0 ;
        replay.Read( &gametime, 4);
        rep.duration = gametime;
        rep.size = replay.Length();
        unsigned long unixtime = 0;
        replay.Seek( 56 );
        replay.Read( &unixtime, 8 );
        wxDateTime dt;
        dt.Set( (time_t) unixtime );
        wxString date = dt.FormatDate();
        rep.date = date;
    }
    catch (...){ }
}

bool ReplayList::DeleteReplay( replay_id_t const& id )
{
    Replay& rep = m_replays[id];
    if ( wxRemoveFile( rep.Filename ) ) {
        m_filenames.Remove( rep.Filename );
        m_replays.erase(id);
        return true;
    }
    return false;
}
