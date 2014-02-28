//---------------------------------------------------------------------------
// Copyright (C) 2010-2014 Krzysztof Grochocki
//
// This file is part of InvShow
//
// InvShow is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// InvShow is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GNU Radio; see the file COPYING. If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street,
// Boston, MA 02110-1301, USA.
//---------------------------------------------------------------------------

#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#pragma argsused
#include <PluginAPI.h>
#include <IdHashMessageDigest.hpp>

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
  return 1;
}
//---------------------------------------------------------------------------

//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
//Uchwyt-do-okna-timera------------------------------------------------------
HWND hTimerFrm;
//Tablica-kontaktow----------------------------------------------------------
TPluginContact ContactsList[100];
//Definicja-struktury-tablicy-unikatowych-ID-timera--------------------------
struct TIdTable
{
  int TimerId;
  int ContactIndex;
};
//Zmienna-tablicy-unikatowych-ID-timera--------------------------------------
TIdTable IdTable[100];
//FORWARD-AQQ-HOOKS----------------------------------------------------------
INT_PTR __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam);
INT_PTR __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam);
//FORWARD-TIMER--------------------------------------------------------------
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//---------------------------------------------------------------------------

//Pobieranie sciezki katalogu prywatnego wtyczek
UnicodeString GetPluginUserDir()
{
  return StringReplace((wchar_t*)PluginLink.CallService(AQQ_FUNCTION_GETPLUGINUSERDIR,0,0), "\\", "\\\\", TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------

//Pobieranie indeksu wolnego rekordu listy kontaktow
int ReciveFreeContact()
{
  for(int Count=0;Count<100;Count++)
  {
	if(!ContactsList[Count].cbSize)
	 return Count;
  }
  return -1;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu wolnego rekordu listy unikatowych ID timera
int ReciveFreeTable()
{
  for(int Count=0;Count<100;Count++)
  {
	if(!IdTable[Count].TimerId)
	 return Count;
  }
  return -1;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu tabeli na podstawie indeksu rekordu listy kontaktow
int ReciveTableIndex(int ID)
{
  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].ContactIndex==ID)
	 return Count;
  }
  return -1;
}
//---------------------------------------------------------------------------

//Pobieranie indeksu rekordu listy kontaktow na podstawie ID timera
int ReciveContactIndex(int TimerID)
{
  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].TimerId==TimerID)
	 return IdTable[Count].ContactIndex;
  }
  return -1;
}
//---------------------------------------------------------------------------

//Sprawdzanie czy ID timera zostal wygenerowany przez wtyczke
bool IsContactTimer(int ID)
{
  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].TimerId==ID)
	 return true;
  }
  return false;
}
//---------------------------------------------------------------------------

//Procka okna timera
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  //Notfikacja timera
  if(uMsg==WM_TIMER)
  {
    //ID timera wygenerowany przez wtyczke
	if(IsContactTimer(wParam))
	{
	  //Zatrzymanie timera
	  KillTimer(hTimerFrm,wParam);
	  //Pobieranie indeksu rekordu listy kontaktow na podstawie ID timera
	  int ContactIndex = ReciveContactIndex(wParam);
	  //Pobranie indeksu tabeli na podstawie indeksu rekordu listy kontaktow
	  int TableIndex = ReciveTableIndex(ContactIndex);
	  //Zmiania stanu kontaktu na "rozlaczony"
	  PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)&ContactsList[ContactIndex]);
	  //Kasowanie danych nt. kontaktu
	  ZeroMemory(&ContactsList[ContactIndex],sizeof(TPluginContact));
	  IdTable[TableIndex].TimerId = -1;
	  IdTable[TableIndex].ContactIndex = -1;
	}

	return 0;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

