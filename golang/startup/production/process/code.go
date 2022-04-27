package process

import (
	"flag"
	"strings"
)

var golang = flag.Bool("golang", false, "")
var comment = flag.Bool("comment", false, "")

type CodeSaver struct {
	Enable   bool
	SavePath string
	FileName string
}

func (s *CodeSaver) Normalize() {
	s.SavePath = strings.ReplaceAll(s.SavePath, "\\", "/")
	s.SavePath = strings.TrimSuffix(s.SavePath, ".h")
	s.SavePath = strings.TrimSuffix(s.SavePath, ".cpp")
	index := strings.LastIndex(s.SavePath, "/")
	s.FileName = s.SavePath[index+1:]
}

func (s *CodeSaver) GetHeaderPath() string {
	return s.SavePath + ".h"
}

func (s *CodeSaver) GetSourcePath() string {
	return s.SavePath + ".cpp"
}

func (s *CodeSaver) GetGolangPath() string {
	return s.SavePath + ".go"
}

func (s *CodeSaver) GetIncludeName() string {
	return s.FileName + ".h"
}
