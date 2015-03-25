/*
 *     Copyright (C) 2010-2015 Marvell International Ltd.
 *     Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */
#if 0
#define __FSKMENU_PRIV__
#include "FskMenu.h"
#include "FskTextConvert.h"
#include "FskUtilities.h"
#include "FskWindow.h"

#if TARGET_OS_MAC
	#if TARGET_OS_IPHONE
		#include "FskCocoaSupportPhone.h"
	#else
		#include "FskCocoaSupport.h"
	#endif
#endif

#if TARGET_OS_WIN32
	void rebuildMenuBar(FskMenuBar mb);
#endif /* OS */

FskErr FskMenuBarNew(FskMenuBar *mbOut)
{
	FskErr err;
	FskMenuBar mb;

	err = FskMemPtrNewClear(sizeof(FskMenuBarRecord), (FskMemPtr *)&mb);
	BAIL_IF_ERR(err);

#if TARGET_OS_WIN32
	mb->nativeMenuBar = CreateMenu();
#endif

bail:
	if (kFskErrNone != err) {
		FskMenuBarDispose(mb);
		mb = NULL;
	}

	*mbOut = mb;

	return err;
}

FskErr FskMenuBarDispose(FskMenuBar mb)
{
	if (mb) {
		while (mb->menus)
			FskMenuDispose(mb->menus);

#if TARGET_OS_WIN32
		DestroyMenu(mb->nativeMenuBar);
#endif

		FskMemPtrDispose(mb);
	}

	return kFskErrNone;
}

FskErr FskMenuBarAddMenu(FskMenuBar mb, FskMenu menu)
{
	char *encodedText;

	if (menu->menuBar)
		return kFskErrBadState;

	FskTextToPlatform(menu->title, FskStrLen(menu->title), &encodedText, NULL);

#if TARGET_OS_WIN32
	AppendMenu(mb->nativeMenuBar, MF_POPUP, (UINT)menu->nativeMenu, encodedText);
#endif

	FskMemPtrDispose(encodedText);

	FskListAppend((FskList *)&mb->menus, menu);
	menu->menuBar = mb;

#if TARGET_OS_MAC
    rebuildMenuBar(mb);
#endif

	return kFskErrNone;
}

FskErr FskMenuBarRemoveMenu(FskMenuBar mb, FskMenu menu)
{
	if (menu->menuBar != mb)
		return kFskErrBadState;

	FskListRemove((FskList *)&mb->menus, menu);
	menu->menuBar = NULL;

	if (menu->beingDisposed)
		return kFskErrNone;

#if TARGET_OS_WIN32
	rebuildMenuBar(mb);		// windows doesn't seem to be able to remove a menu from the menu bar... so we just rebuild the whole thing.
#elif TARGET_OS_MAC
    rebuildMenuBar(mb);
#endif

	return kFskErrNone;
}

FskMenu FskMenuBarGetMenuByID(FskMenuBar mb, UInt32 uniqueID)
{
	FskMenu walker;
	for (walker = mb->menus; NULL != walker; walker = walker->next)
		if (walker->uniqueID == uniqueID)
			return walker;
	return NULL;
}

FskMenu FskMenuBarGetMenuByIndex(FskMenuBar mb, UInt32 index)
{
	FskMenu walker;
	for (walker = mb->menus; NULL != walker; walker = walker->next)
		if (--index == 0)
			return walker;
	return NULL;
}

#if TARGET_OS_WIN32

FskErr FskMenuBarEventToUniqueID(FskMenuBar mb, UInt16 key, UInt32 modifiers, UInt32 *uniqueID)
{
	FskMenu menu;

	for (menu = mb->menus; NULL != menu; menu = menu->next) {
		FskMenuItem item;

		for (item = menu->items; NULL != item; item = item->next) {
			char *k = item->key;
			if (!k || !item->enabled) continue;

			if (kFskEventModifierShift & modifiers) {
				if (*k++ != 's')
					continue;
			}

			if ((key == k[0]) && !k[1]) {
				*uniqueID = item->uniqueID;
				return kFskErrNone;
			}
		}
	}

	*uniqueID = 0;

	return kFskErrOperationFailed;
}

#endif

FskErr FskMenuNew(FskMenu *menuOut, char *title, UInt32 uniqueID)
{
	FskErr err;
	FskMenu menu;

	err = FskMemPtrNewClear(sizeof(FskMenuRecord), (FskMemPtr *)&menu);
	BAIL_IF_ERR(err);

	menu->uniqueID = uniqueID;
	menu->title = FskStrDoCopy(title);

#if TARGET_OS_WIN32
	menu->nativeMenu = CreateMenu();
#elif TARGET_OS_MAC
    FskCocoaMenuNew(menu);
#endif

bail:
	if (kFskErrNone != err) {
		FskMenuDispose(menu);
		menu = NULL;
	}

	*menuOut = menu;

	return err;
}

FskErr FskMenuDispose(FskMenu menu)
{
	if (menu) {
		menu->beingDisposed = true;

		if (menu->menuBar)
			FskMenuBarRemoveMenu(menu->menuBar, menu);

		while (menu->items)
			FskMenuRemoveItem(menu, menu->items);

#if TARGET_OS_WIN32
		DestroyMenu(menu->nativeMenu);
#elif TARGET_OS_MAC
    FskCocoaMenuRemove(menu);
#endif

		FskMemPtrDispose(menu);
	}

	return kFskErrNone;
}

