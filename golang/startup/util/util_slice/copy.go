package utilslice

func CopyString(slice []string) []string {
	newSlice := make([]string, len(slice))
	copy(newSlice, slice)
	return newSlice
}

func CopyStringSlice(slice [][]string) [][]string {
	newSlice := make([][]string, len(slice))
	for i, v := range slice {
		newSlice[i] = CopyString(v)
	}
	return newSlice
}
