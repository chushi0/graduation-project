package set_test

import (
	"testing"

	"github.com/chushi0/graduation_project/golang/startup/util/set"
)

func TestSet(t *testing.T) {
	s := set.NewIntSet()
	if s.Contains(8) {
		t.Fail()
	}
	s.Put(8)
	if !s.Contains(8) {
		t.Fail()
	}
	if s.Equals(set.NewIntSet()) {
		t.Fail()
	}
	if s.Equals(set.NewIntSet(5)) {
		t.Fail()
	}
	if !s.Equals(set.NewIntSet(8)) {
		t.Fail()
	}
}
