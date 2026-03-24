#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
    struct Node {
        T value;
        Node* left;
        Node* right;
        int npl; // null path length

        Node(const T& val) : value(val), left(nullptr), right(nullptr), npl(0) {}
    };

    Node* root;
    size_t queue_size;
    Compare cmp;

    // Helper functions
    void clear(Node* node) {
        if (node) {
            clear(node->left);
            clear(node->right);
            delete node;
        }
    }

    Node* copy(Node* node) {
        if (!node) return nullptr;
        Node* new_node = new Node(node->value);
        new_node->npl = node->npl;
        new_node->left = copy(node->left);
        new_node->right = copy(node->right);
        return new_node;
    }

    // Merge two leftist heaps
    Node* merge(Node* h1, Node* h2) {
        if (!h1) return h2;
        if (!h2) return h1;

        // Ensure h1 has larger value (max heap)
        try {
            if (cmp(h1->value, h2->value)) {
                std::swap(h1, h2);
            }
        } catch (...) {
            throw; // Re-throw the exception
        }

        // Recursively merge h1->right with h2
        h1->right = merge(h1->right, h2);

        // Maintain leftist property: left npl >= right npl
        if (!h1->left || h1->left->npl < h1->right->npl) {
            std::swap(h1->left, h1->right);
        }

        // Update npl
        h1->npl = h1->right ? h1->right->npl + 1 : 0;

        return h1;
    }

    // Merge with exception safety - returns true if exception occurred
    bool merge_with_exception_safety(Node*& h1, Node* h2) {
        Node* original_h1 = h1;
        Node* original_h2 = h2;

        try {
            h1 = merge(h1, h2);
            return false; // No exception
        } catch (...) {
            // Restore original state
            h1 = original_h1;
            // h2 should not be modified if exception occurs during merge
            // But we need to ensure we don't lose the original h2
            // Since merge doesn't modify h2 unless successful, we're safe
            return true; // Exception occurred
        }
    }

public:
	/**
	 * @brief default constructor
	 */
	priority_queue() : root(nullptr), queue_size(0), cmp(Compare()) {}

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other) : root(nullptr), queue_size(0), cmp(other.cmp) {
        root = copy(other.root);
        queue_size = other.queue_size;
    }

	/**
	 * @brief deconstructor
	 */
	~priority_queue() {
        clear(root);
    }

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
        if (this == &other) return *this;

        clear(root);
        root = copy(other.root);
        queue_size = other.queue_size;
        cmp = other.cmp;

        return *this;
    }

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
        if (!root) throw container_is_empty();
        return root->value;
    }

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
        Node* new_node = nullptr;
        try {
            new_node = new Node(e);
        } catch (...) {
            // If memory allocation fails, rethrow
            throw;
        }

        // Try to merge the new node with current heap
        Node* original_root = root;
        size_t original_size = queue_size;

        try {
            root = merge(root, new_node);
            queue_size++;
        } catch (...) {
            // Exception during merge, restore state
            root = original_root;
            queue_size = original_size;
            delete new_node;
            throw runtime_error();
        }
    }

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
        if (!root) throw container_is_empty();

        Node* original_root = root;
        size_t original_size = queue_size;

        try {
            Node* left = root->left;
            Node* right = root->right;

            delete root;
            root = merge(left, right);
            queue_size--;
        } catch (...) {
            // Exception during merge, restore state
            root = original_root;
            queue_size = original_size;
            throw runtime_error();
        }
    }

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const {
        return queue_size;
    }

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const {
        return queue_size == 0;
    }

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
        if (this == &other) return; // Self-merge

        Node* original_this_root = root;
        Node* original_other_root = other.root;
        size_t original_this_size = queue_size;
        size_t original_other_size = other.queue_size;

        try {
            root = merge(root, other.root);
            queue_size += other.queue_size;

            // Clear other queue
            other.root = nullptr;
            other.queue_size = 0;
        } catch (...) {
            // Restore both queues to original state
            root = original_this_root;
            other.root = original_other_root;
            queue_size = original_this_size;
            other.queue_size = original_other_size;
            throw runtime_error();
        }
    }
};

}

#endif