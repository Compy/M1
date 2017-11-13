//
// M1 by R. Belmont
//
// Macintosh version by Richard Bannister
// © 2004-2010, All Rights Reserved
//
// No part of this code may be reused in any other project without the
// express written permission of Richard Bannister.
//
// http://www.bannister.org/software/
//

// mac-window.cpp

#include "m1ui.h"
#include "m1snd.h"
#include "wavelog.h"

#include "mac-m1.h"
#include "mac-roms.h"
#include "mac-snd.h"
#include "mac-window.h"

// Exported globals


// Private defines
#define NUM_TABS			4
#define MIN_WIDTH			800
#define MIN_HEIGHT			357
#define WIDGETS_SETTINGS	4
#define WIDGETS_ABOUT		3
#define WIDGET_WIDTH		500
#define MAX_CHANS			64
#define WINDOW_EVENTS		5
enum {
	kBrowserProperty_Name = 'NAME',
	kBrowserProperty_Manuf = 'MANU',
	kBrowserProperty_Zipname = 'ZIPN',
	kBrowserProperty_Volume = 'SQEK',
	kBrowserProperty_Pan = 'PAN '
};
enum {
	TAB_ABOUT = 1,
	TAB_MAIN = 2,
	TAB_CHANNELS = 3,
	TAB_SETTINGS = 4
};
static struct { Str255 text; int enable_if; Boolean value; } settings_widgets[WIDGETS_SETTINGS] =
{
	{ "\pShow Missing ROM Sets", -1, false },
	{ "\pNormalize volume levels", -1, true },
	{ "\pSave audio to .WAV files", -1, false },
	{ "\pShow Oscilloscope", -1, true },
};
static struct { Rect bounds; SInt16 font; SInt16 style; Str255 text; } about_widgets[WIDGETS_ABOUT] = 
{
	{ { 165, 0, 185, WIDGET_WIDTH }, kControlFontBigSystemFont, bold, "\pM1 by R. Belmont" },
	{ { 185, 0, 205, WIDGET_WIDTH }, kControlFontBigSystemFont, 0, "\pMac OS X version by Richard Bannister" },
	{ { 220, 0, 235, WIDGET_WIDTH }, kControlFontBigSystemFont, bold, "\pThis software is freeware." }
};
static const EventTypeSpec gWindowEvents[WINDOW_EVENTS] = 
{
	{ kEventClassKeyboard, kEventRawKeyUp },
	{ kEventClassWindow, kEventWindowClose },
	{ kEventClassWindow, kEventWindowBoundsChanging },
	{ kEventClassWindow, kEventWindowBoundsChanged },
	{ kEventClassControl, kEventControlHit },
};
static const EventTypeSpec	gScopeEvents = { kEventClassControl, kEventControlDraw };

// Private globals
typedef struct
{
	char	name[256];
	int		st;
	int		chan;
	int		level;
	int		pan;
} m1_channel;
static m1_channel			m1_chans[MAX_CHANS];
static int					gChannels = 0;
static WindowRef			gMainWindow = nil;
static int					gActiveTab = TAB_MAIN;
static Boolean				gScopeEnabled = true, gScopeGG = false;
static ControlRef			gGroupInfo = nil, gGroupScope = nil, gScopeControl = nil;
static ControlRef			gChannelsWarning = nil, gChannelBrowser = nil;
static ControlRef			gMainDisplayText[9];
static ControlRef			gTabsControl = nil, gIconCtl = nil;
static ControlRef			gAboutWidget[WIDGETS_ABOUT];
static ControlRef			gSettingsWidget[WIDGETS_SETTINGS];
static ControlRef			gButtonNext = nil, gButtonPrev = nil, gButtonRestart = nil, gMainBrowser = nil;
static IconFamilyHandle		gAboutIconFamilyHandle = NULL;
static IconRef				gAboutIconRef = NULL;
static int					gListWidth = 520, gWindowHeight = 540;
static Boolean				gLogWAV = false, gShowMissing = false;
static char					*gDrwbwlStr = "drwbwl";
static int					gDrwbwlCnt = 0;

// Private prototypes
static pascal OSStatus		window_scope_render_callback (EventHandlerCallRef nextHandler, EventRef theEvent, void * userData);
static OSStatus				window_control_hit(ControlRef ctl);
static void					window_set_active_tab(void);
static int					window_setup_about_tab(void);
static int					window_setup_main_tab(void);
static int					window_setup_channels_tab(void);
static int					window_setup_settings_tab(void);
static void					window_clobber_about_tab(void);
static void					window_clobber_main_tab(void);
static void					window_clobber_channels_tab(void);
static void					window_clobber_settings_tab(void);
static void					window_resize_and_move_controls(void);
static void					window_sync_settings(void);
static void					window_set_titlebar(void);
static void					window_main_browser_update(Boolean add_all);
static pascal Boolean		window_main_browser_compare_callback(ControlRef ref, DataBrowserItemID itemOne, DataBrowserItemID itemTwo, DataBrowserPropertyID sortProperty);
static pascal void			window_main_browser_notification_callback (ControlRef ref, DataBrowserItemID itemID, DataBrowserItemNotification message);
static pascal OSStatus		window_main_browser_data_callback (ControlRef ref, DataBrowserItemID itemID, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue);
static pascal Boolean		window_channel_browser_compare_callback(ControlRef ref, DataBrowserItemID itemOne, DataBrowserItemID itemTwo, DataBrowserPropertyID sortProperty);
static pascal void			window_channel_browser_notification_callback (ControlRef ref, DataBrowserItemID itemID, DataBrowserItemNotification message);
static pascal OSStatus		window_channel_browser_data_callback (ControlRef ref, DataBrowserItemID itemID, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue);
static pascal OSStatus		window_event_dispatcher(EventHandlerCallRef ref, EventRef event, void *unused);


//
// window_open
// Create the main display window
//

