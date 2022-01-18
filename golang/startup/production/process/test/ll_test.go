package process_test

import (
	"fmt"
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production/process"
)

func TestLLRemoveLeftRecusion(t *testing.T) {
	entry := process.CreateLLProcessEntry("A := A b\n|c")
	dbg := debug.NewDebugContext()
	dbg.AddBreakPoint(&debug.Point{
		Name: "ExtractCommonPrefix",
		Line: 0,
	})
	dbg.SwitchRunMode(debug.RunMode_Run)
	go entry(dbg)
	for {
		variables := dbg.GetVariables()
		if variables != nil {
			fmt.Printf("%+v\n", dbg.GetCurrentPoint())
			dbg.SwitchRunMode(debug.RunMode_Pause)
			continue
		}
		if dbg.GetCurrentRunMode() == debug.RunMode_Exit {
			break
		}
	}
	t.Fail()
}
