package production_test

import (
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/production"
)

func TestProduction(t *testing.T) {
	_, e := production.ParseProduction(`
	E := E + E
	| E - E
	| (E)
	`)
	if len(e.Errors) > 0 {
		t.Fail()
	}
}
