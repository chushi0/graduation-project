package utilslice

func ReplaceStringArray(prods [][]string, rm []string, add ...[]string) [][]string {
	res := make([][]string, 0)
	for _, prod := range prods {
		if StringEquals(prod, rm) {
			res = append(res, add...)
		} else {
			res = append(res, prod)
		}
	}
	return res
}
