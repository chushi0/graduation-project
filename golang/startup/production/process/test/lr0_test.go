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
	@S'
	S' := S
	S := B B
	B := b B
	B := a
	`
	ctx.SLR = false
	ctx.CodeSaver.Enable = true
	ctx.CodeSaver.SavePath = "F:\\test.h"
	ctx.CodeSaver.Normalize()
	entry := ctx.CreateLR0ProcessEntry()
	dbg := debug.NewDebugContext()
	dbg.SwitchRunMode(debug.RunMode_Run)
	entry(dbg)
	json, _ := json.Marshal(ctx.KeyVariables)
	ioutil.WriteFile("test.json", json, 0660)
}
