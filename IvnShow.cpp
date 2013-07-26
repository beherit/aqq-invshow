//---------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#include <process.h>
#pragma hdrstop
#pragma argsused
#include "Aqq.h"
#include "IdTable.h"
//---------------------------------------------------------------------------

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
  return 1;
}
//---------------------------------------------------------------------------

TPluginLink PluginLink;
TPluginInfo PluginInfo;

//OnWindowsEvent
PPluginWindowEvent WindowEvent;
int Event;
UnicodeString Typ;

//Tablica otwartych zak³adek
PPluginContact Contact;
int State;
bool Temporary;
bool IsChat;
bool FromPlugin;
UnicodeString JID;
UnicodeString ContactsTable[100];

//Tablica konktaów do przywracania stanu
TPluginContact FPContact[100];
//Tablica unikatowych ID timera dla danego kontaktu
TIdTable IdTable[100];

//Uchwyt do okna glownego
HWND hwndMain;
//Nazwa klasy okna
char hwndClassName[2048];
//PID procesu AQQ
DWORD AQQPID;
DWORD AQQPIDTemp;
//Do przywracania stanu dla wtyczek
int Free;
int FreeID;
int ID;

//Do porównywania
UnicodeString tmpJID;
UnicodeString tmpNick;
UnicodeString tmpResource;
UnicodeString tmpGroups;
int tmpUserIdx;

//Szukanie okna glownego AQQ
bool CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam)
{
  GetClassName(hWnd, hwndClassName, sizeof(hwndClassName));

  if((UnicodeString)hwndClassName=="TfrmMain")
  {
	GetWindowThreadProcessId(hWnd, &AQQPIDTemp);

	if(AQQPIDTemp==AQQPID)
	{
	  hwndMain = hWnd;
	}
  }
  return true;
}
//---------------------------------------------------------------------------

int ReciveFreeIdTable()
{
  int Index;

  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].ID==0)
	{
	  Index = Count;
	  Count = 100;
	}
	else Index = -1;
  }

  return Index;
}
//---------------------------------------------------------------------------

int ReciveIdTable(int ContactID)
{
  int Index;

  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].ID==ContactID)
	{
	  Index = Count;
	  Count = 100;
	}
	else Index = -1;
  }

  return Index;
}
//---------------------------------------------------------------------------

int ReciveCountTable(int ContactCount)
{
  int Index;

  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].Count==ContactCount)
	{
	  Index = Count;
	  Count = 100;
	}
	else Index = -1;
  }

  return Index;
}
//---------------------------------------------------------------------------


bool TimerID(int TimerIdEvent)
{
  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].ID==TimerIdEvent)
	 return true;
  }
  return false;
}
//---------------------------------------------------------------------------

int ReciveFreeFPContact()
{
  int Index;

  for(int Count=0;Count<100;Count++)
  {
	if(FPContact[Count].cbSize==0)
	{
	  Index = Count;
	  Count = 100;
	}
	else Index = -1;
  }

  return Index;
}
//---------------------------------------------------------------------------

int ReciveFPContact(int ContactID)
{
  int Index;

  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].ID==ContactID)
	{
	  Index = IdTable[Count].Count;
	  Count = 100;
	}
	else Index = -1;
  }

  return Index;
}
//---------------------------------------------------------------------------

VOID CALLBACK Timer(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
  switch(msg)
  {
	case WM_TIMER:
	{
	  if(TimerID(idEvent)==true)
	  {
		 ID = ReciveFPContact(idEvent);
		 PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)&FPContact[ID]);
		 FPContact[ID].cbSize = 0;
		 IdTable[ReciveIdTable(idEvent)].ID = 0;
		 KillTimer(hwndMain,idEvent);
	  }

	  break;
	}
  }
}
//---------------------------------------------------------------------------

