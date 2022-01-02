package set

type IntSet map[int]interface{}

func NewIntSet(val ...int) IntSet {
	res := make(map[int]interface{})
	for _, v := range val {
		res[v] = true
	}
	return res
}

func (s IntSet) Put(val ...int) {
	for _, v := range val {
		s[v] = true
	}
}

func (s IntSet) Contains(n int) bool {
	_, ok := s[n]
	return ok
}

func (s IntSet) Clone() IntSet {
	res := NewIntSet()
	for n := range s {
		res.Put(n)
	}
	return res
}

func (s IntSet) Equals(o IntSet) bool {
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

func (s IntSet) Merge(o IntSet) {
	for v := range o {
		s[v] = true
	}
}
