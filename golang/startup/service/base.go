package service

import (
	"encoding/json"
	"fmt"
	"runtime"
)

type Service = func(req json.RawMessage) (code int, resp interface{}, err error)

var services map[string]Service = make(map[string]Service)

func Init() {

}

func CallService(rawReq []byte) (rawResp []byte, err error) {
	defer func() {
		if errRecover := recover(); errRecover != nil {
			rawResp = nil
			buf := make([]byte, 1<<16)
			len := runtime.Stack(buf, false)
			err = fmt.Errorf("service panic: %v\n%v", errRecover, buf[:len])
		}
	}()

	var req struct {
		Action string          `json:"action"`
		Data   json.RawMessage `json:"data"`
	}
	var resp struct {
		Code int         `json:"code"`
		Data interface{} `json:"data"`
	}

	err = json.Unmarshal(rawReq, &req)
	if err != nil {
		return nil, fmt.Errorf("unmarshal json fail: %w", err)
	}

	service, exist := services[req.Action]
	if !exist {
		return nil, fmt.Errorf("service %s not found", req.Action)
	}

	resp.Code, resp.Data, err = service(req.Data)
	if err != nil {
		return nil, fmt.Errorf("service return error: %w", err)
	}

	rawResp, err = json.Marshal(resp)
	if err != nil {
		return nil, fmt.Errorf("marshal to json fail: %w", err)
	}
	return rawResp, nil
}

func RegisteService(name string, service Service) {
	if _, ok := services[name]; ok {
		panic(fmt.Sprintf("service %v has been regester", name))
	}
	services[name] = service
}
