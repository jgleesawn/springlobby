//
// Class: IconImageList
//

#ifndef _ICONIMAGELIST_H_
#define _ICONIMAGELIST_H_

#include <wx/imaglist.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/colour.h>
#include <wx/string.h>

#include "utils.h"

#define ICON_ADMIN 0
#define ICON_ADMIN_AWAY 1
#define ICON_ADMIN_INGAME 2
#define ICON_BOT 3
#define ICON_NOSTATE -1
#define ICON_AWAY 4
#define ICON_INGAME 5

#define ICON_UP 6
#define ICON_DOWN 7

#define ICON_RANK_NONE -1
#define ICON_RANK0 8
#define ICON_RANK1 9
#define ICON_RANK2 10
#define ICON_RANK3 11
#define ICON_RANK4 12

#define ICON_READY 13
#define ICON_NREADY 15

#define ICON_GAME_UNKNOWN -1
#define ICON_OPEN_GAME 13
#define ICON_OPEN_PW_GAME 14
#define ICON_CLOSED_GAME 15
#define ICON_CLOSED_PW_GAME 16
#define ICON_STARTED_GAME 17

#define ICON_READY_UNSYNC 18
#define ICON_NREADY_UNSYNC 19
#define ICON_READY_QSYNC 20
#define ICON_NREADY_QSYNC 21

#define ICON_NEXISTS 22
#define ICON_EXISTS 23

#define ICON_SPECTATOR 24

#define ICON_ARM 25
#define ICON_CORE 26

#define ICON_COLOURS_START 27
#define NUM_COLOUR_ICONS 16

#define ICON_UNK_FLAG ICON_COLOURS_START+NUM_COLOUR_ICONS
#define ICON_FLAGS_BASE ICON_UNK_FLAG+1

#include <string>
#include <wx/imaglist.h>
class Battle;
struct UserStatus;

class IconImageList : public wxImageList
{
  public:
    IconImageList();
  
    // IconImageList interface
  
    static int GetUserStateIcon( const UserStatus& us );
    static int GetRankIcon( const int& rank, const bool& showlowest = true );
    static int GetFlagIcon( const std::string& flagname );
    static int GetBattleStatusIcon( Battle& battle );
    
    static int GetColourIcon( const int& num );
    void SetColourIcon( const int& num, const wxColour& colour );
    static int GetSideIcon( const std::string& side );
    static int GetReadyIcon( const bool& ready, const int& sync = 1 );
    
  protected:
    // IconImageList variables

};

IconImageList& icons();

#endif  //_ICONIMAGELIST_H_

