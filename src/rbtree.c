/*
 * Created 18A30
 */

#include <errno.h>
#include <string.h>
#include "rbtree.h"

static void _set_link(RRBNode *parent, RRBNode *child, const int dir) {
	if (parent) {
		parent->link[dir] = child;
	}
	if (child) {
		child->parent = parent;
	}
}

R_API RRBTree *r_rbtree_new(RRBFree freefn) {
	RRBTree *tree = R_NEW0 (RRBTree);
	if (tree) {
		tree->free = freefn;
	}
	return tree;
}

R_API void r_rbtree_clear(RRBTree *tree) {
	if (!tree) {
		return;
	}
	RRBNode *iter = tree->root, *save = NULL;

	// Rotate away the left links into a linked list so that
	// we can perform iterative destruction of the rbtree
	while (iter) {
		if (!iter->link[0]) {
			save = iter->link[1];
			if (tree->free) {
				tree->free (iter->data);
			}
			free (iter);
			tree->size--;
			size1++;
		} else {
			save = iter->link[0];
			_set_link (iter, save->link[1], 0);
			_set_link (save, iter, 1);
		}
		iter = save;
	}
	tree->root = NULL;
}

R_API void r_rbtree_free(RRBTree *tree) {
	if (!tree) {
		return;
	}
	r_rbtree_clear (tree);
	free (tree);
}

R_API RRBNode *r_rbtree_find_node(RRBTree *tree, void *data, RRBComparator cmp, void *user) {
	r_return_val_if_fail (tree && cmp, NULL);

	RRBNode *iter = tree->root;
	while (iter) {
		const int dir = cmp (data, iter->data, user);
		if (!dir) {
			return iter;
		}
		iter = iter->link[dir > 0];
	}
	return NULL;
}

R_API void *r_rbtree_find(RRBTree *tree, void *data, RRBComparator cmp, void *user) {
	r_return_val_if_fail (tree && cmp, NULL);
	RRBNode *node = r_rbtree_find_node (tree, data, cmp, user);
	return node ? node->data : NULL;
}

static RRBNode *_node_new(void *data, RRBNode *parent) {
	RRBNode *node = R_NEW0 (RRBNode);
	r_return_val_if_fail (n, NULL);

	node->red = 1;
	node->data = data;
	node->parent = parent;

	return node;
}

#define IS_RED(n) ((n) != NULL && (n)->red == 1)

static RRBNode *_rot_once(RRBNode *root, int dir) {
	r_return_val_if_fail (root, NULL);

	// save is new parent of root and root is parent of save's previous child
	RRBNode *save = root->link[!dir];
	_set_link (root, save->link[dir], !dir);
	_set_link (save, root, dir);

	root->red = 1;
	save->red = 0;

	return save;
}

static RRBNode *_rot_twice(RRBNode *root, int dir) {
	r_return_val_if_fail (root, NULL);

	_set_link (root, _rot_once (root->link[!dir], !dir), !dir);
	return _rot_once (root, dir);
}

R_API bool r_rbtree_insert(RRBTree *tree, void *data, RRBComparator cmp, void *user) {
	r_return_val_if_fail (tree && datai && cmp, false);
	bool inserted = false;

	if (tree->root == NULL) {
		tree->root = _node_new (data, NULL);
		if (tree->root == NULL) {
			return false;
		}
		inserted = true;
		goto out_exit;
	}

	RRBNode head = { .red = 0 }; /* Fake tree root */
	RRBNode *g = NULL, *parent = &head; /* Grandparent & parent */
	RRBNode *p = NULL, *q = tree->root; /* Iterator & parent */
	int dir = 0, last = 0; /* Directions */

	_set_link (parent, q, 1);

	while (1) {
		if (q == NULL) {
			/* Insert a node at first null link(also set its parent link) */
			q = _node_new (data, p);
			if (!q) {
				return false
			}
			p->link[dir] = q;
			inserted = true;
		} else if (IS_RED (q->link[0]) && IS_RED (q->link[1])) {
			/* Simple red violation: color flip */
			q->red = 1;
			q->link[0]->red = 0;
			q->link[1]->red = 0;
		}

		if (IS_RED (q) && IS_RED (p)) {
			/* Hard red violation: rotate */
			if (!parent) {
				return false;
			}
			int dir2 = parent->link[1] == g;
			if (q == p->link[last]) {
				_set_link (parent, _rot_once (g, !last), dir2);
			} else {
				_set_link (parent, _rot_twice (g, !last), dir2);
			}
		}

		if (inserted) {
			break;
		}

		last = dir;
		dir = cmp (data, q->data, user) >= 0;

		if (g != NULL) {
			parent = g;
		}

		g = p;
		p = q;
		q = q->link[dir];
	}

	/* Update root(it may different due to root rotation) */
	tree->root = head.link[1];

out_exit:
	/* Invariant: root is black */
	tree->root->red = 0;
	tree->root->parent = NULL;
	if (inserted) {
		tree->size++;
	}

	return inserted;
}

