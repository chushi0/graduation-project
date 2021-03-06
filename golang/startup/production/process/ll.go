package process

import (
	"bufio"
	"fmt"
	"math/rand"
	"os"
	"sort"
	"strings"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production"
	"github.com/chushi0/graduation_project/golang/startup/util/set"
	utilslice "github.com/chushi0/graduation_project/golang/startup/util/util_slice"
)

// LL 处理
type LLContext struct {
	Context       *debug.DebugContext
	Grammer       *Grammer
	Code          string
	WithTranslate bool
	KeyVariables  *LLKeyVariables
	CodeSaver
}

type LLKeyVariables struct {
	Productions   []production.Production `json:"productions"`
	LoopVariableI int                     `json:"loop_variable_i"`
	LoopVariableJ int                     `json:"loop_variable_j"`
	LoopVariableK int                     `json:"loop_variable_k"`
	ModifiedFlag  bool                    `json:"modified_flag"`
	Terminals     []string                `json:"terminals"`

	NonterminalOrders        []string                `json:"nonterminal_orders"`
	CurrentProcessProduction production.Production   `json:"current_process_production"`
	RemoveProduction         []production.Production `json:"remove_production"`
	AddProduction            []production.Production `json:"add_production"`
	ReplaceProduction        []ReplaceProduction     `json:"replace_production"`

	CommonPrefix []string `json:"common_prefix"`

	FirstSet  map[string]set.StringSet `json:"first"`
	FollowSet map[string]set.StringSet `json:"follow"`
	SelectSet []set.StringSet          `json:"select"`

	Automaton map[string]map[string]int `json:"automaton"`

	CodePath string `json:"code_path"`
}

type LLResult struct {
	Code      int             `json:"code"`
	Variables *LLKeyVariables `json:"variables"`
}

type ReplaceProduction struct {
	Original production.Production `json:"original"`
	Replace  production.Production `json:"replace"`
}

const (
	LL_Success              = 0
	LL_Error_ParseCode      = 1
	LL_Error_SelectConflict = 2
)

func NewLLContext() *LLContext {
	return &LLContext{
		KeyVariables: &LLKeyVariables{},
	}
}

func (ctx *LLContext) CreateLLProcessEntry() func(*debug.DebugContext) {
	return func(dc *debug.DebugContext) {
		ctx.Context = dc
		ctx.bury("start", 0)
		ctx.RunPipeline()
		ctx.shutdownPipeline(&LLResult{
			Code: LL_Success,
		})
	}
}

func (ctx *LLContext) bury(name string, line int) {
	ctx.Context.RunReport(&debug.Point{
		Name: name,
		Line: line,
	}, ctx.KeyVariables)
}

func (ctx *LLContext) shutdownPipeline(res *LLResult) {
	res.Variables = ctx.KeyVariables
	ctx.Context.ExitResult = res
	ctx.Context.SwitchRunMode(debug.RunMode_Exit)
	ctx.bury("shutdown", 0)
}

func (ctx *LLContext) RunPipeline() {
	// 准备：解析产生式代码
	ctx.Prepare()
	// 转换：去除左递归、提取公共前缀
	ctx.Translate()
	// 计算：计算First集、计算Follow集、计算Select集
	ctx.Calculate()
	// 生成：检查Select集冲突、生成自动机、生成自动机代码
	ctx.Build()
}

func (ctx *LLContext) Prepare() {
	// 解析产生式代码
	ctx.ParseCode()
}

func (ctx *LLContext) Translate() {
	if ctx.WithTranslate {
		// 去除左递归
		ctx.RemoveLeftRecusion()
		// 提取公共前缀
		ctx.ExtractCommonPrefix()
	}
}

func (ctx *LLContext) Calculate() {
	// 计算First集
	ctx.ComputeFirstSet()
	// 计算Follow集
	ctx.ComputeFollowSet()
	// 计算Select集
	ctx.ComputeSelectSet()
}

func (ctx *LLContext) Build() {
	// 生成自动机
	ctx.GenerateAutomaton()
	if ctx.CodeSaver.Enable {
		ctx.GenerateYaccCode()
	}
}

