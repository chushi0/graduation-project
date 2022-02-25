package service

import (
	"encoding/json"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	lr "github.com/chushi0/graduation_project/golang/startup/production/process"
	"github.com/go-basic/uuid"
)

type LR0Process struct {
	DebugContext *debug.DebugContext
	LR0Context   *lr.LR0Context
}

var lr0Process map[string]*LR0Process = make(map[string]*LR0Process)

func init() {
	RegisteService("lr0_process_request", LR0ProcessRequest)
	RegisteService("lr0_process_switchmode", LR0ProcessSwitchMode)
	RegisteService("lr0_process_release", LR0ProcessRelease)
	RegisteService("lr0_process_setbreakpoints", LR0ProcessSetBreakpoints)
	RegisteService("lr0_process_variables", LR0ProcessGetVariables)
	RegisteService("lr0_process_exit", LR0ProcessGetExitResult)
}

func LR0ProcessRequest(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		Code string `json:"code"`
		SLR  bool   `json:"slr"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	process := &LR0Process{}
	process.LR0Context = lr.NewLR0Context()
	process.LR0Context.Code = reqStruct.Code
	process.LR0Context.SLR = reqStruct.SLR
	entry := process.LR0Context.CreateLR0ProcessEntry()
	process.DebugContext = debug.StartDebugGoroutine(entry)
	id := uuid.New()
	lr0Process[id] = process
	var respStruct struct {
		ID string `json:"id"`
	}
	respStruct.ID = id
	resp = respStruct
	return
}

func LR0ProcessSwitchMode(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID   string `json:"id"`
		Mode int    `json:"mode"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr0Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.SwitchRunMode(debug.RunMode(reqStruct.Mode))
	return
}

func LR0ProcessRelease(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr0Process[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.SwitchRunMode(debug.RunMode_Exit)
	delete(lr0Process, reqStruct.ID)
	return
}

func LR0ProcessSetBreakpoints(req json.RawMessage) (code int, resp interface{}, err error) {
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
	proc, exist := lr0Process[reqStruct.ID]
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

func LR0ProcessGetVariables(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr0Process[reqStruct.ID]
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

func LR0ProcessGetExitResult(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := lr0Process[reqStruct.ID]
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
