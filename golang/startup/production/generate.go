package production

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
	asciiSymbol := NewFinateAutomatonFromRegexpOrPanic([]rune("+|-|\\*|/|=|\\(|\\)|;|!|@|#|$|%|^|&|\\[|]|{|}|:|'|<|>|,|\\.|?|\\||~|`"))
	escape := NewFinateAutomatonFromRegexpOrPanic([]rune("\"([\\u0000-\\u0009]|[\\u000B-\\u0021]|[\\u0023-\\u005B]|[\\u005D-\\uFFFF]|\\\\\\.)*\"")) // \u0022 = "

	identify.SetAcceptTag(tagIdentify)
	product.SetAcceptTag(tagProduct)
	asciiSymbol.SetAcceptTag(tagSymbol)
	escape.SetAcceptTag(tagEscape)

	fa = identify.MergeOr(product).MergeOr(asciiSymbol).MergeOr(escape).AsDFA()
}

func ParseProduction(code string, interruptFlag *bool) ([]Production, *ErrorContainer) {
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
	for {
		if interruptFlag != nil && *interruptFlag {
			return nil, nil
		}
		token := lexer.NextToken()
		if token == nil {
			break
		}
		if token.Line != lastLineNo {
			if lastProduction != nil {
				result = append(result, lastProduction)
				lastProduction = nil
			}
			if token.Tag == tagIdentify {
				lastProduction = make(Production, 1)
				lastProduction[0] = token.RawValue
				productionProductSymbol = false
				startToken = token
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
	return result, lexer.ErrorContainer
}
