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
PPluginContact Contact;
//Uchwyt-do-okna-timera------------------------------------------------------
HWND hTimerFrm;
//Lista-JID-otwartych-zakladek-----------------------------------------------
TStringList* TabsList = new TStringList;
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
/*int __stdcall OnActiveTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam);
int __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam);
int __stdcall OnFetchAllTabs(WPARAM wParam, LPARAM lParam);
int __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam);*/
//FORWARD-TIMER--------------------------------------------------------------
//LRESULT CALLBACK TimerFrmProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
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

//Hook na aktwyna zakladke lub okno rozmowy
int __stdcall OnActiveTab(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontatku
  Contact = (PPluginContact)lParam;
  //Pobieranie identyfikatora kontatku
  UnicodeString JID = (wchar_t*)Contact->JID;
  if(!Contact->IsChat) JID = JID + "/" + (wchar_t*)Contact->Resource;
  //Dodawanie JID do listy otwartych zakladek
  if(TabsList->IndexOf(JID)==-1) TabsList->Add(JID);

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zamkniecie okna rozmowy lub zakladki
int __stdcall OnCloseTab(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontatku
  Contact = (PPluginContact)lParam;
  //Pobieranie identyfikatora kontatku
  UnicodeString JID = (wchar_t*)Contact->JID;
  if(!Contact->IsChat) JID = JID + "/" + (wchar_t*)Contact->Resource;
  //Usuwanie JID z listy aktywnych zakladek
  if(TabsList->IndexOf(JID)!=-1)
   TabsList->Delete(TabsList->IndexOf(JID));
  //Jezeli kontakt jest niewidoczny oraz jest na liscie kontaktow
  if((!Contact->Temporary)&&(Contact->State==6))
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
	ContactsList[FreeContact].cbSize = Contact->cbSize;
	//JID
	ContactsList[FreeContact].JID = (wchar_t*)realloc(ContactsList[FreeContact].JID, sizeof(wchar_t)*(wcslen(Contact->JID)+1));
	memcpy(ContactsList[FreeContact].JID, Contact->JID, sizeof(wchar_t)*wcslen(Contact->JID));
	ContactsList[FreeContact].JID[wcslen(Contact->JID)] = L'\0';
	//Nick
	ContactsList[FreeContact].Nick = (wchar_t*)realloc(ContactsList[FreeContact].Nick, sizeof(wchar_t)*(wcslen(Contact->Nick)+1));
	memcpy(ContactsList[FreeContact].Nick, Contact->Nick, sizeof(wchar_t)*wcslen(Contact->Nick));
	ContactsList[FreeContact].Nick[wcslen(Contact->Nick)] = L'\0';
	//Resource
	ContactsList[FreeContact].Resource = (wchar_t*)realloc(ContactsList[FreeContact].Resource, sizeof(wchar_t)*(wcslen(Contact->Resource)+1));
	memcpy(ContactsList[FreeContact].Resource, Contact->Resource, sizeof(wchar_t)*wcslen(Contact->Resource));
	ContactsList[FreeContact].Resource[wcslen(Contact->Resource)] = L'\0';
	//Groups
	ContactsList[FreeContact].Groups = (wchar_t*)realloc(ContactsList[FreeContact].Groups, sizeof(wchar_t)*(wcslen(Contact->Groups)+1));
	memcpy(ContactsList[FreeContact].Groups, Contact->Groups, sizeof(wchar_t)*wcslen(Contact->Groups));
	ContactsList[FreeContact].Groups[wcslen(Contact->Groups)] = L'\0';
	//State
	ContactsList[FreeContact].State = 0;
	//Status
	ContactsList[FreeContact].Status = (wchar_t*)realloc(ContactsList[FreeContact].Status, sizeof(wchar_t)*(wcslen(Contact->Status)+1));
	memcpy(ContactsList[FreeContact].Status, Contact->Status, sizeof(wchar_t)*wcslen(Contact->Status));
	ContactsList[FreeContact].Status[wcslen(Contact->Status)] = L'\0';
    //Other
	ContactsList[FreeContact].Temporary = Contact->Temporary;
	ContactsList[FreeContact].FromPlugin = Contact->FromPlugin;
	ContactsList[FreeContact].UserIdx = Contact->UserIdx;
	ContactsList[FreeContact].Subscription = Contact->Subscription;
	ContactsList[FreeContact].IsChat = Contact->Subscription;
	//Wlacznie timera ustawienia rozlaczonego stanu kontatku
	SetTimer(hTimerFrm,TimerID,300000,(TIMERPROC)TimerFrmProc);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na zmianê stanu kontaktu
int __stdcall OnContactsUpdate(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontatku
  Contact = (PPluginContact)wParam;
  //Jezeli kontakt jest na liscie kontaktow i jego stan jest rozny od niewidocznego
  if((!Contact->Temporary)&&(Contact->State!=6))
  {
	//Przeszukiwanie tablicy
	for(int Count=0;Count<100;Count++)
	{
      //Jezeli rekord nie jest pusty
	  if(ContactsList[Count].cbSize)
	  {
        //Porwnanie zapamietego rekordu z danymi z notyfikacji
		if((ContactsList[Count].JID==Contact->JID)
		&&(ContactsList[Count].Resource==Contact->Resource)
		&&(ContactsList[Count].UserIdx==Contact->UserIdx))
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

//Pobieranie listy wszystkich otartych zakladek/okien
int __stdcall OnFetchAllTabs(WPARAM wParam, LPARAM lParam)
{
  if((wParam!=0)&&(lParam!=0))
  {
	//Pobieranie danych kontatku
	Contact = (PPluginContact)lParam;
	//Pobieranie identyfikatora kontatku
	UnicodeString JID = (wchar_t*)Contact->JID;
	if(!Contact->IsChat) JID = JID + "/" + (wchar_t*)Contact->Resource;
	//Dodawanie JID do listy otwartych zakladek
	if(TabsList->IndexOf(JID)==-1) TabsList->Add(JID);
  }

  return 0;
}
//---------------------------------------------------------------------------

//Hook na odbieranie wiadomosci
int __stdcall OnRecvMsg(WPARAM wParam, LPARAM lParam)
{
  //Pobieranie danych kontatku
  Contact = (PPluginContact)wParam;
  //Jezeli kontakt jest rozlazony i jest na liscie kontaktow
  if((!Contact->Temporary)&&(Contact->State==0))
  {
	//Pobieranie identyfikatora kontatku
	UnicodeString JID = (wchar_t*)Contact->JID;
	if(!Contact->IsChat) JID = JID + "/" + (wchar_t*)Contact->Resource;
	//Jezeli zakladka z kontaktem jest otwarta
	if(TabsList->IndexOf(JID)!=-1)
	{
      //Ustawienie stanu kontatku
	  Contact->State = 6;
	  //Zmiana stanu kontatku na liscie
	  PluginLink.CallService(AQQ_CONTACTS_UPDATE,0,(LPARAM)Contact);
	}
  }

  return 0;
}
//---------------------------------------------------------------------------

extern "C" int __declspec(dllexport) __stdcall Load(PPluginLink Link)
{
  //Linkowanie wtyczki z komunikatorem
  PluginLink = *Link;
  //Wszystkie moduly zostaly zaladowane
  if(PluginLink.CallService(AQQ_SYSTEM_MODULESLOADED,0,0))
  {
	//Hook na pobieranie aktywnych zakladek
	PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_FETCHALLTABS,OnFetchAllTabs);
	//Pobieranie aktywnych zakladek
	PluginLink.CallService(AQQ_CONTACTS_BUDDY_FETCHALLTABS,0,0);
	//Usuniecie hooka na pobieranie aktywnych zakladek
	PluginLink.UnhookEvent(OnFetchAllTabs);
  }
  //Hook na aktwyna zakladke lub okno rozmowy
  PluginLink.HookEvent(AQQ_CONTACTS_BUDDY_ACTIVETAB,OnActiveTab);
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
  PluginLink.UnhookEvent(OnActiveTab);
  PluginLink.UnhookEvent(OnCloseTab);
  PluginLink.UnhookEvent(OnContactsUpdate);
  PluginLink.UnhookEvent(OnRecvMsg);
  //Usuniecie wskaznikow do zmiennych
  delete TabsList;
  //Usuniecie wskaznikow do zmiennych
  delete Contact;

  return 0;
}
//---------------------------------------------------------------------------

//Informacje o wtyczce
extern "C" __declspec(dllexport) PPluginInfo __stdcall AQQPluginInfo(DWORD AQQVersion)
{
  PluginInfo.cbSize = sizeof(TPluginInfo);
  PluginInfo.ShortName = L"InvShow";
  PluginInfo.Version = PLUGIN_MAKE_VERSION(1,1,0,0);
  PluginInfo.Description = L"Wtyczka oferuje funkcjonalnoœæ znan¹ z AQQ 1.x. Gdy rozmawiamy z kontaktem, który ma stan \"roz³¹czony\", jego stan zostanie zmieniony na \"niewidoczny\" a¿ do momentu, gdy roz³¹czy siê on z sieci¹ lub po prostu zmieni swój stan.";
  PluginInfo.Author = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.AuthorMail = L"kontakt@beherit.pl";
  PluginInfo.Copyright = L"Krzysztof Grochocki (Beherit)";
  PluginInfo.Homepage = L"http://beherit.pl";

  return &PluginInfo;
}
//---------------------------------------------------------------------------