int __stdcall OnContactsUpdate (WPARAM wParam, LPARAM lParam)
{
  Contact = (PPluginContact)wParam;
  State = Contact->State;
  Temporary = Contact->Temporary;
  FromPlugin = Contact->FromPlugin;

  //Jezeli kontakt pochodzi z wtyczki i jest na liscie kontaktow
  if((FromPlugin==true)&&(Temporary==false))
  {
	for(int Count=0;Count<100;Count++)
	{
	  if(FPContact[Count].cbSize!=0)
	  {
		tmpJID = (wchar_t*)Contact->JID;
		tmpNick = (wchar_t*)Contact->Nick;
		tmpResource = (wchar_t*)Contact->Resource;
		tmpGroups = (wchar_t*)Contact->Groups;
		tmpUserIdx = Contact->UserIdx;

		if(
		((wchar_t*)FPContact[Count].JID==tmpJID)&&
		((wchar_t*)FPContact[Count].Nick==tmpNick)&&
		((wchar_t*)FPContact[Count].Resource==tmpResource)&&
		((wchar_t*)FPContact[Count].Groups==tmpGroups)&&
		(FPContact[Count].UserIdx==tmpUserIdx))
		{
		  FPContact[Count].cbSize = 0;
		  IdTable[ReciveCountTable(Count)].ID = 0;
		  Count = 100;
		}
	  }
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

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
	IsChat = Contact->IsChat;
	if(IsChat==false)
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
  IsChat = Contact->IsChat;
  if(IsChat==false)
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
  IsChat = Contact->IsChat;
  if(IsChat==false)
   JID = JID + "/" + (wchar_t*)(Contact->Resource);

  if(ReciveTableIndex(JID)!=-1)
   ContactsTable[ReciveTableIndex(JID)]="";

  State = Contact->State;
  Temporary = Contact->Temporary;
  FromPlugin = Contact->FromPlugin;
  //Jezeli kontakt pochodzi z wtyczki, jest niewidoczny orazn jest na liscie kontaktow
  if((FromPlugin==true)&&(State==6)&&(Temporary==false))
  {
	Contact->State = 0;

	Free = ReciveFreeFPContact();
	FreeID = ReciveFreeIdTable();
	ID = PluginLink.CallService(AQQ_FUNCTION_GETNUMID,0,0);
	IdTable[FreeID].ID = ID;
	IdTable[FreeID].Count = Free;

	FPContact[Free].cbSize = Contact->cbSize;

	FPContact[Free].JID = (wchar_t*)realloc(FPContact[Free].JID, sizeof(wchar_t)*(wcslen(Contact->JID)+1));
	memcpy(FPContact[Free].JID, Contact->JID, sizeof(wchar_t)*wcslen(Contact->JID));
	FPContact[Free].JID[wcslen(Contact->JID)] = L'\0';

	FPContact[Free].Nick = (wchar_t*)realloc(FPContact[Free].Nick, sizeof(wchar_t)*(wcslen(Contact->Nick)+1));
	memcpy(FPContact[Free].Nick, Contact->Nick, sizeof(wchar_t)*wcslen(Contact->Nick));
	FPContact[Free].Nick[wcslen(Contact->Nick)] = L'\0';

	FPContact[Free].Resource = (wchar_t*)realloc(FPContact[Free].Resource, sizeof(wchar_t)*(wcslen(Contact->Resource)+1));
	memcpy(FPContact[Free].Resource, Contact->Resource, sizeof(wchar_t)*wcslen(Contact->Resource));
	FPContact[Free].Resource[wcslen(Contact->Resource)] = L'\0';

	FPContact[Free].Groups = (wchar_t*)realloc(FPContact[Free].Groups, sizeof(wchar_t)*(wcslen(Contact->Groups)+1));
	memcpy(FPContact[Free].Groups, Contact->Groups, sizeof(wchar_t)*wcslen(Contact->Groups));
	FPContact[Free].Groups[wcslen(Contact->Groups)] = L'\0';

	FPContact[Free].State = Contact->State;

	FPContact[Free].Status = (wchar_t*)realloc(FPContact[Free].Status, sizeof(wchar_t)*(wcslen(Contact->Status)+1));
	memcpy(FPContact[Free].Status, Contact->Status, sizeof(wchar_t)*wcslen(Contact->Status));
	FPContact[Free].Status[wcslen(Contact->Status)] = L'\0';

	FPContact[Free].Temporary = Contact->Temporary;
	FPContact[Free].FromPlugin = Contact->FromPlugin;
	FPContact[Free].UserIdx = Contact->UserIdx;
	FPContact[Free].Subscription = Contact->Subscription;
	FPContact[Free].IsChat = Contact->Subscription;

	SetTimer(hwndMain,ID,300000,(TIMERPROC)Timer);
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
	IsChat = Contact->IsChat;
	if(IsChat==false)
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
  //Sprawdzanie wersji AQQ
  if(CompareVersion(AQQVersion,PLUGIN_MAKE_VERSION(2,2,0,33))<0)
  {
	MessageBox(Application->Handle,
	  "Wymagana wesja AQQ przez wtyczkê to minimum 2.2.0.33!\n"
	  "Wtyczka InvShow nie bêdzie dzia³aæ poprawnie!",
	  "Nieprawid³owa wersja AQQ",
	  MB_OK | MB_ICONEXCLAMATION);
  }
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = (wchar_t*)L"InvShow";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,0,4,0);
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

  //Pobranie PID procesu AQQ
  AQQPID = getpid();
  //Szukanie uchwytu do okna glownego AQQ
  EnumWindows((WNDENUMPROC)FindMainWindow,0);

  //Resetowanie tablicy
  for(int TableCount=0;TableCount<100;TableCount++)
   ContactsTable[TableCount] = "";

  for(int TableCount=0;TableCount<100;TableCount++)
   FPContact[TableCount].cbSize = 0;

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
  //Hook na zmianê stanu kontaktu
  PluginLink.HookEvent(AQQ_CONTACTS_UPDATE,OnContactsUpdate);

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Unload()
{
  PluginLink.UnhookEvent(OnFetchAllTabs);
  PluginLink.UnhookEvent(OnReceiveMessage);
  PluginLink.UnhookEvent(OnActiveTab);
  PluginLink.UnhookEvent(OnCloseTab);
  PluginLink.UnhookEvent(OnContactsUpdate);

  return 0;
}
//---------------------------------------------------------------------------
