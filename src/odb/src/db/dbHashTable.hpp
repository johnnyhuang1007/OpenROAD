// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include "dbCore.h"
#include "dbHashTable.h"

namespace odb {

inline unsigned int hash_string(const char* str)
{
  unsigned int hash = 0;
  int c;

  while ((c = static_cast<unsigned char>(*str++)) != '\0') {
    hash = c + (hash << 6) + (hash << 16) - hash;
  }

  return hash;
}

template <class T, uint page_size>
dbHashTable<T, page_size>::dbHashTable()
{
  _obj_tbl = nullptr;
  _num_entries = 0;
}

template <class T, uint page_size>
dbHashTable<T, page_size>::dbHashTable(const dbHashTable<T, page_size>& t)
    : _hash_tbl(t._hash_tbl), _num_entries(t._num_entries), _obj_tbl(t._obj_tbl)
{
}

template <class T, uint page_size>
bool dbHashTable<T, page_size>::operator==(
    const dbHashTable<T, page_size>& rhs) const
{
  if (_num_entries != rhs._num_entries) {
    return false;
  }

  if (_hash_tbl != rhs._hash_tbl) {
    return false;
  }

  return true;
}

template <class T, uint page_size>
void dbHashTable<T, page_size>::growTable()
{
  uint sz = _hash_tbl.size();
  dbId<T> entries;
  uint i;

  for (i = 0; i < sz; ++i) {
    dbId<T> cur = _hash_tbl[i];

    while (cur != 0) {
      T* entry = _obj_tbl->getPtr(cur);
      dbId<T> next = entry->_next_entry;
      entry->_next_entry = entries;
      entries = entry->getOID();
      cur = next;
    }

    _hash_tbl[i] = 0;
  }

  // double the size of the hash-table
  dbId<T> nullId;
  for (i = 0; i < sz; ++i) {
    _hash_tbl.push_back(nullId);
  }

  // reinsert the entries
  sz = _hash_tbl.size() - 1;
  dbId<T> cur = entries;

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);
    dbId<T> next = entry->_next_entry;
    uint hid = hash_string(entry->_name) & sz;
    dbId<T>& e = _hash_tbl[hid];
    entry->_next_entry = e;
    e = entry->getOID();
    cur = next;
  }
}

template <class T, uint page_size>
void dbHashTable<T, page_size>::shrinkTable()
{
  uint sz = _hash_tbl.size();
  dbId<T> entries;
  uint i;

  for (i = 0; i < sz; ++i) {
    dbId<T> cur = _hash_tbl[i];

    while (cur != 0) {
      T* entry = _obj_tbl->getPtr(cur);
      dbId<T> next = entry->_next_entry;
      entry->_next_entry = entries;
      entries = entry->getOID();
      cur = next;
    }

    _hash_tbl[i] = 0;
  }

  // TODO: add method to dbPagedVector to resize the table
  _hash_tbl.clear();
  sz >>= 1;

  // halve the size of the hash-table
  dbId<T> nullId;
  for (i = 0; i < sz; ++i) {
    _hash_tbl.push_back(nullId);
  }

  sz -= 1;
  // reinsert the entries
  dbId<T> cur = entries;

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);
    dbId<T> next = entry->_next_entry;
    uint hid = hash_string(entry->_name) & sz;
    dbId<T>& e = _hash_tbl[hid];
    entry->_next_entry = e;
    e = entry->getOID();
    cur = next;
  }
}

template <class T, uint page_size>
void dbHashTable<T, page_size>::insert(T* object)
{
  ++_num_entries;
  uint sz = _hash_tbl.size();

  if (sz == 0) {
    dbId<T> nullId;
    _hash_tbl.push_back(nullId);
    sz = 1;
  } else {
    uint r = _num_entries / sz;

    if (r > CHAIN_LENGTH) {
      growTable();
      sz = _hash_tbl.size();
    }
  }

  uint hid = hash_string(object->_name) & (sz - 1);
  dbId<T>& e = _hash_tbl[hid];
  object->_next_entry = e;
  e = object->getOID();
}

template <class T, uint page_size>
T* dbHashTable<T, page_size>::find(const char* name)
{
  uint sz = _hash_tbl.size();

  if (sz == 0) {
    return nullptr;
  }

  uint hid = hash_string(name) & (sz - 1);
  dbId<T> cur = _hash_tbl[hid];

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);

    if (strcmp(entry->_name, name) == 0) {
      return entry;
    }

    cur = entry->_next_entry;
  }

  return nullptr;
}

template <class T, uint page_size>
int dbHashTable<T, page_size>::hasMember(const char* name)
{
  uint sz = _hash_tbl.size();

  if (sz == 0) {
    return false;
  }

  uint hid = hash_string(name) & (sz - 1);
  dbId<T> cur = _hash_tbl[hid];

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);

    if (strcmp(entry->_name, name) == 0) {
      return true;
    }

    cur = entry->_next_entry;
  }

  return false;
}

template <class T, uint page_size>
void dbHashTable<T, page_size>::remove(T* object)
{
  uint sz = _hash_tbl.size();
  uint hid = hash_string(object->_name) & (sz - 1);
  dbId<T> cur = _hash_tbl[hid];
  dbId<T> prev;

  while (cur != 0) {
    T* entry = _obj_tbl->getPtr(cur);

    if (entry == object) {
      if (prev == 0) {
        _hash_tbl[hid] = entry->_next_entry;
      } else {
        T* p = _obj_tbl->getPtr(prev);
        p->_next_entry = entry->_next_entry;
      }

      --_num_entries;

      uint r = (_num_entries + _num_entries / 10) / sz;

      if ((r < (CHAIN_LENGTH >> 1)) && (sz > 1)) {
        shrinkTable();
      }

      return;
    }

    prev = cur;
    cur = entry->_next_entry;
  }
}

template <class T, uint page_size>
dbOStream& operator<<(dbOStream& stream, const dbHashTable<T, page_size>& table)
{
  stream << table._hash_tbl;
  stream << table._num_entries;
  return stream;
}

template <class T, uint page_size>
dbIStream& operator>>(dbIStream& stream, dbHashTable<T, page_size>& table)
{
  stream >> table._hash_tbl;
  stream >> table._num_entries;
  return stream;
}

}  // namespace odb
