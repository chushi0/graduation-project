package process

import (
	"fmt"
	"sort"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production"
	"github.com/chushi0/graduation_project/golang/startup/util/set"
)

type LR0Context struct {
	Context      *debug.DebugContext
	Grammer      *Grammer
	Code         string
	KeyVariables *LR0Variables
	SLR          bool
}

type LR0Variables struct {
	Productions   []production.Production `json:"productions"`
	LoopVariableI int                     `json:"loop_variable_i"`
	LoopVariableJ int                     `json:"loop_variable_j"`
	LoopVariableK int                     `json:"loop_variable_k"`
	ModifiedFlag  bool                    `json:"modified_flag"`
	Terminals     []string                `json:"terminals"`

	NonterminalOrders    []string      `json:"nonterminal_orders"`
	ProcessedSymbol      set.StringSet `json:"process_symbol"`
	CurrentProcessSymbol string        `json:"current_symbol"`

	FirstSet  map[string]set.StringSet `json:"first"`
	FollowSet map[string]set.StringSet `json:"follow"`

	ClosureMap     *LR0ItemClosureMap `json:"closure_map"`
	CurrentClosure *LR0ItemClosure    `json:"current_closure"`

	ActionTable []map[string]string `json:"action_table"`
	GotoTable   []map[string]int    `json:"goto_table"`
}

const (
	LR0_Action_ShiftReduce  = "移入-归约冲突"
	LR0_Action_ReduceReduce = "归约-归约冲突"
)

type LR0Result struct {
	Code      int           `json:"code"`
	Variables *LR0Variables `json:"variables"`
}

const (
	LR0_Success             = 0
	LR0_Error_ParseCode     = 1
	LR0_Error_NoStartSymbol = 2
	LR0_Error_Conflict      = 3
)

func NewLR0Context() *LR0Context {
	return &LR0Context{
		KeyVariables: &LR0Variables{},
	}
}

func (ctx *LR0Context) CreateLR0ProcessEntry() func(*debug.DebugContext) {
	return func(dc *debug.DebugContext) {
		ctx.Context = dc
		ctx.bury("start", 0)
		ctx.RunPipeline()
		dc.SwitchRunMode(debug.RunMode_Exit)
	}
}
func (ctx *LR0Context) bury(name string, line int) {
	ctx.Context.RunReport(&debug.Point{
		Name: name,
		Line: line,
	}, ctx.KeyVariables)
}

func (ctx *LR0Context) shutdownPipeline(res *LR0Result) {
	res.Variables = ctx.KeyVariables
	ctx.Context.ExitResult = res
	ctx.Context.SwitchRunMode(debug.RunMode_Exit)
	ctx.bury("shutdown", 0)
}

func (ctx *LR0Context) RunPipeline() {
	// 解析产生式代码
	ctx.ParseCode()
	// 转换为增广文法
	ctx.Translate()
	// SLR的情况：计算 First 集和 Follow 集
	if ctx.SLR {
		ctx.ComputeFirstSet()
		ctx.ComputeFollowSet()
	}
	// 计算项目集闭包
	ctx.ComputeItemClosure()
	// 生成自动机
	ctx.GenerateAutomaton()
}

// 按照一定顺序排列非终结符
func (ctx *LR0Context) sortNonterminals() {
	ctx.KeyVariables.NonterminalOrders = make([]string, 0)
	for nonterminal := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, nonterminal)
	}
	sort.Strings(ctx.KeyVariables.NonterminalOrders)
	sortProductions := make([]production.Production, 0)
	for _, nonterminal := range ctx.KeyVariables.NonterminalOrders {
		sortProductions = append(sortProductions, ctx.Grammer.Productions[nonterminal]...)
	}
	ctx.KeyVariables.Productions = sortProductions
}

func (ctx *LR0Context) ParseCode() {
	prods, errs := production.ParseProduction(ctx.Code, nil)
	if len(errs.Errors) > 0 {
		ctx.shutdownPipeline(&LR0Result{
			Code: LR0_Error_ParseCode,
		})
	}
	ctx.KeyVariables.Productions = prods
	ctx.Grammer = NewGrammer(prods)
	ctx.KeyVariables.Terminals = make([]string, 0, len(ctx.Grammer.Terminals))
	for terminal := range ctx.Grammer.Terminals {
		ctx.KeyVariables.Terminals = append(ctx.KeyVariables.Terminals, terminal)
	}
}

