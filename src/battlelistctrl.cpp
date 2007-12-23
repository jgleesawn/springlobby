/* Copyright (C) 2007 The SpringLobby Team. All rights reserved. */

#include <wx/intl.h>
#include <wx/menu.h>
#include <wx/string.h>

#include "battlelistctrl.h"
#include "utils.h"
#include "user.h"
#include "iconimagelist.h"
#include "battle.h"
#include "uiutils.h"
#include "ui.h"
#include "server.h"

#define TOOLTIP_DELAY 1000

BEGIN_EVENT_TABLE(BattleListCtrl, customListCtrl)

  EVT_LIST_ITEM_SELECTED   ( BLIST_LIST, BattleListCtrl::OnSelected )
  EVT_LIST_ITEM_DESELECTED ( BLIST_LIST, BattleListCtrl::OnDeselected )
  EVT_LIST_DELETE_ITEM     ( BLIST_LIST, BattleListCtrl::OnDeselected )
  EVT_LIST_ITEM_RIGHT_CLICK( BLIST_LIST, BattleListCtrl::OnListRightClick )
  EVT_LIST_COL_CLICK       ( BLIST_LIST, BattleListCtrl::OnColClick )
  EVT_MENU                 ( BLIST_DLMAP, BattleListCtrl::OnDLMap )
  EVT_MENU                 ( BLIST_DLMOD, BattleListCtrl::OnDLMod )
#ifndef __WXMSW__ //disables tooltips on win
  EVT_MOTION(BattleListCtrl::OnMouseMotion)
#endif

END_EVENT_TABLE()

#ifdef __WXMSW__
	#define nonIcon ICON_EMPTY
#else
	#define nonIcon -1
#endif

Ui* BattleListCtrl::m_ui_for_sort = 0;

BattleListCtrl::BattleListCtrl( wxWindow* parent, Ui& ui ):
  customListCtrl(parent, BLIST_LIST, wxDefaultPosition, wxDefaultSize, wxSUNKEN_BORDER | wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_ALIGN_LEFT),
  m_selected(-1),
  m_ui(ui)
{

  SetImageList( &icons(), wxIMAGE_LIST_NORMAL );
  SetImageList( &icons(), wxIMAGE_LIST_SMALL );
  SetImageList( &icons(), wxIMAGE_LIST_STATE );
  

  wxListItem col;

  col.SetText( _T("s") );
  col.SetImage( nonIcon );
  InsertColumn( 0, col, _T("Status"), false );

  col.SetText( _T("c") );
  col.SetImage( nonIcon );
  InsertColumn( 1, col, _T("Country"), false);

  col.SetText( _T("r") );
  col.SetImage(  nonIcon);
  InsertColumn( 2, col, _T("Minimum rank to join"), false );

  col.SetText( _("Description") );
  col.SetImage( nonIcon );
  InsertColumn( 3, col, _T("Game description") );

  col.SetText( _("Map") );
  col.SetImage( nonIcon );
  InsertColumn( 4, col, _T("Mapname") );

  col.SetText( _("Mod") );
  col.SetImage( nonIcon );
  InsertColumn( 5, col, _T("Modname") );

  col.SetText( _("Host") );
  col.SetImage( nonIcon);
  InsertColumn( 6, col, _T("Name of the Host") );
  
  col.SetText( _("s") );
  col.SetImage( nonIcon );
  InsertColumn( 7, col, _T("Number of Spectators"), false );

  col.SetText( _("p") );
  col.SetImage( nonIcon );
  InsertColumn( 8, col, _T("Number of Players joined"), false );

  col.SetText( _("m") );
  col.SetImage(  nonIcon);
  InsertColumn( 9, col, _T("Maximum number of Players that can join"), false );

  m_sortorder[0].col = 0;
  m_sortorder[0].direction = true;
  m_sortorder[1].col = 5;
  m_sortorder[1].direction = true;
  m_sortorder[2].col = 9;
  m_sortorder[2].direction = true;
  m_sortorder[3].col = 4;
  m_sortorder[3].direction = true;
  Sort( );

#ifdef __WXMSW__
  SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
  SetColumnWidth( 1, wxLIST_AUTOSIZE_USEHEADER );
  SetColumnWidth( 2, wxLIST_AUTOSIZE_USEHEADER );
  SetColumnWidth( 7, wxLIST_AUTOSIZE_USEHEADER );
  SetColumnWidth( 8, wxLIST_AUTOSIZE_USEHEADER );
  SetColumnWidth( 9, wxLIST_AUTOSIZE_USEHEADER );

#else
  SetColumnWidth( 0, 20 );
  SetColumnWidth( 1, 20 );
  SetColumnWidth( 2, 20 ); 

  SetColumnWidth( 7, 28 ); // alittle more than before for dual digets
  SetColumnWidth( 8, 28 );
  SetColumnWidth( 9, 28 );
#endif
 
  SetColumnWidth( 3, 170 );
  SetColumnWidth( 4, 140 );
  SetColumnWidth( 5, 130 );
  SetColumnWidth( 6, 110 );
 
  m_popup = new wxMenu( _T("") );
  m_popup->Append( BLIST_DLMAP, _("Download &map") );
  m_popup->Append( BLIST_DLMOD, _("Download m&od") );
}


