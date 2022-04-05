package production

import (
	"errors"
	"fmt"
	"io"
	"sort"
	"strconv"
	"unicode/utf8"

	"github.com/chushi0/graduation_project/golang/startup/util/set"
	utilslice "github.com/chushi0/graduation_project/golang/startup/util/util_slice"
)

func NewIOFromString(code string) *IO {
	return &IO{
		Filename:        "<input>",
		RuneBuffer:      []rune(code),
		RuneBufferIndex: 0,
		IgnoreLF:        false,
		Line:            1,
		Column:          0,
	}
}

func (pio *IO) ReadChar() (rune, error) {
	if pio.RuneBufferIndex >= len(pio.RuneBuffer) {
		return 0, io.EOF
	}
	rn := pio.RuneBuffer[pio.RuneBufferIndex]
	pio.RuneBufferIndex++
	if rn == '\r' {
		pio.IgnoreLF = true
		pio.Line += 1
		pio.Column = 0
		return '\n', nil
	}
	if rn == '\n' {
		if pio.IgnoreLF {
			pio.IgnoreLF = false
			return pio.ReadChar()
		}
		pio.Line += 1
		pio.Column = 0
	} else {
		pio.IgnoreLF = false
		pio.Column += 1
	}
	return rn, nil
}

func (pio *IO) Save() {
	pio.SaveLine = pio.Line
	pio.SaveColumn = pio.Column
	pio.SaveIndex = pio.RuneBufferIndex
	pio.SaveIgnoreLF = pio.IgnoreLF
}

func (pio *IO) Restore() {
	pio.RuneBufferIndex = pio.SaveIndex
	pio.Line = pio.SaveLine
	pio.Column = pio.SaveColumn
	pio.IgnoreLF = pio.SaveIgnoreLF
}

// 检查是否是ε转换
func (r *RuneRange) isEpsilon() bool {
	return r.RuneStart == 0 && r.RuneEnd == 0
}

// 检查是否包含指定集合
func (r *RuneRange) contains(o *RuneRange) bool {
	return r.RuneStart <= o.RuneStart && r.RuneEnd >= o.RuneEnd
}

// 检查是否相交
func (r *RuneRange) isIntersect(o *RuneRange) bool {
	// 若两者相等，只能是连续
	return r.RuneEnd > o.RuneStart && o.RuneEnd > r.RuneStart
}

// 分割相交的区域
func (r *RuneRange) splitWith(o *RuneRange) []*RuneRange {
	var r1, r2, r3 RuneRange
	if r.RuneStart < o.RuneStart {
		r1.RuneStart = r.RuneStart
		r1.RuneEnd = o.RuneStart
		r2.RuneStart = o.RuneStart
	} else if r.RuneStart > o.RuneStart {
		r1.RuneStart = o.RuneStart
		r1.RuneEnd = r.RuneStart
		r2.RuneStart = r.RuneStart
	} else {
		r2.RuneStart = r.RuneStart
	}
	if r.RuneEnd < o.RuneEnd {
		r3.RuneStart = r.RuneEnd
		r3.RuneEnd = o.RuneEnd
		r2.RuneEnd = r.RuneEnd
	} else if r.RuneEnd > o.RuneEnd {
		r3.RuneStart = o.RuneEnd
		r3.RuneEnd = r.RuneEnd
		r2.RuneEnd = o.RuneEnd
	} else {
		r2.RuneEnd = r.RuneEnd
	}

	result := make([]*RuneRange, 0)
	if !r1.isEpsilon() {
		result = append(result, &r1)
	}
	result = append(result, &r2)
	if !r3.isEpsilon() {
		result = append(result, &r3)
	}
	return result
}

