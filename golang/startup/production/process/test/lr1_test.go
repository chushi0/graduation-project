package process_test

import (
	"encoding/json"
	"io/ioutil"
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	"github.com/chushi0/graduation_project/golang/startup/production/process"
)

func TestLR1ItemClosure(t *testing.T) {
	ctx := process.NewLR1Context()
	ctx.Code = `
	S := S0
	S0 := L = R
	S0 := R
	L := * R
	L := id
	R := L
	`
	ctx.LALR = false
	entry := ctx.CreateLR1ProcessEntry()
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
	json, _ := json.Marshal(ctx.KeyVariables)
	ioutil.WriteFile("test.json", json, 0660)
}