BattleListCtrl::~BattleListCtrl()
{
  delete m_popup;
}


void BattleListCtrl::OnSelected( wxListEvent& event )
{
  m_selected = GetItemData( event.GetIndex() );
  event.Skip();
}


void BattleListCtrl::OnDeselected( wxListEvent& event )
{
  if ( m_selected == (int)GetItemData( event.GetIndex() )  )
  m_selected = -1;
}


int BattleListCtrl::GetSelectedIndex()
{
  return m_selected;
}


void BattleListCtrl::OnListRightClick( wxListEvent& event )
{
  PopupMenu( m_popup );
}


void BattleListCtrl::OnDLMap( wxCommandEvent& event )
{
  if ( m_selected != -1 ) {
    if ( m_ui.GetServer().battles_iter->BattleExists(m_selected) ) {
      m_ui.DownloadMap( m_ui.GetServer().battles_iter->GetBattle(m_selected).GetMapName() );
    }
  }
}


void BattleListCtrl::OnDLMod( wxCommandEvent& event )
{
  if ( m_selected != -1 ) {
    if ( m_ui.GetServer().battles_iter->BattleExists(m_selected) ) {
      m_ui.DownloadMod( m_ui.GetServer().battles_iter->GetBattle(m_selected).GetModName() );
    }
  }
}


void BattleListCtrl::OnColClick( wxListEvent& event )
{
  if ( event.GetColumn() == -1 ) return;
  wxListItem col;
  GetColumn( m_sortorder[0].col, col );
  col.SetImage( nonIcon );
  SetColumn( m_sortorder[0].col, col );

  int i;
  for ( i = 0; m_sortorder[i].col != event.GetColumn() && i < 4; ++i ) {}
  if ( i > 3 ) { i = 3; }
  for ( ; i > 0; i--) { m_sortorder[i] = m_sortorder[i-1]; }
  m_sortorder[0].col = event.GetColumn();
  m_sortorder[0].direction = !m_sortorder[0].direction;


  GetColumn( m_sortorder[0].col, col );
  //col.SetImage( ( m_sortorder[0].direction )?ICON_UP:ICON_DOWN );
  SetColumn( m_sortorder[0].col, col );

  Sort();
}


void BattleListCtrl::Sort()
{
  BattleListCtrl::m_ui_for_sort = &m_ui;
  if (!m_ui_for_sort || !m_ui_for_sort->GetServerStatus()  ) return;
  for (int i = 3; i >= 0; i--) {
    switch ( m_sortorder[ i ].col ) {
      case 0 : SortItems( ( m_sortorder[ i ].direction )?&CompareStatusUP:&CompareStatusDOWN , 0 ); break;
      case 1 : SortItems( ( m_sortorder[ i ].direction )?&CompareCountryUP:&CompareCountryDOWN , 0 ); break;
      case 2 : SortItems( ( m_sortorder[ i ].direction )?&CompareRankUP:&CompareRankDOWN , 0 ); break;
      case 3 : SortItems( ( m_sortorder[ i ].direction )?&CompareDescriptionUP:&CompareDescriptionDOWN , 0 ); break;
      case 4 : SortItems( ( m_sortorder[ i ].direction )?&CompareMapUP:&CompareMapDOWN , 0 ); break;
      case 5 : SortItems( ( m_sortorder[ i ].direction )?&CompareModUP:&CompareModDOWN , 0 ); break;
      case 6 : SortItems( ( m_sortorder[ i ].direction )?&CompareHostUP:&CompareHostDOWN , 0 ); break;
      case 7 : SortItems( ( m_sortorder[ i ].direction )?&CompareSpectatorsUP:&CompareSpectatorsDOWN , 0 ); break;
      case 8 : SortItems( ( m_sortorder[ i ].direction )?&ComparePlayerUP:&ComparePlayerDOWN , 0 ); break;
      case 9 : SortItems( ( m_sortorder[ i ].direction )?&CompareMaxPlayerUP:&CompareMaxPlayerDOWN , 0 ); break;
    }
  }
}