//Hook na zamkniecie okna rozmowy lub zakladki
INT_PTR __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontaktu
  TPluginContact CloseTabContact = *(PPluginContact)lParam;
  //Jezeli kontakt jest rozlazony, jest na liscie kontaktow oraz nie jest czatem
  if((!CloseTabContact.Temporary)&&(!CloseTabContact.IsChat)&&(CloseTabContact.State==6))
  {
	//Pobranie wolnych rekordow
	int FreeContact = ReciveFreeContact();
	int FreeTable = ReciveFreeTable();
	//Wygenerowanie losowego ID timera
	int TimerID = PluginLink.CallService(AQQ_FUNCTION_GETNUMID,0,0);
	//Przypisanie danych w tablicy unikatowych ID timera
	IdTable[FreeTable].TimerId = TimerID;
	IdTable[FreeTable].ContactIndex = FreeContact;
	//Zapisanie danych w tablicy kontaktow
    //cbSize
	ContactsList[FreeContact].cbSize = CloseTabContact.cbSize;
	//JID
	ContactsList[FreeContact].JID = (wchar_t*)realloc(ContactsList[FreeContact].JID, sizeof(wchar_t)*(wcslen(CloseTabContact.JID)+1));
	memcpy(ContactsList[FreeContact].JID, CloseTabContact.JID, sizeof(wchar_t)*wcslen(CloseTabContact.JID));
	ContactsList[FreeContact].JID[wcslen(CloseTabContact.JID)] = L'\0';
	//Nick
	ContactsList[FreeContact].Nick = (wchar_t*)realloc(ContactsList[FreeContact].Nick, sizeof(wchar_t)*(wcslen(CloseTabContact.Nick)+1));
	memcpy(ContactsList[FreeContact].Nick, CloseTabContact.Nick, sizeof(wchar_t)*wcslen(CloseTabContact.Nick));
	ContactsList[FreeContact].Nick[wcslen(CloseTabContact.Nick)] = L'\0';
	//Resource
	ContactsList[FreeContact].Resource = (wchar_t*)realloc(ContactsList[FreeContact].Resource, sizeof(wchar_t)*(wcslen(CloseTabContact.Resource)+1));
	memcpy(ContactsList[FreeContact].Resource, CloseTabContact.Resource, sizeof(wchar_t)*wcslen(CloseTabContact.Resource));
	ContactsList[FreeContact].Resource[wcslen(CloseTabContact.Resource)] = L'\0';
	//Groups
	ContactsList[FreeContact].Groups = (wchar_t*)realloc(ContactsList[FreeContact].Groups, sizeof(wchar_t)*(wcslen(CloseTabContact.Groups)+1));
	memcpy(ContactsList[FreeContact].Groups, CloseTabContact.Groups, sizeof(wchar_t)*wcslen(CloseTabContact.Groups));
	ContactsList[FreeContact].Groups[wcslen(CloseTabContact.Groups)] = L'\0';
	//State
	ContactsList[FreeContact].State = 0;
	//Status
	ContactsList[FreeContact].Status = (wchar_t*)realloc(ContactsList[FreeContact].Status, sizeof(wchar_t)*(wcslen(CloseTabContact.Status)+1));
	memcpy(ContactsList[FreeContact].Status, CloseTabContact.Status, sizeof(wchar_t)*wcslen(CloseTabContact.Status));
	ContactsList[FreeContact].Status[wcslen(CloseTabContact.Status)] = L'\0';
	//Other
	ContactsList[FreeContact].Temporary = CloseTabContact.Temporary;
	ContactsList[FreeContact].FromPlugin = CloseTabContact.FromPlugin;
	ContactsList[FreeContact].UserIdx = CloseTabContact.UserIdx;
	ContactsList[FreeContact].Subscription = CloseTabContact.Subscription;
	ContactsList[FreeContact].IsChat = CloseTabContact.Subscription;
	//Wlacznie timera ustawienia rozlaczonego stanu kontatku
	SetTimer(hTimerFrm,TimerID,300000,(TIMERPROC)TimerFrmProc);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianê stanu kontaktu
INT_PTR __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych nt. kontaku
  TPluginContact ContactsUpdateContact = *(PPluginContact)wParam;
  //Jezeli kontakt jest rozlazony, jest na liscie kontaktow oraz nie jest czatem
  if((!ContactsUpdateContact.Temporary)&&(!ContactsUpdateContact.IsChat)&&(ContactsUpdateContact.State!=6))
  {
	//Przeszukiwanie tablicy
	for(int Count=0;Count<100;Count++)
	{
      //Jezeli rekord nie jest pusty
	  if(ContactsList[Count].cbSize)
	  {
		//Pobranie danych rekordu
		UnicodeString JID = (wchar_t*)ContactsList[Count].JID;
		UnicodeString Resource = (wchar_t*)ContactsList[Count].Resource;
		//Porwnanie zapamietego rekordu z danymi z notyfikacji
		if(((wchar_t*)ContactsUpdateContact.JID==JID)
		&&((wchar_t*)ContactsUpdateContact.Resource==Resource)
		&&(ContactsUpdateContact.UserIdx==ContactsList[Count].UserIdx))
		{
		  //Pobranie indeksu tabeli na podstawie indeksu rekordu listy kontaktow
		  int TableIndex = ReciveTableIndex(Count);
		  //Zatrzymanie timera
		  KillTimer(hTimerFrm,IdTable[TableIndex].TimerId);
		  //Kasowanie danych nt. kontaktu
		  ZeroMemory(&ContactsList[Count],sizeof(TPluginContact));
		  IdTable[TableIndex].TimerId = -1;
		  IdTable[TableIndex].ContactIndex = -1;
		  //Zakonczenie petli
		  Count = 100;
		}
	  }
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na odbieranie wiadomosci
INT_PTR __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych nt. kontaktu
  TPluginContact RecvMsgContact = *(PPluginContact)wParam;
  //Jezeli kontakt jest rozlazony, jest na liscie kontaktow oraz nie jest czatem
  if((!RecvMsgContact.Temporary)&&(!RecvMsgContact.IsChat)&&(RecvMsgContact.State==0))
  {
	//Pobieranie danych nt. wiadomosci
	TPluginMessage RecvMsgMessage = *(PPluginMessage)lParam;
	//Wiadomosc nie jest offline'owa
	if(!RecvMsgMessage.Offline)
	{
	  //Ustawienie stanu kontatku
	  RecvMsgContact.State = 6;
	  //Zmiana stanu kontatku na liscie
	  PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)&RecvMsgContact);
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

//Zapisywanie zasobów
void ExtractRes(wchar_t* FileName, wchar_t* ResName, wchar_t* ResType)
{
  TPluginTwoFlagParams PluginTwoFlagParams;
  PluginTwoFlagParams.cbSize = sizeof(TPluginTwoFlagParams);
  PluginTwoFlagParams.Param1 = ResName;
  PluginTwoFlagParams.Param2 = ResType;
  PluginTwoFlagParams.Flag1 = (int)HInstance;
  PluginLink.CallService(AQQ_FUNCTION_SAVERESOURCE,(WPARAM)&PluginTwoFlagParams,(LPARAM)FileName);
}
//---------------------------------------------------------------------------

//Obliczanie sumy kontrolnej pliku
UnicodeString MD5File(UnicodeString FileName)
{
  if(FileExists(FileName))
  {
	UnicodeString Result;
	TFileStream *fs;

	fs = new TFileStream(FileName, fmOpenRead | fmShareDenyWrite);
	try
	{
	  TIdHashMessageDigest5 *idmd5= new TIdHashMessageDigest5();
	  try
	  {
		Result = idmd5->HashStreamAsHex(fs);
	  }
	  __finally
	  {
		delete idmd5;
	  }
	}
	__finally
	{
	  delete fs;
	}

	return Result;
  }
  else
   return 0;
}
//---------------------------------------------------------------------------

//Zaladowanie wtyczki
extern "C" INT_PTR __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
  //Wypakiwanie ikonki InvShow.dll.png
  //E344374D3F234C4C7024F0B992B6DDA3
  if(!DirectoryExists(GetPluginUserDir()+"\\\\Shared"))
   CreateDir(GetPluginUserDir()+"\\\\Shared");
  if(!FileExists(GetPluginUserDir()+"\\\\Shared\\\\InvShow.dll.png"))
   ExtractRes((GetPluginUserDir()+"\\\\Shared\\\\InvShow.dll.png").w_str(),L"SHARED",L"DATA");
  else if(MD5File(GetPluginUserDir()+"\\\\Shared\\\\InvShow.dll.png")!="E344374D3F234C4C7024F0B992B6DDA3")
   ExtractRes((GetPluginUserDir()+"\\\\Shared\\\\InvShow.dll.png").w_str(),L"SHARED",L"DATA");
  //Hook na zamkniecie okna rozmowy lub zakladki
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_CLOSETAB,OnCloseTab);
  //Hook na zmianê stanu kontaktu
  PluginLink.HookEvent(AQQ_CONTACTS_UPDATE,OnContactsUpdate);
  //Hook na nowa wiadomosc
  PluginLink.HookEvent(AQQ_CONTACTS_RECVMSG,OnRecvMsg);
  //Rejestowanie klasy okna timera
  WNDCLASSEX wincl;
  wincl.cbSize = sizeof (WNDCLASSEX);
  wincl.style = 0;
  wincl.lpfnWndProc = TimerFrmProc;
  wincl.cbClsExtra = 0;
  wincl.cbWndExtra = 0;
  wincl.hInstance = HInstance;
  wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
  wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
  wincl.lpszMenuName = NULL;
  wincl.lpszClassName = L"TInvShowTimer";
  wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
  RegisterClassEx(&wincl);
  //Tworzenie okna timera
  hTimerFrm = CreateWindowEx(0, L"TInvShowTimer", L"",	0, 0, 0, 0, 0, NULL, NULL, HInstance, NULL);

  return 0;
}
//---------------------------------------------------------------------------

//Wyladowanie wtyczki
extern "C" INT_PTR __declspec(dllexport) __stdcall Unload()
{
  //Zatrzymanie wszystkich timerow
  for(int Count=0;Count<100;Count++)
  {
	if(IdTable[Count].TimerId)
	 KillTimer(hTimerFrm,IdTable[Count].TimerId);
  }
  //Usuwanie okna timera
  DestroyWindow(hTimerFrm);
  //Wyrejestowanie klasy okna timera
  UnregisterClass(L"TInvShowTimer",HInstance);
  //Wyladowanie wszystkich hookow
  PluginLink.UnhookEvent(OnCloseTab);
  PluginLink.UnhookEvent(OnContactsUpdate);
  PluginLink.UnhookEvent(OnRecvMsg);

  return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" PPluginInfo __declspec(dllexport) __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = L"InvShow";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,4,0,0);
  PluginInfo.Description = L"Wtyczka oferuje funkcjonalnoœæ znan¹ z AQQ 1.x. Gdy rozmawiamy z kontaktem, który ma stan \"roz³¹czony\", jego stan zostanie zmieniony na \"niewidoczny\" a¿ do momentu, gdy roz³¹czy siê on z sieci¹ lub po prostu zmieni swój stan.";
  PluginInfo.Author = L"Krzysztof Grochocki";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki";
  PluginInfo.Homepage = L"http://beherit.pl";
  PluginInfo.Flag = 0;
  PluginInfo.ReplaceDefaultModule = 0;

  return &PluginInfo;
}
//---------------------------------------------------------------------------