// 两个非确定有穷自动机作或运算 a|b
// 合并初始状态
func (fa *FiniteAutomaton) MergeOr(o *FiniteAutomaton) *FiniteAutomaton {
	stateCount := fa.StateCount + o.StateCount - 1
	result := &FiniteAutomaton{
		StateCount:     stateCount,
		JumpTables:     make([][]*JumpMap, stateCount),
		AcceptStates:   set.NewIntSet(),
		AcceptStateTag: make(map[int]string),
	}

	// 填充 JumpTables
	for i := 0; i < fa.StateCount; i++ {
		jumpTable := make([]*JumpMap, 0)
		for _, jumpMap := range fa.JumpTables[i] {
			jumpTable = append(jumpTable, &JumpMap{
				RuneRange: jumpMap.RuneRange,
				Target:    jumpMap.Target,
			})
		}
		result.JumpTables[i] = jumpTable
	}
	for _, jumpMap := range o.JumpTables[0] {
		result.JumpTables[0] = append(result.JumpTables[0], &JumpMap{
			RuneRange: jumpMap.RuneRange,
			Target:    jumpMap.Target + fa.StateCount - 1,
		})
	}
	for i := 1; i < o.StateCount; i++ {
		jumpTable := make([]*JumpMap, 0)
		for _, jumpMap := range o.JumpTables[i] {
			jumpTable = append(jumpTable, &JumpMap{
				RuneRange: jumpMap.RuneRange,
				Target:    jumpMap.Target + fa.StateCount - 1,
			})
		}
		result.JumpTables[i+fa.StateCount-1] = jumpTable
	}

	// 填充 AcceptStates
	result.AcceptStates = fa.AcceptStates.Clone()
	for state := range o.AcceptStates {
		if state == 0 {
			result.AcceptStates.Put(0)
		}
		result.AcceptStates.Put(state + fa.StateCount - 1)
	}

	// 填充 Tags
	for state, tag := range fa.AcceptStateTag {
		result.AcceptStateTag[state] = tag
	}
	for state, tag := range o.AcceptStateTag {
		if state == 0 {
			result.AcceptStateTag[0] = tag
		} else {
			result.AcceptStateTag[state+fa.StateCount-1] = tag
		}
	}

	return result
}

// 两个非确定有穷自动机作连接运算 a+b
// 将前者的接受状态连接后者的初始状态
func (fa *FiniteAutomaton) MergeConnect(o *FiniteAutomaton) *FiniteAutomaton {
	stateCount := fa.StateCount + o.StateCount
	result := &FiniteAutomaton{
		StateCount:     stateCount,
		JumpTables:     make([][]*JumpMap, stateCount),
		AcceptStates:   set.NewIntSet(),
		AcceptStateTag: make(map[int]string),
	}

	// 填充 JumpTables
	for i := 0; i < fa.StateCount; i++ {
		jumpTable := make([]*JumpMap, 0)
		for _, jumpMap := range fa.JumpTables[i] {
			jumpTable = append(jumpTable, &JumpMap{
				RuneRange: jumpMap.RuneRange,
				Target:    jumpMap.Target,
			})
		}
		result.JumpTables[i] = jumpTable
	}
	for i := 0; i < o.StateCount; i++ {
		jumpTable := make([]*JumpMap, 0)
		for _, jumpMap := range o.JumpTables[i] {
			jumpTable = append(jumpTable, &JumpMap{
				RuneRange: jumpMap.RuneRange,
				Target:    jumpMap.Target + fa.StateCount,
			})
		}
		result.JumpTables[i+fa.StateCount] = jumpTable
	}
	for state := range fa.AcceptStates {
		result.JumpTables[state] = append(result.JumpTables[state], &JumpMap{
			RuneRange: RuneRange{
				RuneStart: 0,
				RuneEnd:   0,
			},
			Target: fa.StateCount,
		})
	}

	// 填充 AcceptStates
	for state := range o.AcceptStates {
		result.AcceptStates.Put(state + fa.StateCount)
	}

	// 填充 AcceptStateTag
	for state := range o.AcceptStates {
		result.AcceptStateTag[state+fa.StateCount] = o.AcceptStateTag[state]
	}
	return result
}

