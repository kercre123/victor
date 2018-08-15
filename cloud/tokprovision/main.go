package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"strings"

	"golang.org/x/crypto/ssh/terminal"
)

func main() {
	token, err := getToken()
	if err != nil {
		log.Fatalln("Error getting token:", err)
	}
	fmt.Println("Successfully logged in!")
	if err := provision(token); err != nil {
		log.Fatalln("Error provisioning robot:", err)
	}
}

func getBoolQuestion(q string) bool {
	var input string
	scanner := bufio.NewScanner(os.Stdin)
	for strings.ToLower(input) != "y" && strings.ToLower(input) != "n" {
		fmt.Print(q + " [y/n]: ")
		scanner.Scan()
		input = scanner.Text()
	}
	return strings.ToLower(input) == "y"
}

func getStringPrompt(p string) string {
	scanner := bufio.NewScanner(os.Stdin)
	fmt.Print(p + ": ")
	scanner.Scan()
	return scanner.Text()
}

func getPasswordPrompt(p string) string {
	fmt.Print(p + ": ")
	s, _ := terminal.ReadPassword(int(os.Stdin.Fd()))
	fmt.Print("\n")
	return string(s)
}
