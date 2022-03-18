package process_test

import (
	"bufio"
	"encoding/csv"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
	"testing"
	"time"

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

func TestLR1ItemClosureFromFile(t *testing.T) {
	code, _ := ioutil.ReadFile("input.txt")
	ctx := process.NewLR1Context()
	ctx.Code = string(code)
	ctx.LALR = false
	entry := ctx.CreateLR1ProcessEntry()
	dbg := debug.StartDebugGoroutine(entry)
	dbg.SwitchRunMode(debug.RunMode_Run)
	for dbg.GetCurrentRunMode() != debug.RunMode_Exit {
		time.Sleep(time.Millisecond * 100)
	}
	umlfile, _ := os.Create("closures.uml")
	uml := bufio.NewWriter(umlfile)
	defer func() {
		uml.Flush()
		umlfile.Close()
	}()
	uml.WriteString("@startuml\n\n[*] --> Item0\n")
	for i, closure := range ctx.KeyVariables.ClosureMap.Closures {
		start := fmt.Sprintf("Item%d", i)
		for _, item := range *closure {
			prod := ctx.KeyVariables.Productions[item.Prod]
			prodstr := prod[0] + " :="
			for j := 1; j < len(prod); j++ {
				prodstr += " "
				if j-1 == item.Progress {
					prodstr += "·"
				}
				prodstr += prod[j]
			}
			prodstr += " , " + item.Lookahead
			uml.WriteString(fmt.Sprintf("%s : %s\n", start, prodstr))
		}
	}
	for _, edge := range ctx.KeyVariables.ClosureMap.Edges {
		uml.WriteString(fmt.Sprintf("Item%d -> Item%d : %s\n", edge.From, edge.To, edge.Symbol))
	}
	uml.WriteString("\n\n@enduml")
	json, _ := json.Marshal(ctx.KeyVariables)
	ioutil.WriteFile("all.json", json, 0660)
	tablefile, _ := os.Create("table.csv")
	table := bufio.NewWriter(tablefile)
	tableCsv := csv.NewWriter(table)
	defer func() {
		tableCsv.Flush()
		table.Flush()
		tablefile.Close()
	}()
	csvLine := make([]string, 0)
	csvLine = append(csvLine, "状态")
	csvLine = append(csvLine, "Action")
	for range ctx.Grammer.Terminals {
		csvLine = append(csvLine, "Action")
	}
	for range ctx.Grammer.Nonterminals {
		csvLine = append(csvLine, "Goto")
	}
	tableCsv.Write(csvLine)
	csvLine = make([]string, 0)
	csvLine = append(csvLine, "状态")
	terminals := ctx.KeyVariables.Terminals
	terminals = append(terminals, "$")
	csvLine = append(csvLine, terminals...)
	csvLine = append(csvLine, ctx.KeyVariables.NonterminalOrders...)
	tableCsv.Write(csvLine)
	for i := range ctx.KeyVariables.ClosureMap.Closures {
		csvLine = make([]string, 0)
		csvLine = append(csvLine, strconv.Itoa(i))
		for _, terminal := range terminals {
			csvLine = append(csvLine, ctx.KeyVariables.ActionTable[i][terminal])
		}
		for _, nonterminal := range ctx.KeyVariables.NonterminalOrders {
			g := ctx.KeyVariables.GotoTable[i][nonterminal]
			if g != -1 {
				csvLine = append(csvLine, strconv.Itoa(g))
			} else {
				csvLine = append(csvLine, "")
			}
		}
		tableCsv.Write(csvLine)
	}
}

func TestGetItemLink(t *testing.T) {
	data, _ := ioutil.ReadFile("all.json")
	var variables process.LR1Variables
	json.Unmarshal(data, &variables)

	target := 64
	link := ""
	for target != 0 {
		for _, edge := range variables.ClosureMap.Edges {
			if edge.To == target && edge.From < target {
				target = edge.From
				link = fmt.Sprintf("%s(%d) %s", edge.Symbol, edge.From, link)
			}
		}
	}
	panic(link)
}