int window_open(void)
{
	ControlTabEntry		tabs[NUM_TABS];
	Rect				bounds;
	OSStatus			status;
	CFStringRef			window_title;
	int					taberr;
	
	// Allocate interface tabs
	memset(tabs, 0, sizeof(ControlTabEntry) * NUM_TABS);
	tabs[0].name = CFSTR("About");
	tabs[1].name = CFSTR("Main");
	tabs[2].name = CFSTR("Channels");
	tabs[3].name = CFSTR("Settings");
	tabs[0].enabled = true;
	tabs[1].enabled = true;
	tabs[2].enabled = true;
	tabs[3].enabled = true;

	// Create window
	SetRect(&bounds, 0, 0, MIN_WIDTH, MIN_HEIGHT);
	
	// Create and center window
	status = CreateNewWindow(kDocumentWindowClass, kWindowStandardHandlerAttribute | kWindowLiveResizeAttribute | kWindowCloseBoxAttribute | kWindowCompositingAttribute | kWindowCollapseBoxAttribute | kWindowResizableAttribute, &bounds, &gMainWindow);
	assert(status == noErr);
	
	// Position it
	status = RepositionWindow(gMainWindow, nil, kWindowAlertPositionOnMainScreen);
	assert(status == noErr);
	
	// Set title
	window_set_titlebar();

	// Tab controls
	InsetRect(&bounds, 10, 10);
	status = CreateTabsControl(gMainWindow, &bounds, kControlTabSizeLarge, kControlTabDirectionSouth, NUM_TABS, tabs, &gTabsControl);
	assert(status == noErr);
	
	// Individual tabs
	taberr  = window_setup_about_tab();
	taberr |= window_setup_main_tab();
	taberr |= window_setup_channels_tab();
	taberr |= window_setup_settings_tab();
	if (taberr)
		return taberr;

	// Set the current tab to what it should be
	SetControlValue(gTabsControl, gActiveTab);
	window_set_active_tab();

	// Move everything to where it should be
	window_resize_and_move_controls();
	
	// Draw it
	ShowWindow(gMainWindow);
	SetPortWindowPort(gMainWindow);

	// Sync settings
	window_sync_settings();

	// Install event handler on window
	InstallWindowEventHandler(gMainWindow, NewEventHandlerUPP(window_event_dispatcher), WINDOW_EVENTS, gWindowEvents, 0, NULL);
	
	// Success
	return 1;
}


//
// window_setup_about_tab
// Set up the controls used for the about tab
//

static int window_setup_about_tab(void)
{
	OSStatus						status;
	ControlButtonContentInfo		content;
	ControlFontStyleRec				style;
	CFStringRef						string;
	int								i;
	Rect							bounds;
	
	// Step one - load the icon
	gAboutIconFamilyHandle = (IconFamilyHandle)GetResource('icns', 128);
	if (gAboutIconFamilyHandle != NULL)
	{
		status = RegisterIconRefFromIconFamily('R-BE', 'AM1!', gAboutIconFamilyHandle, &gAboutIconRef);
	}
	if ((gAboutIconFamilyHandle == NULL) || (status != noErr))
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to load icon family.", 0);
		return -1;
	}
	
	content.contentType = kControlContentIconRef;
	content.u.iconRef = gAboutIconRef;
		
	// Install control
	SetRect(&bounds, 112 + gListWidth, 0, 240 + gListWidth, 128);
	status = CreateIconControl(gMainWindow, &bounds, &content, true, &gIconCtl);
	if (status != noErr)
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to create icon control.", 0);
		return -1;
	}
	
	// Text widgets
	style.flags = kControlUseFontMask + kControlUseJustMask + kControlAddToMetaFontMask + kControlUseFaceMask;
	style.just = teCenter;
	for (i = 0; i < WIDGETS_ABOUT; i++)
	{		
		style.font = about_widgets[i].font;
		style.style = about_widgets[i].style;
		string =  CFStringCreateWithPascalString(NULL, about_widgets[i].text, GetApplicationTextEncoding());		
		status = CreateStaticTextControl(gMainWindow, &about_widgets[i].bounds, string, &style, &gAboutWidget[i]);
		if (status != noErr)
		{
			m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to create text control for about box.", 0);
			return -1;
		}
		CFRelease(string);
	}

	// OK
	return 0;
}


//
// window_clobber_about_tab
// Shut down resources for about tab
//

static void window_clobber_about_tab(void)
{
	int i;
	
	// Hide icon control
	if (gIconCtl)
	{
		HideControl(gIconCtl);
		DisposeControl(gIconCtl);
		gIconCtl = NULL;
	}
	
	// Hide and dispose text widgets
	for (i = 0; i < WIDGETS_ABOUT; i++)
	{
		if (gAboutWidget[i])
		{
			HideControl(gAboutWidget[i]);
			DisposeControl(gAboutWidget[i]);
			gAboutWidget[i] = nil;
		}
	}
	
	// Unload the icon resource
	if (gAboutIconRef)
	{
		ReleaseIconRef(gAboutIconRef);
		gAboutIconRef = NULL;
	}
	
	// And the icon family
	if (gAboutIconFamilyHandle)
	{
		ReleaseResource((Handle)gAboutIconFamilyHandle);
		gAboutIconFamilyHandle = NULL;
	}
}


//
// window_setup_main_tab
// Set up the controls used for the main tab
//

