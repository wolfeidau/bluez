/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include "src/shared/util.h"
#include "src/shared/queue.h"

static void test_basic(void)
{
	struct queue *queue;
	unsigned int n, i;

	queue = queue_new();
	g_assert(queue != NULL);

	for (n = 0; n < 1024; n++) {
		for (i = 1; i < n + 2; i++)
			queue_push_tail(queue, UINT_TO_PTR(i));

		g_assert(queue_length(queue) == n + 1);

		for (i = 1; i < n + 2; i++) {
			void *ptr;

			ptr = queue_pop_head(queue);
			g_assert(ptr != NULL);
			g_assert(i == PTR_TO_UINT(ptr));
		}

		g_assert(queue_isempty(queue) == true);
	}

	queue_destroy(queue, NULL);
}

static void foreach_destroy(void *data, void *user_data)
{
	struct queue *queue = user_data;

	queue_destroy(queue, NULL);
}

static void test_foreach_destroy(void)
{
	struct queue *queue;

	queue = queue_new();
	g_assert(queue != NULL);

	queue_push_tail(queue, UINT_TO_PTR(1));
	queue_push_tail(queue, UINT_TO_PTR(2));

	queue_foreach(queue, foreach_destroy, queue);
}

static void foreach_remove(void *data, void *user_data)
{
	struct queue *queue = user_data;

	g_assert(queue_remove(queue, data));
}

static void test_foreach_remove(void)
{
	struct queue *queue;

	queue = queue_new();
	g_assert(queue != NULL);

	queue_push_tail(queue, UINT_TO_PTR(1));
	queue_push_tail(queue, UINT_TO_PTR(2));

	queue_foreach(queue, foreach_remove, queue);
	queue_destroy(queue, NULL);
}

static void foreach_remove_all(void *data, void *user_data)
{
	struct queue *queue = user_data;

	queue_remove_all(queue, NULL, NULL, NULL);
}

static void test_foreach_remove_all(void)
{
	struct queue *queue;

	queue = queue_new();
	g_assert(queue != NULL);

	queue_push_tail(queue, UINT_TO_PTR(1));
	queue_push_tail(queue, UINT_TO_PTR(2));

	queue_foreach(queue, foreach_remove_all, queue);
	queue_destroy(queue, NULL);
}

static void foreach_remove_backward(void *data, void *user_data)
{
	struct queue *queue = user_data;

	queue_remove(queue, UINT_TO_PTR(2));
	queue_remove(queue, UINT_TO_PTR(1));
}

static void test_foreach_remove_backward(void)
{
	struct queue *queue;

	queue = queue_new();
	g_assert(queue != NULL);

	queue_push_tail(queue, UINT_TO_PTR(1));
	queue_push_tail(queue, UINT_TO_PTR(2));

	queue_foreach(queue, foreach_remove_backward, queue);
	queue_destroy(queue, NULL);
}

static struct queue *static_queue;

static void destroy_remove(void *user_data)
{
	queue_remove(static_queue, user_data);
}

static void test_destroy_remove(void)
{
	static_queue = queue_new();

	g_assert(static_queue != NULL);

	queue_push_tail(static_queue, UINT_TO_PTR(1));
	queue_push_tail(static_queue, UINT_TO_PTR(2));

	queue_destroy(static_queue, destroy_remove);
}

static void test_push_after(void)
{
	struct queue *queue;
	unsigned int len, i;

	queue = queue_new();
	g_assert(queue != NULL);

	/*
	 * Pre-populate queue. Initial elements are:
	 *   [ NULL, 2, 5 ]
	 */
	g_assert(queue_push_tail(queue, NULL));
	g_assert(queue_push_tail(queue, UINT_TO_PTR(2)));
	g_assert(queue_push_tail(queue, UINT_TO_PTR(5)));
	g_assert(queue_length(queue) == 3);

	/* Invalid insertion */
	g_assert(!queue_push_after(queue, UINT_TO_PTR(6), UINT_TO_PTR(1)));

	/* Valid insertions */
	g_assert(queue_push_after(queue, NULL, UINT_TO_PTR(1)));
	g_assert(queue_push_after(queue, UINT_TO_PTR(2), UINT_TO_PTR(3)));
	g_assert(queue_push_after(queue, UINT_TO_PTR(3), UINT_TO_PTR(4)));
	g_assert(queue_push_after(queue, UINT_TO_PTR(5), UINT_TO_PTR(6)));

	g_assert(queue_peek_head(queue) == NULL);
	g_assert(queue_peek_tail(queue) == UINT_TO_PTR(6));

	/*
	 * Queue should contain 7 elements:
	 *   [ NULL, 1, 2, 3, 4, 5, 6 ]
	 */
	len = queue_length(queue);
	g_assert(len == 7);

	for (i = 0; i < 7; i++)
		g_assert(queue_pop_head(queue) == UINT_TO_PTR(i));

	/* Test with identical elements */
	g_assert(queue_push_head(queue, UINT_TO_PTR(1)));
	g_assert(queue_push_head(queue, UINT_TO_PTR(1)));
	g_assert(queue_push_head(queue, UINT_TO_PTR(1)));
	g_assert(queue_push_after(queue, UINT_TO_PTR(1), UINT_TO_PTR(0)));

	g_assert(queue_pop_head(queue) == UINT_TO_PTR(1));
	g_assert(queue_pop_head(queue) == UINT_TO_PTR(0));
	g_assert(queue_pop_head(queue) == UINT_TO_PTR(1));
	g_assert(queue_pop_head(queue) == UINT_TO_PTR(1));

	queue_destroy(queue, NULL);
}

static bool match_int(const void *a, const void *b)
{
	int i = PTR_TO_INT(a);
	int j = PTR_TO_INT(b);

	return i == j;
}

static bool match_ptr(const void *a, const void *b)
{
	return a == b;
}

static void test_remove_all(void)
{
	struct queue *queue;

	queue = queue_new();
	g_assert(queue != NULL);

	g_assert(queue_push_tail(queue, INT_TO_PTR(10)));

	g_assert(queue_remove_all(queue, match_int, INT_TO_PTR(10), NULL) == 1);
	g_assert(queue_isempty(queue));

	g_assert(queue_push_tail(queue, NULL));
	g_assert(queue_remove_all(queue, match_ptr, NULL, NULL) == 1);
	g_assert(queue_isempty(queue));

	g_assert(queue_push_tail(queue, UINT_TO_PTR(0)));
	g_assert(queue_remove_all(queue, match_int, UINT_TO_PTR(0), NULL) == 1);
	g_assert(queue_isempty(queue));

	queue_destroy(queue, NULL);
}

int main(int argc, char *argv[])
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/queue/basic", test_basic);
	g_test_add_func("/queue/foreach_destroy", test_foreach_destroy);
	g_test_add_func("/queue/foreach_remove", test_foreach_remove);
	g_test_add_func("/queue/foreach_remove_all", test_foreach_remove_all);
	g_test_add_func("/queue/foreach_remove_backward",
						test_foreach_remove_backward);
	g_test_add_func("/queue/destroy_remove", test_destroy_remove);
	g_test_add_func("/queue/push_after", test_push_after);
	g_test_add_func("/queue/remove_all", test_remove_all);

	return g_test_run();
}