FskErr FskMenuAddItem(FskMenu menu, char *title, UInt32 uniqueID, Boolean enabled, Boolean checked, char *key, FskMenuItem *itemOut)
{
	FskErr err;
	FskMenuItem item;

	err = FskMemPtrNewClear(sizeof(FskMenuItemRecord), (FskMemPtr *)&item);
	BAIL_IF_ERR(err);

	FskListAppend((FskList *)&menu->items, item);

	item->uniqueID = uniqueID;
	item->menu = menu;

#if TARGET_OS_WIN32
	if (0 != FskStrLen(title)) {

		AppendMenu(menu->nativeMenu, MF_STRING, uniqueID, "");
		err = FskMenuItemSetProperties(item, title, enabled, checked, key);
		BAIL_IF_ERR(err);
	}
	else
		AppendMenu(menu->nativeMenu, MF_SEPARATOR, uniqueID, NULL);
#elif TARGET_OS_MAC
    FskCocoaMenuItemAdd(menu, item, (FskStrLen(title) == 0));

    err = FskMenuItemSetProperties(item, title, enabled, checked, key);
    BAIL_IF_ERR(err);
#endif

bail:
	if (kFskErrNone != err) {
		FskMenuRemoveItem(menu, item);
		item = NULL;
	}

	if (itemOut) *itemOut = item;

	return err;
}

FskErr FskMenuRemoveItem(FskMenu menu, FskMenuItem item)
{
	if (item) {
		UInt32 position = 1;
		FskMenuItem walker;
		for (walker = menu->items; NULL != walker; walker = walker->next, ++position) {
			if (item == walker)
				break;
		}

		if (!menu->beingDisposed) {
#if TARGET_OS_WIN32
			RemoveMenu(menu->nativeMenu, item->uniqueID, MF_BYCOMMAND);
#elif TARGET_OS_MAC
        FskCocoaMenuItemRemove(menu, item);
#endif
		}

		FskListRemove((FskList *)&menu->items, item);
		FskMemPtrDispose(item->title);
		FskMemPtrDispose(item->key);
		FskMemPtrDispose(item);
	}

	return kFskErrNone;
}

FskMenuItem FskMenuGetItemByID(FskMenu menu, UInt32 uniqueID)
{
	FskMenuItem walker;
	for (walker=menu->items; NULL != walker; walker=walker->next)
		if (uniqueID == walker->uniqueID)
			return walker;
	return NULL;
}

FskMenuItem FskMenuGetItemByIndex(FskMenu menu, UInt32 index)
{
	FskMenuItem walker;
	for (walker = menu->items; NULL != walker; walker = walker->next)
		if (--index == 0)
			return walker;
	return NULL;
}

FskErr FskMenuItemSetProperties(FskMenuItem item, char *title, Boolean enabled, Boolean checked, char *key)
{
	char *encodedText;

	title = FskStrDoCopy(title);	// this copy is a little bit convoluted to allow this function to be used by rebuildMenuBar
	FskMemPtrDispose(item->title);
	item->title = title;

	item->enabled = enabled;
	item->checked = checked;

	key = FskStrDoCopy(key);		// this copy is a little bit convoluted to allow this function to be used by rebuildMenuBar
	FskMemPtrDispose(item->key);
	item->key = key;

	FskTextToPlatform(item->title, FskStrLen(item->title), &encodedText, NULL);

#if TARGET_OS_WIN32
	if (key) {
		char *t;
		UInt32 len = 9 + FskStrLen(encodedText);
		if ('s' == key[0])
			len += 6;
		FskMemPtrNew(len, (FskMemPtr *)&t);
		FskStrCopy(t, encodedText);
		FskStrCat(t, "\t");
		if ('s' == key[0]) {
			FskStrCat(t, "Shift+");
			key += 1;
		}
		FskStrCat(t, "Ctrl+");
		FskStrCat(t, key);

		FskMemPtrDispose(encodedText);
		encodedText = t;
	}

	ModifyMenu(item->menu->nativeMenu, item->uniqueID, MF_BYCOMMAND | MF_STRING | (enabled ? MF_ENABLED : MF_GRAYED) |
				(checked ? MF_CHECKED : 0), item->uniqueID, encodedText);
#elif TARGET_OS_MAC
    FskCocoaMenuItemSetProperties(item->menu, item);
#endif

	FskMemPtrDispose(encodedText);

	return kFskErrNone;
}

#if TARGET_OS_WIN32

void rebuildMenuBar(FskMenuBar mb)
{
	FskMenu menu;
	FskWindow window = mb->window;

	if (mb->nativeMenuBar) {
		if (mb->window)
			FskWindowSetMenuBar(mb->window, NULL);
		DestroyMenu(mb->nativeMenuBar);
	}
	mb->nativeMenuBar = CreateMenu();

	for (menu = mb->menus; NULL != menu; menu = menu->next) {
		char *encodedText;
		FskMenuItem item;

		menu->nativeMenu = CreateMenu();

		FskTextToPlatform(menu->title, FskStrLen(menu->title), &encodedText, NULL);
		AppendMenu(mb->nativeMenuBar, MF_POPUP, (UINT)menu->nativeMenu, encodedText);

		FskMemPtrDispose(encodedText);

		for (item = menu->items; NULL != item; item = item->next) {
			if (0 != FskStrLen(item->title)) {
				AppendMenu(menu->nativeMenu, MF_STRING, item->uniqueID, "");
				FskMenuItemSetProperties(item, item->title, item->enabled, item->checked, item->key);
			}
			else
				AppendMenu(menu->nativeMenu, MF_SEPARATOR, menu->uniqueID, NULL);
		}
	}

	if (window)
		FskWindowSetMenuBar(window, mb);
}

#elif TARGET_OS_MAC
    void rebuildMenuBar(FskMenuBar mb)
    {
        FskMenu menu;

        FskCocoaMenuBarClear();

        for (menu = mb->menus; NULL != menu; menu = menu->next)
            FskCocoaMenuAdd(menu);
    }
#endif

#endif
