package process_test

import (
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production/process"
)

func TestLLRemoveLeftRecusion(t *testing.T) {
	ctx := process.NewLLContext()
	ctx.Code = "A := b c d\n| b c e\n|b f"
	entry := ctx.CreateLLProcessEntry()
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
}

func TestLLComputeFirstSetRecusion(t *testing.T) {
	ctx := process.NewLLContext()
	ctx.Code = `
	S := E
	E := T E0
	E0 := + T E0
		|
	T := F T0
	T0 := * F T0
		|
	F := ( E )
		| id
	`
	entry := ctx.CreateLLProcessEntry()
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
}
