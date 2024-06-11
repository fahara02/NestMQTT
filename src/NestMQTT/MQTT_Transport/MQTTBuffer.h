#ifndef MQTT_BUFFER_H_
#define MQTT_BUFFER_H_

#include "Arduino.h"
#include "MQTTConstants.h"
#include <utility>

namespace MQTTTransport {

template <typename T> class Node {
  template <typename U> friend class Buffer;

public:
  template <typename... Args>
  explicit Node(Args &&...args)
      : data(std::forward<Args>(args)...), nextLink(nullptr) {}

  ~Node() = default;

  T getData() const { return data; }

private:
  T data;
  Node *nextLink = nullptr;
};

template <typename T> class Buffer {
public:
  class Iterator {
    friend class Buffer;

  public:
    Iterator() : currentNode(nullptr), prevNode(nullptr) {}

    T &operator*() const { return currentNode->data; }

    T *operator->() const { return &(currentNode->data); }

    Iterator &operator++() {
      if (currentNode) {
        prevNode = currentNode;
        currentNode = currentNode->nextLink;
      }
      return *this;
    }

    Iterator &operator--() {
      // This operation isn't typical for singly linked lists
      return *this;
    }

    bool operator==(const Iterator &other) const {
      return currentNode == other.currentNode;
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }

    explicit operator bool() const { return currentNode != nullptr; }

    void advance(size_t steps) {
      for (size_t i = 0; i < steps && currentNode; ++i) {
        prevNode = currentNode;
        currentNode = currentNode->nextLink;
      }
    }

    T *get() const {
      if (currentNode)
        return &(currentNode->data);
      return nullptr;
    }

    T *getPrev() const { return (prevNode) ? &(prevNode->data) : nullptr; }

  private:
    Node<T> *currentNode = nullptr;
    Node<T> *prevNode = nullptr;
  };

  Buffer()
      : _head(nullptr), _tail(nullptr), _current(nullptr), _prev(nullptr),
        _bufferState(*this) {}

  Buffer(const Buffer &other) : Buffer() {
    Node<T> *current = other._head;
    while (current) {
      pushBack(current->data);
      current = current->nextLink;
    }
  }

  Buffer(Buffer &&other) noexcept
      : _head(other._head), _tail(other._tail), _current(other._current),
        _prev(other._prev), _bufferState(other._bufferState) {
    other._head = other._tail = other._current = other._prev = nullptr;
  }

  Buffer &operator=(const Buffer &other) {
    if (this != &other) {
      Buffer temp(other);
      swap(temp);
    }
    return *this;
  }

  ~Buffer() { clear(); }

  void clear() {
    while (_head) {
      Node<T> *temp = _head;
      _head = _head->nextLink;
      delete temp;
    }
    _tail = _current = _prev = nullptr;
  }

  void swap(Buffer &other) noexcept {
    std::swap(_head, other._head);
    std::swap(_tail, other._tail);
    std::swap(_current, other._current);
    std::swap(_prev, other._prev);
  }

  struct BufferState {
    size_t currentSize = 0;
    size_t freeSize = 0;

    const Buffer<T> &buffer;

    BufferState(const Buffer<T> &buffer) : buffer(buffer) { update(); }

    void update() {
      currentSize = buffer.getBufferSize();
      freeSize = buffer.getFreeBufferSize();
    }

    bool isEmpty() const { return currentSize == 0; }

    bool isFull() const {
      return MQTTCore::TX_BUFFER_MAX_SIZE_BYTE > 0 &&
             currentSize >= MQTTCore::TX_BUFFER_MAX_SIZE_BYTE;
    }
  };

  String getStatus() const {
    String result = "Current Size: " + String(_bufferState.currentSize) + "   ";
    result += "Free Size: " + String(_bufferState.freeSize) + "   ";
    return result;
  }

  T *getHead() const { return (_head) ? &(_head->data) : nullptr; }

  T *getTail() const { return (_tail) ? &(_tail->data) : nullptr; }

  T *getCurrent() const { return (_current) ? &(_current->data) : nullptr; }

  T *getPrev() const { return (_prev) ? &(_prev->data) : nullptr; }

  template <class... Args> Iterator pushBack(Args &&...args) {
    Iterator it;
    Node<T> *newNode = new (std::nothrow) Node<T>(std::forward<Args>(args)...);
    if (newNode != nullptr) {
      newNode->nextLink = nullptr;

      if (!_head) {
        _head = _current = newNode;
      } else {
        _tail->nextLink = newNode;
      }
      _current = _head;
      _tail = newNode;
      it.currentNode = _current;
      it.prevNode = _prev;

      _bufferState.update();
    }
    return it;
  }

  template <class... Args> Iterator pushFront(Args &&...args) {
    Iterator it;
    Node<T> *newNode = new (std::nothrow) Node<T>(std::forward<Args>(args)...);
    if (newNode != nullptr) {
      newNode->nextLink = nullptr;

      if (!_head) {
        _head = _current = _tail = newNode;
      } else {
        newNode->nextLink = _head;
        _head = newNode;
      }
      _current = _head;
      _prev = nullptr;
      it.currentNode = _current;
      it.prevNode = _prev;

      _bufferState.update();
    }
    return it;
  }

  void _remove(Node<T> *prev, Node<T> *node) {
    if (!node)
      return;

    if (_head == _tail) {
      _head = _tail = nullptr;
    } else if (_head == node) {
      _head = node->nextLink;
    } else if (_tail == node) {
      if (prev) {
        prev->nextLink = nullptr;
        _tail = prev;
      } else {
        _head = nullptr;
        _tail = nullptr;
      }
    } else {
      prev->nextLink = node->nextLink;
    }

    if (_current == node) {
      _current = node->nextLink;
    }

    if (_prev == node) {
      _prev = prev;
    }

    delete node;

    _bufferState.update();
  }

  void remove(Iterator &it) {
    if (!it)
      return;

    Node<T> *currentNode = it.currentNode;
    Node<T> *prevNode = it.prevNode;

    if (_head == currentNode) {
      _head = currentNode->nextLink;
    } else if (_tail == currentNode) {
      if (prevNode) {
        prevNode->nextLink = nullptr;
        _tail = prevNode;
      } else {
        _head = nullptr;
        _tail = nullptr;
      }
    } else {
      ++it;
    }

    _remove(prevNode, currentNode);

    if (!_head) {
      _tail = nullptr;
    }

    if (_prev == currentNode) {
      _prev = prevNode;
    }

    it = end(); // Reset iterator after removal
  }

  void remove(const T &data) {
    Iterator it = find(data);
    remove(it);
  }

  void removeCurrent() { _remove(_prev, _current); }

  void resetCurrent() { _current = _head; }

  size_t getBufferSize() const {
    size_t count = 0;
    Node<T> *n = _head;
    while (n) {
      ++count;
      n = n->nextLink;
    }
    return count;
  }

  size_t getFreeBufferSize() const {
    if (MQTTCore::TX_BUFFER_MAX_SIZE_BYTE > 0) {
      size_t currentSize = getBufferSize();
      if (currentSize < MQTTCore::TX_BUFFER_MAX_SIZE_BYTE) {
        return MQTTCore::TX_BUFFER_MAX_SIZE_BYTE - currentSize;
      } else {
        return 0;
      }
    } else {
      return 0;
    }
  }

  bool isEmptyBuffer() const { return !_head; }

  bool isFullBuffer() const {
    return MQTTCore::TX_BUFFER_MAX_SIZE_BYTE > 0 &&
           getBufferSize() >= MQTTCore::TX_BUFFER_MAX_SIZE_BYTE;
  }

  Iterator begin() const {
    Iterator it;
    it.currentNode = _head;
    it.prevNode = nullptr;
    return it;
  }

  Iterator end() const {
    Iterator it;
    it.currentNode = nullptr;
    it.prevNode = _tail;
    return it;
  }

  Iterator find(const T &data) const {
    Node<T> *current = _head;
    Node<T> *localPrev = nullptr; // Local variable to hold previous node

    while (current) {
      if (current->data == data) {
        Iterator it;
        it.currentNode = current;
        it.prevNode = localPrev;
        return it;
      }
      localPrev = current;
      current = current->nextLink;
    }
    return end();
  }

  Iterator toHead() const { return Iterator(_head); }

  Iterator toTail() const { return Iterator(_tail); }

  void next() {
    if (_current) {
      _prev = _current;
      _current = _current->nextLink;
    }
  }

private:
  Node<T> *_head;
  Node<T> *_tail;
  Node<T> *_current;
  Node<T> *_prev;
  BufferState _bufferState;
};

} // namespace MQTTTransport

