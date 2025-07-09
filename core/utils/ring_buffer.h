#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define DAFAULT_RING_BUFFER_SIZE 100

template<typename T> 
class RingBuffer {
public:

	RingBuffer(const int size = DAFAULT_RING_BUFFER_SIZE);

	~RingBuffer();

	/**
	 * Adds an element to the beginning of m_buffer: the operation returns `false` if the addition caused overwriting an existing element.
	 */
	bool unshift(T value);

	/**
	 * Adds an element to the end of m_buffer: the operation returns `false` if the addition caused overwriting an existing element.
	 */
	bool push(T value);

	/**
	 * Removes an element from the beginning of the m_buffer.
	 */
	T shift();

	/**
	 * Removes an element from the end of the m_buffer.
	 */
	T pop();

	/**
	 * Returns the element at the beginning of the m_buffer.
	 */
	T inline first();

	/**
	 * Returns the element at the end of the m_buffer.
	 */
	T inline last();

	/**
	 * Array-like access to m_buffer
	 */
	T operator [] (int index);

	/**
	 * Returns how many elements are actually stored in the m_buffer.
	 */
	int inline size();

	/**
	 * Returns how many elements can be safely pushed into the m_buffer.
	 */
	int inline available();

	/**
	 * Returns how many elements can be potentially stored into the m_buffer.
	 */
	int inline capacity();

	/**
	 * Returns `true` if no elements can be removed from the m_buffer.
	 */
	bool inline is_empty();

	/**
	 * Returns `true` if no elements can be added to the m_buffer without overwriting existing elements.
	 */
	bool inline is_full();

	/**
	 * Resets the m_buffer to a clean status, making all m_buffer positions available.
	 */
	void inline clear();

private:
	T *m_buffer = nullptr;
	T *m_head;
	T *m_tail;
	int m_count;
	int m_size;
};

template<typename T> 
RingBuffer<T>::RingBuffer(const int size) 
{
	m_size = size;
	m_buffer = new T[m_size];
	m_head = m_buffer;
	m_tail = m_buffer;
	m_count = 0;
}

template<typename T> 
RingBuffer<T>::~RingBuffer() 
{
	if (m_buffer)
	{
		delete[] m_buffer;
	}	
}

template<typename T> 
bool RingBuffer<T>::unshift(T value) {
	if (m_head == m_buffer) {
		m_head = m_buffer + m_size;
	}
	*--m_head = value;
	if (m_count == m_size) {
		if (m_tail-- == m_buffer) {
			m_tail = m_buffer + m_size - 1;
		}
		return false;
	} else {
		if (m_count++ == 0) {
			m_tail = m_head;
		}
		return true;
	}
}

template<typename T> 
bool RingBuffer<T>::push(T value) {
	if (++m_tail == m_buffer + m_size) {
		m_tail = m_buffer;
	}
	*m_tail = value;
	if (m_count == m_size) {
		if (++m_head == m_buffer + m_size) {
			m_head = m_buffer;
		}
		return false;
	} else {
		if (m_count++ == 0) {
			m_head = m_tail;
		}
		return true;
	}
}

template<typename T> 
T RingBuffer<T>::shift() {
	void(* crash) (void) = 0;
	if (m_count <= 0) crash();
	T result = *m_head++;
	if (m_head >= m_buffer + m_size) {
		m_head = m_buffer;
	}
	m_count--;
	return result;
}

template<typename T> 
T RingBuffer<T>::pop() {
	void(* crash) (void) = 0;
	if (m_count <= 0) crash();
	T result = *m_tail--;
	if (m_tail < m_buffer) {
		m_tail = m_buffer + m_size - 1;
	}
	m_count--;
	return result;
}

template<typename T> 
T inline RingBuffer<T>::first() {
	return *m_head;
}

template<typename T> 
T inline RingBuffer<T>::last() {
	return *m_tail;
}

template<typename T> 
T RingBuffer<T>::operator [](int index) {
	return *(m_buffer + ((m_head - m_buffer + index) % m_size));
}

template<typename T> 
int inline RingBuffer<T>::size() {
	return m_count;
}

template<typename T> 
int inline RingBuffer<T>::available() {
	return m_size - m_count;
}

template<typename T> 
int inline RingBuffer<T>::capacity() {
	return m_size;
}

template<typename T> 
bool inline RingBuffer<T>::is_empty() {
	return m_count == 0;
}

template<typename T> 
bool inline RingBuffer<T>::is_full() {
	return m_count == m_size;
}

template<typename T> 
void inline RingBuffer<T>::clear() {
	m_head = m_tail = m_buffer;
	m_count = 0;
}

#endif //RING_BUFFER_H
