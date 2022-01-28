package process

import (
	"sort"

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
	ModifiedFlag  bool                    `json:"modified_flag"`

	NonterminalOrders        []string              `json:"nonterminal_orders"`
	CurrentProcessProduction production.Production `json:"current_process_production"`

	FirstSet  map[string]set.StringSet `json:"first"`
	FollowSet map[string]set.StringSet `json:"follow"`
	SelectSet []set.StringSet          `json:"select"`
}

type LLResult struct {
	Code   int                    `json:"code"`
	Detail map[string]interface{} `json:"detail"`
}

const (
	LL_Success         = 0
	LL_Error_ParseCode = 1
)

func CreateLLProcessEntry(code string) func(*debug.DebugContext) {
	ctx := &LLContext{
		Code:         code,
		KeyVariables: &LLKeyVariables{},
	}
	return func(dc *debug.DebugContext) {
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
		ctx.Context.ExitResult = &LLResult{
			Code: LL_Error_ParseCode,
			Detail: map[string]interface{}{
				"errors": errs.Errors,
			},
		}
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
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.FirstSet[nonterminal].Put("") > 0
					break
				}

				// 终结符
				if ctx.Grammer.Terminals.Contains(prod[ctx.KeyVariables.LoopVariableJ]) {
					ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.FirstSet[nonterminal].Put(prod[ctx.KeyVariables.LoopVariableJ]) > 0
					break
				}
				// 非终结符
				nonterminalFirstSet := ctx.KeyVariables.FirstSet[prod[ctx.KeyVariables.LoopVariableJ]]
				ctx.KeyVariables.ModifiedFlag = ctx.KeyVariables.FirstSet[nonterminal].UnionExcept(nonterminalFirstSet, "") > 0
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
}

func (ctx *LLContext) ComputeSelectSet() {
}

func (ctx *LLContext) CheckSelectConflict() {
}

func (ctx *LLContext) GenerateAutomaton() {
}

func (ctx *LLContext) GenerateYaccCode() {
}