static int window_setup_main_tab(void)
{
	OSStatus						status;
	Rect							bounds;
	DataBrowserCallbacks			browser_callbacks;
	DataBrowserListViewColumnDesc	col_desc;
	Boolean							f;
	int								i;
	ControlFontStyleRec				style;
	ControlButtonContentInfo		content;
	
	// First step is to set up the data browser.
	SetRect(&bounds, 20, 20, 20 + gListWidth, gWindowHeight - 40);
	status = CreateDataBrowserControl(gMainWindow, &bounds, kDataBrowserListView, &gMainBrowser);
	if (status == noErr)
	{
		f = true;
		SetControlData(gMainBrowser, 0, kControlDataBrowserIncludesFrameAndFocusTag, sizeof(Boolean), &f);
		SetDataBrowserTableViewHiliteStyle(gMainBrowser, kDataBrowserTableViewFillHilite);
		SetDataBrowserSelectionFlags(gMainBrowser, kDataBrowserSelectOnlyOne);
		SetDataBrowserListViewUsePlainBackground(gMainBrowser, false);
		SetDataBrowserHasScrollBars(gMainBrowser, false, true);
		browser_callbacks.version = kDataBrowserLatestCallbacks;
		status = InitDataBrowserCallbacks(&browser_callbacks);
		if (status == noErr)
		{
			browser_callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(window_main_browser_data_callback);
			browser_callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(window_main_browser_notification_callback);
			browser_callbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(window_main_browser_compare_callback);
			status = SetDataBrowserCallbacks(gMainBrowser, &browser_callbacks);
			if (status == noErr)
			{
				memset(&col_desc, 0, sizeof(DataBrowserListViewColumnDesc));
				col_desc.headerBtnDesc.version = kDataBrowserListViewLatestHeaderDesc;
				col_desc.headerBtnDesc.btnFontStyle.flags = kControlUseThemeFontIDMask;
				col_desc.headerBtnDesc.btnFontStyle.font = kControlFontViewSystemFont;
				col_desc.propertyDesc.propertyType = kDataBrowserTextType;
				col_desc.propertyDesc.propertyFlags = kDataBrowserDefaultPropertyFlags + kDataBrowserListViewSortableColumn + kDataBrowserListViewSelectionColumn;
				for (i = 0; i < 3; i++)
				{
					int			min_widths[3] = { 290, 130, 95 };
					int			max_widths[3] = { 2500, 130, 95 };
					int			properties[3] = { kBrowserProperty_Name, kBrowserProperty_Manuf, kBrowserProperty_Zipname };
					CFStringRef	titles[3] =		{ CFSTR("Name"), CFSTR("Manufacturer"), CFSTR("Set Name") };

					col_desc.propertyDesc.propertyID = properties[i];
					col_desc.headerBtnDesc.titleString = titles[i];
					col_desc.headerBtnDesc.minimumWidth = min_widths[i];
					col_desc.headerBtnDesc.maximumWidth = max_widths[i];
					
					status = AddDataBrowserListViewColumn(gMainBrowser, &col_desc, ULONG_MAX);
					if (status != noErr)
					{
						m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to add column to data browser!", 0);
						return -1;
					}
				}
				SetDataBrowserSortProperty(gMainBrowser, kBrowserProperty_Name);
				AutoSizeDataBrowserListViewColumns(gMainBrowser);
			}
		}
	}
	if (status != noErr)
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"An unexpected error occurred while setting up the main data browser.", 0);
		return -1;
	}
	
	// Next thing is to do our three buttons
	status = noErr;
	content.contentType = kControlContentCIconRes;
	SetRect(&bounds, 0, 0, 69, 36);
	content.u.resID = 200; status |= CreateBevelButtonControl(gMainWindow, &bounds, NULL, kControlBevelButtonNormalBevel, kControlBehaviorPushbutton, &content, 0, nil, nil, &gButtonPrev);
	content.u.resID = 201; status |= CreateBevelButtonControl(gMainWindow, &bounds, NULL, kControlBevelButtonNormalBevel, kControlBehaviorPushbutton, &content, 0, nil, nil, &gButtonNext);
	content.u.resID = 202; status |= CreateBevelButtonControl(gMainWindow, &bounds, NULL, kControlBevelButtonNormalBevel, kControlBehaviorPushbutton, &content, 0, nil, nil, &gButtonRestart);
	if (status != noErr)
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"An unexpected error occurred while setting up the main control buttons.", 0);
		return -1;
	}

	// Create Scope grouping
	SetRect(&bounds, 30 + gListWidth, 25, 255 + gListWidth, 105);
	status |= CreateGroupBoxControl(gMainWindow, &bounds, nil, false, &gGroupScope);
	status |= CreateUserPaneControl(gMainWindow, &bounds, nil, &gScopeControl);
	InstallControlEventHandler(gScopeControl, window_scope_render_callback, 1, &gScopeEvents, gScopeControl, NULL);

	// Create info grouping
	SetRect(&bounds, 30 + gListWidth, 195, 255 + gListWidth, 355);
	status |= CreateGroupBoxControl(gMainWindow, &bounds, nil, false, &gGroupInfo);
	SetControlVisibility(gGroupInfo, true, true);
	
	// Create main text options
	style.flags = kControlUseFontMask + kControlUseJustMask + kControlAddToMetaFontMask + kControlUseFaceMask;
	style.just = 0;
	for (i = 0; i < 9; i++)
	{
		CFStringRef msgs[9] = { CFSTR("Game:"), nil, nil, nil, CFSTR("Hardware:"), nil, nil, CFSTR("ROM File:"), nil };
	
		if (msgs[i])
		{
			bounds.left = 40 + gListWidth;
			bounds.top = 204 + (i * 16);
			bounds.right = bounds.left + 235;
			bounds.bottom = bounds.top + 14;
			style.font = kControlFontSmallBoldSystemFont;
			style.style = bold + condense;
			status |= CreateStaticTextControl(gMainWindow, &bounds, msgs[i], &style, &gMainDisplayText[i]);
		}
		else
		{			
			bounds.left = 50 + gListWidth;
			bounds.top = 204 + (i * 16);
			bounds.right = bounds.left + 225;
			bounds.bottom = bounds.top + 14;
			style.font = kControlFontSmallSystemFont;
			style.style = condense;
			status |= CreateStaticTextControl(gMainWindow, &bounds, CFSTR(""), &style, &gMainDisplayText[i]);
		}
	}  

	// Add in ROMs
	window_main_browser_update(false);

	// OK
	return 0;
}


//
// window_clobber_main_tab
// Shut down resources for main tab
//

static void window_clobber_main_tab(void)
{
	int i;
	
	// Bring down main control buttons
	if (gButtonPrev)
	{
		HideControl(gButtonPrev);
		DisposeControl(gButtonPrev);
		gButtonPrev = nil;
	}
	if (gButtonNext)
	{
		HideControl(gButtonNext);
		DisposeControl(gButtonNext);
		gButtonNext = nil;
	}
	if (gButtonRestart)
	{
		HideControl(gButtonRestart);
		DisposeControl(gButtonRestart);
		gButtonRestart = nil;
	}
	
	// Browser
	if (gMainBrowser)
	{
		HideControl(gMainBrowser);
		DisposeControl(gMainBrowser);
		gMainBrowser = nil;
	}
	
	// Text controls
	for (i = 0; i < 9; i++)
	{
		if (gMainDisplayText[i])
		{
			HideControl(gMainDisplayText[i]);
			DisposeControl(gMainDisplayText[i]);
			gMainDisplayText[i] = nil;
		}
	}
	if (gGroupInfo)
	{
		HideControl(gGroupInfo);
		DisposeControl(gGroupInfo);
		gGroupInfo = nil;
	}
	
	// Scope
	if (gScopeControl)
	{
		HideControl(gScopeControl);
		DisposeControl(gScopeControl);
		gScopeControl = nil;
	}
	if (gGroupScope)
	{
		HideControl(gGroupScope);
		DisposeControl(gGroupScope);
		gGroupScope = nil;
	}
}