int wxCALLBACK BattleListCtrl::CompareStatusUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  int b1 = 0, b2 = 0;

  if ( battle1.GetInGame() )
    b1 += 1000;
  if ( battle2.GetInGame() )
    b2 += 1000;
  if ( battle1.IsLocked() )
    b1 += 100;
  if ( battle2.IsLocked() )
    b2 += 100;
  if ( battle1.IsPassworded() )
    b1 += 50;
  if ( battle2.IsPassworded() )
    b2 += 50;
  if ( battle1.IsFull() )
    b1 += 25;
  if ( battle2.IsFull() )
    b2 += 25;

  if ( b1 > 1000 ) b1 = 1000;
  if ( b2 > 1000 ) b2 = 1000;

  // inverse the order
  if ( b1 < b2 )
      return -1;
  if ( b1 > b2 )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareStatusDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  int b1 = 0, b2 = 0;

  if ( battle1.GetInGame() )
    b1 += 1000;
  if ( battle2.GetInGame() )
    b2 += 1000;
  if ( battle1.IsLocked() )
    b1 += 100;
  if ( battle2.IsLocked() )
    b2 += 100;
  if ( battle1.IsPassworded() )
    b1 += 50;
  if ( battle2.IsPassworded() )
    b2 += 50;
  if ( battle1.IsFull() )
    b1 += 25;
  if ( battle2.IsFull() )
    b2 += 25;

  if ( b1 > 1000 ) b1 = 1000;
  if ( b2 > 1000 ) b2 = 1000;

  // inverse the order
  if ( b1 < b2 )
      return 1;
  if ( b1 > b2 )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareRankUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetRankNeeded() < battle2.GetRankNeeded() )
      return -1;
  if ( battle1.GetRankNeeded() > battle2.GetRankNeeded() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareRankDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetRankNeeded() < battle2.GetRankNeeded() )
      return 1;
  if ( battle1.GetRankNeeded() > battle2.GetRankNeeded() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareCountryUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( WX_STRING(battle1.GetFounder().GetCountry()).MakeUpper() < WX_STRING(battle2.GetFounder().GetCountry()).MakeUpper() )
      return -1;
  if ( WX_STRING(battle1.GetFounder().GetCountry()).MakeUpper() > WX_STRING(battle2.GetFounder().GetCountry()).MakeUpper() )
      return 1;

  return 0;
}



