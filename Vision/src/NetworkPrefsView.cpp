#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <ListItem.h>
#include <ListView.h>
#include <MenuField.h>
#include <Menu.h>
#include <MenuItem.h>
#include <StringView.h>
#include <ScrollView.h>
#include <SupportDefs.h>
#include <TextControl.h>
#include <TextView.h>

#include <stdio.h>

#include "NetworkPrefsView.h"
#include "NetworkWindow.h"
#include "Prompt.h"
#include "Vision.h"

const uint32 M_NETWORK_DEFAULTS = 'mnnd';
const uint32 M_CHOOSE_NETWORK = 'mncn';
const uint32 M_ADD_NEW_NETWORK = 'mnan';
const uint32 M_DUPE_CURRENT_NETWORK = 'mndc';
const uint32 M_REMOVE_CURRENT_NETWORK = 'mnrc';
const uint32 M_SERVER_DIALOG = 'mnsd';
const uint32 M_EXEC_COMMAND_DIALOG = 'mned';
const uint32 M_CONNECT_ON_STARTUP = 'mncs';
const uint32 M_REMOVE_NICK = 'mnrn';
const uint32 M_ADD_NICK = 'mnai';
const uint32 M_NICK_ADDED = 'mnna';
const uint32 M_USE_NICK_DEFAULTS = 'mnun';
const uint32 M_CHANGE_IDENT = 'mnci';
const uint32 M_CHANGE_REALNAME = 'mncr';