func (ctx *LLContext) ParseCode() {
	prods, start, errs := production.ParseProduction(ctx.Code, nil)
	if len(errs.Errors) > 0 {
		ctx.shutdownPipeline(&LLResult{
			Code: LL_Error_ParseCode,
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

/**
 * 算法步骤：
 * 按照一定顺序排列非终结符 A[1]...A[n]
 * 循环 i 从 1 到 n
 *     循环 j 从 1 到 i-1
 *         将 A[i]->A[j]γ 替换为 A[i]->δγ，其中A[j]->δ
 *     清除A[i]的直接左递归
 */
func (ctx *LLContext) RemoveLeftRecusion() {
	ctx.bury("RemoveLeftRecusion", 0)
	ctx.sortNonterminals()
	ctx.bury("RemoveLeftRecusion", 1)

	// 循环 i 从 1 到 n
	ctx.KeyVariables.LoopVariableI = 1
	for {
		ctx.bury("RemoveLeftRecusion", 2)
		if ctx.KeyVariables.LoopVariableI > len(ctx.KeyVariables.NonterminalOrders) {
			break
		}
		A := ctx.KeyVariables.NonterminalOrders[ctx.KeyVariables.LoopVariableI-1]

		// 循环 j 从 1 到 i-1
		ctx.KeyVariables.LoopVariableJ = 1
		for {
			ctx.bury("RemoveLeftRecusion", 3)
			if ctx.KeyVariables.LoopVariableJ >= ctx.KeyVariables.LoopVariableI {
				break
			}
			B := ctx.KeyVariables.NonterminalOrders[ctx.KeyVariables.LoopVariableJ-1]

			// 将 A[i]->A[j]γ 替换为 A[i]->δγ，其中A[j]->δ
			dumpProductions := utilslice.CopyStringSlice(ctx.KeyVariables.Productions)
			for _, prod := range dumpProductions {
				if prod[0] != A {
					continue
				}
				if len(prod) > 1 && prod[1] == B {
					ctx.KeyVariables.RemoveProduction = make([][]string, 0)
					ctx.KeyVariables.AddProduction = make([][]string, 0)
					ctx.KeyVariables.RemoveProduction = append(ctx.KeyVariables.RemoveProduction, prod)
					for _, bprod := range ctx.Grammer.Productions[B] {
						newProd := make([]string, 0)
						newProd = append(newProd, A)
						newProd = append(newProd, bprod[1:]...)
						newProd = append(newProd, prod[2:]...)
						ctx.KeyVariables.AddProduction = append(ctx.KeyVariables.AddProduction, newProd)
					}
					ctx.bury("RemoveLeftRecusion", 5)
					ctx.Grammer.RemoveProduction(ctx.KeyVariables.RemoveProduction[0])
					for _, prod := range ctx.KeyVariables.AddProduction {
						ctx.Grammer.AddNewProduction(prod)
					}
					ctx.KeyVariables.Productions = utilslice.ReplaceStringArray(ctx.KeyVariables.Productions, prod, ctx.KeyVariables.AddProduction...)
				}
				ctx.KeyVariables.RemoveProduction = nil
				ctx.KeyVariables.AddProduction = nil
			}

			ctx.KeyVariables.LoopVariableJ++
		}
		ctx.bury("RemoveLeftRecusion", 6)
		ctx.KeyVariables.LoopVariableJ = 0

		// 清除A[i]的直接左递归
		skip := true
		for _, prod := range ctx.Grammer.Productions[A] {
			if len(prod) > 1 && prod[1] == A {
				skip = false
				break
			}
		}

		if skip {
			ctx.KeyVariables.LoopVariableI++
			continue
		}

		newNonterminal := ctx.Grammer.AddNewNonterminal(A)
		ctx.KeyVariables.AddProduction = make([][]string, 0)
		ctx.KeyVariables.AddProduction = append(ctx.KeyVariables.AddProduction, []string{newNonterminal})
		ctx.KeyVariables.ReplaceProduction = make([]ReplaceProduction, 0)
		for _, prod := range utilslice.CopyStringSlice(ctx.KeyVariables.Productions) {
			if prod[0] != A {
				continue
			}

			if len(prod) > 1 && prod[1] == A {
				// 非终结符新的产生式
				newProd := []string{newNonterminal}
				newProd = append(newProd, prod[2:]...)
				newProd = append(newProd, newNonterminal)
				ctx.KeyVariables.ReplaceProduction = append(ctx.KeyVariables.ReplaceProduction, ReplaceProduction{
					Original: prod,
					Replace:  newProd,
				})
			} else {
				newProd := append(prod, newNonterminal)
				ctx.KeyVariables.ReplaceProduction = append(ctx.KeyVariables.ReplaceProduction, ReplaceProduction{
					Original: prod,
					Replace:  newProd,
				})
			}
		}
		ctx.bury("RemoveLeftRecusion", 9)
		ctx.Grammer.AddNewProduction([]string{newNonterminal})
		ctx.insertProductionBeforeNonterminal(A, []string{newNonterminal})
		for _, rp := range ctx.KeyVariables.ReplaceProduction {
			ctx.KeyVariables.Productions = utilslice.ReplaceStringArray(ctx.KeyVariables.Productions, rp.Original, rp.Replace)
			ctx.Grammer.RemoveProduction(rp.Original)
			ctx.Grammer.AddNewProduction(rp.Replace)
		}
		ctx.KeyVariables.AddProduction = nil
		ctx.KeyVariables.ReplaceProduction = nil

		ctx.KeyVariables.LoopVariableI++
	}

	ctx.KeyVariables.LoopVariableI = 0
	ctx.bury("RemoveLeftRecusion", -1)
}

/**
 * 算法步骤：
 * 按照一定顺序排列非终结符 A[1]...A[n]
 * 循环 i 从 1 到 n
 *     遍历查找最短的公共前缀
 *         提出公共前缀
 */
func (ctx *LLContext) ExtractCommonPrefix() {
	ctx.bury("ExtractCommonPrefix", 0)
	ctx.sortNonterminals()
	ctx.bury("ExtractCommonPrefix", 1)

	// 循环
	ctx.KeyVariables.LoopVariableI = 1
	for {
		ctx.bury("ExtractCommonPrefix", 2)
		if ctx.KeyVariables.LoopVariableI > len(ctx.KeyVariables.NonterminalOrders) {
			break
		}

		nonterminal := ctx.KeyVariables.NonterminalOrders[ctx.KeyVariables.LoopVariableI-1]
		for {
			prefixes := set.NewStringSet()
			commonPrefix := make([]string, 0)
			for _, prod := range ctx.Grammer.Productions[nonterminal] {
				if len(prod) == 1 {
					continue
				}
				prefix := prod[1]
				if prefixes.Contains(prefix) {
					commonPrefix = append(commonPrefix, prod[1:]...)
					break
				}
				prefixes.Put(prefix)
			}

			if len(commonPrefix) == 0 {
				break
			}

			processProds := make([]production.Production, 0)
			for _, prod := range ctx.Grammer.Productions[nonterminal] {
				if len(prod) == 1 || prod[1] != commonPrefix[0] {
					continue
				}
				processProds = append(processProds, prod)

				for i := 0; i < len(commonPrefix) && i < len(prod)-1; i++ {
					if commonPrefix[i] != prod[i+1] {
						commonPrefix = commonPrefix[:i]
						break
					}
				}
			}

			ctx.KeyVariables.CommonPrefix = commonPrefix
			ctx.bury("ExtractCommonPrefix", 3)

			ctx.KeyVariables.AddProduction = make([][]string, 0)
			ctx.KeyVariables.ReplaceProduction = make([]ReplaceProduction, 0)
			newNonterminal := ctx.Grammer.AddNewNonterminal(nonterminal)
			newProd := []string{nonterminal}
			newProd = append(newProd, commonPrefix...)
			newProd = append(newProd, newNonterminal)
			ctx.Grammer.AddNewProduction(newProd)
			ctx.KeyVariables.AddProduction = append(ctx.KeyVariables.AddProduction, newProd)
			ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, newNonterminal)
			for _, prod := range processProds {
				ctx.Grammer.RemoveProduction(prod)
				newProd := []string{newNonterminal}
				newProd = append(newProd, prod[len(commonPrefix)+1:]...)
				ctx.Grammer.AddNewProduction(newProd)
				ctx.KeyVariables.ReplaceProduction = append(ctx.KeyVariables.ReplaceProduction, ReplaceProduction{
					Original: prod,
					Replace:  newProd,
				})
			}

			ctx.bury("ExtractCommonPrefix", 4)
			ctx.KeyVariables.CommonPrefix = nil
			ctx.insertProductionBeforeNonterminal(nonterminal, newProd)
			for _, rp := range ctx.KeyVariables.ReplaceProduction {
				if rp.Replace[0] == newNonterminal {
					ctx.KeyVariables.Productions = append(utilslice.ReplaceStringArray(ctx.KeyVariables.Productions, rp.Original), rp.Replace)
				} else {
					ctx.KeyVariables.Productions = utilslice.ReplaceStringArray(ctx.KeyVariables.Productions, rp.Original, rp.Replace)
				}
			}
			ctx.KeyVariables.AddProduction = nil
			ctx.KeyVariables.ReplaceProduction = nil
		}

		ctx.KeyVariables.LoopVariableI++
	}

	ctx.bury("ExtractCommonPrefix", -1)
}

func (ctx *LLContext) ComputeFirstSet() {
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

func (ctx *LLContext) ComputeFollowSet() {
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

func (ctx *LLContext) ComputeSelectSet() {
	// 初始化 Select 集
	ctx.KeyVariables.SelectSet = make([]set.StringSet, len(ctx.KeyVariables.Productions))
	for i := range ctx.KeyVariables.Productions {
		ctx.KeyVariables.SelectSet[i] = set.NewStringSet()
	}

	ctx.KeyVariables.LoopVariableI = -1
	ctx.KeyVariables.LoopVariableJ = -1
	ctx.KeyVariables.LoopVariableK = -1
	ctx.bury("ComputeSelectSet", 0)

	ctx.KeyVariables.LoopVariableI = 0
	for {
		ctx.bury("ComputeSelectSet", 1)
		if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.Productions) {
			break
		}
		prod := ctx.KeyVariables.Productions[ctx.KeyVariables.LoopVariableI]

		ctx.KeyVariables.LoopVariableJ = 1
		for {
			ctx.bury("ComputeSelectSet", 2)
			ctx.bury("ComputeSelectSet", 3)
			if ctx.KeyVariables.LoopVariableJ >= len(prod) {
				ctx.KeyVariables.SelectSet[ctx.KeyVariables.LoopVariableI].UnionExcept(ctx.KeyVariables.FollowSet[prod[0]])
				break
			}

			ctx.bury("ComputeSelectSet", 4)
			cur := prod[ctx.KeyVariables.LoopVariableJ]
			if ctx.Grammer.Terminals.Contains(cur) {
				ctx.KeyVariables.SelectSet[ctx.KeyVariables.LoopVariableI].Put(cur)
				ctx.bury("ComputeSelectSet", 5)
				break
			}
			ctx.KeyVariables.SelectSet[ctx.KeyVariables.LoopVariableI].UnionExcept(
				ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]], "",
			)
			ctx.bury("ComputeSelectSet", 5)
			if !ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]].Contains("") {
				break
			}
			ctx.bury("ComputeSelectSet", 6)
			ctx.KeyVariables.LoopVariableJ++
		}

		ctx.KeyVariables.LoopVariableJ = -1
		ctx.KeyVariables.LoopVariableI++
	}

	ctx.bury("ComputeSelectSet", -1)
}

