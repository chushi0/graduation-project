package set

type StringSet map[string]interface{}

func NewStringSet(val ...string) StringSet {
	res := make(map[string]interface{})
	for _, v := range val {
		res[v] = true
	}
	return res
}

func (s StringSet) Put(val ...string) {
	for _, v := range val {
		s[v] = true
	}
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

func (s StringSet) Merge(o StringSet) {
	for v := range o {
		s[v] = true
	}
}

func (s StringSet) Remove(val string) {
	delete(s, val)
}
