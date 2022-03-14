package process

type LR1Item struct {
	Prod      int    `json:"prod"`
	Progress  int    `json:"progress"`
	Lookahead string `json:"lookahead"`
}

type LR1ItemClosure []*LR1Item

func NewLr1ItemClosure() *LR1ItemClosure {
	closure := (LR1ItemClosure)(make([]*LR1Item, 0))
	return &closure
}

func (closure *LR1ItemClosure) Contains(item LR1Item) bool {
	for _, i := range *closure {
		if *i == item {
			return true
		}
	}
	return false
}

func (closure *LR1ItemClosure) AddItem(item LR1Item) bool {
	if closure.Contains(item) {
		return false
	}
	*closure = append(*closure, &item)
	return true
}

func (closure *LR1ItemClosure) Equals(another *LR1ItemClosure) bool {
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

type LR1ItemClosureMap struct {
	Closures []*LR1ItemClosure        `json:"closures"`
	Edges    []*LR1ItemClosureMapEdge `json:"edges"`
}

type LR1ItemClosureMapEdge struct {
	From   int    `json:"from"`
	To     int    `json:"to"`
	Symbol string `json:"symbol"`
}

func NewLR1ItemClosureMap() *LR1ItemClosureMap {
	return &LR1ItemClosureMap{
		Closures: make([]*LR1ItemClosure, 0),
		Edges:    make([]*LR1ItemClosureMapEdge, 0),
	}
}

func (mp *LR1ItemClosureMap) FindClosureOrAppend(closure *LR1ItemClosure) int {
	for i, c := range mp.Closures {
		if c.Equals(closure) {
			return i
		}
	}
	mp.Closures = append(mp.Closures, closure)
	return len(mp.Closures) - 1
}
