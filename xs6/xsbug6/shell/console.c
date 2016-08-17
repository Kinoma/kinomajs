#include "xs.h"
#include "FskString.h"
#include "FskManifest.xs.h"

void ConsolePaneBehavior_prototype_splitError(xsMachine* the)
{
	xsStringValue string, p;
	char c;
	xsIndex ids[4];
	size_t offsets[4], lengths[4];
	int i;
	xsVars(1);
	string = p = xsToString(xsArg(0));
	c = *p;
	if (c != '/') goto bail;
	ids[0] = xsID_path;
	offsets[0] = p - string;
	p = FskStrChr(p, ':');
	if (!p) goto bail;
	lengths[0] = (p - string) - offsets[0];
	p++;
	ids[1] = xsID_line;
	offsets[1] = p - string;
	while (((c = *p)) && ('0' <= c) && (c <= '9'))
		p++;
	if (c != ':') goto bail;
	lengths[1] = (p - string) - offsets[1];
	p++;
	c  = *p;
	if (('0' <= c) && (c <= '9')) {
		p++;
		while (((c = *p)) && ('0' <= c) && (c <= '9'))
			p++;
		if (c != ':') goto bail;
		p++;
		c  = *p;
	}
	if (c != ' ') goto bail;
	p++;
	ids[2] = xsID_kind;
	offsets[2] = p - string;
	p = FskStrChr(p, ':');
	if (!p) goto bail;
	lengths[2] = (p - string) - offsets[2];
	p++;
	c = *p;
	if (c != ' ') goto bail;
	p++;
	ids[3] = xsID_reason;
	offsets[3] = p - string;
	lengths[3] = FskStrLen(p);
	xsResult = xsNewInstanceOf(xsObjectPrototype);
	for (i = 0; i < 4; i++) {
		xsVar(0) = xsStringBuffer(NULL, lengths[i]);
		FskMemCopy(xsToString(xsVar(0)), xsToString(xsArg(0)) + offsets[i], lengths[i]);
		xsNewHostProperty(xsResult, ids[i], xsVar(0), xsDefault, xsDontScript);
	}
bail:
	return;
}
