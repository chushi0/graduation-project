package process

import (
	"fmt"

	"github.com/chushi0/graduation_project/golang/startup/production"
	"github.com/chushi0/graduation_project/golang/startup/util/set"
	utilslice "github.com/chushi0/graduation_project/golang/startup/util/util_slice"
)

// 产生式所定义的语法
type Grammer struct {
	StartNonterminal string                             // 开始符号
	Terminals        set.StringSet                      // 终结符
	Nonterminals     set.StringSet                      // 非终结符
	Productions      map[string][]production.Production // 产生式（key：终结符）
	RawNonterminals  map[string]string                  // 映射到原始非终结符
}

func NewGrammer(prods []production.Production) *Grammer {
	grammer := &Grammer{
		Productions:     make(map[string][]production.Production),
		RawNonterminals: make(map[string]string),
	}

	// 计算终结符与非终结符
	terminals, nonterminals := production.GetTerminalsAndNonterminals(prods)
	grammer.Terminals = terminals
	grammer.Nonterminals = nonterminals
	for nonterminal := range nonterminals {
		grammer.Productions[nonterminal] = make([]production.Production, 0)
	}

	// 存入产生式
	for _, prod := range prods {
		grammer.AddNewProduction(prod)
	}

	if nonterminals.Contains("S") {
		grammer.StartNonterminal = "S"
	}

	return grammer
}

// 添加新的产生式
// 如果相同的产生式已经存在，则忽略
func (g *Grammer) AddNewProduction(prod production.Production) {
	prods := g.Productions[prod[0]]
	for _, p := range prods {
		if utilslice.StringEquals(p, prod) {
			return
		}
	}
	prods = append(prods, prod)
	g.Productions[prod[0]] = prods
}

// 添加新的非终结符
// from: 由哪个非终结符生成
func (g *Grammer) AddNewNonterminal(from string) string {
	for {
		original, exist := g.RawNonterminals[from]
		if !exist {
			break
		}
		from = original
	}
	var newNonterminal string
	for i := 0; ; i++ {
		newNonterminal = fmt.Sprintf("%s_%d", from, i)
		if g.Nonterminals.Contains(newNonterminal) {
			continue
		}
		if g.Terminals.Contains(newNonterminal) {
			continue
		}
		break
	}
	g.Nonterminals.Put(newNonterminal)
	g.Productions[newNonterminal] = make([]production.Production, 0)
	g.RawNonterminals[newNonterminal] = from
	return newNonterminal
}

// 移除产生式
func (g *Grammer) RemoveProduction(prod production.Production) {
	prods := g.Productions[prod[0]]
	newProds := make([]production.Production, 0)
	for _, p := range prods {
		if utilslice.StringEquals(prod, p) {
			continue
		}
		newProds = append(newProds, p)
	}
	g.Productions[prod[0]] = newProds
}