NetworkPrefsView::NetworkPrefsView (BRect bounds, const char *name)
	:	BView (
			bounds,
			name,
			B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW),
			nickPrompt (NULL),
			netPrompt (NULL),
			dupePrompt (NULL),
			serverPrefs (NULL)
{
	SetViewColor (ui_color (B_PANEL_BACKGROUND_COLOR));
	BMenu *menu (new BMenu ("Networks"));
	menu->AddItem (new BMenuItem ("Defaults", new BMessage (M_NETWORK_DEFAULTS)));
	menu->AddSeparatorItem();
	menu->AddItem (new BMenuItem ("Add New...", new BMessage (M_ADD_NEW_NETWORK)));
	menu->AddItem (removeItem = new BMenuItem ("Remove current", new BMessage (M_REMOVE_CURRENT_NETWORK)));
	menu->AddItem (dupeItem = new BMenuItem ("Duplicate current", new BMessage (M_DUPE_CURRENT_NETWORK)));
	networkMenu = new BMenuField(BRect (0, 0, 100, 30), "NetList", NULL, menu);
	mainNetBox = new BBox (Bounds().InsetByCopy (5, 5));
	mainNetBox->SetLabel (networkMenu);
	AddChild (mainNetBox);
	BRect boundsRect (Bounds().InsetByCopy(10,15));
	boundsRect.right /= 2;
	boundsRect.right -= 8;
	boundsRect.top += 15;
	boundsRect.bottom -= 30;
	netDetailsBox = new BBox (boundsRect);
	netDetailsBox->SetLabel ("Network Details");
	mainNetBox->AddChild (netDetailsBox);
	personalBox = new BBox (boundsRect);
	personalBox->SetLabel ("Personal Details");
	mainNetBox->AddChild (personalBox);
	personalBox->MoveBy (boundsRect.Width() + 16, 0);
	boundsRect.left += 10;
	boundsRect.right = boundsRect.left + 10;
	boundsRect.top = 10;
	boundsRect.bottom += 20;
	BStringView *stringView1 (new BStringView (boundsRect, NULL,
		"Will connect to"));
	stringView1->ResizeToPreferred();
	stringView1->MoveTo(netDetailsBox->Frame().left, netDetailsBox->Frame().top);
	netDetailsBox->AddChild (stringView1);
	connectServer = new BStringView (boundsRect, NULL,
		"irc.sorcery.net,");
	connectServer->ResizeToPreferred();
	connectServer->MoveTo (stringView1->Frame().left, stringView1->Frame().bottom);
	alternates = new BStringView (boundsRect, NULL,
		"falling back to 9 others.");
	alternates->ResizeToPreferred();
	alternates->MoveTo(connectServer->Frame().left, connectServer->Frame().bottom);
	netDetailsBox->AddChild (connectServer);
	netDetailsBox->AddChild (alternates);
	serverButton = new BButton (boundsRect, NULL, "Change Server"B_UTF8_ELLIPSIS,
		new BMessage (M_SERVER_DIALOG));
	serverButton->ResizeToPreferred();
	serverButton->MoveTo (alternates->Frame().left + alternates->Frame().Width() / 2,
		alternates->Frame().bottom + 10);
	netDetailsBox->AddChild (serverButton);
	BStringView *stringView4 (new BStringView (boundsRect, NULL, "Autoexec:"));
	stringView4->ResizeToPreferred();
	stringView4->MoveTo (alternates->Frame().left, serverButton->Frame().bottom);
	netDetailsBox->AddChild (stringView4);
	boundsRect = netDetailsBox->Frame();
	boundsRect.right -= (20 + B_V_SCROLL_BAR_WIDTH);;
	boundsRect.top = stringView4->Frame().bottom + 5;
	boundsRect.bottom -= 65;
	textView = new BTextView (boundsRect, NULL, BRect (0, 0, boundsRect.Width(), boundsRect.Height()), B_FOLLOW_NONE, B_WILL_DRAW);
	BScrollView *scrollView (new BScrollView (NULL, textView, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0, false, true));
	textView->MakeEditable (true);
	textView->SetStylable (false);
	netDetailsBox->AddChild (scrollView);
	execButton = new BButton (boundsRect, NULL, "Add Common"B_UTF8_ELLIPSIS,
		new BMessage (M_EXEC_COMMAND_DIALOG));
	execButton->ResizeToPreferred();
	execButton->MoveTo (serverButton->Frame().right - execButton->Bounds().Width(), scrollView->Frame().bottom + 5);
	netDetailsBox->AddChild (execButton);
	startupBox = new BCheckBox (boundsRect, NULL, "Connect to this network when Vision starts up",
		new BMessage (M_CONNECT_ON_STARTUP));
	startupBox->ResizeToPreferred();
	startupBox->MoveTo (netDetailsBox->Frame().left, mainNetBox->Frame().bottom - (startupBox->Bounds().Height() + 12));
	mainNetBox->AddChild (startupBox);
	nickDefaultsBox = new BCheckBox (boundsRect, NULL, "Use Defaults",
		new BMessage (M_USE_NICK_DEFAULTS));
	nickDefaultsBox->ResizeToPreferred();
	nickDefaultsBox->MoveTo (netDetailsBox->Frame().left, netDetailsBox->Frame().top);
	personalBox->AddChild (nickDefaultsBox);
	BStringView *stringView5 (new BStringView (boundsRect, NULL, "Preferred Nicks:"));
	stringView5->ResizeToPreferred();
	stringView5->MoveTo (alternates->Frame().left, alternates->Frame().top);
	personalBox->AddChild (stringView5);
	listView = new BListView (scrollView->Frame(), NULL);
	listView->ResizeBy (-B_V_SCROLL_BAR_WIDTH, -10.0);
	listView->MoveTo (listView->Frame().left, stringView5->Frame().bottom + 5);
	BScrollView *listScroll (new BScrollView (NULL, listView, B_FOLLOW_LEFT | B_FOLLOW_TOP,
		0, false, true));
	personalBox->AddChild (listScroll);
	nickAddButton = new BButton (boundsRect, NULL, "Add", new BMessage(M_ADD_NICK));
	nickAddButton->ResizeToPreferred();
	nickRemoveButton = new BButton (boundsRect, NULL, "Remove", new BMessage (M_REMOVE_NICK));
	nickRemoveButton->ResizeToPreferred();
	nickRemoveButton->MoveTo (listScroll->Frame().right - nickRemoveButton->Frame().Width(),
		listScroll->Frame().bottom + 5);
	personalBox->AddChild (nickRemoveButton);
	nickAddButton->MoveTo (nickRemoveButton->Frame().left - (nickAddButton->Frame().Width() + 5),
		nickRemoveButton->Frame().top);
	personalBox->AddChild (nickAddButton);
	ident = new BTextControl (listScroll->Frame(), NULL, "Ident: ", NULL, new BMessage (M_CHANGE_IDENT));
	realName = new BTextControl (listScroll->Frame(), NULL, "Real name: ", NULL, new BMessage (M_CHANGE_REALNAME));
	realName->ResizeTo (listScroll->Frame().Width(), realName->Frame().Height());
	realName->MoveTo (listScroll->Frame().left, nickAddButton->Frame().bottom + 5);
	realName->SetDivider (realName->StringWidth (realName->Label()) + 5);
	personalBox->AddChild (realName);
	ident->ResizeTo (realName->Frame().Width(), realName->Frame().Height());
	ident->MoveTo (listScroll->Frame().left, realName->Frame().bottom + 5);
	ident->SetDivider (realName->Divider());
	personalBox->AddChild (ident);
}