// 非确定有穷自动机作克林闭包 a*
// 将接受状态连接初始状态，然后将初始状态设置为接受状态（清除其他接受状态）
func (fa *FiniteAutomaton) MergeKleene() *FiniteAutomaton {
	result := &FiniteAutomaton{
		StateCount:     fa.StateCount,
		JumpTables:     make([][]*JumpMap, fa.StateCount),
		AcceptStates:   set.NewIntSet(0),
		AcceptStateTag: make(map[int]string),
	}
	for i := 0; i < fa.StateCount; i++ {
		jumpTable := make([]*JumpMap, 0)
		for _, jumpMap := range fa.JumpTables[i] {
			jumpTable = append(jumpTable, &JumpMap{
				RuneRange: jumpMap.RuneRange,
				Target:    jumpMap.Target,
			})
		}
		result.JumpTables[i] = jumpTable
	}
	for state := range fa.AcceptStates {
		result.JumpTables[state] = append(result.JumpTables[state], &JumpMap{
			RuneRange: RuneRange{
				RuneStart: 0,
				RuneEnd:   0,
			},
			Target: 0,
		})
	}
	for state := range fa.AcceptStates {
		result.AcceptStateTag[0] = fa.AcceptStateTag[state]
		break
	}
	return result
}

// 为接受状态设置标记
func (fa *FiniteAutomaton) SetAcceptTag(tag string) *FiniteAutomaton {
	for state := range fa.AcceptStates {
		fa.AcceptStateTag[state] = tag
	}
	return fa
}

// 单字符匹配的有穷自动机
func NewFinateAutomaton(runeRange *RuneRange) *FiniteAutomaton {
	return &FiniteAutomaton{
		StateCount: 2,
		JumpTables: [][]*JumpMap{
			{
				&JumpMap{
					RuneRange: *runeRange,
					Target:    1,
				},
			},
			{},
		},
		AcceptStates: set.NewIntSet(1),
		AcceptStateTag: map[int]string{
			1: "",
		},
	}
}

func buildFinateAutomatonFromBracket(content []rune) (fa *FiniteAutomaton, err error) {
	if len(content) == 0 {
		return NewFinateAutomaton(&RuneRange{
			RuneStart: 0,
			RuneEnd:   0,
		}), nil
	}

	errBadBracket := fmt.Errorf("bad bracket regexp: %s", string(content))
	offset := 0

	getRune := func() (rune, error) {
		if offset >= len(content) {
			return 0, errBadBracket
		}
		if content[offset] == '\\' {
			offset++
			if offset >= len(content) {
				return 0, errBadBracket
			}
			if content[offset] == 'u' {
				if offset+4 >= len(content) {
					return 0, errBadBracket
				}
				offset += 1
				n, err := strconv.ParseInt(string(content[offset:offset+4]), 16, 64)
				if err != nil {
					return 0, fmt.Errorf("parse int error: %v", string(content[offset:offset+4]))
				}
				offset += 4
				return rune(n), nil
			}
		}
		offset++
		return content[offset-1], nil
	}

	runeRange := &RuneRange{}

	runeRange.RuneStart, err = getRune()
	if err != nil {
		return
	}
	if len(content) <= offset || content[offset] != '-' {
		return nil, errBadBracket
	}
	offset++
	runeRange.RuneEnd, err = getRune()
	if err != nil {
		return
	}
	runeRange.RuneEnd++
	if len(content) != offset {
		return nil, errBadBracket
	}

	return NewFinateAutomaton(runeRange), nil
}

