//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#pragma argsused
#include "Aqq.h"
//---------------------------------------------------------------------------

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
  return 1;
}
//---------------------------------------------------------------------------

TPluginLink PluginLink;
TPluginInfo PluginInfo;
PPluginWindowEvent WindowEvent;
PPluginContact Contact;

UnicodeString JID;
UnicodeString Resource;
UnicodeString JIDTable[100];
int TableCount;

int Event;
UnicodeString Typ;

bool IsThere;

int State;
bool Temporary;

int __stdcall OnFetchAllTabs (WPARAM wParam, LPARAM lParam)
{
  if((wParam!=0)&&(lParam!=0))
  {
	Contact = (PPluginContact)lParam;
	JID = (wchar_t*)(Contact->JID);
	Resource = (wchar_t*)(Contact->Resource);
	JID = JID + "/" + Resource;

	//Zapisywanie JID w wolne miejsce w tablicy
	for(TableCount=0;TableCount<100;TableCount++)
	{
	  if(JIDTable[TableCount]=="")
	  {
		JIDTable[TableCount] = JID;
		TableCount = 100;
	  }
	}

  }

  return 0;
}
//---------------------------------------------------------------------------

int __stdcall OnActiveTab (WPARAM wParam, LPARAM lParam)
{
  Contact = (PPluginContact)lParam;
  JID = (wchar_t*)(Contact->JID);
  Resource = (wchar_t*)(Contact->Resource);
  JID = JID + "/" + Resource;

  IsThere = false;

  //Sprawdzanie czy JID juz istnieje w tablicy
  for(TableCount=0;TableCount<100;TableCount++)
  {
	if(JIDTable[TableCount]==JID)
	{
	  TableCount = 100;
	  IsThere = true;
	}
  }

  //Jesli nie to jest dodawany do tablicy
  if(IsThere==false)
  {
	for(TableCount=0;TableCount<100;TableCount++)
	{
	  if(JIDTable[TableCount]=="")
	  {
		JIDTable[TableCount] = JID;
		TableCount=100;
	  }
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

int __stdcall OnCloseTab (WPARAM wParam, LPARAM lParam)
{
  Contact = (PPluginContact)lParam;
  JID = (wchar_t*)(Contact->JID);
  Resource = (wchar_t*)(Contact->Resource);
  JID = JID + "/" + Resource;

  //Szukanie w tablicy JID
  for(TableCount=0;TableCount<100;TableCount++)
  {
	if(JIDTable[TableCount]==JID)
	{
	  JIDTable[TableCount] = "";
	  TableCount = 100;
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

int __stdcall OnReceiveMessage (WPARAM wParam, LPARAM lParam)
{
  Contact = (PPluginContact)wParam;
  State = Contact->State;
  Temporary = Contact->Temporary;

  //Jezeli kontakt jest rozlazony i jest na liscie kontaktow
  if((State==0)&&(Temporary==false))
  {
	JID = (wchar_t*)(Contact->JID);
	Resource = (wchar_t*)(Contact->Resource);

	if(AnsiPos("@plugin.irc",JID)==0)
	{
	  JID = JID + "/" + Resource;

	  IsThere = false;

	  //Sprawdzanie czy mamy otwarta zakladke z kontaktem
	  for(TableCount=0;TableCount<100;TableCount++)
	  {
		if(JIDTable[TableCount]==JID)
		{
		  TableCount = 100;
		  IsThere = true;
		}
	  }

	  //Jezeli tak to zmienia sie stan kontaktu
	  if(IsThere==true)
	  {
		Contact->State = 6;
		PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)Contact);
	  }
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" __declspec(dllexport) PPluginInfo __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = (wchar_t*)L"InvShow";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,2,0);
  PluginInfo.Description = (wchar_t *)L"";
  PluginInfo.Author = (wchar_t *)L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = (wchar_t *)L"sirbeherit@gmail.com";
  PluginInfo.Copyright = (wchar_t *)L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = (wchar_t *)L"http://beherit.pl/";

  return &PluginInfo;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  PluginLink = *Link;

  //Resetowanie tablicy
  for(TableCount=0;TableCount<100;TableCount++)
   JIDTable[TableCount] = "";

  //Hook na pobieranie aktywnych zakladek
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_FETCHALLTABS,OnFetchAllTabs);
  PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);

  //Hook na nowa wiadomosc
  PluginLink.HookEvent(AQQ_CONTACTS_RECVMSG,OnReceiveMessage);
  //Hook na aktywna zakladke
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_ACTIVETAB,OnActiveTab);
  //Hook na zamkniecie zakladki
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_CLOSETAB,OnCloseTab);

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Unload()
{
  PluginLink.UnhookEvent(OnFetchAllTabs);
  PluginLink.UnhookEvent(OnReceiveMessage);
  PluginLink.UnhookEvent(OnActiveTab);
  PluginLink.UnhookEvent(OnCloseTab);

  return 0;
}
//---------------------------------------------------------------------------
