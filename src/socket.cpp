/* This file is part of the Springlobby (GPL v2 or later), see COPYING */

/**
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

DO NOT CHANGE THIS FILE!

this file is deprecated and will be replaced with

lsl/networking/socket.cpp

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
**/


#ifdef _MSC_VER
#ifndef NOMINMAX
    #define NOMINMAX
#endif // NOMINMAX
#include <winsock2.h>
#endif // _MSC_VER

#include <wx/socket.h>
#include <wx/string.h>
#include <wx/convauto.h>
#include <wx/log.h>
#include <stdexcept>
#include <algorithm>

#include "socket.h"
#include "iserver.h"
#include "utils/conversion.h"

#ifdef __WXMSW__
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <wx/msw/winundef.h>
#include <iphlpapi.h>
#elif defined(linux)
#include <sys/ioctl.h>
#include <net/if.h>
#endif

#define LOCK_SOCKET wxCriticalSectionLocker criticalsection_lock(m_lock)


#ifdef __WXMSW__

bool GetMac(std::vector<unsigned char>& mac)
{
    IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information for 16 cards
    DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer

    DWORD dwStatus = GetAdaptersInfo ( AdapterInfo, &dwBufLen); // Get info
                if (dwStatus != NO_ERROR) return wxEmptyString; // Check status
	for(size_t i = 0; i<sizeof(AdapterInfo); i++) {
		mac.resize(AdapterInfo[i].AddressLength);
		mac.assign(AdapterInfo[i].Address, AdapterInfo[i].Address + AdapterInfo[i].AddressLength);
		for (size_t j=0; j< mac.size(); j++) {
			if (mac[j] != 0) {
				return true;
			}
		}
	}
	return false;
}

#elif defined(__APPLE__)

bool GetMac(std::vector<unsigned char>& mac)
{
	//FIXME: implement this, http://lists.freebsd.org/pipermail/freebsd-hackers/2004-June/007415.html
	return false;
}

#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <ifaddrs.h>

bool GetMac(std::vector<unsigned char>& mac)
{
	ifaddrs * ifap = 0;
	if(getifaddrs(&ifap) == 0) {
		ifaddrs * iter = ifap;
		while(iter) {
				sockaddr_ll * sal = reinterpret_cast<sockaddr_ll*>(iter->ifa_addr);
				if(sal->sll_family == AF_PACKET) {
					mac.resize(sal->sll_halen);
					mac.assign(sal->sll_addr, sal->sll_addr + sal->sll_halen);
					for(size_t i=0; i < mac.size(); i++) {
						if (mac[i] != 0) {
							return true;
						}
					}
				}
				iter = iter->ifa_next;
		}
		freeifaddrs(ifap);
	}
	return false;
}

#endif

std::string MacToString(std::vector<unsigned char>& mac)
{
	std::string res;
	for(size_t i=0; i<mac.size(); i++) {
		char buf[3];
		snprintf(buf, sizeof(buf), "%02X", mac[i]);
		if (!res.empty())
			res+=":";
		res.append(buf, 2);
	}
	return res;
}

std::string _GetHandle()
{
	std::vector<unsigned char> mac;
	std::string res;
	if (GetMac(mac)) {
		res.assign(mac.begin(), mac.end());
		wxLogDebug(_T("Found mac: %s"), TowxString(MacToString(mac)).c_str());
		return std::string(mac.begin(), mac.end());
	}
	return res;
}

BEGIN_EVENT_TABLE(SocketEvents, wxEvtHandler)

EVT_SOCKET(SOCKET_ID, SocketEvents::OnSocketEvent)

END_EVENT_TABLE()


void SocketEvents::OnSocketEvent(wxSocketEvent& event)
{
	Socket* sock = (Socket*)event.GetClientData();
	if (sock == NULL) {
		m_net_class.OnError(_T("sock = 0"));
		return;
	}

	if ( event.GetSocketEvent() == wxSOCKET_INPUT ) {
		m_net_class.OnDataReceived( *sock );
	} else if ( event.GetSocketEvent() == wxSOCKET_LOST ) {
		m_net_class.OnDisconnected( *sock );
	} else if ( event.GetSocketEvent() == wxSOCKET_CONNECTION ) {
		m_net_class.OnConnected( *sock );
	} else {
		m_net_class.OnError(_T("Unknown socket event."));
	}
}


//! @brief Constructor
Socket::Socket( iNetClass& netclass):
    m_sock( NULL ),
    m_events( NULL ),
    m_handle( _GetHandle() ),
    m_connecting( false ),
    m_net_class(netclass),
	m_udp_private_port(0),
    m_rate(-1),
    m_sent(0)
{
}


//! @brief Destructor
Socket::~Socket()
{
  LOCK_SOCKET;
	if ( m_sock ) m_sock->Destroy();
  delete m_events;
}


//! @brief Creates an TCP socket and sets it up.
wxSocketClient* Socket::_CreateSocket()
{
    wxSocketClient* sock = new wxSocketClient();

    sock->SetClientData( (void*)this );
	if ( m_events == 0 )
		m_events = new SocketEvents( m_net_class );
	sock->SetFlags( wxSOCKET_NOWAIT );

	sock->SetEventHandler(*m_events, SOCKET_ID);
	sock->SetNotify( wxSOCKET_CONNECTION_FLAG | wxSOCKET_INPUT_FLAG | wxSOCKET_LOST_FLAG );
	sock->Notify(true);
    return sock;
}

