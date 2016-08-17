#include "xs.h"
#include "FskWindow.h"
#include "kprShell.h"

void KPR_shell_changeCursor(xsMachine* the)
{
	KprShell self = xsGetHostData(xsThis);
	(void)FskWindowSetCursorShape(self->window, xsToInteger(xsArg(0)));
}

void KPR_Shell_patch(xsMachine* the)
{
	xsVars(1);
	xsResult = xsGet(xsGlobal, xsID("system"));
	xsVar(0) = xsNewInstanceOf(xsObjectPrototype);
	xsNewHostProperty(xsVar(0), xsID("arrow"), xsInteger(kFskCursorArrow), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("aliasArrow"), xsInteger(kFskCursorAliasArrow), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("copyArrow"), xsInteger(kFskCursorCopyArrow), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("wait"), xsInteger(kFskCursorWait), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("iBeam"), xsInteger(kFskCursorIBeam), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("notAllowed"), xsInteger(kFskCursorNotAllowed), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resize"), xsInteger(kFskCursorResizeAll), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeEW"), xsInteger(kFskCursorResizeLeftRight), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeNS"), xsInteger(kFskCursorResizeTopBottom), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeNESW"), xsInteger(kFskCursorResizeNESW), xsDefault, xsDontScript);
	xsNewHostProperty(xsVar(0), xsID("resizeNWSE"), xsInteger(kFskCursorResizeNWSE), xsDefault, xsDontScript);			
	xsNewHostProperty(xsVar(0), xsID("link"), xsInteger(kFskCursorLink), xsDefault, xsDontScript);			
	xsNewHostProperty(xsVar(0), xsID("resizeColumn"), xsInteger(kFskCursorResizeColumn), xsDefault, xsDontScript);			
	xsNewHostProperty(xsVar(0), xsID("resizeRow"), xsInteger(kFskCursorResizeRow), xsDefault, xsDontScript);			
	xsNewHostProperty(xsResult, xsID("cursors"), xsVar(0), xsDefault, xsDontScript);
}

void KPR_system_beginModal(xsMachine* the)
{
	KprShell self = gShell;
	(*self->handler)(NULL, kFskEventWindowDeactivated, NULL, self);
}

void KPR_system_endModal(xsMachine* the)
{
	KprShell self = gShell;
	(*self->handler)(NULL, kFskEventWindowActivated, NULL, self);
}
