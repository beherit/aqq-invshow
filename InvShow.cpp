//---------------------------------------------------------------------------
// Copyright (C) 2010-2015 Krzysztof Grochocki
//
// This file is part of InvShow
//
// InvShow is free software: you can redistribute it and/or modify
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
// along with GNU Radio. If not, see <http://www.gnu.org/licenses/>.
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
//Definicja-struktury-tablicy-ID-timera-i-kontaktow--------------------------
struct TIdTable
{
	int TimerID;
	TPluginContact PluginContact;
};
//Zmienna-tablicy-ID-timera-i-kontaktow--------------------------------------
DynamicArray<TIdTable> IdTable;
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

//Pobieranie indeksu rekordu na podstawie ID timera
int ReciveContactIndex(int TimerID)
{
	for(int Count=0;Count<IdTable.Length;Count++)
	{
		if(IdTable[Count].TimerID==TimerID)
			return Count;
	}
	return -1;
}
//---------------------------------------------------------------------------

//Sprawdzanie czy ID timera zostal wygenerowany przez wtyczke
bool IsContactTimer(int ID)
{
	for(int Count=0;Count<IdTable.Length;Count++)
	{
		if(IdTable[Count].TimerID==ID)
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
			//Pobieranie indeksu rekordu na podstawie ID timera
			int Idx = ReciveContactIndex(wParam);
			//Zmiania stanu kontaktu na "rozlaczony"
			PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)&IdTable[Idx].PluginContact);
			//Przesuniecie ostatniego rekordu
			IdTable[Idx].TimerID = IdTable[IdTable.Length-1].TimerID;
			IdTable[Idx].PluginContact = IdTable[IdTable.Length-1].PluginContact;
			IdTable.Length = IdTable.Length - 1;
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
		//Ustawianie nowego rekordu
		int Idx = IdTable.Length;
		IdTable.Length = IdTable.Length + 1;
		//Wygenerowanie losowego ID timera
		int TimerID = PluginLink.CallService(AQQ_FUNCTION_GETNUMID,0,0);
		//Zapisanie ID timera w tablicy
		IdTable[Idx].TimerID = TimerID;
		//Zapisanie danych w tablicy kontaktow
		//cbSize
		IdTable[Idx].PluginContact.cbSize = CloseTabContact.cbSize;
		//JID
		IdTable[Idx].PluginContact.JID = (wchar_t*)realloc(IdTable[Idx].PluginContact.JID, sizeof(wchar_t)*(wcslen(CloseTabContact.JID)+1));
		memcpy(IdTable[Idx].PluginContact.JID, CloseTabContact.JID, sizeof(wchar_t)*wcslen(CloseTabContact.JID));
		IdTable[Idx].PluginContact.JID[wcslen(CloseTabContact.JID)] = L'\0';
		//Nick
		IdTable[Idx].PluginContact.Nick = (wchar_t*)realloc(IdTable[Idx].PluginContact.Nick, sizeof(wchar_t)*(wcslen(CloseTabContact.Nick)+1));
		memcpy(IdTable[Idx].PluginContact.Nick, CloseTabContact.Nick, sizeof(wchar_t)*wcslen(CloseTabContact.Nick));
		IdTable[Idx].PluginContact.Nick[wcslen(CloseTabContact.Nick)] = L'\0';
		//Resource
		IdTable[Idx].PluginContact.Resource = (wchar_t*)realloc(IdTable[Idx].PluginContact.Resource, sizeof(wchar_t)*(wcslen(CloseTabContact.Resource)+1));
		memcpy(IdTable[Idx].PluginContact.Resource, CloseTabContact.Resource, sizeof(wchar_t)*wcslen(CloseTabContact.Resource));
		IdTable[Idx].PluginContact.Resource[wcslen(CloseTabContact.Resource)] = L'\0';
		//Groups
		IdTable[Idx].PluginContact.Groups = (wchar_t*)realloc(IdTable[Idx].PluginContact.Groups, sizeof(wchar_t)*(wcslen(CloseTabContact.Groups)+1));
		memcpy(IdTable[Idx].PluginContact.Groups, CloseTabContact.Groups, sizeof(wchar_t)*wcslen(CloseTabContact.Groups));
		IdTable[Idx].PluginContact.Groups[wcslen(CloseTabContact.Groups)] = L'\0';
		//State
		IdTable[Idx].PluginContact.State = 0;
		//Status
		IdTable[Idx].PluginContact.Status = (wchar_t*)realloc(IdTable[Idx].PluginContact.Status, sizeof(wchar_t)*(wcslen(CloseTabContact.Status)+1));
		memcpy(IdTable[Idx].PluginContact.Status, CloseTabContact.Status, sizeof(wchar_t)*wcslen(CloseTabContact.Status));
		IdTable[Idx].PluginContact.Status[wcslen(CloseTabContact.Status)] = L'\0';
		//Other
		IdTable[Idx].PluginContact.Temporary = CloseTabContact.Temporary;
		IdTable[Idx].PluginContact.FromPlugin = CloseTabContact.FromPlugin;
		IdTable[Idx].PluginContact.UserIdx = CloseTabContact.UserIdx;
		IdTable[Idx].PluginContact.Subscription = CloseTabContact.Subscription;
		IdTable[Idx].PluginContact.IsChat = CloseTabContact.Subscription;
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
	//Jezeli kontakt jest rozlaczony, jest na liscie kontaktow oraz nie jest czatem
	if((!ContactsUpdateContact.Temporary)&&(!ContactsUpdateContact.IsChat)&&(ContactsUpdateContact.State!=6))
	{
		//Przeszukiwanie tablicy
		for(int Count=0;Count<IdTable.Length;Count++)
		{
			//Pobranie danych rekordu
			UnicodeString JID = (wchar_t*)IdTable[Count].PluginContact.JID;
			UnicodeString Resource = (wchar_t*)IdTable[Count].PluginContact.Resource;
			//Porwnanie zapamietego rekordu z danymi z notyfikacji
			if(((wchar_t*)ContactsUpdateContact.JID==JID)
			&&((wchar_t*)ContactsUpdateContact.Resource==Resource)
			&&(ContactsUpdateContact.UserIdx==IdTable[Count].PluginContact.UserIdx))
			{
				//Zatrzymanie timera
				KillTimer(hTimerFrm,IdTable[Count].TimerID);
				//Przesuniecie ostatniego rekordu
				IdTable[Count].TimerID = IdTable[IdTable.Length-1].TimerID;
				IdTable[Count].PluginContact = IdTable[IdTable.Length-1].PluginContact;
				IdTable.Length = IdTable.Length - 1;
				//Zakonczenie petli
				break;
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
	else return 0;
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
	for(int Count=0;Count<IdTable.Length;Count++)
	{
		if(IdTable[Count].TimerID)
			KillTimer(hTimerFrm,IdTable[Count].TimerID);
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
	PluginInfo.Version = PLUGIN_MAKE_VERSION(1,5,0,0);
	PluginInfo.Description = L"Zmienia stan kontaktu z \"roz³¹czony\" na \"niewidoczny\" podczas rozmowy. Stan kontaktu jest przywracany, gdy roz³¹czy siê on z sieci¹, zmieni swój stan lub po up³ywie 5 minut od zamkniêcia okna rozmowy.";
	PluginInfo.Author = L"Krzysztof Grochocki";
	PluginInfo.AuthorMail = L"kontakt@beherit.pl";
	PluginInfo.Copyright = L"Krzysztof Grochocki";
	PluginInfo.Homepage = L"http://beherit.pl";
	PluginInfo.Flag = 0;
	PluginInfo.ReplaceDefaultModule = 0;

	return &PluginInfo;
}
//---------------------------------------------------------------------------