int wxCALLBACK BattleListCtrl::CompareCountryDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( WX_STRING(battle1.GetFounder().GetCountry()).MakeUpper() < WX_STRING(battle2.GetFounder().GetCountry()).MakeUpper() )
      return 1;
  if ( WX_STRING(battle1.GetFounder().GetCountry()).MakeUpper() > WX_STRING(battle2.GetFounder().GetCountry()).MakeUpper() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareDescriptionUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( WX_STRING(battle1.GetDescription()).MakeUpper() < WX_STRING(battle2.GetDescription()).MakeUpper() )
      return -1;
  if ( WX_STRING(battle1.GetDescription()).MakeUpper() > WX_STRING(battle2.GetDescription()).MakeUpper() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareDescriptionDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( (WX_STRING(battle1.GetDescription()).MakeUpper()) < (WX_STRING(battle2.GetDescription()).MakeUpper()) )
   {
      return 1;
   }
  if ( (WX_STRING(battle1.GetDescription()).MakeUpper()) > (WX_STRING(battle2.GetDescription()).MakeUpper()) )
    {
      return -1;
    }

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareMapUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( RefineMapname( battle1.GetMapName() ).MakeUpper() < RefineMapname( battle2.GetMapName() ).MakeUpper() )
      return -1;
  if ( RefineMapname( battle1.GetMapName() ).MakeUpper() > RefineMapname( battle2.GetMapName() ).MakeUpper() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareMapDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( RefineMapname( battle1.GetMapName() ).MakeUpper() < RefineMapname( battle2.GetMapName() ).MakeUpper() )
      return 1;
  if ( RefineMapname( battle1.GetMapName() ).MakeUpper() > RefineMapname( battle2.GetMapName() ).MakeUpper() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareModUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( RefineModname( battle1.GetModName() ).MakeUpper() < RefineModname( battle2.GetModName() ).MakeUpper() )
      return -1;
  if ( RefineModname( battle1.GetModName() ).MakeUpper() > RefineModname( battle2.GetModName() ).MakeUpper() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareModDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( RefineModname( battle1.GetModName() ).MakeUpper() < RefineModname( battle2.GetModName() ).MakeUpper() )
      return 1;
  if ( RefineModname( battle1.GetModName() ).MakeUpper() > RefineModname( battle2.GetModName() ).MakeUpper() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareHostUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( WX_STRING(battle1.GetFounder().GetNick()).MakeUpper() < WX_STRING(battle2.GetFounder().GetNick()).MakeUpper() )
      return -1;
  if ( WX_STRING(battle1.GetFounder().GetNick()).MakeUpper() > WX_STRING(battle2.GetFounder().GetNick()).MakeUpper() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareHostDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( WX_STRING(battle1.GetFounder().GetNick()).MakeUpper() < WX_STRING(battle2.GetFounder().GetNick()).MakeUpper() )
      return 1;
  if ( WX_STRING(battle1.GetFounder().GetNick()).MakeUpper() > WX_STRING(battle2.GetFounder().GetNick()).MakeUpper() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareSpectatorsUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetSpectators() < battle2.GetSpectators() )
      return -1;
  if ( battle1.GetSpectators() > battle2.GetSpectators() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareSpectatorsDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetSpectators() < battle2.GetSpectators() )
      return 1;
  if ( battle1.GetSpectators() > battle2.GetSpectators() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::ComparePlayerUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetNumUsers() - battle1.GetSpectators() < battle2.GetNumUsers() - battle2.GetSpectators() )
      return -1;
  if ( battle1.GetNumUsers() - battle1.GetSpectators() > battle2.GetNumUsers() - battle2.GetSpectators() )
      return 1;

  return 0;
}



int wxCALLBACK BattleListCtrl::ComparePlayerDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetNumUsers() - battle1.GetSpectators() < battle2.GetNumUsers() - battle2.GetSpectators() )
      return 1;
  if ( battle1.GetNumUsers() - battle1.GetSpectators() > battle2.GetNumUsers() - battle2.GetSpectators() )
      return -1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareMaxPlayerUP(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetMaxPlayers() < battle2.GetMaxPlayers() )
      return -1;
  if ( battle1.GetMaxPlayers() > battle2.GetMaxPlayers() )
      return 1;

  return 0;
}


int wxCALLBACK BattleListCtrl::CompareMaxPlayerDOWN(long item1, long item2, long sortData)
{
  Ui* ui = m_ui_for_sort;
  Battle& battle1 = ui->GetServer().battles_iter->GetBattle(item1);
  Battle& battle2 = ui->GetServer().battles_iter->GetBattle(item2);

  if ( battle1.GetMaxPlayers() < battle2.GetMaxPlayers() )
      return 1;
  if ( battle1.GetMaxPlayers() > battle2.GetMaxPlayers() )
      return -1;

  return 0;
}

void BattleListCtrl::OnMouseMotion(wxMouseEvent& event)
{
	if (tipTimer.IsRunning() == true)
	{
		tipTimer.Stop();
	}

	wxPoint position = event.GetPosition();

	int flag = wxLIST_HITTEST_ONITEM;
	long *ptrSubItem = new long;
	try{ 
		tipTimer.Start(TOOLTIP_DELAY, wxTIMER_ONE_SHOT);
		long item = GetItemData(HitTest(position, flag, ptrSubItem));

		if (item != wxNOT_FOUND)
		{
			Ui* ui = m_ui_for_sort;
			Battle& battle = ui->GetServer().battles_iter->GetBattle(item);
			int coloumn = getColoumnFromPosition(position);
			switch (coloumn)
			{
			case 0: // status
			m_tiptext = IconImageList::GetBattleStatus(battle);
				break;	
			case 1: // country
				m_tiptext = WX_STRING(battle.GetFounder().GetCountry());
				break;	
			case 2: // rank_min
				m_tiptext = WX_STRING(m_colinfovec[coloumn].first);
				break;	
			case 3: // descrp
				m_tiptext = WX_STRING(battle.GetDescription());
				break;
			case 4: //map
				m_tiptext = WX_STRING(battle.GetMapName());
				break;
			case 5: //mod
				m_tiptext = WX_STRING(battle.GetModName());
				break;
			case 6: // host
				m_tiptext = WX_STRING(battle.GetFounder().GetNick());
				break;
			case 7: // specs
				m_tiptext = _T("Spectators:\n");
				for (unsigned int i = battle.GetNumUsers()-1; i > battle.GetNumUsers() - battle.GetSpectators()-1;--i)
				{
					if (i < battle.GetNumUsers()-1)
						m_tiptext << _T("\n");
					m_tiptext << WX_STRING(battle.GetUser(i).GetNick()) ;
				}
				break;
			case 8: // player
				m_tiptext = _T("Active Players:\n");
				for (unsigned int i = 0; i < battle.GetNumUsers()-battle.GetSpectators();++i)
				{
					if ( i> 0)
						m_tiptext << _T("\n");
					m_tiptext << WX_STRING(battle.GetUser(i).GetNick());
				}
				break;
			case 9: //may player
				m_tiptext = (m_colinfovec[coloumn].first);
				break;  	

			default: m_tiptext = _T("");
				break;
			}
		}
	}
	catch(...){}
}


