/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <optional>

template<class T, class Allocator>
class AVLTree {

	struct Node {
		T value;
		Node* parent {nullptr};
		Node* leftChild {nullptr};
		Node* rightChild {nullptr};
		int height {0};
	};

public:

	AVLTree(Allocator allocator) 
		: allocator {allocator} {}

    void insert(T item) {
		auto node = allocator.template allocate<Node>();
		node->value = item;
		nodeCount++;

		if (root == nullptr) {
			root = node;
		}
		else {
			auto iterator = root;

			while (iterator != nullptr) {

				if (iterator->value >= item) {

					if (iterator->leftChild != nullptr) {
						iterator = iterator->leftChild;
					}
					else {
						iterator->leftChild = node;
						break;
					}
				}
				else {

					if (iterator->rightChild != nullptr) {
						iterator = iterator->rightChild;
					}
					else {
						iterator->rightChild = node;
						break;
					}
				}
			}

			node->parent = iterator;
			iterator->height = height(iterator->leftChild, iterator->rightChild);
			recalculateHeights(iterator->parent);
			rebalance(iterator);
		}
    }

	int getNumberOfNodes() const {
		return nodeCount;
	}

	int getHeight() const {
		if (root != nullptr) {
			return root->height;
		}
		else {
			return -1;
		}
	}

	template<class Container>
	void traverseInOrder(Container& result) {

		if (root == nullptr) {
			return;
		}

		int index = 0;

		auto f = [&](Node* node) {
			result[index++] = node->value;
		};

		traverse(root, f);
	}

	T findMinimum() {
		auto iterator = root;

		while (iterator != nullptr) {
			if (iterator->leftChild == nullptr) {
				return iterator->value;
			}
			else {
				iterator = iterator->leftChild;
			}
		}
	}

	std::optional<T> findAtLeast(T item) {
		auto iterator = root;
		Node* successor = nullptr;

		while (iterator != nullptr) {
			if (item >= iterator->value) {
				if (item == iterator->value) {
					successor = iterator;
				}

				iterator = iterator->rightChild;
			}
			else {
				successor = iterator;

				if (iterator->leftChild != nullptr) {
					iterator = iterator->leftChild;
				}
				else {
					break;
				}
			}
		}

		if (successor != nullptr) {
			return {successor->value};
		}
		else {
			return {};
		}

	}

	void remove(T item) {

	}	

private:

	template<class F>
	void traverse(Node* node, F&& f) {
		if (node->leftChild != nullptr) {
			traverse(node->leftChild, f);
		}

		f(node);

		if (node->rightChild != nullptr) {
			traverse(node->rightChild, f);
		}
	}

	int height(Node* left, Node* right) {
		auto leftHeight = (left != nullptr ? left->height : -1);
		auto rightHeight = (right != nullptr ? right->height : -1);

		return 1 + (leftHeight > rightHeight ? leftHeight : rightHeight);
	}

	int height(Node* node) {
		if (node != nullptr) {
			return height(node->leftChild, node->rightChild);
		}
		else {
			return -1;
		}
	}

	void leftRotate(Node* node) {
		auto right = node->rightChild;
		node->rightChild = right->leftChild;
		
		if (node->parent->leftChild == node) {
			node->parent->leftChild = right;
		}
		else {
			node->parent->rightChild = right;
		}

		right->parent = node->parent;
		node->parent = right;

		node->height = height(node->leftChild, node->rightChild);
		right->height = height(right->leftChild, right->rightChild);

		recalculateHeights(right->parent);
	}

	void recalculateHeights(Node* node) {

		if (node == nullptr) {
			return;
		}

		node->height = height(node->leftChild, node->rightChild);

		if (node->parent != nullptr) {
			recalculateHeights(node->parent);
		}
	}

	void rightRotate(Node* node) {
		auto left = node->leftChild;
		node->leftChild = left->rightChild;

		if (node->parent->leftChild == node) {
			node->parent->leftChild = left;
		}
		else {
			node->parent->rightChild = left;
		}

		left->parent = node->parent;
		node->parent = left;

		node->height = height(node->leftChild, node->rightChild);
		left->height = height(left->leftChild, left->rightChild);

		recalculateHeights(left->parent);
	}

	void rebalance(Node* start) {
		auto difference = height(start->leftChild) - height(start->rightChild);
		bool rightHeavy = difference <= 0;

		if (difference <= 0) {
			difference = -difference;
		}
		
		if (difference <= 1) {
			return;
		}

		if (rightHeavy) {
			difference = height(start->rightChild->leftChild) - height(start->rightChild->rightChild);
			bool rightRightHeavy = difference <= 0;

			if (rightRightHeavy) {
				rightRotate(start);
			}
			else {
				rightRotate(start->rightChild);
				leftRotate(start);
			}
		}
		else {
			difference = height(start->leftChild->leftChild) - height(start->leftChild->rightChild);
			bool leftRightHeavy = difference <= 0;

			if (leftRightHeavy) {
				leftRotate(start->leftChild);
				rightRotate(start);
			}
			else {
				leftRotate(start);
			}
		}

		if (start->parent != nullptr) {
			rebalance(start->parent);
		}
	}

	Node* root {nullptr};
	Allocator allocator;
	int nodeCount {0};
};
