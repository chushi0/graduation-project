package production_process

import (
	"fmt"

	"github.com/chushi0/graduation_project/golang/startup/production"
	utilslice "github.com/chushi0/graduation_project/golang/startup/util/util_slice"
)

// 产生式所定义的语法
// 为了便于展示，使用slice表示终结符与非终结符，而不用性能更好的set
type Grammer struct {
	Terminals       []string                           // 终结符
	Nonterminals    []string                           // 非终结符
	AllProductions  []production.Production            // 产生式（顺序）
	Productions     map[string][]production.Production // 产生式（key：终结符）
	RawNonterminals map[string]string                  // 映射到原始非终结符
}

func NewGrammer(prods []production.Production) *Grammer {
	grammer := &Grammer{
		Terminals:       make([]string, 0),
		Nonterminals:    make([]string, 0),
		AllProductions:  make([]production.Production, 0),
		Productions:     make(map[string][]production.Production),
		RawNonterminals: make(map[string]string),
	}

	// 计算终结符与非终结符
	terminals, nonterminals := production.GetTerminalsAndNonterminals(prods)
	for terminal := range terminals {
		grammer.Terminals = append(grammer.Terminals, terminal)
	}
	for nonterminal := range nonterminals {
		grammer.Nonterminals = append(grammer.Nonterminals, nonterminal)
		grammer.Productions[nonterminal] = make([]production.Production, 0)
	}

	// 存入产生式
	for _, prod := range prods {
		grammer.AddNewProduction(prod)
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
	g.AllProductions = append(g.AllProductions, prod)
}

// 添加新的非终结符
// from: 由哪个非终结符生成
func (g *Grammer) AddNewNonterminal(from string) {
	var newNonterminal string
	for i := 0; ; i++ {
		newNonterminal = fmt.Sprintf("%s_%d", from, i)
		if utilslice.StringLinearSearch(g.Nonterminals, newNonterminal) != -1 {
			continue
		}
		if utilslice.StringLinearSearch(g.Terminals, newNonterminal) != -1 {
			continue
		}
		break
	}
	g.Nonterminals = append(g.Nonterminals, newNonterminal)
	g.Productions[newNonterminal] = make([]production.Production, 0)
	g.RawNonterminals[newNonterminal] = from
}
