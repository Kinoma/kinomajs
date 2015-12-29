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
typedef void (*mc_thread_proc_f)(void *data);
extern int mc_thread_create(mc_thread_proc_f thread_main, void *arg);

typedef void *mc_semaphore_t;
extern int mc_semaphore_create(mc_semaphore_t *sem);
extern void mc_semaphore_wait(mc_semaphore_t *sem);
extern void mc_semaphore_post(mc_semaphore_t *sem);
extern void mc_semaphore_delete(mc_semaphore_t *sem);

typedef void *mc_queue_t;
extern int mc_queue_create(mc_queue_t *queue, size_t size);
extern int mc_queue_send(mc_queue_t *queue, const void *data, size_t sz);
extern int mc_queue_recv(mc_queue_t *queue, void *data, size_t sz);
extern void mc_queue_delete(mc_queue_t *queue);