//
// window_setup_channels_tab
// Set up the controls used for the channels tab
//

static int window_setup_channels_tab(void)
{
	ControlFontStyleRec				style;
	DataBrowserCallbacks			browser_callbacks;
	DataBrowserListViewColumnDesc	col_desc;
	Boolean							f;
	OSStatus						status;
	Rect							bounds;
	int								i;

	// Text Control
	memset(&style, 0, sizeof(ControlFontStyleRec));
	style.flags = kControlUseFontMask + kControlUseJustMask + kControlAddToMetaFontMask + kControlUseFaceMask;
	style.just = teCenter;
	style.font = kControlFontBigSystemFont;
	style.style = bold;
	SetRect(&bounds, 20, 20, gListWidth + 260, 40);
	CreateStaticTextControl(gMainWindow, &bounds, CFSTR("Changes take up to two seconds to take effect!"), &style, &gChannelsWarning);
	
	// Now lets do the data browser
	SetRect(&bounds, 20, 45, gListWidth + 260, gWindowHeight - 40);
	status = CreateDataBrowserControl(gMainWindow, &bounds, kDataBrowserListView, &gChannelBrowser);
	if (status == noErr)
	{
		f = true;
		SetControlData(gChannelBrowser, 0, kControlDataBrowserIncludesFrameAndFocusTag, sizeof(Boolean), &f);
		SetDataBrowserSelectionFlags(gChannelBrowser, kDataBrowserSelectOnlyOne);
		SetDataBrowserListViewUsePlainBackground(gChannelBrowser, false);
		SetDataBrowserHasScrollBars(gChannelBrowser, false, true);
		browser_callbacks.version = kDataBrowserLatestCallbacks;
		status = InitDataBrowserCallbacks(&browser_callbacks);
		if (status == noErr)
		{
			browser_callbacks.u.v1.itemDataCallback = NewDataBrowserItemDataUPP(window_channel_browser_data_callback);
			browser_callbacks.u.v1.itemNotificationCallback = NewDataBrowserItemNotificationUPP(window_channel_browser_notification_callback);
			browser_callbacks.u.v1.itemCompareCallback = NewDataBrowserItemCompareUPP(window_channel_browser_compare_callback);	
			status = SetDataBrowserCallbacks(gChannelBrowser, &browser_callbacks);
			if (status == noErr)
			{
				memset(&col_desc, 0, sizeof(DataBrowserListViewColumnDesc));
				col_desc.headerBtnDesc.version = kDataBrowserListViewLatestHeaderDesc;
				col_desc.headerBtnDesc.btnFontStyle.flags = kControlUseThemeFontIDMask;
				col_desc.headerBtnDesc.btnFontStyle.font = kControlFontViewSystemFont;
				for (i = 0; i < 3; i++)
				{
					CFStringRef cfs[3] = { CFSTR("Channel Name"), CFSTR("Volume"), CFSTR("Panning") };
					int			min_widths[3] = { 300, 300, 100 };
					int			max_widths[3] = { 300, 300, 100 };
					int			properties[3] = { kBrowserProperty_Name, kBrowserProperty_Volume, kBrowserProperty_Pan };
					int			property_types[3] = { kDataBrowserTextType, kDataBrowserSliderType, kDataBrowserSliderType };
					int			property_flags[3] = { kDataBrowserDefaultPropertyFlags, kDataBrowserDefaultPropertyFlags + kDataBrowserPropertyIsEditable, kDataBrowserDefaultPropertyFlags + kDataBrowserPropertyIsEditable};
					
					col_desc.propertyDesc.propertyID = properties[i];
					col_desc.propertyDesc.propertyType = property_types[i];
					col_desc.propertyDesc.propertyFlags = property_flags[i];
					col_desc.headerBtnDesc.titleString = cfs[i];
					col_desc.headerBtnDesc.minimumWidth = min_widths[i];
					col_desc.headerBtnDesc.maximumWidth = max_widths[i];
					
					status = AddDataBrowserListViewColumn(gChannelBrowser, &col_desc, ULONG_MAX);
					if (status != noErr)
					{
						m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to add column to channel browser!", 0);
						return -1;
					}
				}
			}
		}
	}
	if (status != noErr)
	{
		m1ui_message(NULL, M1_MSG_ERROR, (char *)"An unexpected error occurred while setting up the channel data browser.", 0);
		return -1;
	}
	
	// OK
	return 0;
}


//
// window_clobber_channels_tab
// Shut down resources for channels tab
//

static void window_clobber_channels_tab(void)
{
	if (gChannelBrowser)
	{
		HideControl(gChannelBrowser);
		DisposeControl(gChannelBrowser);
		gChannelBrowser = nil;
	}
	
	if (gChannelsWarning)
	{
		HideControl(gChannelsWarning);
		DisposeControl(gChannelsWarning);
		gChannelsWarning = nil;
	}
}


//
// window_setup_settings_tab
// Set up the controls used for the settings tab
//

static int window_setup_settings_tab(void)
{
	OSStatus		status;
	CFStringRef		string;
	Rect			bounds;
	int				i;
	
	// Settings widgets
	for (i = 0; i < WIDGETS_SETTINGS; i++)
	{	
		SetRect(&bounds, 30, 30 + (i * 25), 500, 30 + ((i + 1) * 25));	
		string =  CFStringCreateWithPascalString(NULL, settings_widgets[i].text, GetApplicationTextEncoding());		
		status = CreateCheckBoxControl(gMainWindow, &bounds, string, settings_widgets[i].value, true, &gSettingsWidget[i]);
		if (status != noErr)
		{
			m1ui_message(NULL, M1_MSG_ERROR, (char *)"Unable to create check box control for settings panel.", 0);
			return -1;
		}
		CFRelease(string);
	}
	
	// OK
	return 0;
}


