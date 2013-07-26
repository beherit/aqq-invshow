#include <vcl.h>
#include <windows.h>
#pragma hdrstop
#pragma argsused
#include <PluginAPI.h>

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
  return 1;
}
//---------------------------------------------------------------------------

//Struktury-glowne-----------------------------------------------------------
TPluginLink PluginLink;
TPluginInfo PluginInfo;
PPluginContact CloseTabContact;
PPluginContact ContactsUpdateContact;
PPluginContact RecvMsgContact;
PPluginMessage RecvMsgMessage;
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
int __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam);
int __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam);
//FORWARD-TIMER--------------------------------------------------------------
LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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
	  //"Kasowanie" danych nt. kontaktu
	  ContactsList[ContactIndex].cbSize = NULL;
	  IdTable[TableIndex].TimerId = -1;
	  IdTable[TableIndex].ContactIndex = -1;
	}

	return 0;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------------

//Hook na zamkniecie okna rozmowy lub zakladki
int __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontaktu
  CloseTabContact = (PPluginContact)lParam;
  //Jezeli kontakt jest rozlazony, jest na liscie kontaktow oraz nie jest czatem
  if((!CloseTabContact->Temporary)&&(!CloseTabContact->IsChat)&&(CloseTabContact->State==6))
  {
	//Pobranie wolnych rekordow
	int FreeContact = ReciveFreeContact();
	int FreeTable = ReciveFreeTable();
	//Wygenerowanie losowego ID timera
	int TimerID = PluginLink.CallService(AQQ_FUNCTION_GETNUMID,0,0);
	//Przypisanie danych w tablicy unikatowych ID timera
	IdTable[FreeTable].TimerId = TimerID;
	IdTable[FreeTable].ContactIndex = FreeContact;
	//Przypisanie danych w tablicy kontaktow
	//cbSize
	ContactsList[FreeContact].cbSize = CloseTabContact->cbSize;
	//JID
	ContactsList[FreeContact].JID = (wchar_t*)realloc(ContactsList[FreeContact].JID, sizeof(wchar_t)*(wcslen(CloseTabContact->JID)+1));
	memcpy(ContactsList[FreeContact].JID, CloseTabContact->JID, sizeof(wchar_t)*wcslen(CloseTabContact->JID));
	ContactsList[FreeContact].JID[wcslen(CloseTabContact->JID)] = L'\0';
	//Nick
	ContactsList[FreeContact].Nick = (wchar_t*)realloc(ContactsList[FreeContact].Nick, sizeof(wchar_t)*(wcslen(CloseTabContact->Nick)+1));
	memcpy(ContactsList[FreeContact].Nick, CloseTabContact->Nick, sizeof(wchar_t)*wcslen(CloseTabContact->Nick));
	ContactsList[FreeContact].Nick[wcslen(CloseTabContact->Nick)] = L'\0';
	//Resource
	ContactsList[FreeContact].Resource = (wchar_t*)realloc(ContactsList[FreeContact].Resource, sizeof(wchar_t)*(wcslen(CloseTabContact->Resource)+1));
	memcpy(ContactsList[FreeContact].Resource, CloseTabContact->Resource, sizeof(wchar_t)*wcslen(CloseTabContact->Resource));
	ContactsList[FreeContact].Resource[wcslen(CloseTabContact->Resource)] = L'\0';
	//Groups
	ContactsList[FreeContact].Groups = (wchar_t*)realloc(ContactsList[FreeContact].Groups, sizeof(wchar_t)*(wcslen(CloseTabContact->Groups)+1));
	memcpy(ContactsList[FreeContact].Groups, CloseTabContact->Groups, sizeof(wchar_t)*wcslen(CloseTabContact->Groups));
	ContactsList[FreeContact].Groups[wcslen(CloseTabContact->Groups)] = L'\0';
	//State
	ContactsList[FreeContact].State = 0;
	//Status
	ContactsList[FreeContact].Status = (wchar_t*)realloc(ContactsList[FreeContact].Status, sizeof(wchar_t)*(wcslen(CloseTabContact->Status)+1));
	memcpy(ContactsList[FreeContact].Status, CloseTabContact->Status, sizeof(wchar_t)*wcslen(CloseTabContact->Status));
	ContactsList[FreeContact].Status[wcslen(CloseTabContact->Status)] = L'\0';
    //Other
	ContactsList[FreeContact].Temporary = CloseTabContact->Temporary;
	ContactsList[FreeContact].FromPlugin = CloseTabContact->FromPlugin;
	ContactsList[FreeContact].UserIdx = CloseTabContact->UserIdx;
	ContactsList[FreeContact].Subscription = CloseTabContact->Subscription;
	ContactsList[FreeContact].IsChat = CloseTabContact->Subscription;
	//Wlacznie timera ustawienia rozlaczonego stanu kontatku
	SetTimer(hTimerFrm,TimerID,300000,(TIMERPROC)TimerFrmProc);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianê stanu kontaktu
int __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontaku
  ContactsUpdateContact = (PPluginContact)wParam;
  //Jezeli kontakt jest rozlazony, jest na liscie kontaktow oraz nie jest czatem
  if((!ContactsUpdateContact->Temporary)&&(!ContactsUpdateContact->IsChat)&&(ContactsUpdateContact->State!=6))
  {
	//Przeszukiwanie tablicy
	for(int Count=0;Count<100;Count++)
	{
      //Jezeli rekord nie jest pusty
	  if(ContactsList[Count].cbSize)
	  {
        //Porwnanie zapamietego rekordu z danymi z notyfikacji
		if((ContactsList[Count].JID==ContactsUpdateContact->JID)
		&&(ContactsList[Count].Resource==ContactsUpdateContact->Resource)
		&&(ContactsList[Count].UserIdx==ContactsUpdateContact->UserIdx))
		{
          //Pobranie indeksu tabeli na podstawie indeksu rekordu listy kontaktow
		  int TableIndex = ReciveTableIndex(Count);
		  //Zatrzymanie timera
		  KillTimer(hTimerFrm,IdTable[TableIndex].TimerId);
		  //"Kasowanie" danych nt. kontaktu
		  ContactsList[Count].cbSize = NULL;
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
int __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontaktu
  RecvMsgContact = (PPluginContact)wParam;
  //Jezeli kontakt jest rozlazony, jest na liscie kontaktow oraz nie jest czatem
  if((!RecvMsgContact->Temporary)&&(!RecvMsgContact->IsChat)&&(RecvMsgContact->State==0))
  {
	//Pobieranie danych wiadomosci
	RecvMsgMessage = (PPluginMessage)lParam;
	//Wiadomosc nie jest offline'owa
	if(!RecvMsgMessage->Offline)
	{
	  //Ustawienie stanu kontatku
	  RecvMsgContact->State = 6;
	  //Zmiana stanu kontatku na liscie
	  PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)RecvMsgContact);
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
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

extern "C" int __declspec(dllexport) __stdcall Unload()
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
extern "C" __declspec(dllexport) PPluginInfo __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = L"InvShow";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,2,0,0);
  PluginInfo.Description = L"Wtyczka oferuje funkcjonalnoœæ znan¹ z AQQ 1.x. Gdy rozmawiamy z kontaktem, który ma stan \"roz³¹czony\", jego stan zostanie zmieniony na \"niewidoczny\" a¿ do momentu, gdy roz³¹czy siê on z sieci¹ lub po prostu zmieni swój stan.";
  PluginInfo.Author = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = L"http://beherit.pl";

  return &PluginInfo;
}
//---------------------------------------------------------------------------