NetworkPrefsView::~NetworkPrefsView (void)
{
  if (nickPrompt)
    nickPrompt->PostMessage (B_QUIT_REQUESTED);
  if (netPrompt)
    netPrompt->PostMessage (B_QUIT_REQUESTED);
  if (dupePrompt)
    dupePrompt->PostMessage (B_QUIT_REQUESTED);
  if (serverPrefs)
    serverPrefs->PostMessage (B_QUIT_REQUESTED);
}

void
NetworkPrefsView::AttachedToWindow (void)
{
	networkMenu->Menu()->SetTargetForItems (this);
	serverButton->SetTarget (this);
	execButton->SetTarget (this);
	startupBox->SetTarget (this);
	nickDefaultsBox->SetTarget (this);
	nickAddButton->SetTarget (this);
	nickRemoveButton->SetTarget (this);
	realName->SetTarget (this);
	ident->SetTarget (this);
	dynamic_cast<BInvoker *>(networkMenu->Menu()->ItemAt(0))->Invoke();
	BuildNetworkList();
}

static void
InsertionSort (BMenu *menu, BMenuItem *item)
{
	if ((menu == NULL) || (item == NULL))
		return;
	
	const char *itemLabel (NULL);
	const char *network (item->Label());
	for (int32 i = 0; i < menu->CountItems(); i++)
	{
		itemLabel = menu->ItemAt(i)->Label();
		if (!strcmp (itemLabel, "Defaults"))
		{
			if (i)
				i -= 1;
			menu->AddItem(item, i);
			break;
		}
		if (strcmp (itemLabel, network) > 0)
		{
			if (i)
				i -= 1;
			menu->AddItem (item, i);
			break;
		}
	}
}

void
NetworkPrefsView::DetachedFromWindow (void)
{
    // save changes to active network
    // get autoexec commands
    const char *autoexec = textView->Text();
    if (!activeNetwork.HasString ("autoexec"))
      activeNetwork.AddString ("autoexec", autoexec);
    else
      activeNetwork.ReplaceString ("autoexec", autoexec);

    const char *name (activeNetwork.FindString ("name"));
    if (name)
      vision_app->SetNetwork (name, &activeNetwork);
}

void
NetworkPrefsView::BuildNetworkList (void)
{
  if (networkMenu == NULL)
    return;
  
  BMessage msg;
  
  BMenu *menu (networkMenu->Menu());
  
  for (int32 i = 0; (msg = vision_app->GetNetwork (i)), !msg.HasBool ("error"); i++)
  {
    BMenuItem *item (new BMenuItem (msg.FindString ("name"), new BMessage (M_CHOOSE_NETWORK)));
    InsertionSort (menu, item);
    item->SetTarget (this);
  }
}

void
NetworkPrefsView::SetConnectServer (const char *serverName)
{
	connectServer->SetText (serverName);
	connectServer->ResizeToPreferred();
}