void printBufferState(const MQTTTransport::Buffer<int> &buffer) {
  String status = buffer.getStatus();
  Serial.println(status);
}

void printBuffer(const MQTTTransport::Buffer<int> &buffer) {
  Serial.print("Buffer: ");
  MQTTTransport::Buffer<int>::Iterator it = buffer.begin();
  while (it != buffer.end()) {
    Serial.print(*it);
    Serial.print(" ");
    ++it;
  }
  Serial.println();
}

void testBuffer() {
  Serial.println("-------------------------------------------------------------"
                 "----------------");
  Serial.println("-------------------------------------------------------------"
                 "----------------");
  Serial.println("---------------------------BUFFER TEST FOR ESP "
                 "32-----------------------------");
  Serial.println("-------------------------------------------------------------"
                 "----------------");
  Serial.println("-------------------------------------------------------------"
                 "----------------");
  // Create a buffer object
  MQTTTransport::Buffer<int> buffer;

  // Test pushing nodes to the back

  // Test pushing nodes to the back
  Serial.println("Pushing nodes to the back...");
  buffer.pushBack(1);
  printBufferState(buffer);
  printBuffer(buffer);
  buffer.pushBack(2);
  printBufferState(buffer);
  printBuffer(buffer);
  buffer.pushBack(3);
  printBufferState(buffer);
  printBuffer(buffer);

  // Test pushing nodes to the front
  Serial.println("Pushing nodes to the front...");
  buffer.pushFront(0);
  printBufferState(buffer);
  printBuffer(buffer);
  buffer.pushFront(-1);
  printBufferState(buffer);
  printBuffer(buffer);
  buffer.pushFront(-2);
  printBufferState(buffer);
  printBuffer(buffer);
  buffer.getStatus();

  // Test iterating through the buffer
  Serial.println("Iterating through the buffer...");
  printBuffer(buffer);

  // Test next() function multiple times
  Serial.println("Testing next() function multiple times:");
  for (int i = 0; i < 3; ++i) {
    Serial.println("Testing next() function " + String(i + 1) + ":");
    buffer.next();
    Serial.println("Next element dequeued.");
    Serial.print("Current element value: ");
    if (buffer.getCurrent()) {
      Serial.println(*(buffer.getCurrent()));
    } else {
      Serial.println("null");
    }
    Serial.print("Previous element value: ");
    if (buffer.getPrev()) {
      Serial.println(*(buffer.getPrev()));
    } else {
      Serial.println("null");
    }
  }

  // Test finding nodes
  Serial.println("Finding nodes...");
  MQTTTransport::Buffer<int>::Iterator it = buffer.find(2);
  if (it != buffer.end()) {
    Serial.println("Found node with data 2.");
  } else {
    Serial.println("Node with data 2 not found.");
  }

  it = buffer.find(10);
  if (it != buffer.end()) {
    Serial.println("Found node with data 10.");
  } else {
    Serial.println("Node with data 10 not found.");
  }

  // Test removing nodes
  Serial.println("Removing nodes...");
  buffer.remove(0); // Removing node from the middle
  printBuffer(buffer);
  buffer.remove(-2); // Removing node from the beginning
  printBuffer(buffer);
  buffer.remove(3); // Removing node from the end
  printBuffer(buffer);

  // Test edge cases
  Serial.println("Testing edge cases...");
  Serial.println("Is the buffer empty? " +
                 String(buffer.isEmptyBuffer() ? "Yes" : "No"));
  Serial.println("Is the buffer full? " +
                 String(buffer.isFullBuffer() ? "Yes" : "No"));
  Serial.println("Size of the buffer: " + String(buffer.getBufferSize()));
  Serial.println("Free buffer size: " + String(buffer.getFreeBufferSize()));

  // Test getStatus function
  Serial.println("Buffer status:");
  String status = buffer.getStatus();
  Serial.println(status);

  // // Testing the copy constructor
  // Serial.println("Testing copy constructor...");
  // MQTTTransport::Buffer<int> copiedBuffer = buffer;
  // Serial.println("Copied buffer:");
  // printBuffer(copiedBuffer);

  // Testing the move constructor
  Serial.println("Testing move constructor...");
  MQTTTransport::Buffer<int> movedBuffer = std::move(buffer);
  Serial.println("Moved buffer:");
  printBuffer(movedBuffer);

  // Test clearing the buffer
  Serial.println("Clearing the buffer...");
  buffer.clear();
  printBuffer(buffer);

  Serial.println("Test completed.");
}

#endif