//
// window_clobber_settings_tab
// Shut down resources for settings tab
//

static void window_clobber_settings_tab(void)
{
	int i;

	// Hide and dispose text widgets
	for (i = 0; i < WIDGETS_SETTINGS; i++)
	{
		if (gSettingsWidget[i])
		{
			HideControl(gSettingsWidget[i]);
			DisposeControl(gSettingsWidget[i]);
			gSettingsWidget[i] = nil;
		}
	}	
}


//
// window_close
// Tear down resources used for window
//

void window_close(void)
{
	// Individual tabs
	window_clobber_about_tab();
	window_clobber_main_tab();
	window_clobber_channels_tab();
	window_clobber_settings_tab();
	
	// Main window
	if (gMainWindow)
	{
		HideWindow(gMainWindow);
		DisposeWindow(gMainWindow);
		gMainWindow = NULL;
	}
}


//
// window_draw_scope
// Force a redraw of the oscilloscope
//

void window_draw_scope(void)
{
	if ((gActiveTab == TAB_MAIN) && (gScopeEnabled))
	{
		HIViewSetNeedsDisplay(gScopeControl, true);
	}
}


//
// window_scope_render_callback
// Actual scope renderer
//

static pascal OSStatus window_scope_render_callback (EventHandlerCallRef nextHandler, EventRef theEvent, void * userData)
{
    OSStatus 		err;
    CGContextRef	context;
	short 			*p;
	int				i;

    err = GetEventParameter( theEvent, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(CGContextRef), NULL, &context);
	if (err == noErr)
	{
		if (!gScopeEnabled)
		{
			char *msg = (char *)"Oscilloscope Disabled";

			CGContextSetTextMatrix(context, CGContextGetCTM(context));			
			CGContextSelectFont(context, "Geneva", 10, kCGEncodingMacRoman);
			CGContextShowTextAtPoint(context, 60, 34.5, msg, strlen(msg));
		}
		else if (!gScopeGG)
		{
			if (m1sdr_CurrentlyPlaying())
			{
				p = m1sdr_GetPlayBufferPtr();
				
				for (i = 0; i < 225; i++)
				{
					int calc;
							
					calc = (*p++ / 1024);
					if (i == 0)
						CGContextMoveToPoint(context, i + 0.5, calc + 33.5);
					else
						CGContextAddLineToPoint(context, i + 0.5, calc + 33.5);
				}
				CGContextSetLineWidth(context, 1);
				CGContextStrokePath(context);
			}
			else
			{	
				CGContextMoveToPoint(context, 0, 34.5);
				CGContextAddLineToPoint(context, 225, 34.5);
				CGContextSetLineWidth(context, 1);
				CGContextStrokePath(context);
			}
		}
		else
		{
			if (m1sdr_CurrentlyPlaying())
			{
				p = m1sdr_GetPlayBufferPtr();
				
				CGContextSetTextMatrix(context, CGContextGetCTM(context));			
				CGContextSelectFont(context, "Geneva", 10, kCGEncodingMacRoman);

				for (i = 0; i < 225; i+=5)
				{
					int calc;
								
					calc = (*p / 1024);
					p += 5;
					
					CGContextShowTextAtPoint(context, i, calc + 33.5, "g", 1);					
				}
			}
			else
			{
				// To anybody reading this code and wondering what the hell...
				// This is a private joke. Those that understand it will know precisely
				// what it means. To anyone else, I'll gladly explain the pun if and only
				// if you stun me with a large sum of money! :-)
				char *msg = (char *)"gggggggggggggggggggggggggggggggggggggggggggggg";

				CGContextSetTextMatrix(context, CGContextGetCTM(context));			
				CGContextSelectFont(context, "Geneva", 10, kCGEncodingMacRoman);
				CGContextShowTextAtPoint(context, 0, 33.5, msg, strlen(msg));
			}			
		}
		
		// Do time stamp
		{
			char msg[40];
			
			// Ask sound driver for time lapse
			m1sdr_GetPlayTimeStr(msg);
			
			// Render it
			CGContextSetTextMatrix(context, CGContextGetCTM(context));			
			CGContextSelectFont(context, "Geneva", 10, kCGEncodingMacRoman);
			CGContextShowTextAtPoint(context, 85, 73.5, msg, strlen(msg));
		}
	}
	
    return err;
}


//
// window_set_active_tab
// Our window contains a lot of ControlRefs. The purpose of this function is to set the visibility of the ones
// corresponding to the current tab. It might be nicer to swap in controls as needed, but this works for me.
//

static void window_set_active_tab(void)
{
	int i;
	
	gActiveTab = GetControlValue(gTabsControl);

	// About tab enable/disable
	if (gActiveTab == TAB_ABOUT)
	{
		SetControlVisibility(gIconCtl, true, true);
		for (i = 0; i < WIDGETS_ABOUT; i++)
			SetControlVisibility(gAboutWidget[i], true, true);
	}
	else
	{
		SetControlVisibility(gIconCtl, false, false);
		for (i = 0; i < WIDGETS_ABOUT; i++)
			SetControlVisibility(gAboutWidget[i], false, false);
	}

	// Main tab enable/disable
	if (gActiveTab == TAB_MAIN)
	{
		SetControlVisibility(gButtonRestart, true, true);
		SetControlVisibility(gButtonPrev, true, true);
		SetControlVisibility(gButtonNext, true, true);
		SetControlVisibility(gMainBrowser, true, true);
		SetControlVisibility(gGroupScope, true, true);
		SetControlVisibility(gGroupInfo, true, true);
	 	SetControlVisibility(gScopeControl, true, true);
		
		for (i = 0; i < 9; i++)
		{
			SetControlVisibility(gMainDisplayText[i], true, true);
		}
	}
	else
	{
		SetControlVisibility(gButtonRestart, false, false);
		SetControlVisibility(gButtonPrev, false, false);
		SetControlVisibility(gButtonNext, false, false);
		SetControlVisibility(gMainBrowser, false, false);
		SetControlVisibility(gGroupScope, false, false);
		SetControlVisibility(gGroupInfo, false, false);
		SetControlVisibility(gScopeControl, false, false);

		for (i = 0; i < 9; i++)
		{
			SetControlVisibility(gMainDisplayText[i], false, false);
		}
	}

	// Settings tab enable/disable
	if (gActiveTab == TAB_SETTINGS)
	{
		for (i = 0; i < WIDGETS_SETTINGS; i++)
			SetControlVisibility(gSettingsWidget[i], true, true);
	}
	else
	{
		for (i = 0; i < WIDGETS_SETTINGS; i++)
			SetControlVisibility(gSettingsWidget[i], false, false);
	}

	// Channels tab
	if (gActiveTab == TAB_CHANNELS)
	{
		SetControlVisibility(gChannelBrowser, true, true);
		SetControlVisibility(gChannelsWarning, true, true);
	}
	else
	{
		SetControlVisibility(gChannelBrowser, false, false);
		SetControlVisibility(gChannelsWarning, false, false);
	}
}


