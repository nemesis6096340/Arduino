#ifndef _ULIST_h
#define _ULIST_h

#include "Arduino.h"

template <typename T>

class UList
{
public:
    UList();
    UList(size_t capacity);
    ~UList();

    size_t Capacity() const;
    size_t Count() const;

    T &operator[](const size_t index);

    bool Contains(T item);
    size_t IndexOf(T item);

    T &First();
    T &Last();

    void Add(T item);
    void AddRange(T *items, size_t numItems);

    void Insert(T item);
    void Insert(size_t index, T item);
    void InsertRange(T *items, size_t numItems);
    void InsertRange(size_t index, T *items, size_t numItems);

    void RemoveFirst();
    void Remove(size_t index);
    void RemoveLast();
    void RemoveRange(size_t index, size_t numItems);

    void Replace(size_t index, T item);
    void ReplaceRange(size_t index, T *items, size_t numItems);

    void Reverse();
    void Clear();
    bool IsEmpty();
    bool IsFull();
    void Trim();
    void Trim(size_t reserve);

    T *ToArray();
    T *ToArray(size_t index, size_t numItems);
    void CopyTo(T *items);
    void CopyTo(T *items, size_t index, size_t numItems);
    void FromArray(T *items, size_t numItems);

private:
    T *_items;

    size_t _count = 0;
    size_t _capacity = 4;

    void shift(size_t index, size_t numItems);
    void unshift(size_t index, size_t numItems);
    void reserve(size_t size);
    void resize(size_t size);
};

template <typename T>
UList<T>::UList()
{
    _items = new T[_capacity];
}

template <typename T>
UList<T>::UList(size_t capacity)
{
    _capacity = capacity;
    _items = new T[_capacity];
}

template <typename T>
UList<T>::~UList()
{
    delete[] _items;
}

template <typename T>
T &UList<T>::operator[](const size_t index)
{
    return _items[index];
}

template <typename T>
size_t UList<T>::Capacity() const
{
    return _capacity;
}

template <typename T>
size_t UList<T>::Count() const
{
    return _count;
}

template <typename T>
T &UList<T>::First()
{
    return _items[0];
}

template <typename T>
T &UList<T>::Last()
{
    return _items[_count - 1];
}

template <typename T>
void UList<T>::Add(T item)
{
    ++_count;
    reserve(_count);
    _items[_count - 1] = item;
}

template <typename T>
void UList<T>::AddRange(T *items, size_t numItems)
{
    reserve(_count + numItems);
    memmove(_items + _count, items, numItems * sizeof(T));
    _count += numItems;
}

template <typename T>
void UList<T>::Insert(T item)
{
    ++_count;
    reserve(_count);
    shift(0, 1);
    _items[0] = item;
}

template <typename T>
void UList<T>::InsertRange(T *items, size_t numItems)
{
    _count += numItems;
    reserve(_count);
    shift(0, numItems);
    memmove(_items, items, numItems * sizeof(T));
}

template <typename T>
void UList<T>::Insert(size_t index, T item)
{
    if (index > _count - 1)
        return;

    ++_count;
    reserve(_count);
    shift(index, 1);
    _items[index] = item;
}

template <typename T>
void UList<T>::InsertRange(size_t index, T *items, size_t numItems)
{
    if (index > _count - 1)
        return;

    _count += numItems;
    reserve(_count);
    shift(index, numItems);
    memmove(_items + index, items, numItems * sizeof(T));
}

template <typename T>
void UList<T>::RemoveFirst()
{
    if (_count == 0)
        return;

    unshift(0, 1);
    --_count;
}

template <typename T>
void UList<T>::Remove(size_t index)
{
    if (index > _count - 1)
        return;

    unshift(index, 1);
    --_count;
}

template <typename T>
void UList<T>::RemoveLast()
{
    if (_count == 0)
        return;

    --_count;
}

template <typename T>
void UList<T>::RemoveRange(size_t index, size_t numItems)
{
    unshift(index, numItems);
    _count -= numItems;
}

template <typename T>
void UList<T>::Replace(size_t index, T item)
{
    if (index > _count - 1)
        return;

    _items[index] = item;
}

template <typename T>
void UList<T>::ReplaceRange(size_t index, T *items, size_t numItems)
{
    memmove(_items + index, items, numItems * sizeof(T));
}

template <typename T>
void UList<T>::Reverse()
{
    T item;
    for (int index = 0; index < _count / 2; index++)
    {
        item = _items[index];
        _items[index] = _items[_count - 1 - index];
        _items[_count - 1 - index] = item;
    }
}

template <typename T>
void UList<T>::Clear()
{
    _count = 0;
}

template <typename T>
bool UList<T>::IsEmpty()
{
    return (_count == 0);
}

template <typename T>
bool UList<T>::IsFull()
{
    return (_count >= _capacity);
}

template <typename T>
void UList<T>::Trim()
{
    resize(_count);
}

template <typename T>
void UList<T>::Trim(size_t reserve)
{
    resize(_count + reserve);
}

template <typename T>
T *UList<T>::ToArray()
{
    T *items = new T[_count];
    memmove(items, _items, _count * sizeof(T));
    return items;
}

template <typename T>
T *UList<T>::ToArray(size_t index, size_t numItems)
{
    T *items = new T[numItems];
    memmove(items, _items + index, numItems * sizeof(T));
    return items;
}

template <typename T>
void UList<T>::CopyTo(T *items)
{
    memmove(items, _items, _count * sizeof(T));
}

template <typename T>
void UList<T>::CopyTo(T *items, size_t index, size_t numItems)
{
    memmove(items, _items + index, numItems * sizeof(T));
}

template <typename T>
void UList<T>::FromArray(T *items, size_t numItems)
{
    reserve(numItems);
    _count = numItems;
    memmove(_items, items, numItems * sizeof(T));
}

template <typename T>
bool UList<T>::Contains(T item)
{
    for (int index = 0; index < _count; index++)
        if (_items[index] == item)
            return true;
    return false;
}

template <typename T>
size_t UList<T>::IndexOf(T item)
{
    for (int index = 0; index < _count; index++)
        if (_items[index] == item)
            return index;
    return -1;
}

template <typename T>
void UList<T>::shift(size_t index, size_t numItems)
{
    memmove(_items + index + numItems, _items + index, (_count - index - 1) * sizeof(T));
}

template <typename T>
void UList<T>::unshift(size_t index, size_t numItems)
{
    memmove(_items + index, _items + index + numItems, (_count - index - numItems + 1) * sizeof(T));
}

template <typename T>
void UList<T>::reserve(size_t size)
{
    if (_count > _capacity)
    {
        size_t newSize = _capacity * 2 > size ? _capacity * 2 : size;
        resize(newSize);
    }
}

template <typename T>
void UList<T>::resize(size_t size)
{
    if (_count > size)
        return;
    if (_capacity == size)
        return;

    T *newItems = new T[size];
    memmove(newItems, _items, _count * sizeof(T));
    delete[] _items;
    _capacity = size;
    _items = newItems;
}

#endif
