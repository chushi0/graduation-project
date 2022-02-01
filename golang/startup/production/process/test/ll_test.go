package process_test

import (
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production/process"
)

func TestLLRemoveLeftRecusion(t *testing.T) {
	entry := process.CreateLLProcessEntry("A := b c d\n| b c e\n|b f")
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
}

func TestLLComputeFirstSetRecusion(t *testing.T) {
	entry := process.CreateLLProcessEntry(`
	S := E
	E := T E0
	E0 := + T E0
		|
	T := F T0
	T0 := * F T0
		|
	F := ( E )
		| id
	`)
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
}