//
// window_resize_and_move_controls
// Resize the various controls after the window size is changed
//

static void window_resize_and_move_controls(void)
{
	Rect	rect;
	int		i, tot;
	
	GetWindowPortBounds(gMainWindow, &rect);
	
	gListWidth = rect.right - 280;
	gWindowHeight = rect.bottom - rect.top;

	SizeControl(gTabsControl, rect.right - rect.left - 20, rect.bottom - rect.top - 20);
	
	// About tab moving
	MoveControl(gIconCtl, (rect.right - rect.left - 128) / 2, 30);
	for (i = 0; i < WIDGETS_ABOUT; i++)
	{
		MoveControl(gAboutWidget[i], (rect.right - rect.left - WIDGET_WIDTH) / 2, about_widgets[i].bounds.top);
	}
	
	// Channels tab
	SizeControl(gChannelBrowser, rect.right - rect.left - 40, rect.bottom - rect.top - 85);	
	SizeControl(gChannelsWarning, rect.right - rect.left - 40, 20);	
	
	// Main tab moving
	SizeControl(gMainBrowser, gListWidth, rect.bottom - rect.top - 60);
	AutoSizeDataBrowserListViewColumns(gMainBrowser);
	MoveControl(gGroupInfo, 30 + gListWidth, 156);
	MoveControl(gButtonPrev, 30 + gListWidth, 113);
	MoveControl(gButtonRestart, 108 + gListWidth, 113);
	MoveControl(gButtonNext, 186 + gListWidth, 113);
	MoveControl(gGroupScope, 30 + gListWidth, 22);
	MoveControl(gScopeControl, 30 + gListWidth, 22);
	tot = 0;
	for (i = 0; i < 9; i++)
	{
		if ((i == 0) || (i == 4) || (i == 7))
		{
			tot += 2;
			MoveControl(gMainDisplayText[i], 40 + gListWidth, 158 + (i * 16) + tot);
		}
		else
		{
			MoveControl(gMainDisplayText[i], 50 + gListWidth, 158 + (i * 16) + tot);
		}
	}
}


//
// window_main_browser_update
// Update the list in the main data browser
//

static void window_main_browser_update(Boolean add_all)
{
	int 	i;
	OSErr	err;
	
	err = RemoveDataBrowserItems(gMainBrowser, kDataBrowserNoItem, 0, NULL, 0);
	if (err != noErr)
	{
		m1ui_message(NULL, M1_MSG_ERROR, "Error while clearing data browser. This shouldn't happen.", 0);
	}
	
	for (i = 0; i < gTotalGames; i++)
	{
		if (add_all || path_roms[i].present || path_pars[i].present)
		{
			DataBrowserItemID	browserid;
			
			browserid = i + 1;
			err = AddDataBrowserItems(gMainBrowser, kDataBrowserNoItem, 1, &browserid, 0);
			if (err != noErr)
			{
				m1ui_message(NULL, M1_MSG_ERROR, "Unable to add item to data browser!", 0);
				ExitToShell();
			}
		}
	}
}


//
// window_sync_settings
// Synchronises the current values of the settings checkboxes with internal preferences.
//

static void window_sync_settings(void)
{
	int i;
	for (i = 0; i < WIDGETS_SETTINGS; i++)
	{
		if (settings_widgets[i].enable_if != -1)
		{
			HiliteControl(gSettingsWidget[i], settings_widgets[settings_widgets[i].enable_if].value ? 0 : 255);
		}
	}
	
	if (gShowMissing != settings_widgets[0].value)
	{
		gShowMissing = settings_widgets[0].value;
		window_main_browser_update(gShowMissing);
	}
	m1snd_setoption(M1_OPT_NORMALIZE, settings_widgets[1].value);
	if (gLogWAV != settings_widgets[2].value)
	{
		gLogWAV = settings_widgets[2].value;
		m1snd_setoption(M1_OPT_WAVELOG, gLogWAV);
		if (gLogWAV)
		{
			waveLogStart();
		}
		else
		{
			waveLogStop();
		}
	}
	gScopeEnabled = settings_widgets[3].value;
}


//
// window_set_titlebar
// Set the title for the window
//

static void window_set_titlebar(void)
{
	CFStringRef	window_title;
	
	// Window title
	if (!gScopeGG)
		window_title = CFStringCreateWithFormat(NULL, NULL, CFSTR("M1 v%s (%i games, %i found)"), m1snd_get_info_str(M1_SINF_COREVERSION, 0), gTotalGames, mac_roms_count_found());
	else
		window_title = CFStringCreateWithFormat(NULL, NULL, CFSTR("Omnes arx vestrum sunt adiuncta nobis!"));
			
	SetWindowTitleWithCFString(gMainWindow, window_title);	
	CFRelease(window_title);
}


//
// window_main_browser_compare_callback
// Used for sorting the game list
//

