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
#include "mc_stdio.h"
#include "mc_file.h"
#include "mc_misc.h"
#include "mc_env.h"

static mc_env_t mc_default_env;
#define MC_ENV_SEED	"env"

void
mc_env_free(mc_env_t *env)
{
	if (env == NULL)
		return;
	if (env->buf != NULL) {
		mc_free(env->buf);
		env->buf = NULL;
	}
	if (env->fname != NULL) {
		mc_free(env->fname);
		env->fname = NULL;
	}
	env->size = 0;
}

int
mc_env_load(mc_env_t *env)
{
	MC_FILE *fp = NULL;
	int status = -1;

	if (env->fname == NULL) {
		errno = EINVAL;
		return -1;
	}
	if (env->buf != NULL) {
		mc_free(env->buf);
		env->buf = NULL;
	}
	env->size = 0;
	if ((fp = mc_fopen(env->fname, "r")) == NULL) {
		mc_log_error("mc_env_load: mc_fopen %s failed. %d\n", env->fname, errno);
		goto bail;
	}
	env->size = mc_fsize(fp);
	if (env->size == 0)
		goto bail;	/* unset errno */
	if ((env->buf = mc_malloc(env->size)) == NULL)
		goto bail;
	mc_fseek(fp, 0, SEEK_SET);
	if (mc_fread(env->buf, 1, env->size, fp) != env->size)
		goto bail;
	if (env->encrypt)
		mc_rng_process(env->buf, env->size, (uint8_t *)MC_ENV_SEED, sizeof(MC_ENV_SEED));
	status = 0;
bail:
	if (fp != NULL)
		mc_fclose(fp);
	if (status != 0) {
		if (env->buf != NULL) {
			mc_free(env->buf);
			env->buf = NULL;
		}
		env->size = 0;
	}
	return status;
}

int
mc_env_store(mc_env_t *env)
{
	MC_FILE *fp;

	if (env == NULL)
		env = &mc_default_env;
	if (env->buf == NULL) {
		errno = ENOENT;
		return -1;
	}
	if ((fp = mc_fopen(env->fname, "w+")) == NULL) {
		mc_log_error("mc_env_store: %s open failed. %d\n", env->fname, errno);
		return -1;
	}
	/* encrypt data right on the buffer and decrypt it afterward to save the memory -- should be fast enough.. */
	if (env->encrypt)
		mc_rng_process(env->buf, env->size, (uint8_t *)MC_ENV_SEED, sizeof(MC_ENV_SEED));
	mc_fwrite(env->buf, env->size, 1, fp);
	if (env->encrypt)
		mc_rng_process(env->buf, env->size, (uint8_t *)MC_ENV_SEED, sizeof(MC_ENV_SEED));
	mc_fclose(fp);
	return 0;
}

int
mc_env_new(mc_env_t *env, const char *path, int encrypt)
{
	int err = 0;
	char buf[PATH_MAX];

	env->buf = NULL;
	env->size = 0;
	env->autosave = 0;
	env->encrypt = encrypt;
	if (path[0] != '/') {
		snprintf(buf, sizeof(buf), "%s/%s", mc_get_special_dir("preferencesDirectory"), path);
		path = buf;
	}
	if ((env->fname = mc_strdup(path)) == NULL) {
		err = -1;
		goto bail;
	}
	if (mc_env_load(env) != 0 || env->size < sizeof(MC_ENV_MAGIC) || memcmp(env->buf, MC_ENV_MAGIC, sizeof(MC_ENV_MAGIC) - 1) != 0) {
		mc_log_notice("mc_env: initializing %s\n", path);
		if (env->buf != NULL)
			mc_free(env->buf);
		env->size = sizeof(MC_ENV_MAGIC);
		if ((env->buf = mc_malloc(env->size)) == NULL) {
			err = -1;
			goto bail;
		}
		memcpy(env->buf, MC_ENV_MAGIC, sizeof(MC_ENV_MAGIC));	/* copy the magic number including the last \0 */
		err = mc_env_store(env);
	}
bail:
	if (err != 0) {
		mc_log_error("mc_env: failed to initialize: %d\n", errno);
		if (env->buf != NULL) {
			mc_free(env->buf);
			env->buf = NULL;
		}
		if (env->fname != NULL) {
			mc_free(env->fname);
			env->fname = NULL;
		}
		env->size = 0;
	}
	return err;
}