// 从正则表达式构造 NFA
// 语法：
// 匹配单字符：直接写
// 匹配范围：[1-9]、[a-z]，不可写为[12-9]、[a-zA-Z]等
// 或 1|2
// 连接：不写默认为连接，优先于或
// 克林闭包：*，写在后面，优先于连接
// 括号：改变优先级
// \：转义，例如 \[、\*、\|、\\ 等
// \u1234 表示用后面的 unicode
// . 匹配任意字符
func NewFinateAutomatonFromRegexp(regexp []rune) (*FiniteAutomaton, error) {
	list := make([]interface{}, 0)
	operators := make([]rune, 0)
	needOperator := false
	for i := 0; i < len(regexp); i++ {
		rn := regexp[i]
		switch rn {
		case '[':
			i++
			start := i
			for i < len(regexp) && regexp[i] != ']' {
				i++
			}
			if i >= len(regexp) {
				return nil, fmt.Errorf("%w: bracket mismatch (start at %d)", ErrorRegexpParse, start)
			}
			content := regexp[start:i]
			fa, err := buildFinateAutomatonFromBracket(content)
			if err != nil {
				return nil, fmt.Errorf("%w: %s (at %d)", ErrorRegexpParse, err.Error(), i)
			}
			if needOperator {
				operators = append(operators, '+')
			}
			list = append(list, fa)
			needOperator = true
		case '*':
			list = append(list, '*')
		case '|':
			if !needOperator {
				return nil, fmt.Errorf("%w: current not need operator (at %d)", ErrorRegexpParse, i)
			}
			for len(operators) > 0 && operators[len(operators)-1] == '+' {
				operators = operators[:len(operators)-1]
				list = append(list, '+')
			}
			operators = append(operators, '|')
			needOperator = false
		case '(':
			if needOperator {
				operators = append(operators, '+')
			}
			operators = append(operators, '(')
			needOperator = false
		case ')':
			for {
				if len(operators) == 0 {
					return nil, fmt.Errorf("%w: bracket mismatch (at %d, left bracket not found)", ErrorRegexpParse, i)
				}
				if operators[len(operators)-1] == '(' {
					operators = operators[:len(operators)-1]
					break
				}
				list = append(list, operators[len(operators)-1])
				operators = operators[:len(operators)-1]
			}
			needOperator = true
		case '\\':
			if needOperator {
				operators = append(operators, '+')
			}
			needOperator = true
			i++
			if i >= len(regexp) {
				return nil, fmt.Errorf("%w: eof", ErrorRegexpParse)
			}
			rn := regexp[i]
			if rn != 'u' {
				list = append(list, NewFinateAutomaton(&RuneRange{
					RuneStart: rn,
					RuneEnd:   rn + 1,
				}))
			} else {
				i++
				if i+3 >= len(regexp) {
					return nil, fmt.Errorf("%w: eof", ErrorRegexpParse)
				}
				rn, err := strconv.ParseInt(string(regexp[i:i+4]), 16, 64)
				if err != nil {
					return nil, fmt.Errorf("%w: parse int error: %v (at %d)", ErrorRegexpParse, string(regexp[i:i+4]), i)
				}
				list = append(list, NewFinateAutomaton(&RuneRange{
					RuneStart: rune(rn),
					RuneEnd:   rune(rn + 1),
				}))
				i += 4
			}
		case '.':
			if needOperator {
				operators = append(operators, '+')
			}
			list = append(list, NewFinateAutomaton(&RuneRange{
				RuneStart: 0,
				RuneEnd:   utf8.MaxRune,
			}))
		default:
			if needOperator {
				operators = append(operators, '+')
			}
			needOperator = true
			list = append(list, NewFinateAutomaton(&RuneRange{
				RuneStart: rn,
				RuneEnd:   rn + 1,
			}))
		}
	}
	for len(operators) > 0 {
		op := operators[len(operators)-1]
		if op == '(' {
			return nil, fmt.Errorf("%w: bracket mismatch (right bracket not found)", ErrorRegexpParse)
		}
		operators = operators[:len(operators)-1]
		list = append(list, op)
	}

	result := make([]*FiniteAutomaton, 0)
	for _, item := range list {
		if fa, ok := item.(*FiniteAutomaton); ok {
			result = append(result, fa)
			continue
		}
		if op, ok := item.(rune); ok {
			switch op {
			case '|':
				if len(result) < 2 {
					return nil, fmt.Errorf("%w: error while mergeOr", ErrorRegexpParse)
				}
				result = append(result[:len(result)-2], result[len(result)-2].MergeOr(result[len(result)-1]))
			case '+':
				if len(result) < 2 {
					return nil, fmt.Errorf("%w: error while mergeConnect", ErrorRegexpParse)
				}
				result = append(result[:len(result)-2], result[len(result)-2].MergeConnect(result[len(result)-1]))
			case '*':
				if len(result) < 1 {
					return nil, fmt.Errorf("%w: error while mergeKleene", ErrorRegexpParse)
				}
				result[len(result)-1] = result[len(result)-1].MergeKleene()
			default:
				panic(fmt.Sprintf("unknown op: %v", op))
			}
			continue
		}
		panic(fmt.Sprintf("unknown type: %+v", item))
	}
	if len(result) == 1 {
		return result[0], nil
	}
	return nil, fmt.Errorf("%w unknown error", ErrorRegexpParse)
}

