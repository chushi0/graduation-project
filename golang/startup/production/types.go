package production

import (
	"errors"
	"fmt"

	"github.com/chushi0/graduation_project/golang/startup/util/set"
)

type Production = []string

// IO
type IO struct {
	Filename        string // 文件名
	RuneBuffer      []rune // 字符
	RuneBufferIndex int    // 当前读取的字符缓存位置
	IgnoreLF        bool   // 忽略一次`\n`

	Line   int // 当前行号
	Column int // 当前列

	SaveLine     int  // 保存的行号
	SaveColumn   int  // 保存的列
	SaveIndex    int  // 保存的字符缓存读取位置
	SaveIgnoreLF bool // 保存是否忽略`\n`
}

// 有穷自动状态机
// 包含NFA和DFA
type FiniteAutomaton struct {
	StateCount     int            // 状态数
	JumpTables     [][]*JumpMap   // 转移函数
	AcceptStates   set.IntSet     // 接受的状态
	AcceptStateTag map[int]string // 接受的状态标签
}

type RuneRange struct {
	// 字符范围开始与字符范围结束均为 0 时，表示无字符跳转
	// 仅在 NFA 中有效
	RuneStart rune // 字符范围开始（包含）
	RuneEnd   rune // 字符范围结束（不包含）
}

type JumpMap struct {
	RuneRange
	Target int // 跳转到的状态
}

// 词法分析器
type Lexer struct {
	ErrorContainer *ErrorContainer  // 错误
	Io             *IO              // 文件 IO 接口
	DFA            *FiniteAutomaton // DFA
}

// 词法分析器读到的内容
type Token struct {
	RawValue string // 原始值
	State    int    // 接受的状态
	Tag      string // tag
	Line     int    // 行号
	Column   int    // 列号
	File     string // 文件名
}

type ErrorContainer struct {
	Fatal    []*Error `json:"fatal"`
	Errors   []*Error `json:"error"`
	Warnings []*Error `json:"warning"`
}

type ErrorType uint

type Error struct {
	Type   ErrorType `json:"type"`   // 错误类型
	File   string    `json:"file"`   // 所属文件
	Line   int       `json:"line"`   // 行号
	Column int       `json:"column"` // 列号
	Detail string    `json:"detail"` // 详细信息
	Length int       `json:"length"` // 长度
}

func NewErrorContainer() *ErrorContainer {
	return &ErrorContainer{
		Errors:   make([]*Error, 0),
		Warnings: make([]*Error, 0),
	}
}

var (
	ErrorFinateAutomatonInput = errors.New("finate automaton not accpet this input")
	ErrorRegexpParse          = errors.New("parse fail")
)

type IOError struct {
	Original error
}

func (e *IOError) Error() string {
	return fmt.Sprintf("io exception: %v", e.Original.Error())
}

func (e *IOError) Unwrap() error {
	return e.Original
}

const (
	// 文件读取错误
	ErrorType_SystemFileError ErrorType = 10001

	// 词法分析：无法解析出正确的单词
	ErrorType_UnexpectedToken ErrorType = 20001
	// 语法分析：缺少非终结符
	ErrorType_MissingNonterminal ErrorType = 20002
	// 语法分析：无效语句
	ErrorType_InvalidSyntax ErrorType = 20003
	// 语法分析：缺少生成标记
	ErrorType_MissingProduct ErrorType = 20004
	// 语法分析：过多的生成标记
	ErrorType_TooManyProduct ErrorType = 20005
	// 语法分析：过多的开始符号定义
	ErrorType_TooManyStartSymbolDeclear ErrorType = 20006

	// 语义分析：缺少开始符号
	ErrorType_NoStartSymbol ErrorType = 30001
	// 语义分析：不推荐的非终结符
	ErrorType_NotSuggestNonterminal ErrorType = 30002
	// 语义分析：未声明开始符号
	ErrorType_StartSymbolNotDeclear ErrorType = 30003
	// 语义分析：重复产生式
	ErrorType_SameProduction ErrorType = 30004
)
