package set

import (
	"encoding/json"
	"sort"
)

type StringSet map[string]interface{}

func NewStringSet(val ...string) StringSet {
	res := make(map[string]interface{})
	for _, v := range val {
		res[v] = true
	}
	return res
}

func (s StringSet) Put(val ...string) int {
	c := 0
	for _, v := range val {
		if s.Contains(v) {
			continue
		}
		s[v] = true
		c += 1
	}
	return c
}

func (s StringSet) UnionExcept(set StringSet, except ...string) int {
	exceptSet := NewStringSet(except...)
	c := 0
	for v := range set {
		if exceptSet.Contains(v) {
			continue
		}
		if s.Contains(v) {
			continue
		}
		s[v] = true
		c += 1
	}
	return c
}

func (s StringSet) Contains(n string) bool {
	_, ok := s[n]
	return ok
}

func (s StringSet) Clone() StringSet {
	res := NewStringSet()
	for n := range s {
		res.Put(n)
	}
	return res
}

func (s StringSet) Equals(o StringSet) bool {
	if len(s) != len(o) {
		return false
	}
	for n := range s {
		if !o.Contains(n) {
			return false
		}
	}
	return true
}

func (s StringSet) Remove(val string) {
	delete(s, val)
}

func (s StringSet) Intersection(o StringSet) StringSet {
	res := NewStringSet()
	for v := range s {
		if o.Contains(v) {
			res.Put(v)
		}
	}
	return res
}

func (s StringSet) MarshalJSON() ([]byte, error) {
	arr := make([]string, 0, len(s))
	for v := range s {
		arr = append(arr, v)
	}
	sort.Strings(arr)
	return json.Marshal(arr)
}
