package process

import (
	"bufio"
	"errors"
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
	Context      *debug.DebugContext
	Grammer      *Grammer
	Code         string
	KeyVariables *LLKeyVariables
}

type LLKeyVariables struct {
	Productions   []production.Production `json:"productions"`
	LoopVariableI int                     `json:"loop_variable_i"`
	LoopVariableJ int                     `json:"loop_variable_j"`
	LoopVariableK int                     `json:"loop_variable_k"`
	ModifiedFlag  bool                    `json:"modified_flag"`

	NonterminalOrders        []string              `json:"nonterminal_orders"`
	CurrentProcessProduction production.Production `json:"current_process_production"`

	FirstSet  map[string]set.StringSet `json:"first"`
	FollowSet map[string]set.StringSet `json:"follow"`
	SelectSet []set.StringSet          `json:"select"`
}

type LLResult struct {
	Code      int                    `json:"code"`
	Detail    map[string]interface{} `json:"detail"`
	Variables *LLKeyVariables        `json:"variables"`
}

const (
	LL_Success              = 0
	LL_Error_ParseCode      = 1
	LL_Error_SelectConflict = 2
)

var errPipelineShutdown = errors.New("pipeline shutdown")

func CreateLLProcessEntry(code string) func(*debug.DebugContext) {
	ctx := &LLContext{
		Code:         code,
		KeyVariables: &LLKeyVariables{},
	}
	return func(dc *debug.DebugContext) {
		defer func() {
			if err := recover(); err != nil {
				if err == errPipelineShutdown {
					return
				}
				panic(err)
			}
		}()
		ctx.Context = dc
		ctx.RunPipeline()
		dc.SwitchRunMode(debug.RunMode_Exit)
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
	panic(errPipelineShutdown)
}

func (ctx *LLContext) RunPipeline() {
	// 准备：解析产生式代码
	ctx.bury("pipeline", 0)
	ctx.Prepare()
	// 转换：去除左递归、提取公共前缀
	ctx.bury("pipeline", 1)
	ctx.Translate()
	// 计算：计算First集、计算Follow集、计算Select集
	ctx.bury("pipeline", 2)
	ctx.Calculate()
	// 生成：检查Select集冲突、生成自动机、生成自动机代码
	ctx.bury("pipeline", 3)
	ctx.Build()
	ctx.bury("pipeline", -1)
}

func (ctx *LLContext) Prepare() {
	// 解析产生式代码
	ctx.bury("prepare", 0)
	ctx.ParseCode()
	ctx.bury("prepare", -1)
}

func (ctx *LLContext) Translate() {
	// 去除左递归
	ctx.bury("translate", 0)
	ctx.RemoveLeftRecusion()
	// 提取公共前缀
	ctx.bury("translate", 1)
	ctx.ExtractCommonPrefix()
	ctx.bury("translate", -1)
}

func (ctx *LLContext) Calculate() {
	// 计算First集
	ctx.bury("calculate", 0)
	ctx.ComputeFirstSet()
	// 计算Follow集
	ctx.bury("calculate", 1)
	ctx.ComputeFollowSet()
	// 计算Select集
	ctx.bury("calculate", 2)
	ctx.ComputeSelectSet()
	ctx.bury("calculate", -1)
}

func (ctx *LLContext) Build() {
	// 检查Select集冲突
	ctx.bury("build", 0)
	ctx.CheckSelectConflict()
	// 生成自动机
	ctx.bury("build", 1)
	ctx.GenerateAutomaton()
	// 生成代码
	ctx.bury("build", 2)
	ctx.GenerateYaccCode()
	ctx.bury("build", -1)
}

func (ctx *LLContext) ParseCode() {
	ctx.bury("ParseCode", 0)

	prods, errs := production.ParseProduction(ctx.Code, nil)
	if len(errs.Errors) > 0 {
		ctx.shutdownPipeline(&LLResult{
			Code: LL_Error_ParseCode,
			Detail: map[string]interface{}{
				"errors": errs.Errors,
			},
		})
	}
	ctx.KeyVariables.Productions = prods
	ctx.Grammer = NewGrammer(prods)

	ctx.bury("ParseCode", -1)
}

/**
 * 算法步骤：
 * 按照一定顺序排列非终结符 A[1]...A[n]
 * 循环 i 从 1 到 n
 *     循环 j 从 1 到 i-1
 *         将 A[i]->A[j]γ 替换为 A[i]->δγ，其中A[j]->δ
 *     清除A[i]的直接左递归
 *
 * 断点解释：
 * 1 - 非终结符排列完毕
 * 2 - i 变动
 * 3 - j 变动
 * 4 - 即将对产生式处理
 * 5 - 产生式处理完毕
 * 6 - j 循环完毕
 * 7 - 跳过清除左递归
 * 8 - 即将处理产生式
 * 9 - 清除左递归完成
 */
func (ctx *LLContext) RemoveLeftRecusion() {
	ctx.bury("RemoveLeftRecusion", 0)

	// 按照一定顺序排列非终结符
	ctx.KeyVariables.NonterminalOrders = make([]string, 0)
	for nonterminal := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, nonterminal)
	}
	sort.Strings(ctx.KeyVariables.NonterminalOrders)
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
			dumpProductions := utilslice.CopyStringSlice(ctx.Grammer.Productions[A])
			for _, prod := range dumpProductions {
				ctx.KeyVariables.CurrentProcessProduction = prod
				ctx.bury("RemoveLeftRecusion", 4)
				if len(prod) > 1 && prod[1] == B {
					ctx.Grammer.RemoveProduction(prod)
					for _, bprod := range ctx.Grammer.Productions[B] {
						newProd := make([]string, 0)
						newProd = append(newProd, A)
						newProd = append(newProd, bprod[1:]...)
						newProd = append(newProd, prod[2:]...)
						ctx.Grammer.AddNewProduction(newProd)
					}
					ctx.bury("RemoveLeftRecusion", 5)
				}
			}

			ctx.KeyVariables.LoopVariableJ++
		}
		ctx.bury("RemoveLeftRecusion", 6)

		// 清除A[i]的直接左递归
		skip := true
		for _, prod := range ctx.Grammer.Productions[A] {
			if len(prod) > 1 && prod[1] == A {
				skip = false
				break
			}
		}

		if skip {
			ctx.bury("RemoveLeftRecusion", 7)
			ctx.KeyVariables.LoopVariableI++
			continue
		}

		newNonterminal := ctx.Grammer.AddNewNonterminal(A)
		ctx.Grammer.AddNewProduction([]string{newNonterminal})
		for _, prod := range utilslice.CopyStringSlice(ctx.Grammer.Productions[A]) {
			ctx.KeyVariables.CurrentProcessProduction = prod
			ctx.bury("RemoveLeftRecusion", 8)

			if len(prod) > 1 && prod[1] == A {
				// 非终结符新的产生式
				newProd := []string{newNonterminal}
				newProd = append(newProd, prod[2:]...)
				newProd = append(newProd, newNonterminal)
				ctx.Grammer.AddNewProduction(newProd)
				// 移除原产生式
				ctx.Grammer.RemoveProduction(prod)
			} else {
				newProd := append(prod, newNonterminal)
				ctx.Grammer.AddNewProduction(newProd)
				ctx.Grammer.RemoveProduction(prod)
			}

			ctx.bury("RemoveLeftRecusion", 9)
		}

		ctx.KeyVariables.LoopVariableI++
	}

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

	// 按照一定顺序排列非终结符
	ctx.KeyVariables.NonterminalOrders = make([]string, 0)
	for nonterminal := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, nonterminal)
	}
	sort.Strings(ctx.KeyVariables.NonterminalOrders)
	ctx.bury("ExtractCommonPrefix", 1)

	// 循环
	ctx.KeyVariables.LoopVariableI = 0
	for {
		if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.NonterminalOrders) {
			break
		}

		nonterminal := ctx.KeyVariables.NonterminalOrders[ctx.KeyVariables.LoopVariableI]
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

			newNonterminal := ctx.Grammer.AddNewNonterminal(nonterminal)
			newProd := []string{nonterminal}
			newProd = append(newProd, commonPrefix...)
			newProd = append(newProd, newNonterminal)
			ctx.Grammer.AddNewProduction(newProd)
			ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, newNonterminal)
			for _, prod := range processProds {
				ctx.Grammer.RemoveProduction(prod)
				newProd := []string{newNonterminal}
				newProd = append(newProd, prod[len(commonPrefix)+1:]...)
				ctx.Grammer.AddNewProduction(newProd)
			}
		}

		ctx.KeyVariables.LoopVariableI++
	}

	ctx.bury("ExtractCommonPrefix", -1)

	// 更新产生式
	prods := make([]production.Production, 0)
	for nonterminal := range ctx.Grammer.Nonterminals {
		prods = append(prods, ctx.Grammer.Productions[nonterminal]...)
	}
	ctx.KeyVariables.Productions = prods
}