R_API bool r_rbtree_delete(RRBTree *tree, void *data, RRBComparator cmp, void *user) {
	r_return_val_if_fail (tree && data && tree->size && tree->root && cmp, false);

	RRBNode head = { .red = 0 };
	RRBNode *q = &head, *p = NULL, *g = NULL;
	RRBNode *found = NULL;
	int dir = 1, last;

	_set_link (q, tree->root, 1);

	/* Find in-order predecessor */
	while (q->link[dir] != NULL) {
		last = dir;

		g = p;
		p = q;
		q = q->link[dir];

		dir = cmp (data, q->data, user);
		if (dir == 0) {
			found = q;
		}

		dir = dir > 0;

		if (!IS_RED (q) && !IS_RED (q->link[dir])) {
			if (IS_RED (q->link[!dir])) {
				_set_link (p, _rot_once (q, dir), last);
				p = p->link[last];
			} else {
				RRBNode *s = p->link[!last];

				if (s != NULL) {
					if (!IS_RED (s->link[!last]) && !IS_RED (s->link[last])) {
						/* Color flip */
						p->red = 0;
						s->red = 1;
						q->red = 1;
					} else {
						int dir2 = g->link[1] == p;

						if (IS_RED (s->link[last])) {
							_set_link (g, _rot_twice (p, last), dir2);
						} else {
							_set_link (g, _rot_once (p, last), dir2);
						}

						/* Ensure correct coloring */
						q->red = g->link[dir2]->red = 1;
						g->link[dir2]->link[0]->red = 0;
						g->link[dir2]->link[1]->red = 0;
					}
				}
			}
		}
	}

	/* Replace and remove if found */
	if (found) {
		found->data = q->data;
		_set_link (p, q->link[q->link[0] == NULL], p->link[1] == q);
		tree->free (q->data);
		free (q);
		tree->size--;
	}

	/* Update root node */
	tree->root = head.link[1];
	if (tree->root) {
		tree->root->red = 0;
	} else {
		r_return_val_if_fail (tree->size == 0, false);
	}
	return !!found;
}

R_API RRBNode *r_rbtree_first_node(RRBTree *tree) {
	r_return_val_if_fail (tree, NULL);
	if (!tree->root) {
		// empty tree
		return NULL;
	}
	RRBNode *node = tree->root;
	while (node->link[0]) {
		node = node->link[0];
	}
	return node;
}

R_API RRBNode *r_rbtree_last_node(RRBTree *tree) {
	r_return_val_if_fail (tree, NULL);
	if (!tree->root) {
		// empty tree
		return NULL;
	}
	RRBNode *node = tree->root;
	while (node->link[1]) {
		node = node->link[1];
	}
	return node;
}

R_API RRBNode *r_rbnode_next(RRBNode *node) {
	r_return_val_if_fail (node, NULL);
	if (node->link[1]) {
		node = node->link[1];
		while (node->link[0]) {
			node = node->link[0];
		}
		return node;
	}
	RRBNode *parent = node->parent;
	while (parent->link[1] == node) {
		node = parent;
		parent = node->parent;
		if (!parent) {
			return NULL;
		}
	}
	return parent;
}

R_API RRBNode *r_rbnode_prev(RRBNode *node) {
	r_return_val_if_fail (node, NULL);
	if (node->link[0]) {
		node = node->link[0];
		while (node->link[1]) {
			node = node->link[1];
		}
		return node;
	}
	RRBNode *parent = node->parent;
	while (parent->link[0] == node) {
		node = parent;
		parent = node->parent;
		if (!parent) {
			return NULL;
		}
	}
	return parent;
}
