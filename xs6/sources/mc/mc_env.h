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
#ifndef __MC_ENV_H__
#define __MC_ENV_H__

#include "mc_stdio.h"

#define MC_ENV_DEFAULT_PATH	"mc.env"
#define MC_ENV_MAGIC	".ver\0001\000"

typedef struct {
	char *buf;
	size_t size;
	char *fname;
	int encrypt;
	int autosave;
} mc_env_t;

extern int mc_env_init();
extern void mc_env_fin();
extern int mc_env_new(mc_env_t *env, const char *partname, int encrypt, int recovery);
extern void mc_env_free(mc_env_t *env);
extern int mc_env_load(mc_env_t *env);
extern int mc_env_store(mc_env_t *env);
extern const char *mc_env_get(mc_env_t *env, const char *name);
extern int mc_env_set(mc_env_t *env, const char *name, const char *val, int pos);
extern int mc_env_unset(mc_env_t *env, const char *name);
extern int mc_env_clear(mc_env_t *env);
extern void mc_env_autosave(mc_env_t *env, int flag);
extern void mc_env_dump(mc_env_t *env, int reload);
extern char *mc_env_getbuf(mc_env_t *env);

#define mc_env_get_default(name)	mc_env_get(NULL, name)
#define mc_env_set_default(name, value)	mc_env_set(NULL, name, value, -1)

#endif /* __MC_ENV_H__ */