func NewFinateAutomatonFromRegexpOrPanic(regexp []rune) *FiniteAutomaton {
	fa, err := NewFinateAutomatonFromRegexp(regexp)
	if err != nil {
		panic(err)
	}
	return fa
}

// 能够从 NFA 的指定状态只通过ε转换到达的 NFA 状态的集合
func (fa *FiniteAutomaton) closure(state int) set.IntSet {
	res := set.NewIntSet()
	for state >= 0 {
		res.Put(state)
		table := fa.JumpTables[state]
		state = -1
		for _, jumpMap := range table {
			if jumpMap.isEpsilon() {
				state = jumpMap.Target
				break
			}
		}
	}
	return res
}

func (fa *FiniteAutomaton) closureSet(states set.IntSet) set.IntSet {
	result := set.NewIntSet()
	for state := range states {
		result.Merge(fa.closure(state))
	}
	return result
}

// 分割所有字符范围
func (fa *FiniteAutomaton) splitRange() []*RuneRange {
	ranges := make([]*RuneRange, 0)
	points := set.NewIntSet()
	pointSlice := make([]int, 0)
	for _, jumpTable := range fa.JumpTables {
		for _, jumpMap := range jumpTable {
			if jumpMap.isEpsilon() {
				continue
			}
			points.Put(int(jumpMap.RuneStart), int(jumpMap.RuneEnd))
		}
	}
	for p := range points {
		pointSlice = append(pointSlice, p)
	}
	sort.Ints(pointSlice)
	for i := 0; i < len(pointSlice)-1; i++ {
		rng := RuneRange{rune(pointSlice[i]), rune(pointSlice[i+1])}
		for _, jumpTable := range fa.JumpTables {
			for _, jumpMap := range jumpTable {
				if jumpMap.isEpsilon() {
					continue
				}
				if jumpMap.isIntersect(&rng) {
					goto intersect
				}
			}
		}
		continue
	intersect:
		ranges = append(ranges, &rng)
	}
	return ranges
}

// NFA 转 DFA
func (fa *FiniteAutomaton) AsDFA() *FiniteAutomaton {
	result := &FiniteAutomaton{
		JumpTables:     make([][]*JumpMap, 0),
		AcceptStates:   set.NewIntSet(),
		AcceptStateTag: make(map[int]string),
	}
	// 拆分字符范围
	ranges := fa.splitRange()
	// 状态
	states := make([]set.IntSet, 0)
	states = append(states, fa.closure(0))
	for i := 0; i < len(states); i++ {
		curState := states[i]
		// 检查当前状态集是否可以接受
		for state := range curState {
			if fa.AcceptStates.Contains(state) {
				result.AcceptStates.Put(i)
				result.AcceptStateTag[i] = fa.AcceptStateTag[state]
				break
			}
		}
		// 计算转移函数和更多状态
		jumpTable := make([]*JumpMap, 0)
		for _, rng := range ranges {
			moveTo := set.NewIntSet()
			for state := range curState {
				for _, jumpMap := range fa.JumpTables[state] {
					if jumpMap.isEpsilon() {
						continue
					}
					if jumpMap.contains(rng) {
						moveTo.Put(jumpMap.Target)
					}
				}
			}
			if len(moveTo) == 0 {
				continue
			}
			moveTo = fa.closureSet(moveTo)
			index := utilslice.LinearSearch(len(states), func(i int) bool {
				return states[i].Equals(moveTo)
			})
			if index == -1 {
				states = append(states, moveTo)
				index = len(states) - 1
			}
			jumpTable = append(jumpTable, &JumpMap{
				RuneRange: *rng,
				Target:    index,
			})
		}
		result.JumpTables = append(result.JumpTables, jumpTable)
	}
	result.StateCount = len(states)
	return result
}