void
NetworkPrefsView::SetAlternateCount (uint32 altCount)
{
	if (altCount > 0)
	{
		BString text ("falling back to ");
		text << altCount;
		text += " other";
		text += (altCount > 1) ? "s" : "";
		text += ".";
		alternates->SetText(text.String());
		alternates->ResizeToPreferred();
	}
	else
		alternates->SetText("");
}

void
NetworkPrefsView::UpdateNetworkData (BMessage &msg)
{
	// enable network controls
	startupBox->SetEnabled (true);
	serverButton->SetEnabled (true);
	textView->MakeEditable (true);
	execButton->SetEnabled (true);

	bool startup (false);
	if (msg.FindBool ("connectOnStartup", &startup) == B_OK)
		startupBox->SetValue ((startup) ? B_CONTROL_ON : B_CONTROL_OFF);
	else
		startupBox->SetValue (B_CONTROL_OFF);
	const char *autoexec (NULL);
	if ((autoexec = msg.FindString ("autoexec")) != NULL)
	  textView->SetText (autoexec);
	uint32 altCount (0);
	int32 size;
	const ServerData *data (NULL);
	for (int32 i = 0; msg.FindData ("server", B_ANY_TYPE, i,
	  reinterpret_cast<const void **>(&data), &size) == B_OK; i++)
	{
	  if (data->state == 0)
	    SetConnectServer (data->serverName);
	  else if (data->state == 1)
	    ++altCount;
	}
    SetAlternateCount (altCount);
}

void
NetworkPrefsView::UpdatePersonalData (BMessage &msg)
{
	const char *curIdent (NULL);
	const char *curRname (NULL);
	const char *curNick (NULL);
	int32 count (listView->CountItems());
	for (int32 i = 0; i < count; i++)
		delete (listView->RemoveItem (0L));

	if ((msg.HasBool ("useDefaults") && msg.FindBool ("useDefaults")))
	{
		BMessage defaults (vision_app->GetNetwork ("defaults"));
		defaults.FindString ("ident", &curIdent);
		defaults.FindString ("realname", &curRname);
		for (int32 i = 0; defaults.FindString ("nick", i, &curNick) == B_OK; i++)
			listView->AddItem (new BStringItem (curNick));
		ident->SetEnabled (false);
		realName->SetEnabled (false);
		nickAddButton->SetEnabled (false);
		nickRemoveButton->SetEnabled (false);
		nickDefaultsBox->SetValue (B_CONTROL_ON);
	}
	else
	{
		msg.FindString ("ident", &curIdent);
		msg.FindString ("realname", &curRname);
		for (int32 i = 0; msg.FindString ("nick", i, &curNick) == B_OK; i++)
			listView->AddItem (new BStringItem (curNick));
		ident->SetEnabled (true);
		realName->SetEnabled (true);
		nickAddButton->SetEnabled (true);
		nickRemoveButton->SetEnabled (true);
		nickDefaultsBox->SetValue (B_CONTROL_OFF);
	}

	if (activeNetwork.what != VIS_NETWORK_DEFAULTS)
		nickDefaultsBox->SetEnabled (true);

	if (curIdent)
		ident->SetText (curIdent);
	else
		ident->SetText ("");
	if (curRname)
		realName->SetText (curRname);
	else
		realName->SetText ("");
	
	if (listView->CountItems() == 0)
		nickRemoveButton->SetEnabled (false);
}

void
NetworkPrefsView::SetupDefaults (BMessage &msg)
{
	// disable appropriate controls
	textView->MakeEditable (false);
	textView->SetText ("");
	serverButton->SetEnabled (false);
	execButton->SetEnabled (false);
	SetConnectServer ("<N/A>");
	SetAlternateCount (0);
	nickDefaultsBox->SetEnabled (false);
	startupBox->SetEnabled (false);
	
	// update personal data
	UpdatePersonalData(msg);
}

