package service

import (
	"encoding/json"

	"github.com/chushi0/graduation_project/golang/startup/production"
	"github.com/chushi0/graduation_project/golang/startup/util/set"
	"github.com/go-basic/uuid"
)

type ProductionProcess struct {
	Exit      bool
	Result    ProductionResult
	Interrupt bool
}

type ProductionResult struct {
	Terminals    []string                   `json:"terminal"`
	Nonterminals []string                   `json:"nonterminal"`
	Productions  []production.Production    `json:"productions"`
	Errors       *production.ErrorContainer `json:"errors"`
}

var productionParse map[string]*ProductionProcess = make(map[string]*ProductionProcess)

func init() {
	RegisteService("production_parse_start", ProductionParseRequest)
	RegisteService("production_parse_query", ProductionParseQuery)
	RegisteService("production_parse_cancel", ProductionParseCancel)
}

func ProductionParseRequest(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		Code string `json:"code"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	process := &ProductionProcess{}
	go ProcessProductionParse(reqStruct.Code, process)
	id := uuid.New()
	productionParse[id] = process
	var respStruct struct {
		ID string `json:"id"`
	}
	respStruct.ID = id
	resp = respStruct
	return
}

func ProductionParseQuery(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := productionParse[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	if !proc.Exit {
		code = 1002
		return
	}
	resp = proc.Result
	delete(productionParse, reqStruct.ID)
	return
}

func ProductionParseCancel(req json.RawMessage) (code int, resp interface{}, err error) {
	var reqStruct struct {
		ID string `json:"id"`
	}
	err = json.Unmarshal(req, &reqStruct)
	if err != nil {
		return
	}
	proc, exist := productionParse[reqStruct.ID]
	if !exist {
		code = 1001
		return
	}
	proc.Interrupt = true
	delete(productionParse, reqStruct.ID)
	return
}

func ProcessProductionParse(code string, process *ProductionProcess) {
	defer func() { process.Exit = true }()
	productions, errors := production.ParseProduction(code, &process.Interrupt)
	if process.Interrupt {
		return
	}
	process.Result.Productions = productions
	process.Result.Errors = errors

	nonterminals := set.NewStringSet()
	terminals := set.NewStringSet()
	for _, production := range productions {
		if len(production) == 0 {
			continue
		}
		if production[0] != "" {
			nonterminals.Put(production[0])
		}
	}
	for _, production := range productions {
		for i := 1; i < len(production); i++ {
			if production[i] == "" {
				continue
			}
			if nonterminals.Contains(production[i]) {
				continue
			}
			terminals.Put(production[i])
		}
	}
	if process.Interrupt {
		return
	}
	process.Result.Nonterminals = make([]string, 0, len(nonterminals))
	process.Result.Terminals = make([]string, 0, len(terminals))
	for s := range nonterminals {
		process.Result.Nonterminals = append(process.Result.Nonterminals, s)
	}
	for s := range terminals {
		process.Result.Terminals = append(process.Result.Terminals, s)
	}
}
