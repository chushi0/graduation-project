package set

type SetKey interface {
	HashCode() int
	Equals(o SetKey) bool
}

type HashSet map[int][]SetKey

type HashSetKey HashSet

func NewHashSet(val ...SetKey) HashSet {
	res := HashSet(make(map[int][]SetKey))
	res.Put(val...)
	return res
}

func (s HashSet) Put(val ...SetKey) {
	for _, v := range val {
		hash := v.HashCode()
		if _, ok := s[hash]; !ok {
			s[hash] = make([]SetKey, 0)
		}
		exist := false
		for _, o := range s[hash] {
			if v.Equals(o) {
				exist = true
				break
			}
		}
		if !exist {
			s[hash] = append(s[hash], v)
		}
	}
}

func (s HashSet) Contains(n SetKey) bool {
	hash := n.HashCode()
	if arr, ok := s[hash]; ok {
		for _, v := range arr {
			if v.Equals(n) {
				return true
			}
		}
	}
	return false
}

func (s HashSet) Foreach(proc func(SetKey)) {
	for _, array := range s {
		for _, value := range array {
			proc(value)
		}
	}
}

func (s HashSet) Clone() HashSet {
	res := NewHashSet()
	s.Foreach(func(sk SetKey) {
		res.Put(sk)
	})
	return res
}

func (s HashSet) Equals(o HashSet) bool {
	if len(s) != len(o) {
		return false
	}
	for n, arr := range s {
		if len(s[n]) != len(o[n]) {
			return false
		}
		for _, v := range arr {
			if !o.Contains(v) {
				return false
			}
		}
	}
	return true
}

func (s HashSet) Merge(o HashSet) {
	o.Foreach(func(sk SetKey) {
		s.Put(sk)
	})
}

func (s HashSet) AsHashSetKey() HashSetKey {
	return HashSetKey(s)
}

func (s HashSetKey) HashCode() int {
	v := 0
	s.AsHashSet().Foreach(func(sk SetKey) {
		v = 31*v + sk.HashCode()
	})
	return v
}

func (s HashSetKey) Equals(o SetKey) bool {
	oKey, ok := o.(HashSetKey)
	if !ok {
		return false
	}
	return s.AsHashSet().Equals(oKey.AsHashSet())
}

func (s HashSetKey) AsHashSet() HashSet {
	return HashSet(s)
}
