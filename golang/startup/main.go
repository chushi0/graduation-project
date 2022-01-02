package main

import (
	"bufio"
	"io"
	"log"
	"os"
	"os/exec"

	"github.com/chushi0/graduation_project/golang/startup/service"
)

var (
	rpcinChannel  chan []byte
	rpcoutChannel chan []byte
)

func init() {
	rpcinChannel = make(chan []byte, 1)
	rpcoutChannel = make(chan []byte, 1)
}

func main() {
	defer func() {
		log.Println("Process Exit")
	}()

	logFile, err := os.Create("golang-log.log")
	if err != nil {
		log.Fatal(err)
	}
	defer logFile.Close()
	log.SetOutput(logFile)

	service.Init()

	proc := exec.Command("main")
	stdin, _ := proc.StdinPipe()
	stdout, _ := proc.StdoutPipe()
	stderr, _ := proc.StderrPipe()
	go rpcinReader(stdout)
	go rpcoutWriter(stdin)
	go serviceProc()
	go logProc(stderr)
	err = proc.Run()
	if err != nil {
		log.Fatalf("proc process exit: %v", err)
	}
}

func rpcinReader(pipe io.ReadCloser) {
	buf := bufio.NewReader(pipe)
	for {
		line, _, err := buf.ReadLine()
		if err != nil {
			log.Fatalf("rpcinReader goroutine broken: %v", err)
			return
		}
		rpcinChannel <- line
	}
}

func rpcoutWriter(pipe io.WriteCloser) {
	buf := bufio.NewWriter(pipe)
	for {
		resp := <-rpcoutChannel
		resp = append(resp, '\n')
		_, err := buf.Write(resp)
		if err != nil {
			log.Fatalf("rpcoutWriter goroutine broken: %v", err)
		}
		buf.Flush()
	}
}

func serviceProc() {
	for {
		req := <-rpcinChannel
		log.Printf("rpc request: %v", string(req))
		resp, err := service.CallService(req)
		if err != nil {
			log.Printf("service process fail: %s (req: %s)", err, req)
			resp = []byte(`{"code":500,"data":{}}`)
		}
		log.Printf("rpc response: %v", string(resp))
		rpcoutChannel <- resp
	}
}

func logProc(pipe io.ReadCloser) {
	buf := bufio.NewReader(pipe)
	for {
		line, _, err := buf.ReadLine()
		if err != nil {
			log.Printf("logProc goroutine broken: %v", err)
			return
		}
		log.Printf("logProc: %s", line)
	}
}
