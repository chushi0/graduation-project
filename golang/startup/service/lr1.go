package service

import (
	"encoding/json"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	lr "github.com/chushi0/graduation_project/golang/startup/production/process"
	"github.com/go-basic/uuid"
)

type LR1Process struct {
	DebugContext *debug.DebugContext
	LR1Context   *lr.LR1Context
}

var lr1Process map[string]*LR1Process = make(map[string]*LR1Process)

func init() {
	RegisteService("lr1_process_request", LR1ProcessRequest)
	RegisteService("lr1_process_switchmode", LR1ProcessSwitchMode)
	RegisteService("lr1_process_release", LR1ProcessRelease)
	RegisteService("lr1_process_setbreakpoints", LR1ProcessSetBreakpoints)
	RegisteService("lr1_process_variables", LR1ProcessGetVariables)
	RegisteService("lr1_process_exit", LR1ProcessGetExitResult)
}

func LR1ProcessRequest(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		Code string `json:"code"`
		LALR bool   `json:"lalr"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	process := &LR1Process{}
	process.LR1Context = lr.NewLR1Context()
	process.LR1Context.Code = reqStruct.Code
	process.LR1Context.LALR = reqStruct.LALR
	entry := process.LR1Context.CreateLR1ProcessEntry()
	process.DebugContext = debug.StartDebugGoroutine(entry)
	id := uuid.New()
	lr1Process[id] = process
	var respStruct struct {
		ID string `json:"id"`
	}
	respStruct.ID = id
	resp = respStruct
	return
}

func LR1ProcessSwitchMode(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID   string `json:"id"`
		Mode int    `json:"mode"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr1Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.SwitchRunMode(debug.RunMode(reqStruct.Mode))
	return
}

func LR1ProcessRelease(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr1Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.SwitchRunMode(debug.RunMode_Exit)
	delete(lr1Process, reqStruct.ID)
	return
}

func LR1ProcessSetBreakpoints(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID          string `json:"id"`
		Breakpoints []struct {
			Name string `json:"name"`
			Line int    `json:"line"`
		} `json:"breakpoints"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr1Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.ClearBreakPoints()
	for _, bp := range reqStruct.Breakpoints {
		proc.DebugContext.AddBreakPoint(&debug.Point{
			Name: bp.Name,
			Line: bp.Line,
		})
	}
	return
}

func LR1ProcessGetVariables(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr1Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	variables, point := proc.DebugContext.GetVariables()
	if variables == nil {
		code = 1003
		return
	}
	resp = map[string]interface{}{
		"var":   variables,
		"point": point,
	}
	return
}

func LR1ProcessGetExitResult(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr1Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	if proc.DebugContext.GetCurrentRunMode() != debug.RunMode_Exit {
		code = 1004
		return
	}
	resp = proc.DebugContext.ExitResult
	return
}
