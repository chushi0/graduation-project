package process

import (
	"bufio"
	"fmt"
	"math/rand"
	"os"
	"sort"
	"strconv"
	"strings"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production"
	"github.com/chushi0/graduation_project/golang/startup/util/set"
)

type LR1Context struct {
	Context      *debug.DebugContext
	Grammer      *Grammer
	Code         string
	KeyVariables *LR1Variables
	LALR         bool
	CodeSaver
}

type LR1Variables struct {
	Productions   []production.Production `json:"productions"`
	LoopVariableI int                     `json:"loop_variable_i"`
	LoopVariableJ int                     `json:"loop_variable_j"`
	LoopVariableK int                     `json:"loop_variable_k"`
	ModifiedFlag  bool                    `json:"modified_flag"`
	Terminals     []string                `json:"terminals"`

	NonterminalOrders []string                 `json:"nonterminal_orders"`
	FirstSet          map[string]set.StringSet `json:"first"`

	ProcessedSymbol      set.StringSet `json:"process_symbol"`
	CurrentProcessSymbol string        `json:"current_symbol"`

	ClosureMap       *LR1ItemClosureMap `json:"closure_map"`
	CurrentClosure   *LR1ItemClosure    `json:"current_closure"`
	LALRDropClosures set.IntSet         `json:"lalr_drop_closures"`

	ActionTable []map[string]string `json:"action_table"`
	GotoTable   []map[string]int    `json:"goto_table"`

	CodePath string `json:"code_path"`
}

const (
	LR1_Action_ShiftReduce  = "移入-归约冲突"
	LR1_Action_ReduceReduce = "归约-归约冲突"
)

type LR1Result struct {
	Code      int           `json:"code"`
	Variables *LR1Variables `json:"variables"`
}

const (
	LR1_Success             = 0
	LR1_Error_ParseCode     = 1
	LR1_Error_NoStartSymbol = 2
	LR1_Error_Conflict      = 3
)

func NewLR1Context() *LR1Context {
	return &LR1Context{
		KeyVariables: &LR1Variables{},
	}
}

func (ctx *LR1Context) CreateLR1ProcessEntry() func(*debug.DebugContext) {
	return func(dc *debug.DebugContext) {
		ctx.Context = dc
		ctx.bury("start", 0)
		ctx.RunPipeline()
		ctx.shutdownPipeline(&LR1Result{
			Code: LL_Success,
		})
	}
}

func (ctx *LR1Context) bury(name string, line int) {
	ctx.Context.RunReport(&debug.Point{
		Name: name,
		Line: line,
	}, ctx.KeyVariables)
}

func (ctx *LR1Context) shutdownPipeline(res *LR1Result) {
	res.Variables = ctx.KeyVariables
	ctx.Context.ExitResult = res
	ctx.Context.SwitchRunMode(debug.RunMode_Exit)
	ctx.bury("shutdown", 0)
}

func (ctx *LR1Context) RunPipeline() {
	ctx.ParseCode()
	ctx.Translate()
	ctx.ComputeFirstSet()
	ctx.ComputeItemClosure()
	if ctx.LALR {
		// LALR：在 LR(1) 基础上合并相似的项目集闭包
		ctx.MergeItemClosure()
		// 清除多余的闭包
		ctx.ClearUnusedItemClosure()
	}
	ctx.GenerateAutomaton()
	if ctx.CodeSaver.Enable {
		ctx.GenerateYaccCode()
	}
}