static pascal Boolean window_main_browser_compare_callback(ControlRef ref, DataBrowserItemID itemOne, DataBrowserItemID itemTwo, DataBrowserPropertyID sortProperty)
{
	char *a = NULL, *b = NULL;
	int compareResult;
	
	switch(sortProperty)
	{
		case kBrowserProperty_Name:
		{
			a = m1snd_get_info_str(M1_SINF_VISNAME, itemOne - 1);
			b = m1snd_get_info_str(M1_SINF_VISNAME, itemTwo - 1);
		}
		break;

		case kBrowserProperty_Manuf:
		{
			a = m1snd_get_info_str(M1_SINF_MAKER, itemOne - 1);
			b = m1snd_get_info_str(M1_SINF_MAKER, itemTwo - 1);
		}
		break;

		case kBrowserProperty_Zipname:
		{
			a = m1snd_get_info_str(M1_SINF_ROMNAME, itemOne - 1);
			b = m1snd_get_info_str(M1_SINF_ROMNAME, itemTwo - 1);
		}
		break;
	}
	
	if (a)
	{
		compareResult = strcmp(a, b);
		if (compareResult < 0) return true;
		else if (compareResult > 0) return false;
	}
	
	return itemOne < itemTwo;
}


//
// window_main_browser_notification_callback
// Used to signal when the user has clicked on our browser
//

static pascal void window_main_browser_notification_callback (ControlRef ref, DataBrowserItemID itemID, DataBrowserItemNotification message)
{
	switch (message)
	{
		case kDataBrowserItemSelected:
		{
			mac_switch_game(itemID - 1);
		}
		break;
	}
}


//
// window_main_browser_data_callback
// Returns the requisite text for the data browser 
//

static pascal OSStatus window_main_browser_data_callback (ControlRef ref, DataBrowserItemID itemID, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue)
{
	OSStatus	err = noErr;
	CFStringRef	tmp;
	
	if (!setValue)
	{
		switch(property)
		{
			case kBrowserProperty_Name:
			{
				tmp = CFStringCreateWithCString(NULL, m1snd_get_info_str(M1_SINF_VISNAME, itemID - 1), NULL);
				if (tmp)
				{
					SetDataBrowserItemDataText(itemData, tmp);
					CFRelease(tmp);
				}
			}
			break;
			
			case kBrowserProperty_Manuf:
			{
				tmp = CFStringCreateWithCString(NULL, m1snd_get_info_str(M1_SINF_MAKER, itemID - 1), NULL);
				if (tmp)
				{
					SetDataBrowserItemDataText(itemData, tmp);
					CFRelease(tmp);
				}
			}
			break;
			
			case kBrowserProperty_Zipname:
			{
				tmp = CFStringCreateWithCString(NULL, m1snd_get_info_str(M1_SINF_ROMNAME, itemID - 1), NULL);
				if (tmp)
				{
					SetDataBrowserItemDataText(itemData, tmp);
					CFRelease(tmp);
				}
			}
			break;
			
			case kDataBrowserItemIsActiveProperty:
			case kDataBrowserItemIsSelectableProperty:
			{
				err = SetDataBrowserItemDataBooleanValue(itemData, path_roms[itemID - 1].present || path_pars[itemID - 1].present);
				assert(err == noErr);
			}
			break;
		}
	}
	
	return err;
}


//
// window_control_hit
// Process the fact that someone has clicked on one of our controls
//

static OSStatus window_control_hit(ControlRef ctl)
{
	OSStatus	result;
	int			i;
	
	// Default result = nothing
	result = eventNotHandledErr;
					
	if (ctl == gButtonNext)
	{
		mac_switch_song(+1);
		result = noErr;
	}
	else if (ctl == gButtonPrev)
	{
		mac_switch_song(-1);
		result = noErr;
	}
	else if (ctl == gButtonRestart)
	{
		mac_switch_song(0);
		result = noErr;
	}
	else if (ctl == gTabsControl)
	{
		window_set_active_tab();
		result = noErr;
	}
	else
	{
		for (i = 0; i < WIDGETS_SETTINGS; i++)
		{
			if (ctl == gSettingsWidget[i])
			{
				settings_widgets[i].value = (settings_widgets[i].value == 1) ? 0 : 1;
				result = noErr;
				window_sync_settings();
			}
		}
	}
}


//
// window_channel_browser_update
// Update the list in the channel data browser
//

void window_channel_browser_update(void)
{
	DataBrowserItemID	browserid;
	int					i;
	OSStatus			err;
	int					numst, st, ch;

	// Create the channel names
	gChannels = 0;
	memset(m1_chans, 0, sizeof(m1_channel) * MAX_CHANS);
	
	// Fill the details
	numst = m1snd_get_info_int(M1_IINF_NUMSTREAMS, 0);
	for (st = 0; st < numst; st++)
	{
		for (ch = 0; ch < m1snd_get_info_int(M1_IINF_NUMCHANS, st); ch++)
		{
			sprintf(m1_chans[gChannels].name, "st %02d ch %02d: [%s]", st, ch, m1snd_get_info_str(M1_SINF_CHANNAME, st<<16|ch));
			m1_chans[gChannels].st = st;
			m1_chans[gChannels].chan = ch;
			m1_chans[gChannels].level = m1snd_get_info_int(M1_IINF_CHANLEVEL, st<<16|ch);
			m1_chans[gChannels].pan = m1snd_get_info_int(M1_IINF_CHANPAN, st<<16|ch);
			gChannels++;
			assert(gChannels < MAX_CHANS);
		}
	}
		
	// Remove old
	err = RemoveDataBrowserItems(gChannelBrowser, kDataBrowserNoItem, 0, NULL, 0);
	assert(err == noErr);
		
	// Add new
	for (i = 0; i < gChannels; i++)
	{
		browserid = i + 1;
		err = AddDataBrowserItems(gChannelBrowser, kDataBrowserNoItem, 1, &browserid, 0);
		assert(err == noErr);
	}
}


//
// window_channel_browser_data_callback
// Returns the requisite text for the data browser 
//
	
