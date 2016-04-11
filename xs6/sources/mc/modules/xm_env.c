/*
 *     Copyright (C) 2010-2016 Marvell International Ltd.
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
#include "mc_stdio.h"
#include "mc_file.h"
#include "mc_env.h"
#include "mc_module.h"

#if !XS_ARCHIVE
#include "xm_env.xs.c"
MC_MOD_DECL(env);
#endif

void
xs_env_get(xsMachine *the)
{
	mc_env_t *env;
	const char *name, *val;
	int typeOf;

	if (xsToInteger(xsArgc) < 1)
		goto bail;

	env = xsGetHostData(xsThis);
	typeOf = xsTypeOf(xsArg(0));
	if ((xsIntegerType == typeOf) || (xsNumberType == typeOf)) {
		char *buf;
		int index = xsToInteger(xsArg(0));
		if ((buf = mc_env_getbuf(env)) == NULL)
			mc_xs_throw(the, "mc_env: no buf");

		while (*buf != '\0') {
			if (0 == index--) {
				xsSetString(xsResult, buf);
				return;
			}

			buf += strlen(buf) + 1;
			buf += strlen(buf) + 1;
		}
	}
	else {
		name = xsToString(xsArg(0));
		if ((val = mc_env_get(env, name)) != NULL) {
			xsSetString(xsResult, (xsStringValue)val);
			return;
		}
	}

bail:
	xsSetNull(xsResult);
}

void
xs_env_set(xsMachine *the)
{
	mc_env_t *env;
	char *val, *name;
	int argc = xsToInteger(xsArgc);

	if (argc < 1)
		return;
	env = xsGetHostData(xsThis);
	if (1 == argc || xsTypeOf(xsArg(1)) == xsUndefinedType) {
		int result;

		name = xsToString(xsArg(0));
		result = mc_env_unset(env, name);
		xsSetBoolean(xsResult, result == 0);
	}
	else {
		if ((val = mc_strdup(xsToString(xsArg(1)))) == NULL)
			return;
		name = xsToString(xsArg(0));
		mc_env_set(env, name, val);
		mc_free(val);
	}
}

void
xs_env_save(xsMachine *the)
{
	mc_env_t *env = xsGetHostData(xsThis);
	int result;

	result = mc_env_store(env);
	xsSetBoolean(xsResult, result == 0);
}

void
xs_env_constructor(xsMachine *the)
{
	int ac = xsToInteger(xsArgc);
	const char *partname = ac > 0 ? xsToString(xsArg(0)) : MC_ENV_DEFAULT_PATH;
	int autosave = ac > 1 && xsTest(xsArg(1));
	int encrypt = ac > 2 && xsTest(xsArg(2));
	mc_env_t *env;

	if (mc_env_init())
		mc_xs_throw(the, "mc_env: init fail");

	if (strcmp(partname, MC_ENV_DEFAULT_PATH) == 0)
		env = NULL;
	else {
		if ((env = mc_malloc(sizeof(mc_env_t))) == NULL)
			mc_xs_throw(the, "mc_env: no mem");
		if (mc_env_new(env, partname, encrypt) != 0)
			mc_xs_throw(the, "mc_env: new fail");
	}
	mc_env_autosave(env, autosave);
	xsSetHostData(xsThis, env);
}

void
xs_env_close(xsMachine *the)
{
	mc_env_t *env = xsGetHostData(xsThis);
	int argc = xsToInteger(xsArgc);
	int result = 0;

	if (argc && xsTest(xsArg(0)))
		result = mc_env_clear(env);

	mc_env_free(env);
	xsSetBoolean(xsResult, result == 0);
}

void
xs_env_destructor(void *data)
{
	if (data != NULL) {
		mc_env_t *env = data;
		mc_env_free(env);
		mc_free(env);
	}
}

void
xs_env_next(xsMachine *the)
{
	char *p = xsGetHostData(xsThis);
	char *pattern = xsToInteger(xsArgc) > 0 ? xsToString(xsArg(0)) : NULL;

	while (*p != '\0') {
		char *name = p;
		p += strlen(p) + 1;
		p += strlen(p) + 1;
		if (pattern == NULL || *pattern == '\0' || strncmp(name, pattern, strlen(pattern)) == 0) {
			xsSetString(xsResult, name);
			xsSetHostData(xsThis, p);
			return;
		}
	}
	xsSetNull(xsResult);
}

void
xs_env_Iterator(xsMachine *the)
{
	mc_env_t *env = xsGetHostData(xsArg(0));
	char *buf;

	if ((buf = mc_env_getbuf(env)) == NULL)
		mc_xs_throw(the, "mc_env: no buf");
	xsSetHostData(xsThis, buf);
}

void
xs_env_iter_destructor(void *data)
{
	/* no-op */
}

void
xs_env_dump(xsMachine *the)
{
	mc_env_t *env = xsGetHostData(xsThis);

	mc_env_dump(env, xsToInteger(xsArgc) > 0 && xsToBoolean(xsArg(0)));
}
