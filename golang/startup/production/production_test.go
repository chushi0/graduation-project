package production_test

import (
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/production"
)

func TestProduction(t *testing.T) {
	_, _, e := production.ParseProduction(`
	E := E + E
	| E - E
	| (E)
	`, nil)
	if len(e.Errors) > 0 {
		t.Fail()
	}
}
