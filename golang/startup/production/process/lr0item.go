package process

type LR0Item struct {
	Prod     int `json:"prod"`
	Progress int `json:"progress"`
}

type LR0ItemClosure []*LR0Item

func NewLr0ItemClosure() *LR0ItemClosure {
	closure := (LR0ItemClosure)(make([]*LR0Item, 0))
	return &closure
}

func (closure *LR0ItemClosure) Contains(item LR0Item) bool {
	for _, i := range *closure {
		if *i == item {
			return true
		}
	}
	return false
}

func (closure *LR0ItemClosure) AddItem(item LR0Item) bool {
	if closure.Contains(item) {
		return false
	}
	*closure = append(*closure, &item)
	return true
}

func (closure *LR0ItemClosure) Equals(another *LR0ItemClosure) bool {
	if len(*closure) != len(*another) {
		return false
	}
	for _, item := range *closure {
		if !another.Contains(*item) {
			return false
		}
	}
	return true
}

type LR0ItemClosureMap struct {
	Closures []*LR0ItemClosure
	Edges    []*LR0ItemClosureMapEdge
}

type LR0ItemClosureMapEdge struct {
	From   int    `json:"from"`
	To     int    `json:"to"`
	Symbol string `json:"symbol"`
}

func NewLr0ItemClosureMap() *LR0ItemClosureMap {
	return &LR0ItemClosureMap{
		Closures: make([]*LR0ItemClosure, 0),
		Edges:    make([]*LR0ItemClosureMapEdge, 0),
	}
}

func (mp *LR0ItemClosureMap) FindClosureOrAppend(closure *LR0ItemClosure) int {
	for i, c := range mp.Closures {
		if c.Equals(closure) {
			return i
		}
	}
	mp.Closures = append(mp.Closures, closure)
	return len(mp.Closures) - 1
}
