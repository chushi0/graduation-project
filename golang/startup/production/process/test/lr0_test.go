package process_test

import (
	"encoding/json"
	"io/ioutil"
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production/process"
)

func TestLR0ItemClosure(t *testing.T) {
	ctx := process.NewLR0Context()
	ctx.Code = `
	S := E
	E := E + T
	E := T
	T := T * F
	T := F
	F := id
	F := (E)
	`
	ctx.SLR = true
	entry := ctx.CreateLR0ProcessEntry()
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
	json, _ := json.Marshal(ctx.KeyVariables)
	ioutil.WriteFile("test.json", json, 0660)
}