//! @brief Connect to remote host.
void Socket::Connect( const wxString& addr, const int port )
{
	LOCK_SOCKET;

	wxIPV4address wxaddr;
	m_connecting = true;
	m_buffer = "";

	if (!wxaddr.Hostname( addr )) {
		m_net_class.OnError(_T("Invalid Hostname"));
		return;
	}
	if (!wxaddr.Service( port )) {
		m_net_class.OnError(_T("Invalid Port"));
		return;
	}

	if ( m_sock != 0 ) m_sock->Destroy();
	m_sock = _CreateSocket();
	m_sock->Connect( wxaddr, false);
	m_sock->SetTimeout( 40 );
}

void Socket::SetTimeout( const int seconds )
{
    if ( m_sock != 0 )
        m_sock->SetTimeout( seconds );
}

//! @brief Disconnect from remote host if connected.
void Socket::Disconnect( )
{
  if ( m_sock ) m_sock->SetTimeout( 0 );
  m_buffer = "";

  if ( m_sock )
  {
    m_sock->Destroy();
    m_sock = 0;
  }
  m_net_class.OnDisconnected( *this );
}


//! @brief Send data over connection.
bool Socket::Send( const wxString& data )
{
  LOCK_SOCKET;
  return _Send( data );
}


//! @brief Internal send function.
//! @note Does not lock the criticalsection.
bool Socket::_Send( const wxString& data )
{
  if ( !m_sock )
  {
    m_net_class.OnError( _T("Socket NULL") );
    return false;
  }

	m_buffer += (const char*)data.mb_str(wxConvUTF8);
	int crop = m_buffer.length();
  if ( m_rate > 0 )
  {
  	 int max = m_rate - m_sent;
  	 if ( crop > 0 ) crop = max;
  }
	std::string send = m_buffer.substr( 0, crop );
	//wxLogMessage( _T("send: %d  sent: %d  max: %d   :  buff: %d"), send.length() , m_sent, max, m_buffer.length() );
	m_sock->Write( send.c_str(), send.length() );
	if ( !m_sock->Error() )
	{
		wxUint32 sentdata = m_sock->LastCount();
		m_buffer.erase( 0, sentdata );
		m_sent += sentdata;
	}
  return !m_sock->Error();
}


wxString convert(char* buff, const int len)
{
	wxString ret = wxString(buff, wxConvUTF8, len );
	if ( !ret.IsEmpty() ) {
		return ret;
	}
	ret = wxString(buff, wxConvLibc, len);
	if (!ret.empty()) {
		return ret;
	}
	ret = wxString(buff, wxConvLocal, len );
	if ( !ret.IsEmpty() ) {
		return ret;
	}
	ret = wxString(buff, wxConvISO8859_1, len );
	if (!ret.empty()) {
		return ret;
	}
	ret = wxString(buff, wxConvAuto(), len );
	if (!ret.empty()) {
		return ret;
	}
	std::string tmp(buff, len);
	wxLogDebug(_T("Error: invalid charset, replacing invalid chars: '%s'"), TowxString(tmp).c_str());

	//worst case, couldn't convert, replace unknown chars!
	for(int i=0; i<len; i++) {
	if ((buff[i] < '!') || (buff[i] > '~')){
			buff[i] = '_';
		}
	}
	ret = wxString(buff, wxConvUTF8, len );
	if (!ret.empty()) {
		return ret;
	}
	wxLogDebug(_T("Fatal Error: couldn't convert: '%s' in socket.receive()"), TowxString(tmp).c_str());
	return wxEmptyString;
}

//! @brief Receive data from connection
wxString Socket::Receive()
{
	wxString ret;
	if ( m_sock == 0 ) {
		m_net_class.OnError( _T("Socket NULL") );
		return ret;
	}

	LOCK_SOCKET;

	static const int chunk_size = 1500;
	char buf[chunk_size];
	int readnum = 0;

	do {
		m_sock->Read( buf, chunk_size );
		const int readnum = m_sock->LastCount();
		ret += convert(buf, readnum);
	} while ( readnum > 0 );

	return ret;
}

//! @brief Get curent socket state
SockState Socket::State( )
{
  if ( m_sock == 0 ) return SS_Closed;

  LOCK_SOCKET;
  if ( m_sock->IsConnected() ) {
    m_connecting = false;
    return SS_Open;
  } else {
    if ( m_connecting ) {
      return SS_Connecting;
    } else {
      return SS_Closed;
    }
  }
}


//! @brief Get socket error code
//! @todo Implement
SockError Socket::Error( ) const
{
  return (SockError)-1;
}


//! @brief used to retrieve local ip address behind NAT to communicate to the server on login
wxString Socket::GetLocalAddress() const
{
  if ( !m_sock || !m_sock->IsConnected() )
    return wxEmptyString;

  wxIPV4address localaddr;
  m_sock->GetLocal( localaddr );

  return localaddr.IPAddress();
}


//! @brief Set the maximum upload ratio.
void Socket::SetSendRateLimit( int Bps )
{
  m_rate = Bps;
}


void Socket::OnTimer( int mselapsed )
{
  LOCK_SOCKET;

  if ( m_rate > 0 ) {
    m_sent -= int( ( mselapsed / 1000.0 ) * m_rate );
    if ( m_sent < 0 ) m_sent = 0;
    if ( m_buffer.length() > 0 ) _Send(wxEmptyString);
  } else {
    m_sent = 0;
  }
}

