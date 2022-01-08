package utilslice

import "github.com/chushi0/graduation_project/golang/startup/production"

func CopyString(slice []string) []string {
	newSlice := make([]string, len(slice))
	for i, v := range slice {
		newSlice[i] = v
	}
	return newSlice
}

func CopyProductions(slice []production.Production) []production.Production {
	newSlice := make([]production.Production, len(slice))
	for i, v := range slice {
		newSlice[i] = v
	}
	return newSlice
}
