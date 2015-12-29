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
#include "mc_ipc.h"
#if mxMC
#include <wm_os.h>
#else
#include <pthread.h>
#include <semaphore.h>
#include <sys/msg.h>
#endif

typedef struct {
#if mxMC
	os_thread_t thread;
#else
	pthread_t thread;
#endif
	mc_thread_proc_f func;
	void *arg;
} mc_thread_t;

#if mxMC
static void
#else
static void *
#endif
mc_thread_main(void *data)
{
	mc_thread_t *th = data;

	(*th->func)(th->arg);
	mc_free(th);
#if mxMC
	os_thread_delete(NULL);
#else
	return NULL;
#endif
}

int
mc_thread_create(mc_thread_proc_f thread_main, void *arg)
{
	mc_thread_t *th;
#if mxMC
	char name[13];
	static int sequence = 0;
	static os_thread_stack_define(stack, 2048);
#endif

	if ((th = mc_malloc(sizeof(mc_thread_t))) == NULL) {
		errno = ENOMEM;
		return -1;
	}
	th->func = thread_main;
	th->arg = arg;
#if mxMC
	snprintf(name, sizeof(name), "th%d", sequence++);
	if (os_thread_create(&th->thread, name, mc_thread_main, th, &stack, OS_PRIO_3) != WM_SUCCESS) {
		errno = EBUSY;
		mc_free(th);
		th = NULL;
	}
#else
	if (pthread_create(&th->thread, NULL, mc_thread_main, th) != 0) {
		mc_free(th);
		th = NULL;
	}
#endif
	return th == NULL ? -1 : 0;
}

int
mc_semaphore_create(mc_semaphore_t *sem)
{
	char name[13];
	static int sequence = 0;

	snprintf(name, sizeof(name), "sem%d", sequence++);
#if mxMC
	if (os_semaphore_create_counting(sem, name, 10, 0) == WM_SUCCESS)
		return 0;
	else
		return -1;
#else
	if ((*sem = sem_open(name, O_CREAT, 0777, 0)) != SEM_FAILED)
		return 0;
	else
		return -1;
#endif
}

void
mc_semaphore_wait(mc_semaphore_t *sem)
{
#if mxMC
	os_semaphore_get(sem, OS_WAIT_FOREVER);
#else
	sem_wait(*sem);
#endif
}

void
mc_semaphore_post(mc_semaphore_t *sem)
{
#if mxMC
	os_semaphore_put(sem);
#else
	sem_post(*sem);
#endif
}

void
mc_semaphore_delete(mc_semaphore_t *sem)
{
#if mxMC
	os_semaphore_delete(sem);
#else
	sem_close(*sem);
#endif
}

int
mc_queue_create(mc_queue_t *queue, size_t size)
{
#if mxMC
	char name[13];
	static int sequence = 0;
	os_queue_pool_define(pool, 20 * size);

	snprintf(name, sizeof(name), "q%d", sequence++);
	if (os_queue_create(queue, name, size, &pool) == WM_SUCCESS)
		return 0;
	else
		return -1;
#else
	int d;

	if ((d = msgget(IPC_PRIVATE, IPC_CREAT)) >= 0) {
		*queue = (mc_queue_t)d;
		return 0;
	}
	else
		return -1;
#endif
}

int
mc_queue_send(mc_queue_t *queue, const void *data, size_t sz)
{
#if mxMC
	if (os_queue_send(queue, data, OS_NO_WAIT) == WM_SUCCESS)
		return sz;
	else {
		wmprintf("os_queue_send failed\r\n");
		return -1;
	}
#else
	return msgsnd((int)*queue, data, sz, 0);
#endif
}

int
mc_queue_recv(mc_queue_t *queue, void *data, size_t sz)
{
#if mxMC
	if (os_queue_recv(queue, data, OS_WAIT_FOREVER) == WM_SUCCESS)
		return sz;
	else
		return -1;
#else
	return msgrcv((int)*queue, data, sz, 0, 0);
#endif
}

void
mc_queue_delete(mc_queue_t *queue)
{
#if mxMC
	os_queue_delete(queue);
#else
	msgctl((int)*queue, IPC_RMID, NULL);
#endif
}
