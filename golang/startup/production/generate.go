package production

import "github.com/chushi0/graduation_project/golang/startup/util/set"

// 词法分析自动机
var fa *FiniteAutomaton

const (
	tagIdentify = "identify"
	tagProduct  = "product"
	tagSymbol   = "symbol"
	tagEscape   = "escape"
)

func init() {
	identify := NewFinateAutomatonFromRegexpOrPanic([]rune("([a-z]|[A-Z]|[0-9]|\\_)([a-z]|[A-Z]|[0-9]|\\_|\\')*"))
	product := NewFinateAutomatonFromRegexpOrPanic([]rune(":="))
	asciiSymbol := NewFinateAutomatonFromRegexpOrPanic([]rune("+|-|\\*|/|=|\\(|\\)|;|!|@|#|%|^|&|\\[|\\]|{|}|:|'|<|>|,|\\.|?|\\||~|`"))
	escape := NewFinateAutomatonFromRegexpOrPanic([]rune("\"([\\u0000-\\u0009]|[\\u000B-\\u0021]|[\\u0023-\\u005B]|[\\u005D-\\uFFFF]|\\\\\\.)*\"")) // \u0022 = "

	identify.SetAcceptTag(tagIdentify)
	product.SetAcceptTag(tagProduct)
	asciiSymbol.SetAcceptTag(tagSymbol)
	escape.SetAcceptTag(tagEscape)

	fa = identify.MergeOr(product).MergeOr(asciiSymbol).MergeOr(escape).AsDFA()
}

func ParseProduction(code string, interruptFlag *bool) ([]Production, string, *ErrorContainer) {
	lexer := &Lexer{
		ErrorContainer: NewErrorContainer(),
		Io:             NewIOFromString(code),
		DFA:            fa,
	}

	result := make([]Production, 0)

	var lastProduction Production = nil
	lastLineNo := 0
	productionProductSymbol := false
	var startToken *Token = nil
	var startSymbol string
	startSymbolState := 0
	for {
		if interruptFlag != nil && *interruptFlag {
			return nil, "", nil
		}
		token := lexer.NextToken()
		if token == nil {
			break
		}
		if startSymbolState != -1 {
			switch startSymbolState {
			case 0:
				if token.RawValue != "@" {
					lexer.ErrorContainer.Warnings = append(lexer.ErrorContainer.Warnings, &Error{
						Type: ErrorType_StartSymbolNotDeclear,
					})
					startSymbolState = -1
					startSymbol = "S"
					goto nocontinue
				}
				startSymbolState = 1
				startToken = token
			case 1:
				if startToken.Line != token.Line || token.Tag != tagIdentify {
					lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
						Type:   ErrorType_InvalidSyntax,
						Line:   startToken.Line,
						Column: startToken.Column + len([]rune(startToken.RawValue)),
					})
					startSymbol = "S"
					startSymbolState = -1
					goto nocontinue
				}
				startSymbol = token.RawValue
				startSymbolState = 2
			case 2:
				if startToken.Line != token.Line {
					startSymbolState = -1
					goto nocontinue
				}
				lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
					Type:   ErrorType_TooManyStartSymbolDeclear,
					Line:   token.Line,
					Column: token.Column,
					Length: len([]rune(token.RawValue)),
				})
			}
			continue
		nocontinue:
			startToken = nil
		}
		if token.Line != lastLineNo {
			if startToken != nil && !productionProductSymbol {
				productionProductSymbol = true
				lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
					Type:   ErrorType_MissingProduct,
					Line:   startToken.Line,
					Column: startToken.Column + len([]rune(startToken.RawValue)),
				})
			}
			if lastProduction != nil {
				result = append(result, lastProduction)
				lastProduction = nil
			}
			if token.Tag == tagIdentify {
				lastProduction = make(Production, 1)
				lastProduction[0] = token.RawValue
				productionProductSymbol = false
				startToken = token
				if startChar := token.RawValue[0]; startChar >= '0' && startChar <= '9' {
					lexer.ErrorContainer.Warnings = append(lexer.ErrorContainer.Warnings, &Error{
						Type:   ErrorType_NotSuggestNonterminal,
						Line:   token.Line,
						Column: token.Column,
						Length: len([]rune(token.RawValue)),
					})
				}
			} else if token.Tag == tagProduct {
				lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
					Type:   ErrorType_MissingNonterminal,
					Line:   token.Line,
					Column: token.Column,
				})
				lastProduction = make(Production, 1)
				lastProduction[0] = ""
				productionProductSymbol = true
			} else if token.Tag == tagSymbol && token.RawValue == "|" && len(result) > 0 {
				lastProduction = make(Production, 1)
				lastProduction[0] = result[len(result)-1][0]
				productionProductSymbol = true
			} else {
				lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
					Type:   ErrorType_InvalidSyntax,
					Line:   token.Line,
					Column: token.Column,
					Length: len([]rune(token.RawValue)),
				})
			}
			lastLineNo = token.Line
			continue
		}
		lastLineNo = token.Line
		if lastProduction == nil {
			lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
				Type:   ErrorType_InvalidSyntax,
				Line:   token.Line,
				Column: token.Column,
				Length: len([]rune(token.RawValue)),
			})
			continue
		}
		if !productionProductSymbol {
			productionProductSymbol = true
			if token.Tag == tagProduct {
				continue
			}
			lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
				Type:   ErrorType_MissingProduct,
				Line:   token.Line,
				Column: token.Column,
			})
		}
		if token.Tag == tagProduct {
			lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
				Type:   ErrorType_TooManyProduct,
				Line:   token.Line,
				Column: token.Column,
				Length: len([]rune(token.RawValue)),
			})
			continue
		}
		lastProduction = append(lastProduction, token.RawValue)
	}
	if startSymbolState == 0 {
		startSymbol = "S"
		lexer.ErrorContainer.Warnings = append(lexer.ErrorContainer.Warnings, &Error{
			Type: ErrorType_StartSymbolNotDeclear,
		})
	}
	if startSymbolState == 1 {
		lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
			Type:   ErrorType_InvalidSyntax,
			Line:   startToken.Line,
			Column: startToken.Column + len([]rune(startToken.RawValue)),
		})
		startSymbol = "S"
	}
	if lastProduction != nil {
		if !productionProductSymbol {
			lexer.ErrorContainer.Errors = append(lexer.ErrorContainer.Errors, &Error{
				Type:   ErrorType_MissingProduct,
				Line:   startToken.Line,
				Column: startToken.Column + len([]rune(startToken.RawValue)),
			})
		}
		result = append(result, lastProduction)
	}
	found := false
	for _, prod := range result {
		if prod[0] == startSymbol {
			found = true
			break
		}
	}
	if !found {
		lexer.ErrorContainer.Warnings = append(lexer.ErrorContainer.Warnings, &Error{
			Type: ErrorType_NoStartSymbol,
		})
	}
	return result, startSymbol, lexer.ErrorContainer
}

func GetTerminalsAndNonterminals(productions []Production) (terminals, nonterminals set.StringSet) {
	nonterminals = set.NewStringSet()
	terminals = set.NewStringSet()
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
	return
}
