package process

import (
	"sort"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production"
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

	NonterminalOrders        []string              `json:"nonterminal_orders"`
	CurrentProcessProduction production.Production `json:"current_process_production"`
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
			dumpProductions := utilslice.CopyProductions(ctx.Grammer.Productions[A])
			for _, prod := range dumpProductions {
				ctx.KeyVariables.CurrentProcessProduction = prod
				ctx.bury("RemoveLeftRecusion", 4)
				if len(prod) > 1 && prod[1] == B {
					ctx.Grammer.RemoveProduction(prod)
					for _, bprod := range ctx.Grammer.Productions[B] {
						newProd := make([]string, 0)
						newProd = append(newProd, A)
						newProd = append(newProd, bprod...)
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
		for i, prod := range ctx.Grammer.Productions[A] {
			ctx.KeyVariables.CurrentProcessProduction = prod
			ctx.bury("RemoveLeftRecusion", 8)

			if len(prod) > 1 && prod[1] == A {
				newProd := []string{newNonterminal}
				newProd = append(newProd, prod[2:]...)
				ctx.Grammer.AddNewProduction(newProd)
			} else {
				newProd := []
			}

			ctx.bury("RemoveLeftRecusion", 9)
		}

		ctx.KeyVariables.LoopVariableI++
	}

	ctx.bury("RemoveLeftRecusion", -1)
}