func (ctx *LR0Context) Translate() {
	ctx.bury("Translate", 0)
	if !ctx.Grammer.Nonterminals.Contains(ctx.Grammer.StartNonterminal) {
		ctx.shutdownPipeline(&LR0Result{
			Code: LR0_Error_NoStartSymbol,
		})
	}
	if len(ctx.Grammer.Productions[ctx.Grammer.StartNonterminal]) == 1 {
		ctx.bury("Translate", -1)
		return
	}
	nonterminal := ctx.Grammer.AddNewNonterminal(ctx.Grammer.StartNonterminal)
	ctx.Grammer.AddNewProduction([]string{nonterminal, ctx.Grammer.StartNonterminal})
	ctx.KeyVariables.Productions = append(ctx.KeyVariables.Productions, []string{nonterminal, ctx.Grammer.StartNonterminal})
	ctx.Grammer.StartNonterminal = nonterminal
	ctx.bury("Translate", -1)
}

func (ctx *LR0Context) ComputeFirstSet() {
	// 初始化 First 集
	ctx.KeyVariables.FirstSet = make(map[string]set.StringSet)
	for i := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.FirstSet[i] = set.NewStringSet()
	}

	ctx.sortNonterminals()
	ctx.KeyVariables.LoopVariableI = -1
	ctx.KeyVariables.LoopVariableJ = -1
	ctx.bury("ComputeFirstSet", 0)

	ctx.KeyVariables.ModifiedFlag = true
	for ctx.KeyVariables.ModifiedFlag {
		ctx.KeyVariables.ModifiedFlag = false
		ctx.bury("ComputeFirstSet", 2)

		ctx.KeyVariables.LoopVariableI = 0
		for {
			ctx.bury("ComputeFirstSet", 3)
			if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.Productions) {
				break
			}
			prod := ctx.KeyVariables.Productions[ctx.KeyVariables.LoopVariableI]
			nonterminal := prod[0]

			ctx.KeyVariables.LoopVariableJ = 1
			for {
				ctx.bury("ComputeFirstSet", 4)
				if ctx.KeyVariables.LoopVariableJ >= len(prod) {
					modify := ctx.KeyVariables.FirstSet[nonterminal].Put("") > 0
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag || modify
					break
				}

				// 终结符
				if ctx.Grammer.Terminals.Contains(prod[ctx.KeyVariables.LoopVariableJ]) {
					modify := ctx.KeyVariables.FirstSet[nonterminal].Put(prod[ctx.KeyVariables.LoopVariableJ]) > 0
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag || modify
					ctx.bury("ComputeFirstSet", 5)
					break
				}

				ctx.bury("ComputeFirstSet", 6)
				// 非终结符
				nonterminalFirstSet := ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]]
				modify := ctx.KeyVariables.FirstSet[nonterminal].UnionExcept(nonterminalFirstSet, "") > 0
				ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag || modify
				ctx.bury("ComputeFirstSet", 7)
				if !nonterminalFirstSet.Contains("") {
					break
				}
				ctx.KeyVariables.LoopVariableJ++
			}

			ctx.KeyVariables.LoopVariableI++
		}
		ctx.bury("ComputeFirstSet", 10)
	}

	ctx.bury("ComputeFirstSet", -1)
}

func (ctx *LR0Context) ComputeFollowSet() {
	// 初始化 Follow 集
	ctx.KeyVariables.FollowSet = make(map[string]set.StringSet)
	for i := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.FollowSet[i] = set.NewStringSet()
	}

	ctx.KeyVariables.LoopVariableI = -1
	ctx.KeyVariables.LoopVariableJ = -1
	ctx.KeyVariables.LoopVariableK = -1

	ctx.bury("ComputeFollowSet", 0)

	// 按照一定顺序排列非终结符
	ctx.sortNonterminals()
	ctx.bury("ComputeFollowSet", 1)

	// 开始符号加入结束符
	if _, ok := ctx.KeyVariables.FollowSet[ctx.Grammer.StartNonterminal]; ok {
		ctx.KeyVariables.FollowSet[ctx.Grammer.StartNonterminal].Put("$")
	}
	ctx.bury("ComputeFollowSet", 2)

	ctx.KeyVariables.ModifiedFlag = true
	for ctx.KeyVariables.ModifiedFlag {
		ctx.KeyVariables.ModifiedFlag = false

		ctx.KeyVariables.LoopVariableI = 0
		for {
			ctx.bury("ComputeFollowSet", 4)
			if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.Productions) {
				break
			}
			prod := ctx.KeyVariables.Productions[ctx.KeyVariables.LoopVariableI]
			nonterminal := prod[0]

			// 顺序遍历
			ctx.KeyVariables.LoopVariableJ = 1
			for {
				ctx.bury("ComputeFollowSet", 5)
				if ctx.KeyVariables.LoopVariableJ >= len(prod) {
					break
				}

				ctx.bury("ComputeFollowSet", 8)
				current := prod[ctx.KeyVariables.LoopVariableJ]
				if !ctx.Grammer.Nonterminals.Contains(current) {
					ctx.KeyVariables.LoopVariableJ++
					continue
				}
				ctx.KeyVariables.LoopVariableK = ctx.KeyVariables.LoopVariableJ + 1
				for {
					ctx.bury("ComputeFollowSet", 6)
					ctx.bury("ComputeFollowSet", 7)
					if ctx.KeyVariables.LoopVariableK >= len(prod) {
						modify := ctx.KeyVariables.FollowSet[current].UnionExcept(
							ctx.KeyVariables.FollowSet[nonterminal],
						) > 0
						ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag || modify
						break
					}
					ctx.bury("ComputeFollowSet", 9)
					cur := prod[ctx.KeyVariables.LoopVariableK]
					if ctx.Grammer.Terminals.Contains(cur) {
						modify := ctx.KeyVariables.FollowSet[current].Put(cur) > 0
						ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag || modify
						break
					}

					modify := ctx.KeyVariables.FollowSet[current].UnionExcept(
						ctx.KeyVariables.FirstSet[cur], "",
					) > 0
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag || modify
					ctx.bury("ComputeFollowSet", 10)
					if !ctx.KeyVariables.FirstSet[cur].Contains("") {
						break
					}

					ctx.bury("ComputeFollowSet", 11)
					ctx.KeyVariables.LoopVariableK++
				}
				ctx.KeyVariables.LoopVariableK = -1
				ctx.KeyVariables.LoopVariableJ++
			}
			ctx.KeyVariables.LoopVariableJ = -1
			ctx.KeyVariables.LoopVariableI++
		}
		ctx.bury("ComputeFollowSet", 12)
	}
	ctx.bury("ComputeFollowSet", -1)
}

