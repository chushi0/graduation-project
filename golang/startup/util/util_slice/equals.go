package utilslice

func StringEquals(s1, s2 []string) bool {
	if len(s1) != len(s2) {
		return false
	}
	for i, c := range s1 {
		if c != s2[i] {
			return false
		}
	}
	return true
}
