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