// 计算项目集闭包
func (ctx *LR0Context) ComputeItemClosure() {
	ctx.KeyVariables.LoopVariableI = -1
	ctx.KeyVariables.LoopVariableJ = -1
	ctx.KeyVariables.LoopVariableK = -1
	ctx.bury("ComputeItemClosure", 0)
	ctx.KeyVariables.ClosureMap = NewLr0ItemClosureMap()
	// 开始符号
	for i, prod := range ctx.KeyVariables.Productions {
		if prod[0] == ctx.Grammer.StartNonterminal {
			ctx.ItemClosure([]LR0Item{{
				Prod:     i,
				Progress: 0,
			}})
			break
		}
	}
	ctx.bury("ComputeItemClosure", 1)
	// 循环每个项目集
	for i := 0; i < len(ctx.KeyVariables.ClosureMap.Closures); i++ {
		ctx.KeyVariables.LoopVariableI = i
		ctx.KeyVariables.LoopVariableJ = -1
		ctx.KeyVariables.LoopVariableK = -1
		ctx.bury("ComputeItemClosure", 2)
		closure := ctx.KeyVariables.ClosureMap.Closures[i]
		ctx.KeyVariables.ProcessedSymbol = set.NewStringSet()
		// 循环项目集闭包中的每个项目
		for j, item := range *closure {
			ctx.KeyVariables.LoopVariableJ = j
			ctx.KeyVariables.LoopVariableK = -1
			ctx.bury("ComputeItemClosure", 3)
			prod := ctx.KeyVariables.Productions[item.Prod]
			if item.Progress+1 >= len(prod) {
				continue
			}
			symbol := prod[item.Progress+1]
			if ctx.KeyVariables.ProcessedSymbol.Contains(symbol) {
				continue
			}
			ctx.KeyVariables.CurrentProcessSymbol = symbol
			items := make([]LR0Item, 0)
			for _, it := range *closure {
				prod := ctx.KeyVariables.Productions[it.Prod]
				if it.Progress+1 >= len(prod) {
					continue
				}
				symbol := prod[it.Progress+1]
				if symbol == ctx.KeyVariables.CurrentProcessSymbol {
					items = append(items, LR0Item{
						Prod:     it.Prod,
						Progress: it.Progress + 1,
					})
				}
			}
			to := ctx.ItemClosure(items)
			ctx.KeyVariables.ClosureMap.Edges = append(ctx.KeyVariables.ClosureMap.Edges, &LR0ItemClosureMapEdge{
				From:   i,
				To:     to,
				Symbol: symbol,
			})
			ctx.KeyVariables.LoopVariableK = to
			ctx.KeyVariables.ProcessedSymbol.Put(symbol)
			ctx.bury("ComputeItemClosure", 4)
			ctx.KeyVariables.LoopVariableK = -1
		}
	}
	ctx.bury("ComputeItemClosure", -1)
}

