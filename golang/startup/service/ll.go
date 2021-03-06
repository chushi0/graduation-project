package service

import (
	"encoding/json"

	"github.com/chushi0/graduation_project/golang/startup/debug"
	ll "github.com/chushi0/graduation_project/golang/startup/production/process"
	"github.com/go-basic/uuid"
)

type LLProcess struct {
	DebugContext *debug.DebugContext
	LLContext    *ll.LLContext
}

var llProcess map[string]*LLProcess = make(map[string]*LLProcess)

func init() {
	RegisteService("ll_process_request", LLProcessRequest)
	RegisteService("ll_process_switchmode", LLProcessSwitchMode)
	RegisteService("ll_process_release", LLProcessRelease)
	RegisteService("ll_process_setbreakpoints", LLProcessSetBreakpoints)
	RegisteService("ll_process_variables", LLProcessGetVariables)
	RegisteService("ll_process_exit", LLProcessGetExitResult)
}

func LLProcessRequest(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		Code          string `json:"code"`
		WithTranslate bool   `json:"with_translate"`
		SavePath      string `json:"save_path"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	process := &LLProcess{}
	process.LLContext = ll.NewLLContext()
	process.LLContext.Code = reqStruct.Code
	process.LLContext.WithTranslate = reqStruct.WithTranslate
	if reqStruct.SavePath != "" {
		process.LLContext.Enable = true
		process.LLContext.SavePath = reqStruct.SavePath
		process.LLContext.Normalize()
	}
	entry := process.LLContext.CreateLLProcessEntry()
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

func LLProcessGetExitResult(req json.RawMessage) (code int, resp interface{}, err error) {
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
	if proc.DebugContext.GetCurrentRunMode() != debug.RunMode_Exit {
		code = 1004
		return
	}
	resp = proc.DebugContext.ExitResult
	return
}
