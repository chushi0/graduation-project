package process

import (
	"fmt"

	"github.com/chushi0/graduation_project/golang/startup/util/set"
)

type SerialToken struct {
	SerialString string
	Index        int
}

type SerialTokens struct {
	Map           map[string]*SerialToken
	SerialStrings set.StringSet
	NextIndex     int
}

func NewSerialTokens() *SerialTokens {
	return &SerialTokens{
		Map:           make(map[string]*SerialToken),
		SerialStrings: set.NewStringSet(),
		NextIndex:     0,
	}
}

func (s *SerialTokens) Put(token string) {
	serial := SerialToken{
		SerialString: s.makeSerialString(token),
		Index:        s.NextIndex,
	}

	s.NextIndex++
	s.Map[token] = &serial
	s.SerialStrings.Put(serial.SerialString)
}

func (s *SerialTokens) makeSerialString(token string) string {
	res := ""
	for _, rn := range token {
		if (rn >= 'a' && rn <= 'z') || (rn >= 'A' && rn <= 'Z') || (rn >= '0' && rn <= '9') {
			res += string(rn)
			continue
		}
		switch rn {
		case '+':
			res += "Plus"
		case '-':
			res += "Minus"
		case '*':
			res += "Asterisk"
		case '/':
			res += "Slash"
		case '=':
			res += "Equal"
		case '\\':
			res += "Backslash"
		case '(':
			res += "LeftParentheses"
		case ')':
			res += "RightParentheses"
		case '[':
			res += "LeftBrackets"
		case ']':
			res += "RightBrackets"
		case '{':
			res += "LeftBraces"
		case '}':
			res += "RightBraces"
		case ';':
			res += "Semicolon"
		case ':':
			res += "Colon"
		default:
			res += fmt.Sprintf("u%d", rn)
		}
	}
	if !s.SerialStrings.Contains(res) {
		return res
	}
	i := 0
	for {
		cur := fmt.Sprintf("%s_%d", res, i)
		if !s.SerialStrings.Contains(cur) {
			return cur
		}
		i++
	}
}
