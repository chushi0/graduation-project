package debug

import (
	"errors"
	"sync"

	utilslice "github.com/chushi0/graduation_project/golang/startup/util/util_slice"
)

type RunMode uint8

const (
	RunMode_Run    RunMode = 1
	RunMode_Pause  RunMode = 2
	RunMode_Paused RunMode = 3
	RunMode_Exit   RunMode = 4
)

var errExit = errors.New("exit")

// 断点
type Point struct {
	Name string `json:"name"`
	Line int    `json:"line"`
}

// 调试上下文
type DebugContext struct {
	DebugRunMode RunMode                // 运行模式
	BreakPoints  []*Point               // 断点
	Variables    map[string]interface{} // 当前变量
	Lock         sync.Mutex             // 锁
	Condition    *sync.Cond             // 条件变量
}

func NewDebugContext() *DebugContext {
	ctx := &DebugContext{}
	ctx.BreakPoints = make([]*Point, 0)
	ctx.Condition = sync.NewCond(&ctx.Lock)
	return ctx
}

// 断点埋点
func (ctx *DebugContext) RunReport(point *Point, variables map[string]interface{}) {
	ctx.Lock.Lock()
	defer ctx.Lock.Unlock()
	ctx.Variables = variables
	if utilslice.LinearSearch(len(ctx.BreakPoints), func(i int) bool {
		return ctx.BreakPoints[i].Line == point.Line && ctx.BreakPoints[i].Name == point.Name
	}) != -1 {
		ctx.DebugRunMode = RunMode_Paused
	}
	if ctx.DebugRunMode == RunMode_Pause {
		ctx.DebugRunMode = RunMode_Paused
	}
	for ctx.DebugRunMode == RunMode_Paused {
		ctx.Condition.Wait()
	}
	if ctx.DebugRunMode == RunMode_Exit {
		panic(errExit)
	}
}

// 切换运行模式
// Run: 运行（在断点处中断）
// Pause: 暂停（在下次埋点中断）
// Exit: 停止
// Paused: 不要用
func (ctx *DebugContext) SwitchRunMode(runmode RunMode) {
	ctx.Lock.Lock()
	defer ctx.Lock.Unlock()
	ctx.DebugRunMode = runmode
	ctx.Condition.Signal()
}

// 添加断点
func (ctx *DebugContext) AddBreakPoint(point *Point) {
	ctx.Lock.Lock()
	defer ctx.Lock.Unlock()
	ctx.BreakPoints = append(ctx.BreakPoints, point)
}

// 移除断点
func (ctx *DebugContext) RemoveBreakPoint(point *Point) {
	ctx.Lock.Lock()
	defer ctx.Lock.Unlock()
	index := utilslice.LinearSearch(len(ctx.BreakPoints), func(i int) bool {
		return ctx.BreakPoints[i].Line == point.Line && ctx.BreakPoints[i].Name == point.Name
	})
	if index == -1 {
		return
	}
	breakPoints := append([]*Point{}, ctx.BreakPoints[:index]...)
	breakPoints = append(breakPoints, ctx.BreakPoints[index+1:]...)
	ctx.BreakPoints = breakPoints
}

// 获取变量
// 若未暂停，返回 nil
func (ctx *DebugContext) GetVariables() map[string]interface{} {
	ctx.Lock.Lock()
	defer ctx.Lock.Unlock()
	if ctx.DebugRunMode == RunMode_Paused {
		return ctx.Variables
	}
	return nil
}

// 启动测试 goroutine
func StartDebugGoroutine(entry func(ctx *DebugContext)) *DebugContext {
	ctx := NewDebugContext()
	ctx.SwitchRunMode(RunMode_Pause)
	go func() {
		defer func() {
			ctx.DebugRunMode = RunMode_Exit
			if err := recover(); err != nil {
				if errErr, ok := err.(error); ok {
					if errors.Is(errErr, errExit) {
						return
					}
				}
				panic(err)
			}
		}()
		entry(ctx)
	}()
	return ctx
}