void
NetworkPrefsView::MessageReceived (BMessage *msg)
{
	switch (msg->what)
	{
		case M_NETWORK_DEFAULTS:
		    if (activeNetwork.HasString ("name"))
		    	vision_app->SetNetwork (activeNetwork.FindString ("name"), &activeNetwork);
			activeNetwork = vision_app->GetNetwork ("defaults");
			networkMenu->MenuItem()->SetLabel ("Defaults");
			SetupDefaults (activeNetwork);
			dupeItem->SetEnabled (false);
			removeItem->SetEnabled (false);
			break;
		
		case M_CHOOSE_NETWORK:
			{
		    	BMenuItem *item (NULL);
		    	msg->FindPointer ("source", reinterpret_cast<void **>(&item));
		    	if (item && strcmp(item->Label(), networkMenu->MenuItem()->Label()) != 0)
		    	{
		    	    const char *exec (textView->Text());
		    	    if (activeNetwork.HasString ("autoexec"))
		    	      activeNetwork.ReplaceString ("autoexec", exec);
		    	    else
		    	      activeNetwork.AddString ("autoexec", exec);
		    	      
		    	    if (activeNetwork.HasString ("name"))
	 		    	  vision_app->SetNetwork (activeNetwork.FindString ("name"), &activeNetwork);
			    	activeNetwork = vision_app->GetNetwork (item->Label());
			    	networkMenu->MenuItem()->SetLabel (item->Label());
		    		UpdatePersonalData (activeNetwork);
		    		UpdateNetworkData (activeNetwork);
		    		if (BMessenger (serverPrefs).IsValid())
		    			serverPrefs->SetNetworkData (&activeNetwork);
		    	}
   				dupeItem->SetEnabled (true);
				removeItem->SetEnabled (true);
		    }
			break;
			
		case M_ADD_NEW_NETWORK:
			{
				if (msg->HasString ("text"))
				{
					netPrompt = NULL;
					BString network (msg->FindString ("text"));
					network.RemoveAll (" ");
					BMenu *menu (networkMenu->Menu());
					for (int32 i = 0; i < menu->CountItems(); i++)
					{
						BMenuItem *item (menu->ItemAt (i));
						if (item && network == item->Label())
						{
							dynamic_cast<BInvoker *>(item)->Invoke();
							return;
						}
					}
					BMessage newNet (VIS_NETWORK_DATA);
					newNet.AddString ("name", network.String());
					vision_app->SetNetwork (network.String(), &newNet);
					BMenuItem *item (new BMenuItem (network.String(), new BMessage (M_CHOOSE_NETWORK)));
					InsertionSort (menu, item);
					item->SetTarget (this);
					dynamic_cast<BInvoker *>(item)->Invoke();
				}
				else
				{
					netPrompt = new PromptWindow (BPoint (Window()->Frame().left + Window()->Frame().Width() /2, Window()->Frame().top + Window()->Frame().Height() / 2),
					"Network: ", "Add Network", NULL, this, new BMessage (M_ADD_NEW_NETWORK), NULL, false);
					netPrompt->Show();
				}
			}
			break;
		
		case M_REMOVE_CURRENT_NETWORK:
		    {
		      const char *name (activeNetwork.FindString ("name"));
		      vision_app->RemoveNetwork (name);
		      BMenu *menu (networkMenu->Menu());
		      for (int32 i = 0; i < menu->CountItems(); i++)
		      {
		      	BMenuItem *item (menu->ItemAt (i));
		      	if (!strcmp (item->Label(), name))
		      	{
		      	  delete menu->RemoveItem(i);
		      	  activeNetwork.MakeEmpty();
		      	  dynamic_cast<BInvoker *>(menu->ItemAt(0))->Invoke();
		      	  break;
		      	}
		      }
			}
			break;
			
		case M_DUPE_CURRENT_NETWORK:
			{
				if (msg->HasString ("text"))
				{
					dupePrompt = NULL;
					BString network (msg->FindString ("text"));
					network.RemoveAll (" ");
					BMenu *menu (networkMenu->Menu());
					for (int32 i = 0; i < menu->CountItems(); i++)
					{
						BMenuItem *item (menu->ItemAt (i));
						if (item && network == item->Label())
						{
							dynamic_cast<BInvoker *>(item)->Invoke();
							return;
						}
					}
					BMessage newNet = activeNetwork;
					newNet.ReplaceString ("name", network.String());
					vision_app->SetNetwork (network.String(), &newNet);
					BMenuItem *item (new BMenuItem (network.String(), new BMessage (M_CHOOSE_NETWORK)));
					InsertionSort (menu, item);
					item->SetTarget (this);
					dynamic_cast<BInvoker *>(item)->Invoke();
				}
				else
				{
					dupePrompt = new PromptWindow (BPoint (Window()->Frame().left + Window()->Frame().Width() /2, Window()->Frame().top + Window()->Frame().Height() / 2),
					"Network: ", "Duplicate Network", NULL, this, new BMessage (M_DUPE_CURRENT_NETWORK), NULL, false);
					dupePrompt->Show();
				}
			}
			break;
		
		case M_SERVER_DIALOG:
			{
				BMessenger msgr (serverPrefs);
				if (msgr.IsValid())
					serverPrefs->Activate();
				else
				{
					serverPrefs = new NetPrefServerWindow ();
					serverPrefs->SetNetworkData (&activeNetwork);
					serverPrefs->Show();
				}
			}
			break;
		
		case M_EXEC_COMMAND_DIALOG:
			printf("autoexec command dialog\n");
			break;
		
		case M_CONNECT_ON_STARTUP:
		    {
  				bool value = msg->FindInt32 ("be:value");
				if (activeNetwork.HasBool ("connectOnStartup"))
					activeNetwork.ReplaceBool ("connectOnStartup", value);
				else
					activeNetwork.AddBool ("connectOnStartup", value);
			}
			break;
		
		case M_USE_NICK_DEFAULTS:
			{
				bool value = msg->FindInt32 ("be:value");
				if (activeNetwork.HasBool ("useDefaults"))
					activeNetwork.ReplaceBool ("useDefaults", value);
				else
					activeNetwork.AddBool ("useDefaults", value);
				UpdatePersonalData (activeNetwork);
			}
			break;
		
		case M_ADD_NICK:
			if (msg->HasString ("text"))
			{
				nickPrompt = NULL;
				BString nick (msg->FindString ("text"));
				nick.RemoveAll (" ");
				for (int32 i = 0; i < listView->CountItems(); i++)
				{
					BStringItem *item ((BStringItem *)listView->ItemAt (i));
					if (item && nick == item->Text())
						return;
				}
				activeNetwork.AddString ("nick", nick.String());
				listView->AddItem (new BStringItem (nick.String()));
				if (!nickRemoveButton->IsEnabled())
					nickRemoveButton->SetEnabled (true);
			}
			else
			{
				nickPrompt = new PromptWindow (BPoint (Window()->Frame().left + Window()->Frame().Width() /2, Window()->Frame().top + Window()->Frame().Height() / 2),
				"Nickname: ", "Add NickName", NULL, this, new BMessage (M_ADD_NICK), NULL, false);
				nickPrompt->Show();
			}
			break;
		
		case M_REMOVE_NICK:
			{
				int32 current (listView->CurrentSelection());
				if (current >= 0)
				{
					delete listView->RemoveItem (current);
					activeNetwork.RemoveData ("nick", current);
				}
			}
			if (listView->CountItems() == 0)
				nickRemoveButton->SetEnabled (false);
			break;
		
		case M_CHANGE_REALNAME:
			{
				const char *curRname (realName->Text());
				if (curRname == NULL)
					break;
				if (!activeNetwork.HasString ("realname"))
					activeNetwork.AddString ("realname", curRname);
				else
					activeNetwork.ReplaceString ("realname", curRname);
			}
			break;
		
		case M_CHANGE_IDENT:
			{
				const char *curIdent (ident->Text());
				if (curIdent == NULL)
					break;
				if (!activeNetwork.HasString ("ident"))
					activeNetwork.AddString ("ident", curIdent);
				else
					activeNetwork.ReplaceString ("ident", curIdent);
			}
			break;
			
		default:
			BView::MessageReceived (msg);
			break;
	}
}

