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
UnicodeString ContactsTable[100];

int Event;
UnicodeString Typ;

int State;
bool Temporary;

void AddToContactsTable(UnicodeString TableJID)
{
  for(int Count=0;Count<100;Count++)
  {
	if(ContactsTable[Count]=="")
	{
	  ContactsTable[Count] = TableJID;
	  Count = 100;
	}
  }
}
//---------------------------------------------------------------------------

int ReciveTableIndex(UnicodeString TableJID)
{
  int Index;

  for(int Count=0;Count<100;Count++)
  {
	if(ContactsTable[Count]==TableJID)
	{
	  Index = Count;
      Count = 100;
	}
	else Index = -1;
  }

  return Index;
}
//---------------------------------------------------------------------------

int __stdcall OnFetchAllTabs (WPARAM wParam, LPARAM lParam)
{
  if((wParam!=0)&&(lParam!=0))
  {
	Contact = (PPluginContact)lParam;
	JID = (wchar_t*)(Contact->JID);
	Temporary = Contact->Temporary;
	if(Temporary==false)
	 JID = JID + "/" + (wchar_t*)(Contact->Resource);

	if(ReciveTableIndex(JID)==-1)
	 AddToContactsTable(JID);
  }

  return 0;
}
//---------------------------------------------------------------------------

int __stdcall OnActiveTab (WPARAM wParam, LPARAM lParam)
{
  Contact = (PPluginContact)lParam;
  JID = (wchar_t*)(Contact->JID);
  Temporary = Contact->Temporary;
  if(Temporary==false)
   JID = JID + "/" + (wchar_t*)(Contact->Resource);

  if(ReciveTableIndex(JID)==-1)
   AddToContactsTable(JID);

  return 0;
}
//---------------------------------------------------------------------------

int __stdcall OnCloseTab (WPARAM wParam, LPARAM lParam)
{
  Contact = (PPluginContact)lParam;
  JID = (wchar_t*)(Contact->JID);
  Temporary = Contact->Temporary;
  if(Temporary==false)
   JID = JID + "/" + (wchar_t*)(Contact->Resource);

  if(ReciveTableIndex(JID)!=-1)
   ContactsTable[ReciveTableIndex(JID)]="";

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
	JID = JID + "/" + (wchar_t*)(Contact->Resource);

	if(ReciveTableIndex(JID)!=-1)
	{
		Contact->State = 6;
		PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)Contact);
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" __declspec(dllexport) PPluginInfo __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = (wchar_t*)L"InvShow";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,3,0);
  PluginInfo.Description = (wchar_t *)L"";
  PluginInfo.Author = (wchar_t *)L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = (wchar_t *)L"email@beherit.pl";
  PluginInfo.Copyright = (wchar_t *)L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = (wchar_t *)L"http://beherit.pl/";

  return &PluginInfo;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  PluginLink = *Link;

  //Resetowanie tablicy
  for(int TableCount=0;TableCount<100;TableCount++)
   ContactsTable[TableCount] = "";

  //Pobieranie otwartych zakladek
  if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0)==true)
  {
	PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_FETCHALLTABS,OnFetchAllTabs);
	PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);
	PluginLink.UnhookEvent(OnFetchAllTabs);
  }
  //Hook na aktywna zakladke
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_ACTIVETAB,OnActiveTab);
  //Hook na zamkniecie zakladki
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_CLOSETAB,OnCloseTab);
  //Hook na nowa wiadomosc
  PluginLink.HookEvent(AQQ_CONTACTS_RECVMSG,OnReceiveMessage);

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
