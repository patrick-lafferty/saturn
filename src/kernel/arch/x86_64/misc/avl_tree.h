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
			rebalance(node);
		}
    }

	int getNumberOfNodes() const {
		return nodeCount;
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

	void rebalance(Node* start) {

	}

	Node* root {nullptr};
	Allocator allocator;
	int nodeCount {0};
};