// 产生项目集闭包
func (ctx *LR0Context) ItemClosure(items []LR0Item) int {
	ctx.KeyVariables.CurrentClosure = NewLr0ItemClosure()
	for _, item := range items {
		ctx.KeyVariables.CurrentClosure.AddItem(item)
	}
	for i := 0; i < len(*ctx.KeyVariables.CurrentClosure); i++ {
		item := (*ctx.KeyVariables.CurrentClosure)[i]
		prod := ctx.KeyVariables.Productions[item.Prod]
		if item.Progress+1 >= len(prod) {
			continue
		}
		symbol := prod[item.Progress+1]
		if ctx.Grammer.Nonterminals.Contains(symbol) {
			for j, prod := range ctx.KeyVariables.Productions {
				if prod[0] == symbol {
					ctx.KeyVariables.CurrentClosure.AddItem(LR0Item{
						Prod:     j,
						Progress: 0,
					})
				}
			}
		}
	}
	return ctx.KeyVariables.ClosureMap.FindClosureOrAppend(ctx.KeyVariables.CurrentClosure)
}

func (ctx *LR0Context) GenerateAutomaton() {
	ctx.bury("GenerateAutomaton", 0)
	conflict := false
	itemCount := len(ctx.KeyVariables.ClosureMap.Closures)
	ctx.KeyVariables.ActionTable = make([]map[string]string, itemCount)
	ctx.KeyVariables.GotoTable = make([]map[string]int, itemCount)
	for i := 0; i < itemCount; i++ {
		ctx.KeyVariables.ActionTable[i] = make(map[string]string)
		ctx.KeyVariables.GotoTable[i] = make(map[string]int)
		ctx.KeyVariables.ActionTable[i]["$"] = ""
		for terminal := range ctx.Grammer.Terminals {
			ctx.KeyVariables.ActionTable[i][terminal] = ""
		}
		for nonterminal := range ctx.Grammer.Nonterminals {
			ctx.KeyVariables.GotoTable[i][nonterminal] = -1
		}
	}

	for _, edge := range ctx.KeyVariables.ClosureMap.Edges {
		from := edge.From
		to := edge.To
		symbol := edge.Symbol
		if ctx.Grammer.Terminals.Contains(symbol) {
			ctx.KeyVariables.ActionTable[from][symbol] = fmt.Sprintf("s%d", to)
		} else {
			ctx.KeyVariables.GotoTable[from][symbol] = to
		}
	}

	if ctx.SLR {
		conflict = ctx.GenerateSLRReduceAutomaton()
	} else {
		conflict = ctx.GenerateReduceAutomaton()
	}

	if conflict {
		ctx.shutdownPipeline(&LR0Result{
			Code: LR0_Error_Conflict,
		})
	}
	ctx.bury("GenerateAutomaton", -1)
}

func (ctx *LR0Context) GenerateSLRReduceAutomaton() bool {
	conflict := false
	for i, closure := range ctx.KeyVariables.ClosureMap.Closures {
		for _, item := range *closure {
			prod := ctx.KeyVariables.Productions[item.Prod]
			if len(prod) == item.Progress+1 {
				if prod[0] == ctx.Grammer.StartNonterminal {
					ctx.SetAutomatonReduce(i, "$", "acc")
				} else {
					reduceText := fmt.Sprintf("r%d", item.Prod)
					for terminal := range ctx.KeyVariables.FollowSet[prod[0]] {
						ctx.SetAutomatonReduce(i, terminal, reduceText)
					}
				}
			}
		}
	}
	return conflict
}

func (ctx *LR0Context) GenerateReduceAutomaton() bool {
	conflict := false
	for i, closure := range ctx.KeyVariables.ClosureMap.Closures {
		for _, item := range *closure {
			prod := ctx.KeyVariables.Productions[item.Prod]
			if len(prod) == item.Progress+1 {
				if prod[0] == ctx.Grammer.StartNonterminal {
					ctx.SetAutomatonReduce(i, "$", "acc")
				} else {
					reduceText := fmt.Sprintf("r%d", item.Prod)
					ctx.SetAutomatonReduce(i, "$", reduceText)
					for terminal := range ctx.Grammer.Terminals {
						ctx.SetAutomatonReduce(i, terminal, reduceText)
					}
				}
			}
		}
	}
	return conflict
}

func (ctx *LR0Context) SetAutomatonReduce(closure int, terminal string, reduce string) {
	if ctx.KeyVariables.ActionTable[closure][terminal] != "" {
		switch ctx.KeyVariables.ActionTable[closure][terminal][0] {
		case 'r':
		case 'a':
			ctx.KeyVariables.ActionTable[closure][terminal] = LR0_Action_ReduceReduce
		case 's':
			ctx.KeyVariables.ActionTable[closure][terminal] = LR0_Action_ShiftReduce
		}
		return
	}

	ctx.KeyVariables.ActionTable[closure][terminal] = reduce
}