// 按照一定顺序排列非终结符
func (ctx *LR1Context) sortNonterminals() {
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

func (ctx *LR1Context) ParseCode() {
	prods, start, errs := production.ParseProduction(ctx.Code, nil)
	if len(errs.Errors) > 0 {
		ctx.shutdownPipeline(&LR1Result{
			Code: LR0_Error_ParseCode,
		})
	}
	ctx.KeyVariables.Productions = prods
	ctx.Grammer = NewGrammer(prods)
	ctx.Grammer.StartNonterminal = start
	ctx.KeyVariables.Terminals = make([]string, 0, len(ctx.Grammer.Terminals))
	for terminal := range ctx.Grammer.Terminals {
		ctx.KeyVariables.Terminals = append(ctx.KeyVariables.Terminals, terminal)
	}
}

func (ctx *LR1Context) Translate() {
	ctx.bury("Translate", 0)
	if !ctx.Grammer.Nonterminals.Contains(ctx.Grammer.StartNonterminal) {
		ctx.shutdownPipeline(&LR1Result{
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

func (ctx *LR1Context) ComputeFirstSet() {
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

func (ctx *LR1Context) ComputeItemClosure() {
	ctx.KeyVariables.LoopVariableI = -1
	ctx.KeyVariables.LoopVariableJ = -1
	ctx.KeyVariables.LoopVariableK = -1
	ctx.bury("ComputeItemClosure", 0)
	ctx.KeyVariables.ClosureMap = NewLR1ItemClosureMap()
	// 开始符号
	for i, prod := range ctx.KeyVariables.Productions {
		if prod[0] == ctx.Grammer.StartNonterminal {
			ctx.ItemClosure([]LR1Item{{
				Prod:      i,
				Progress:  0,
				Lookahead: "$",
			}})
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
			prod := ctx.KeyVariables.Productions[item.Prod]
			if item.Progress+1 >= len(prod) {
				continue
			}
			symbol := prod[item.Progress+1]
			if ctx.KeyVariables.ProcessedSymbol.Contains(symbol) {
				continue
			}
			ctx.KeyVariables.CurrentProcessSymbol = symbol
			items := make([]LR1Item, 0)
			for _, it := range *closure {
				prod := ctx.KeyVariables.Productions[it.Prod]
				if it.Progress+1 >= len(prod) {
					continue
				}
				symbol := prod[it.Progress+1]
				if symbol == ctx.KeyVariables.CurrentProcessSymbol {
					items = append(items, LR1Item{
						Prod:      it.Prod,
						Progress:  it.Progress + 1,
						Lookahead: it.Lookahead,
					})
				}
			}
			to := ctx.ItemClosure(items)
			ctx.KeyVariables.ClosureMap.Edges = append(ctx.KeyVariables.ClosureMap.Edges, &LR1ItemClosureMapEdge{
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
func (ctx *LR1Context) ItemClosure(items []LR1Item) int {
	ctx.KeyVariables.CurrentClosure = NewLr1ItemClosure()
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
			first := ctx.subFirst(prod, item.Progress+2, item.Lookahead)
			for j, prod := range ctx.KeyVariables.Productions {
				if prod[0] == symbol {
					for lookahead := range first {
						ctx.KeyVariables.CurrentClosure.AddItem(LR1Item{
							Prod:      j,
							Progress:  0,
							Lookahead: lookahead,
						})
					}
				}
			}
		}
	}
	return ctx.KeyVariables.ClosureMap.FindClosureOrAppend(ctx.KeyVariables.CurrentClosure)
}

func (ctx *LR1Context) subFirst(prod []string, start int, lookahead string) set.StringSet {
	res := set.NewStringSet()
	for i := start; i < len(prod); i++ {
		symbol := prod[i]
		if ctx.Grammer.Terminals.Contains(symbol) {
			res.Put(symbol)
			return res
		}
		first := ctx.KeyVariables.FirstSet[symbol]
		res.UnionExcept(first, "")
		if !first.Contains("") {
			return res
		}
	}
	res.Put(lookahead)
	return res
}

func (ctx *LR1Context) MergeItemClosure() {
	ctx.KeyVariables.LALRDropClosures = set.NewIntSet()
	for i := 0; i < len(ctx.KeyVariables.ClosureMap.Closures); i++ {
		if ctx.KeyVariables.LALRDropClosures.Contains(i) {
			continue
		}
		// 寻找同心项目集
		sameClosures := set.NewIntSet()
		for j := i + 1; j < len(ctx.KeyVariables.ClosureMap.Closures); j++ {
			if ctx.isClosureSame(i, j) {
				sameClosures.Put(j)
			}
		}
		// 合并项目集
		for ano := range sameClosures {
			ctx.mergeItemClosure(i, ano)
			ctx.KeyVariables.LALRDropClosures.Put(ano)
			for _, edge := range ctx.KeyVariables.ClosureMap.Edges {
				if edge.From == ano {
					edge.From = i
				}
				if edge.To == ano {
					edge.To = i
				}
			}
		}
		// 清除重复的边
		for i := 0; i < len(ctx.KeyVariables.ClosureMap.Edges); i++ {
			for j := i + 1; j < len(ctx.KeyVariables.ClosureMap.Edges); j++ {
				if ctx.KeyVariables.ClosureMap.Edges[i] == ctx.KeyVariables.ClosureMap.Edges[j] {
					ctx.KeyVariables.ClosureMap.Edges = append(ctx.KeyVariables.ClosureMap.Edges[:j], ctx.KeyVariables.ClosureMap.Edges[j+1:]...)
				}
			}
		}
	}
}

func (ctx *LR1Context) isClosureSame(i, j int) bool {
	return ctx.isClosureContains(i, j) && ctx.isClosureContains(j, i)
}

func (ctx *LR1Context) isClosureContains(i, j int) bool {
	for _, item := range *ctx.KeyVariables.ClosureMap.Closures[i] {
		found := false
		for _, anoitem := range *ctx.KeyVariables.ClosureMap.Closures[j] {
			if item.Prod == anoitem.Prod && item.Progress == anoitem.Progress {
				found = true
				break
			}
		}
		if !found {
			return false
		}
	}
	return true
}

func (ctx *LR1Context) mergeItemClosure(dst, src int) {
	for _, item := range *ctx.KeyVariables.ClosureMap.Closures[src] {
		found := false
		for _, anoitem := range *ctx.KeyVariables.ClosureMap.Closures[dst] {
			if item.Prod == anoitem.Prod && item.Progress == anoitem.Progress && item.Lookahead == anoitem.Lookahead {
				found = true
				break
			}
		}
		if !found {
			ctx.KeyVariables.ClosureMap.Closures[dst].AddItem(*item)
		}
	}
}

func (ctx *LR1Context) ClearUnusedItemClosure() {
	unusedItemClosureId := make([]int, 0, len(ctx.KeyVariables.LALRDropClosures))
	for item := range ctx.KeyVariables.LALRDropClosures {
		unusedItemClosureId = append(unusedItemClosureId, item)
	}
	sort.Ints(unusedItemClosureId)
	for i := len(unusedItemClosureId) - 1; i >= 0; i-- {
		ctx.removeItemClosure(unusedItemClosureId[i])
	}
}

func (ctx *LR1Context) removeItemClosure(item int) {
	ctx.KeyVariables.ClosureMap.Closures = append(ctx.KeyVariables.ClosureMap.Closures[:item], ctx.KeyVariables.ClosureMap.Closures[item+1:]...)
	for i := 0; i < len(ctx.KeyVariables.ClosureMap.Edges); {
		edge := ctx.KeyVariables.ClosureMap.Edges[i]
		if edge.From == item || edge.To == item {
			ctx.KeyVariables.ClosureMap.Edges = append(ctx.KeyVariables.ClosureMap.Edges[:i], ctx.KeyVariables.ClosureMap.Edges[i+1:]...)
			continue
		}
		i++
		if edge.From > item {
			edge.From--
		}
		if edge.To > item {
			edge.To--
		}
	}
}

func (ctx *LR1Context) GenerateAutomaton() {
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
	ctx.sortNonterminals()
	ctx.bury("GenerateAutomaton", 0)

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

	for i, closure := range ctx.KeyVariables.ClosureMap.Closures {
		for _, item := range *closure {
			prod := ctx.KeyVariables.Productions[item.Prod]
			if len(prod) == item.Progress+1 {
				if prod[0] == ctx.Grammer.StartNonterminal {
					conflict = !ctx.SetAutomatonReduce(i, item.Lookahead, "acc") || conflict
				} else {
					conflict = !ctx.SetAutomatonReduce(i, item.Lookahead, fmt.Sprintf("r%d", item.Prod)) || conflict
				}
			}
		}
	}
	ctx.bury("GenerateAutomaton", -1)

	if conflict && (!ctx.CodeSaver.Enable || !*golang) {
		ctx.shutdownPipeline(&LR1Result{
			Code: LR1_Error_Conflict,
		})
	}
}

func (ctx *LR1Context) SetAutomatonReduce(closure int, terminal string, reduce string) bool {
	if ctx.KeyVariables.ActionTable[closure][terminal] != "" {
		switch ctx.KeyVariables.ActionTable[closure][terminal][0] {
		case 'r', 'a':
			ctx.KeyVariables.ActionTable[closure][terminal] = LR0_Action_ReduceReduce + " " + ctx.KeyVariables.ActionTable[closure][terminal]
		case 's':
			ctx.KeyVariables.ActionTable[closure][terminal] = LR0_Action_ShiftReduce + " " + ctx.KeyVariables.ActionTable[closure][terminal]
		}
		ctx.KeyVariables.ActionTable[closure][terminal] += "/" + reduce
		return false
	}

	ctx.KeyVariables.ActionTable[closure][terminal] = reduce
	return true
}

func (ctx *LR1Context) GenerateYaccCode() {
	if *golang {
		ctx.GenerateGolangYaccCode()
		return
	}
	ctx.KeyVariables.CodePath = ctx.GetSourcePath()
	serials := NewSerialTokens()
	prodSerials := make([]string, len(ctx.KeyVariables.Productions))
	headFile, _ := os.Create(ctx.GetHeaderPath())
	headBufWrite := bufio.NewWriter(headFile)
	cppFile, _ := os.Create(ctx.GetSourcePath())
	cppBufWrite := bufio.NewWriter(cppFile)
	defer func() {
		headBufWrite.Flush()
		headFile.Close()
		cppBufWrite.Flush()
		cppFile.Close()
	}()

	headBufWrite.WriteString(`#pragma once
/**
 * Auto-generate header file.
 * After you re-generate file, ALL YOUR CHANGE WILL BE LOST!
 */
`)
	// 随机命名空间
	randomNamespace := fmt.Sprintf("LR1_%d_%d", rand.Int(), rand.Int())
	headBufWrite.WriteString(fmt.Sprintf(`
// By modify this macro, you can define automaton code in namespace
// #define %s

#ifdef %s
namespace %s {
#endif
`, randomNamespace, randomNamespace, randomNamespace))

	// 写入原始代码
	headBufWrite.WriteString("\n\t// Production Definition Code\n")
	for _, line := range strings.Split(ctx.Code, "\n") {
		headBufWrite.WriteString(fmt.Sprintf("\t// %s\n", line))
	}

	// 写入终结符定义
	headBufWrite.WriteString(`
	// Terminals Definition
	constexpr int TerminalEOF = -1;
`)
	for terminal := range ctx.Grammer.Terminals {
		serials.Put(terminal)
		token := serials.Map[terminal]
		headBufWrite.WriteString(fmt.Sprintf("\tconstexpr int Terminal_%s = %d; // %s\n", token.SerialString, token.Index, terminal))
	}

	// 写入非终结符定义
	headBufWrite.WriteString(`
	// Nonterminals Definition
`)
	for nonterminal := range ctx.Grammer.Nonterminals {
		serials.Put(nonterminal)
		token := serials.Map[nonterminal]
		headBufWrite.WriteString(fmt.Sprintf("\tconstexpr int Nonterminal_%s = %d; // %s\n", token.SerialString, token.Index, nonterminal))
	}

	// 写入产生式定义
	headBufWrite.WriteString(`
	// Productions Definition
`)
	for i, prod := range ctx.KeyVariables.Productions {
		prodSerials[i] = serials.Map[prod[0]].SerialString
		for j := 1; j < len(prod); j++ {
			prodSerials[i] += "_" + serials.Map[prod[j]].SerialString
		}
	}
	for i, prod := range ctx.KeyVariables.Productions {
		headBufWrite.WriteString(fmt.Sprintf("\tconstexpr int Production_%s = %d; // ", prodSerials[i], i))
		headBufWrite.WriteString(prod[0])
		headBufWrite.WriteString(" :=")
		for j := 1; j < len(prod); j++ {
			headBufWrite.WriteString(" ")
			headBufWrite.WriteString(prod[j])
		}
		headBufWrite.WriteString("\n")
	}

	// 其他结构定义
	headBufWrite.WriteString(`
	// Compile Error (will throw by parser function)
	struct CompileError {
		// an array contains terminal's id what parser expect
		int* Expected;
		// size of Expected
		int ExpectedSize;
		// parser read actually
		int Actual;
	};

	// Interface which can be easily defined by user
	class IParser {
	public:
		// Read a terminal from lexer. If no more terminals, return TerminalEOF(-1)
		virtual int Next() = 0;
		// Reduce by production
		virtual void Reduce(int id) = 0;
		// Error occurred when parse (parser will exit)
		virtual void Panic(CompileError* error) = 0;
	};

	// main entry parser
	// true means success, false means fail (after calling IParser.Panic)
	bool Parse(IParser* parser);
`)

	headBufWrite.WriteString(fmt.Sprintf(`
#ifdef %s
}
#endif
`, randomNamespace))

	cppBufWrite.WriteString(fmt.Sprintf(`
/**
 * Auto-generate file. DO NOT MODIFY.
 * all your change WILL BE LOST after you re-generate file.
 */
#include "%s"
#include <vector>
`, ctx.GetIncludeName()))
	cppBufWrite.WriteString(fmt.Sprintf(`
#ifdef %s
using namespace %s;
#endif

#ifdef %s
bool %s::Parse(IParser* parser)
#else
bool Parse(IParser* parser)
#endif
`, randomNamespace, randomNamespace, randomNamespace, randomNamespace))
	cppBufWrite.WriteString(`{
	std::vector<int> stateStack;
	std::vector<int> symbolStack;
	stateStack.push_back(0);
	int token = parser->Next();
	while (true) {
		switch (stateStack.back()) {
`)
	for state := range ctx.KeyVariables.ClosureMap.Closures {
		cppBufWrite.WriteString(fmt.Sprintf("\t\t\tcase %d:\n", state))
		cppBufWrite.WriteString("\t\t\t\tswitch (token) {\n")
		allSelect := make([]string, 0)
		// action
		for terminal, action := range ctx.KeyVariables.ActionTable[state] {
			if action == "" {
				continue
			}
			if terminal == "$" {
				allSelect = append(allSelect, "TerminalEOF")
				cppBufWrite.WriteString("\t\t\t\t\tcase TerminalEOF:\n")
			} else {
				allSelect = append(allSelect, fmt.Sprintf("Terminal_%s", serials.Map[terminal].SerialString))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\tcase Terminal_%s:\n", serials.Map[terminal].SerialString))
			}
			cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\t// %s\n", action))
			switch action[0] {
			case 'a': // acc
				// 归约开始产生式
				prod := (*ctx.KeyVariables.ClosureMap.Closures[0])[0].Prod
				count := len(ctx.KeyVariables.Productions[prod]) - 1
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tparser->Reduce(%d);\n", prod))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tstateStack.erase(stateStack.end() - %d, stateStack.end());\n", count))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tsymbolStack.erase(symbolStack.end() - %d, symbolStack.end());\n", count))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tsymbolStack.push_back(Nonterminal_%s);\n", serials.Map[ctx.KeyVariables.Productions[prod][0]].SerialString))
				cppBufWrite.WriteString("\t\t\t\t\t\treturn true;\n")
			case 's':
				// 移入，并转移到其他状态
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tstateStack.push_back(%s);\n", action[1:]))
				cppBufWrite.WriteString("\t\t\t\t\t\tsymbolStack.push_back(token);\n")
				cppBufWrite.WriteString("\t\t\t\t\t\ttoken = parser->Next();\n")
				cppBufWrite.WriteString("\t\t\t\t\t\tcontinue;\n")
			case 'r':
				// 归约
				prod, _ := strconv.Atoi(action[1:])
				count := len(ctx.KeyVariables.Productions[prod]) - 1
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tparser->Reduce(%s);\n", action[1:]))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tstateStack.erase(stateStack.end() - %d, stateStack.end());\n", count))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tsymbolStack.erase(symbolStack.end() - %d, symbolStack.end());\n", count))
				cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tsymbolStack.push_back(Nonterminal_%s);\n", serials.Map[ctx.KeyVariables.Productions[prod][0]].SerialString))
				cppBufWrite.WriteString("\t\t\t\t\t\tbreak;\n")
			}
		}
		cppBufWrite.WriteString("\t\t\t\t\tdefault: {\n\t\t\t\t\t\tint expected[] = {")
		for i, v := range allSelect {
			if i > 0 {
				cppBufWrite.WriteString(", ")
			}
			cppBufWrite.WriteString(v)
		}
		cppBufWrite.WriteString(`};
						CompileError compileError = {
							expected, sizeof(expected) / sizeof(int), token
						};
						parser->Panic(&compileError);
						return false;
					}
				}
				break;
`)
	}
	cppBufWrite.WriteString("\t\t}\n")
	cppBufWrite.WriteString("\t\tswitch (stateStack.back()) {\n")
	for state := range ctx.KeyVariables.ClosureMap.Closures {
		if !ctx.checkGotoTableAvailable(state) {
			continue
		}
		cppBufWrite.WriteString(fmt.Sprintf("\t\t\tcase %d:\n", state))
		cppBufWrite.WriteString("\t\t\t\tswitch (symbolStack.back()) {\n")
		allSelect := make([]string, 0)
		// goto
		for terminal, nextState := range ctx.KeyVariables.GotoTable[state] {
			if nextState == -1 {
				continue
			}
			allSelect = append(allSelect, fmt.Sprintf("Nonterminal_%s", serials.Map[terminal].SerialString))
			cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\tcase Nonterminal_%s:\n", serials.Map[terminal].SerialString))
			cppBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tstateStack.push_back(%d);\n", nextState))
			cppBufWrite.WriteString("\t\t\t\t\t\tbreak;\n")
		}
		cppBufWrite.WriteString("\t\t\t\t\tdefault: {\n\t\t\t\t\t\tint expected[] = {")
		for i, v := range allSelect {
			if i > 0 {
				cppBufWrite.WriteString(", ")
			}
			cppBufWrite.WriteString(v)
		}
		cppBufWrite.WriteString(`};
						CompileError compileError = {
							expected, sizeof(expected) / sizeof(int), stateStack.back()
						};
						parser->Panic(&compileError);
						return false;
					}
				}
				break;
`)
	}
	cppBufWrite.WriteString(`			default: {
				int expected[] = {};
				CompileError compileError = {
					expected, 0, stateStack.back()
				};
				parser->Panic(&compileError);
				return false;
			}
		}
	}
}`)
}

func (ctx *LR1Context) GenerateGolangYaccCode() {
	ctx.KeyVariables.CodePath = ctx.GetGolangPath()
	serials := NewSerialTokens()
	prodSerials := make([]string, len(ctx.KeyVariables.Productions))
	goFile, _ := os.Create(ctx.GetGolangPath())
	goBufWrite := bufio.NewWriter(goFile)
	defer func() {
		goBufWrite.Flush()
		goFile.Close()
	}()

	goBufWrite.WriteString(`

type YaccCompileError struct {
	Exected []int
	Actual  int
}

type IYaccParser interface {
	Next() int
	Shift()
	Reduce(int)
	Panic(*YaccCompileError)
}

`)

	goBufWrite.WriteString("const (")

	// 写入原始代码
	goBufWrite.WriteString("\n\t// Production Definition Code\n")
	for _, line := range strings.Split(ctx.Code, "\n") {
		goBufWrite.WriteString(fmt.Sprintf("\t// %s\n", line))
	}

	// 写入终结符定义
	goBufWrite.WriteString(`
	// Terminals Definition
	TerminalEOF = -1
`)
	for terminal := range ctx.Grammer.Terminals {
		serials.Put(terminal)
		token := serials.Map[terminal]
		goBufWrite.WriteString(fmt.Sprintf("\tTerminal_%s = %d // %s\n", token.SerialString, token.Index, terminal))
	}

	// 写入非终结符定义
	goBufWrite.WriteString(`
	// Nonterminals Definition
`)
	for nonterminal := range ctx.Grammer.Nonterminals {
		serials.Put(nonterminal)
		token := serials.Map[nonterminal]
		goBufWrite.WriteString(fmt.Sprintf("\tNonterminal_%s = %d // %s\n", token.SerialString, token.Index, nonterminal))
	}

	// 写入产生式定义
	goBufWrite.WriteString(`
	// Productions Definition
`)
	for i, prod := range ctx.KeyVariables.Productions {
		prodSerials[i] = serials.Map[prod[0]].SerialString
		for j := 1; j < len(prod); j++ {
			prodSerials[i] += "_" + serials.Map[prod[j]].SerialString
		}
	}
	for i, prod := range ctx.KeyVariables.Productions {
		goBufWrite.WriteString(fmt.Sprintf("\tProduction_%s = %d // ", prodSerials[i], i))
		goBufWrite.WriteString(prod[0])
		goBufWrite.WriteString(" :=")
		for j := 1; j < len(prod); j++ {
			goBufWrite.WriteString(" ")
			goBufWrite.WriteString(prod[j])
		}
		goBufWrite.WriteString("\n")
	}
	goBufWrite.WriteString(")\n")

	goBufWrite.WriteString(`
func ProductionRightCount(production int) int {
	switch production {
`)
	for i, prod := range ctx.KeyVariables.Productions {
		goBufWrite.WriteString(fmt.Sprintf("\t\tcase Production_%s:\n", prodSerials[i]))
		goBufWrite.WriteString(fmt.Sprintf("\t\t\treturn %d\n", len(prod)-1))
	}
	goBufWrite.WriteString(`	}
	return -1
}
`)

	goBufWrite.WriteString(`
func Parse(parser IYaccParser) (bool, *YaccCompileError) {
	stateStack := make([]int, 0)
	symbolStack := make([]int, 0)
	stateStack = append(stateStack, 0)
	token := parser.Next()
	var res int
	var err *YaccCompileError
	for {
		switch stateStack[len(stateStack)-1] {
`)
	for state := range ctx.KeyVariables.ClosureMap.Closures {
		goBufWrite.WriteString(fmt.Sprintf("\t\t\tcase %d:\n", state))
		goBufWrite.WriteString(fmt.Sprintf("\t\t\t\tres, err = yacc_action_%d(&stateStack, &symbolStack, &token, parser)\n", state))
	}
	goBufWrite.WriteString("\t\t}\n")
	goBufWrite.WriteString(`
			if res == 0 {
				continue
			} else if res == 1 {
				return true, nil
			} else if res == 2 {
				return false, err
			}
`)
	goBufWrite.WriteString("\t\tswitch stateStack[len(stateStack)-1] {\n")
	for state := range ctx.KeyVariables.ClosureMap.Closures {
		if !ctx.checkGotoTableAvailable(state) {
			continue
		}
		goBufWrite.WriteString(fmt.Sprintf("\t\t\tcase %d:\n", state))
		goBufWrite.WriteString(fmt.Sprintf("\t\t\t\tres, err = yacc_goto_%d(symbolStack[len(symbolStack)-1])\n", state))

	}
	goBufWrite.WriteString(`			default: {
			expected := []int{}
			compileError := YaccCompileError{
				expected, stateStack[len(stateStack)-1],
			}
			parser.Panic(&compileError)
			return false, &compileError
		}
}`)
	goBufWrite.WriteString(`
			if err != nil {
				parser.Panic(err)
				return false, err
			}
			stateStack = append(stateStack, res)
		}
	}
`)
	for state, closure := range ctx.KeyVariables.ClosureMap.Closures {
		goBufWrite.WriteString(fmt.Sprintf("\nfunc yacc_action_%d(stateStack, symbolStack *[]int, token *int, parser IYaccParser) (int, *YaccCompileError) {\n", state))
		if *comment {
			for _, item := range *closure {
				prod := ctx.KeyVariables.Productions[item.Prod]
				str := prod[0] + " :="
				for i := 1; i < len(prod); i++ {
					if item.Progress+1 == i {
						str += " ·"
					}
					str += " " + prod[i]
				}
				if item.Progress+1 == len(prod) {
					str += " ·"
				}
				str += " , " + item.Lookahead
				goBufWrite.WriteString(fmt.Sprintf("\t// %s\n", str))
			}
		}
		goBufWrite.WriteString("\t\t\t\tswitch *token {\n")
		allSelect := make([]string, 0)
		// action
		for terminal, action := range ctx.KeyVariables.ActionTable[state] {
			if action == "" {
				continue
			}
			if terminal == "$" {
				allSelect = append(allSelect, "TerminalEOF")
				goBufWrite.WriteString("\t\t\t\t\tcase TerminalEOF:\n")
			} else {
				allSelect = append(allSelect, fmt.Sprintf("Terminal_%s", serials.Map[terminal].SerialString))
				goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\tcase Terminal_%s:\n", serials.Map[terminal].SerialString))
			}
			goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\t// %s\n", action))
			switch action[0] {
			case 'a': // acc
				// 归约开始产生式
				prod := (*ctx.KeyVariables.ClosureMap.Closures[0])[0].Prod
				goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tparser.Reduce(%d)\n", prod))
				goBufWrite.WriteString("\t\t\t\t\t\treturn 1, nil\n")
			case 's':
				// 移入，并转移到其他状态
				goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\t*stateStack = append(*stateStack, %s)\n", action[1:]))
				goBufWrite.WriteString("\t\t\t\t\t\t*symbolStack = append(*symbolStack, *token)\n")
				goBufWrite.WriteString("\t\t\t\t\t\tparser.Shift()\n")
				goBufWrite.WriteString("\t\t\t\t\t\t*token = parser.Next()\n")
				goBufWrite.WriteString("\t\t\t\t\t\treturn 0, nil\n")
			case 'r':
				// 归约
				prod, _ := strconv.Atoi(action[1:])
				count := len(ctx.KeyVariables.Productions[prod]) - 1
				goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tparser.Reduce(%s)\n", action[1:]))
				if count != 0 {
					goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\t*stateStack = (*stateStack)[:len(*stateStack)-%d]\n", count))
					goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\t*symbolStack = (*symbolStack)[:len(*symbolStack)-%d]\n", count))
				}
				goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\t*symbolStack = append(*symbolStack, Nonterminal_%s)\n", serials.Map[ctx.KeyVariables.Productions[prod][0]].SerialString))
				goBufWrite.WriteString("\t\t\t\t\t\treturn 3, nil\n")
			default:
				goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\tpanic(\"%s\")\n", action))
			}
		}
		goBufWrite.WriteString("\t\t\t\t\tdefault: {\n\t\t\t\t\t\texpected := []int{")
		for i, v := range allSelect {
			if i > 0 {
				goBufWrite.WriteString(", ")
			}
			goBufWrite.WriteString(v)
		}
		goBufWrite.WriteString(`}
			compileError := YaccCompileError{
				expected, *token,
			}
			parser.Panic(&compileError)
			return 2, &compileError
		}
	}
}
`)
	}
	for state := range ctx.KeyVariables.ClosureMap.Closures {
		if !ctx.checkGotoTableAvailable(state) {
			continue
		}
		goBufWrite.WriteString(fmt.Sprintf("\nfunc yacc_goto_%d(symbol int) (int, *YaccCompileError) {\n", state))
		goBufWrite.WriteString("\t\t\t\tswitch symbol {\n")
		allSelect := make([]string, 0)
		// goto
		for terminal, nextState := range ctx.KeyVariables.GotoTable[state] {
			if nextState == -1 {
				continue
			}
			allSelect = append(allSelect, fmt.Sprintf("Nonterminal_%s", serials.Map[terminal].SerialString))
			goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\tcase Nonterminal_%s:\n", serials.Map[terminal].SerialString))
			goBufWrite.WriteString(fmt.Sprintf("\t\t\t\t\t\treturn %d, nil\n", nextState))
		}
		goBufWrite.WriteString("\t\t\t\t\tdefault: {\n\t\t\t\t\t\texpected := []int{")
		for i, v := range allSelect {
			if i > 0 {
				goBufWrite.WriteString(", ")
			}
			goBufWrite.WriteString(v)
		}
		goBufWrite.WriteString(`}
				compileError := YaccCompileError{
					expected, symbol,
				}
				return 0, &compileError
			}
		}
	}
`)
	}
}

func (ctx *LR1Context) checkGotoTableAvailable(state int) bool {
	for _, nextState := range ctx.KeyVariables.GotoTable[state] {
		if nextState != -1 {
			return true
		}
	}
	return false
}
