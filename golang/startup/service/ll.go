package service

import (
	"encoding/json"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	ll "github.com/chushi0/graduation_project/golang/startup/production/process"
	"github.com/go-basic/uuid"
)

type LLProcess struct {
	DebugContext *debug.DebugContext
}

var llProcess map[string]*LLProcess = make(map[string]*LLProcess)

func init() {
	RegisteService("ll_process_request", LLProcessRequest)
	RegisteService("ll_process_switchmode", LLProcessSwitchMode)
	RegisteService("ll_process_release", LLProcessRelease)
	RegisteService("ll_process_setbreakpoints", LLProcessSetBreakpoints)
	RegisteService("ll_process_variables", LLProcessGetVariables)
}

func LLProcessRequest(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		Code string `json:"code"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	process := &LLProcess{}
	entry := ll.CreateLLProcessEntry(reqStruct.Code)
	process.DebugContext = debug.StartDebugGoroutine(entry)
	id := uuid.New()
	llProcess[id] = process
	var respStruct struct {
		ID string `json:"id"`
	}
	respStruct.ID = id
	resp = respStruct
	return
}

func LLProcessSwitchMode(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID   string `json:"id"`
		Mode int    `json:"mode"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := llProcess[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.SwitchRunMode(debug.RunMode(reqStruct.Mode))
	return
}

func LLProcessRelease(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := llProcess[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.DebugContext.SwitchRunMode(debug.RunMode_Exit)
	delete(llProcess, reqStruct.ID)
	return
}

func LLProcessSetBreakpoints(req json.RawMessage) (code int, resp interface{}, err error) {
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
	proc, exist := llProcess[reqStruct.ID]
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

func LLProcessGetVariables(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := llProcess[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	variables := proc.DebugContext.GetVariables()
	if variables == nil {
		code = 1003
		return
	}
	resp = variables
	return
}