static pascal OSStatus window_channel_browser_data_callback ( ControlRef control, DataBrowserItemID itemID, DataBrowserPropertyID property, DataBrowserItemDataRef itemData, Boolean setValue)
{
	OSStatus	err = noErr;
	CFStringRef	tmp;
	int			pan_convert[3] = { MIXER_PAN_LEFT, MIXER_PAN_CENTER, MIXER_PAN_RIGHT };

	if (!setValue)
	{
		switch(property)
		{
			case kBrowserProperty_Name:
			{
				tmp = CFStringCreateWithCString(NULL, m1_chans[itemID-1].name, NULL);
				if (tmp)
				{
					SetDataBrowserItemDataText(itemData, tmp);
					CFRelease(tmp);
				}
			}
			break;
			
			case kBrowserProperty_Volume:
			{
				SetDataBrowserItemDataMinimum(itemData, 0);
				SetDataBrowserItemDataMaximum(itemData, 511);
				SetDataBrowserItemDataValue(itemData, m1_chans[itemID-1].level);
			}
			break;
			
			case kBrowserProperty_Pan:
			{
				SetDataBrowserItemDataMinimum(itemData, 0);
				SetDataBrowserItemDataMaximum(itemData, 2);
				SetDataBrowserItemDataValue(itemData, pan_convert[m1_chans[itemID-1].pan]);
			}
		}
	}
	else
	{
		switch (property)
		{
			case kBrowserProperty_Volume:
			{
				SInt32 result;
				GetDataBrowserItemDataValue(itemData, &result);
				m1_chans[itemID-1].level = result;
				m1snd_set_info_int(M1_SIINF_CHANLEVEL, m1_chans[itemID-1].st, m1_chans[itemID-1].chan, m1_chans[itemID-1].level);
			}
			break;
		
			case kBrowserProperty_Pan:
			{
				SInt32 result;
				GetDataBrowserItemDataValue(itemData, &result);
				m1_chans[itemID-1].pan = pan_convert[result];
				m1snd_set_info_int(M1_SIINF_CHANPAN, m1_chans[itemID-1].st, m1_chans[itemID-1].chan, m1_chans[itemID-1].pan);
			}
		}
	}
	
	return err;
}


//
// window_channel_browser_compare_callback
// Unused
//

static pascal Boolean window_channel_browser_compare_callback(ControlRef control, DataBrowserItemID itemOne, DataBrowserItemID itemTwo, DataBrowserPropertyID sortProperty )
{
	// unused
	return false;
}


//
// window_channel_browser_notification_callback
// Unused
//

static pascal void window_channel_browser_notification_callback ( ControlRef control, DataBrowserItemID itemID, DataBrowserItemNotification message )
{
	// unused
}


//
// window_switch_to_about
// Switches tab to the about box. Called from the menu handler.
//

void window_switch_to_about(void)
{
	SetControlValue(gTabsControl, 1);
	window_set_active_tab();
}


//
// window_event_dispatcher
// Main event handler for the active window
//

static pascal OSStatus window_event_dispatcher(EventHandlerCallRef ref, EventRef event, void *unused)
{
	OSStatus		result, err;
	unsigned int	whatClass, whatHappened;

	// Default result is failure
	result = eventNotHandledErr;
	
	// Determine what type of event we've been given and bounce to the appropriate handler
	whatClass = GetEventClass(event);
	whatHappened = GetEventKind(event);
	switch (whatClass)
	{
		case kEventClassKeyboard:
			switch (whatHappened)
			{
				case kEventRawKeyUp:
				{
					char key;
					
					GetEventParameter (event, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &key);
					
					if (key == gDrwbwlStr[gDrwbwlCnt])
					{
						gDrwbwlCnt++;
						if (gDrwbwlCnt == 6)
						{
							gDrwbwlCnt = 0;
							gScopeGG = !gScopeGG;
							window_set_titlebar();
						}
					}
					else
					{
						gDrwbwlCnt = 0;
					}
					
					if (key == 28) // Left
					{
						mac_switch_song(-1);
						result = noErr;
					}
					else if (key == 29) // Right
					{
						mac_switch_song(+1);
						result = noErr;
					}
					else if ((key == '/') || (key == '?'))
					{
						mac_switch_song(0);
						result = noErr;
					}
					
				}
				break;
			}
			break;
			
		case kEventClassControl:
			switch (whatHappened)
			{
				case kEventControlHit:
				{
					ControlHandle ctl;
					
					// Get location
					GetEventParameter(event, kEventParamDirectObject, typeControlRef, NULL, sizeof(ControlRef), NULL, &ctl);
					
					// Pass to the windowing code
					result = window_control_hit(ctl);
				}
				break;
			}
			break;
		
		case kEventClassWindow:
			switch (whatHappened)
			{
				case kEventWindowBoundsChanging:
				{
					Rect	bounds;
					
					err = GetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, NULL, sizeof(Rect), NULL, &bounds);
					
					if (err == noErr)
					{
						if ((bounds.right - bounds.left) < MIN_WIDTH)
						{
							bounds.right = bounds.left + MIN_WIDTH;
						}
						if ((bounds.bottom - bounds.top) < MIN_HEIGHT)
						{
							bounds.bottom = bounds.top + MIN_HEIGHT;
						}
						
						SetEventParameter(event, kEventParamCurrentBounds, typeQDRectangle, sizeof(Rect), &bounds);
					}
					
					return noErr;
				}
				break;
				
				case kEventWindowClose:
				{
					QuitApplicationEventLoop();
				}
				break;
				
				case kEventWindowBoundsChanged:
				{
					UInt32		attributes;

					err = GetEventParameter(event, kEventParamAttributes, typeUInt32, NULL, sizeof(UInt32), NULL, &attributes );
					if (err == noErr)
					{
						if (attributes & kWindowBoundsChangeSizeChanged)
						{
							window_resize_and_move_controls();
							result = noErr;
						}
					}
				}
				break;
			}
			break;
	}
	
	return result;
}


//
// window_set_display_text (and convenience functions)
// Sets the current text display window to passed value, or, if blank, wipes it.
//

static void window_set_display_text(int which, char *what)
{
	if (what == nil)
	{
		SetControlData(gMainDisplayText[which], 0, kControlStaticTextTextTag, nil, nil);
	}
	else
	{
		SetControlData(gMainDisplayText[which], 0, kControlStaticTextTextTag, strlen(what), what);
	}
	HIViewSetNeedsDisplay(gMainDisplayText[which], true);	
}

void window_set_game_name(char *what)		{ window_set_display_text(1, what); }
void window_set_song_number(char *what)		{ window_set_display_text(2, what); }
void window_set_song_title(char *what)		{ window_set_display_text(3, what); }
void window_set_driver_name(char *what)		{ window_set_display_text(5, what); }
void window_set_hardware_desc(char *what)	{ window_set_display_text(6, what); }
void window_set_rom_name(char *what)		{ window_set_display_text(8, what); }