func (ctx *LLContext) ComputeFirstSet() {
	// 初始化 First 集
	ctx.KeyVariables.FirstSet = make(map[string]set.StringSet)
	for i := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.FirstSet[i] = set.NewStringSet()
	}

	// 按照一定顺序排列非终结符
	ctx.KeyVariables.NonterminalOrders = make([]string, 0)
	for nonterminal := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, nonterminal)
	}
	sort.Strings(ctx.KeyVariables.NonterminalOrders)

	ctx.KeyVariables.ModifiedFlag = true
	for ctx.KeyVariables.ModifiedFlag {
		ctx.KeyVariables.ModifiedFlag = false

		ctx.KeyVariables.LoopVariableI = 0
		for {
			if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.Productions) {
				break
			}
			prod := ctx.KeyVariables.Productions[ctx.KeyVariables.LoopVariableI]
			nonterminal := prod[0]

			ctx.KeyVariables.LoopVariableJ = 1
			for {
				if ctx.KeyVariables.LoopVariableJ >= len(prod) {
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
						ctx.KeyVariables.FirstSet[nonterminal].Put("") > 0
					break
				}

				// 终结符
				if ctx.Grammer.Terminals.Contains(prod[ctx.KeyVariables.LoopVariableJ]) {
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
						ctx.KeyVariables.FirstSet[nonterminal].Put(prod[ctx.KeyVariables.LoopVariableJ]) > 0
					break
				}
				// 非终结符
				nonterminalFirstSet := ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]]
				ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
					ctx.KeyVariables.FirstSet[nonterminal].UnionExcept(nonterminalFirstSet, "") > 0
				if !nonterminalFirstSet.Contains("") {
					break
				}

				ctx.KeyVariables.LoopVariableJ++
			}
			ctx.KeyVariables.LoopVariableI++
		}
	}
}