const char *
mc_env_get(mc_env_t *env, const char *name)
{
	const char *p;

	if (env == NULL)
		env = &mc_default_env;
	if (env->buf == NULL)
		return NULL;
	p = env->buf;
	while (*p != '\0') {
		const char *v = p + strlen(p) + 1;
		if (strcmp(p, name) == 0)
			return v;
		p = v + strlen(v) + 1;
#if MC_ENV_SANITY_CHECK
		if (p >= env->buf + env->size) {
			mc_log_error("mc_env: corrupted!\n");
			return NULL;
		}
#endif
	}
	return NULL;
}

int
mc_env_set(mc_env_t *env, const char *name, const char *val)
{
	char *p, *found = NULL;

	if (env == NULL)
		env = &mc_default_env;
	if (env->buf == NULL) {
		errno = ENOENT;
		return -1;
	}
	p = env->buf;
	while (*p != '\0') {
		char *v = p + strlen(p) + 1;
		if (strcmp(p, name) == 0) {
			if (val != NULL && strlen(val) == strlen(v)) {
				strcpy(v, val);
				goto bail;
			}
			found = p;
		}
		p = v + strlen(v) + 1;
	}
	env->size = p - env->buf + 1;	/* adjust the size in case the file size is larger than the actual size */
	if (found != NULL) {
		char *v = found + strlen(found) + 1, *next = v + strlen(v) + 1;
		memmove(found, next, p - next + 1);	/* overwrite the name and value with the rest including the terminator '\0' */
		env->size -= next - found;
	}
	if (val != NULL) {
		/* append the new name&value */
		int len = strlen(name) + strlen(val) + 2 /* each '\0' */;
		env->size += len;
		if ((env->buf = mc_realloc(env->buf, env->size)) == NULL) {
			mc_log_error("env: no more core\n");
			return -1;
		}
		p = env->buf + env->size - len - 1;
		strcpy(p, name);
		p += strlen(name) + 1;
		strcpy(p, val);
		p += strlen(val) + 1;
		*p = '\0';
	}
bail:
	if (env->autosave)
		return mc_env_store(env);
	return 0;
}

int
mc_env_unset(mc_env_t *env, const char *name)
{
	if (env == NULL)
		env = &mc_default_env;
	return mc_env_set(env, name, NULL);
}

int
mc_env_clear(mc_env_t *env)
{
	char *fname;
	int enc;

	if (env == NULL)
		env = &mc_default_env;
	mc_unlink(env->fname);
	if ((fname = mc_strdup(env->fname)) == NULL)
		return -1;
	enc = env->encrypt;
	mc_env_free(env);
	mc_env_new(env, fname, enc);
	mc_free(fname);
	return 0;
}

void
mc_env_autosave(mc_env_t *env, int flag)
{
	if (env == NULL)
		env = &mc_default_env;
	env->autosave = flag;
}

char *
mc_env_getbuf(mc_env_t *env)
{
	if (env == NULL)
		env = &mc_default_env;
	if (env->buf == NULL) {
		if (mc_env_load(env) != 0)
			mc_log_error("mc_env: failed to load!\n");
	}
	return env->buf;
}

/* debugging only */
void
mc_env_dump(mc_env_t *env, int reload)
{
	unsigned char *p;
	size_t i;
	int col;

	if (env == NULL)
		env = &mc_default_env;
	if (reload || env->buf == NULL) {
		if (mc_env_load(env) != 0) {
			mc_log_error("env: load failed\n");
			return;
		}
	}
	fprintf(stdout, "=== %s ===\n", env->fname);
	p = (unsigned char *)env->buf;
	for (i = 0, col = 0; i < env->size; i++) {
		int c = *p++;
		if (c >= 0x20 && c <= 0x7f) {
			fprintf(stdout, "%c", c);
			col++;
			if (c == '\\') {
				fprintf(stdout, "%c", c);
				col++;
			}
		}
		else if (c == 0) {
			fprintf(stdout, "\\0");
			col += 2;
		}
		else {
			fprintf(stdout, "\\x%02x", c);
			col += 4;
		}
		if (col >= 80) {
			fprintf(stdout, "\n");
			col = 0;
		}
	}
	fprintf(stdout, "\n");
}

int
mc_env_init()
{
	if (mc_default_env.buf != NULL)
		return 0;
	mc_file_init();
	return mc_env_new(&mc_default_env, MC_ENV_DEFAULT_PATH, 0);
}

void
mc_env_fin()
{
	mc_env_free(&mc_default_env);
}