func (ctx *LLContext) GenerateAutomaton() {
	conflict := false
	ctx.KeyVariables.Automaton = make(map[string]map[string]int)
	for nonterminal := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.Automaton[nonterminal] = make(map[string]int)
		ctx.KeyVariables.Automaton[nonterminal]["$"] = -1
		for terminal := range ctx.Grammer.Terminals {
			ctx.KeyVariables.Automaton[nonterminal][terminal] = -1
		}
	}
	ctx.bury("GenerateAutomaton", 0)

	for i, prod := range ctx.KeyVariables.Productions {
		selectSet := ctx.KeyVariables.SelectSet[i]
		nonterminal := prod[0]
		for terminal := range selectSet {
			if ctx.KeyVariables.Automaton[nonterminal][terminal] == -1 {
				ctx.KeyVariables.Automaton[nonterminal][terminal] = i
			} else {
				ctx.KeyVariables.Automaton[nonterminal][terminal] = -2
				conflict = true
			}
		}
	}
	ctx.bury("GenerateAutomaton", -1)
	if conflict {
		ctx.shutdownPipeline(&LLResult{
			Code: LL_Error_SelectConflict,
		})
	}
}

func (ctx *LLContext) GenerateYaccCode() {
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

	// 头文件

	headBufWrite.WriteString(`#pragma once
/**
 * Auto-generate header file.
 * After you re-generate file, ALL YOUR CHANGE WILL BE LOST!
 */

 #include <vector>
`)
	// 随机命名空间
	randomNamespace := fmt.Sprintf("LL_%d_%d", rand.Int(), rand.Int())
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

	// ParseContext
	struct ParserContext {
		// current token
		int *token;
		// symbol stack
		std::vector<int> *stack;
	};

	// Interface which can be easily defined by user
	class IParser {
	public:
		// Read a terminal from lexer. If no more terminals, return TerminalEOF(-1)
		virtual int Next() = 0;
		// Infer left nonterminal by production
		virtual void Infer(int id) = 0;
		// Error occurred when parse
		// return true means panic has been recovered;
		// return false means panic cannot be recovered (parse function will return)
		virtual bool Panic(ParserContext *ctx, CompileError* error) = 0;
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

	// 实现文件

	cppBufWrite.WriteString(fmt.Sprintf(`
/**
 * Auto-generate file. DO NOT MODIFY.
 * all your change WILL BE LOST after you re-generate file.
 */
#include "%s"
`, ctx.GetIncludeName()))
	cppBufWrite.WriteString(fmt.Sprintf(`

#ifdef %s
namespace %s {
#endif
`, randomNamespace, randomNamespace))

	for nonterminal := range ctx.Grammer.Nonterminals {
		cppBufWrite.WriteString(fmt.Sprintf(`
inline bool parse_%s(ParserContext* ctx, IParser* parser, bool *result) {
	switch (*ctx->token) {
`, serials.Map[nonterminal].SerialString))
		allSelect := make([]string, 0)
		for i, prod := range ctx.KeyVariables.Productions {
			if prod[0] != nonterminal {
				continue
			}
			for terminal := range ctx.KeyVariables.SelectSet[i] {
				allSelect = append(allSelect, terminal)
				if terminal == "$" {
					cppBufWrite.WriteString("\t\tcase TerminalEOF:\n")
				} else {
					cppBufWrite.WriteString(fmt.Sprintf("\t\tcase Terminal_%s:\n", serials.Map[terminal].SerialString))
				}
			}
			cppBufWrite.WriteString(fmt.Sprintf("\t\t\tparser->Infer(Production_%s);\n", prodSerials[i]))
			for j := len(prod) - 1; j > 0; j-- {
				if ctx.Grammer.Nonterminals.Contains(prod[j]) {
					cppBufWrite.WriteString(fmt.Sprintf("\t\t\tctx->stack->push_back(Nonterminal_%s);\n", serials.Map[prod[j]].SerialString))
				} else {
					cppBufWrite.WriteString(fmt.Sprintf("\t\t\tctx->stack->push_back(Terminal_%s);\n", serials.Map[prod[j]].SerialString))
				}
			}
			cppBufWrite.WriteString("\t\t\tbreak;\n")
		}
		cppBufWrite.WriteString("\t\tdefault: {\n\t\t\tint expected[] = {")
		for i, v := range allSelect {
			if i > 0 {
				cppBufWrite.WriteString(", ")
			}
			if v == "$" {
				cppBufWrite.WriteString("TerminalEOF")
			} else {
				cppBufWrite.WriteString(fmt.Sprintf("Terminal_%s", serials.Map[v].SerialString))
			}
		}
		cppBufWrite.WriteString(`};
			CompileError compileError = {
				expected, sizeof(expected) / sizeof(int), *ctx->token
			};
			*result = false;
			return parser->Panic(ctx, &compileError);
		}
	}
	return true;
}
`)
	}

	cppBufWrite.WriteString(fmt.Sprintf(`
bool Parse(IParser* parser) {
	std::vector<int> stack;
	stack.push_back(Nonterminal_%s);
	int token = parser->Next();
	ParserContext ctx = { &token, &stack };
	bool result = true;
	while (stack.size() > 0) {
		int last = stack.back();
		stack.pop_back();
		bool action = true;
		switch (last) {
`, serials.Map[ctx.Grammer.StartNonterminal].SerialString))
	for nonterminal := range ctx.Grammer.Nonterminals {
		cppBufWrite.WriteString(fmt.Sprintf(`			case Nonterminal_%s:
				action = parse_%s(&ctx, parser, &result);
				break;
`, serials.Map[nonterminal].SerialString, serials.Map[nonterminal].SerialString))
	}
	cppBufWrite.WriteString(`			default: {
				if (last == token) {
					token = parser->Next();
					break;
				}
				int expected[] = {last};
				CompileError compileError = {
					expected, sizeof(expected) / sizeof(int), token
				};
				action = parser->Panic(&ctx, &compileError);
				result = false;
			}
        }
		if (!action) {
			return false;
		}
    }
    if (token == TerminalEOF) {
        return result;
    }
    int expected[] = {TerminalEOF};
    CompileError compileError = {
        expected, sizeof(expected) / sizeof(int), token
    };
    parser->Panic(&ctx, &compileError);
    return false;
}
`)
	cppBufWrite.WriteString(fmt.Sprintf(`
#ifdef %s
}
#endif
`, randomNamespace))
}

// 按照一定顺序排列非终结符
func (ctx *LLContext) sortNonterminals() {
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

func (ctx *LLContext) insertProductionBeforeNonterminal(nonterminal string, prods ...production.Production) {
	newProd := make([]production.Production, 0)
	found := false
	for _, prod := range ctx.KeyVariables.Productions {
		if !found && prod[0] == nonterminal {
			newProd = append(newProd, prods...)
			found = true
		}
		newProd = append(newProd, prod)
	}
	ctx.KeyVariables.Productions = newProd
}
