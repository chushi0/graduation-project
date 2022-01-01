package service

import "encoding/json"

func init() {
	RegisteService("hello", HelloService)
}

func HelloService(req json.RawMessage) (code int, resp interface{}, err error) {
	return 0, map[string]interface{}{
		"text": "Hello world",
	}, nil
}