func (ctx *LLContext) ComputeFollowSet() {
	// 初始化 Follow 集
	ctx.KeyVariables.FollowSet = make(map[string]set.StringSet)
	for i := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.FollowSet[i] = set.NewStringSet()
	}

	// 按照一定顺序排列非终结符
	ctx.KeyVariables.NonterminalOrders = make([]string, 0)
	for nonterminal := range ctx.Grammer.Nonterminals {
		ctx.KeyVariables.NonterminalOrders = append(ctx.KeyVariables.NonterminalOrders, nonterminal)
	}
	sort.Strings(ctx.KeyVariables.NonterminalOrders)

	ctx.KeyVariables.ModifiedFlag = true
	for ctx.KeyVariables.ModifiedFlag {
		ctx.KeyVariables.ModifiedFlag = false

		ctx.KeyVariables.LoopVariableI = 0
		for {
			if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.Productions) {
				break
			}
			prod := ctx.KeyVariables.Productions[ctx.KeyVariables.LoopVariableI]
			nonterminal := prod[0]

			// 开始符号：加入 $ 结束符
			if nonterminal == ctx.Grammer.StartNonterminal {
				ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
					ctx.KeyVariables.FollowSet[nonterminal].Put("$") > 0
			}

			// 顺序遍历
			ctx.KeyVariables.LoopVariableJ = 1
			for {
				if ctx.KeyVariables.LoopVariableJ >= len(prod) {
					break
				}

				current := prod[ctx.KeyVariables.LoopVariableJ]
				if !ctx.Grammer.Nonterminals.Contains(current) {
					ctx.KeyVariables.LoopVariableJ++
					continue
				}
				ctx.KeyVariables.LoopVariableK = ctx.KeyVariables.LoopVariableJ + 1
				for {
					if ctx.KeyVariables.LoopVariableK >= len(prod) {
						ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
							ctx.KeyVariables.FollowSet[current].UnionExcept(
								ctx.KeyVariables.FollowSet[nonterminal],
							) > 0
						break
					}
					cur := prod[ctx.KeyVariables.LoopVariableK]
					if ctx.Grammer.Terminals.Contains(cur) {
						ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
							ctx.KeyVariables.FollowSet[current].Put(cur) > 0
						break
					}

					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.ModifiedFlag ||
						ctx.KeyVariables.FollowSet[current].UnionExcept(
							ctx.KeyVariables.FirstSet[cur], "",
						) > 0
					if !ctx.KeyVariables.FirstSet[cur].Contains("") {
						break
					}

					ctx.KeyVariables.LoopVariableK++
				}
				ctx.KeyVariables.LoopVariableJ++
			}
			ctx.KeyVariables.LoopVariableI++
		}

	}
}

func (ctx *LLContext) ComputeSelectSet() {
	// 初始化 Select 集
	ctx.KeyVariables.SelectSet = make([]set.StringSet, len(ctx.KeyVariables.Productions))
	for i := range ctx.KeyVariables.Productions {
		ctx.KeyVariables.SelectSet[i] = set.NewStringSet()
	}

	ctx.KeyVariables.LoopVariableI = 0
	for {
		if ctx.KeyVariables.LoopVariableI >= len(ctx.KeyVariables.Productions) {
			break
		}
		prod := ctx.KeyVariables.Productions[ctx.KeyVariables.LoopVariableI]

		ctx.KeyVariables.LoopVariableJ = 1
		for {
			if ctx.KeyVariables.LoopVariableJ >= len(prod) {
				ctx.KeyVariables.SelectSet[ctx.KeyVariables.LoopVariableI].UnionExcept(ctx.KeyVariables.FollowSet[prod[0]])
				break
			}

			cur := prod[ctx.KeyVariables.LoopVariableJ]
			if ctx.Grammer.Terminals.Contains(cur) {
				ctx.KeyVariables.SelectSet[ctx.KeyVariables.LoopVariableI].Put(cur)
				break
			}
			ctx.KeyVariables.SelectSet[ctx.KeyVariables.LoopVariableI].UnionExcept(
				ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]], "",
			)
			if !ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]].Contains("") {
				break
			}

			ctx.KeyVariables.LoopVariableJ++
		}

		ctx.KeyVariables.LoopVariableI++
	}
}

