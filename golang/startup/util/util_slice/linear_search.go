package utilslice

func LinearSearch(length int, equal func(i int) bool) int {
	for i := 0; i < length; i++ {
		if equal(i) {
			return i
		}
	}
	return -1
}

func StringLinearSearch(slice []string, target string) int {
	return LinearSearch(len(slice), func(i int) bool { return target == slice[i] })
}