// 根据输入计算自动机的下一个状态
// 假定自动机为 DFA
func (fa *FiniteAutomaton) NextState(state int, input rune) (int, error) {
	jumpTable := fa.JumpTables[state]
	for _, jumpMap := range jumpTable {
		if jumpMap.RuneStart <= input && jumpMap.RuneEnd > input {
			return jumpMap.Target, nil
		}
	}
	return -1, fmt.Errorf("%w: %v", ErrorFinateAutomatonInput, string(input))
}

// 导出可视化的自动机
func (fa *FiniteAutomaton) Dump() string {
	result := fmt.Sprintf("StateCount: %d\nAcceptStates:\n", fa.StateCount)
	for state := 0; state < fa.StateCount; state++ {
		if fa.AcceptStates.Contains(state) {
			result += fmt.Sprintf("  %d (%s)\n", state, fa.AcceptStateTag[state])
		}
	}
	result += "JumpTable:\n"
	for state, jumpTable := range fa.JumpTables {
		result += fmt.Sprintf("  state %d:", state)
		if fa.AcceptStates.Contains(state) {
			result += fmt.Sprintf(" (%s)", fa.AcceptStateTag[state])
		}
		result += "\n"
		for _, jumpMap := range jumpTable {
			result += fmt.Sprintf("    [%s, %s] (%d, %d) -> %d\n", string(jumpMap.RuneStart), string(jumpMap.RuneEnd-1),
				jumpMap.RuneStart, jumpMap.RuneEnd-1, jumpMap.Target)
		}
	}
	return result
}

// 读取下一个 Token
func (lexer *Lexer) NextToken() *Token {
scan_start:
	// 初始化
	err := lexer.clearSpace()
	if err != nil {
		lexer.ErrorContainer.Fatal = append(lexer.ErrorContainer.Fatal, &Error{
			Type:   ErrorType_SystemFileError,
			File:   lexer.Io.Filename,
			Detail: fmt.Sprintf("unexcepted error: %s", err.Error()),
		})
		return nil
	}
	token := &Token{
		Line:   lexer.Io.Line,
		Column: lexer.Io.Column,
		File:   lexer.Io.Filename,
		State:  -1,
	}
	rawVal := ""
	state := 0

	// 循环读取字符
	// 贪心模式匹配尽可能多的字符
	for {
		rn, err := lexer.Io.ReadChar()
		if err != nil {
			if errors.Is(err, io.EOF) {
				if len(rawVal) == 0 {
					return nil
				}
				break
			}
			lexer.ErrorContainer.Fatal = append(lexer.ErrorContainer.Fatal, &Error{
				Type:   ErrorType_SystemFileError,
				File:   lexer.Io.Filename,
				Detail: fmt.Sprintf("unexcepted error: %s", err.Error()),
			})
			return nil
		}
		rawVal += string(rn)
		state, err = lexer.DFA.NextState(state, rn)
		if err != nil {
			break
		}
		if lexer.DFA.AcceptStates.Contains(state) {
			lexer.Io.Save()
			token.RawValue = rawVal
			token.State = state
			token.Tag = lexer.DFA.AcceptStateTag[state]
		}
	}

	// 曾经匹配成功，恢复io后返回
	if token.State != -1 {
		lexer.Io.Restore()
		return token
	}

	// 未匹配成功过，报错后继续扫描
	lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
		Type:   ErrorType_UnexpectedToken,
		File:   lexer.Io.Filename,
		Line:   token.Line,
		Column: token.Column,
		Detail: fmt.Sprintf("unexpected token: %s", rawVal),
	})
	goto scan_start
}

// 清除空白字符
func (lexer *Lexer) clearSpace() error {
	for {
		lexer.Io.Save()
		rn, err := lexer.Io.ReadChar()
		if err != nil {
			if errors.Is(err, io.EOF) {
				return nil
			}
			return err
		}
		if !isSpace(rn) {
			lexer.Io.Restore()
			return nil
		}
	}
}

func isSpace(rn rune) bool {
	switch rn {
	case '\t', '\r', '\n', '\v', '\f', ' ':
		return true
	default:
		return false
	}
}