func (ctx *LLContext) CheckSelectConflict() {
	selectSets := make(map[string]set.StringSet)
	for i, prod := range ctx.KeyVariables.Productions {
		if _, ok := selectSets[prod[0]]; !ok {
			selectSets[prod[0]] = set.NewStringSet()
		}
		intersection := selectSets[prod[0]].Intersection(ctx.KeyVariables.SelectSet[i])
		if len(intersection) > 0 {
			ctx.shutdownPipeline(&LLResult{
				Code: LL_Error_SelectConflict,
			})
		}
		selectSets[prod[0]].UnionExcept(ctx.KeyVariables.SelectSet[i])
	}
}

func (ctx *LLContext) GenerateAutomaton() {
}

func (ctx *LLContext) GenerateYaccCode() {
	serials := NewSerialTokens()
	prodSerials := make([]string, len(ctx.KeyVariables.Productions))
	// FIXME: 测试时临时生成到文件中
	file, _ := os.Create("test.h")
	bufWrite := bufio.NewWriter(file)
	defer func() {
		bufWrite.Flush()
		file.Close()
	}()
	bufWrite.WriteString(`#pragma once
/**
 * Auto-generate header file.
 * After you re-generate file, ALL YOUR CHANGE WILL BE LOST!
 */
`)
	// 随机命名空间
	randomNamespace := fmt.Sprintf("LL_%d_%d", rand.Int(), rand.Int())
	bufWrite.WriteString(fmt.Sprintf(`
// By modify this macro, you can define automaton code in namespace
#define %s

#if %s
namespace %s {
#endif
`, randomNamespace, randomNamespace, randomNamespace))

	// 写入原始代码
	bufWrite.WriteString("\n\t// Production Definition Code\n")
	for _, line := range strings.Split(ctx.Code, "\n") {
		bufWrite.WriteString(fmt.Sprintf("\t// %s\n", line))
	}

	// 写入终结符定义
	bufWrite.WriteString(`
	// Terminals Definition
	constexpr int TerminalEOF = -1;
`)
	for terminal := range ctx.Grammer.Terminals {
		serials.Put(terminal)
		token := serials.Map[terminal]
		bufWrite.WriteString(fmt.Sprintf("\tconstexpr int Terminal_%s = %d; // %s\n", token.SerialString, token.Index, terminal))
	}

	// 写入非终结符定义
	bufWrite.WriteString(`
	// Nonterminals Definition
`)
	for nonterminal := range ctx.Grammer.Nonterminals {
		serials.Put(nonterminal)
		token := serials.Map[nonterminal]
		bufWrite.WriteString(fmt.Sprintf("\tconstexpr int Nonterminal_%s = %d; // %s\n", token.SerialString, token.Index, nonterminal))
	}

	// 写入产生式定义
	bufWrite.WriteString(`
	// Productions Definition
`)
	for i, prod := range ctx.KeyVariables.Productions {
		prodSerials[i] = serials.Map[prod[0]].SerialString
		for j := 1; j < len(prod); j++ {
			prodSerials[i] += "_" + serials.Map[prod[j]].SerialString
		}
	}
	for i, prod := range ctx.KeyVariables.Productions {
		bufWrite.WriteString(fmt.Sprintf("\tconstexpr int Production_%s = %d; // ", prodSerials[i], i))
		bufWrite.WriteString(prod[0])
		bufWrite.WriteString(" :=")
		for j := 1; j < len(prod); j++ {
			bufWrite.WriteString(" ")
			bufWrite.WriteString(prod[j])
		}
		bufWrite.WriteString("\n")
	}

	// 其他结构定义
	bufWrite.WriteString(`
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
		// Infer left nonterminal by production
		virtual void Infer(int id) = 0;
		// Error occurred when parse (parser will exit)
		virtual void Panic(CompileError* error) = 0;
	};

	// main entry parser
	// true means success, false means fail (after calling IParser.Panic)
	bool Parse(IParser* parser);
`)

	bufWrite.WriteString(fmt.Sprintf(`
#if %s
}
#endif
`, randomNamespace))
}
