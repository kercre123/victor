package main

import (
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
)

func provision(token string) error {
	ip := getRobotIP()
	r, err := http.Get("http://" + ip + ":8890/tokenauth?token=" + token)
	if err != nil {
		return err
	}
	defer r.Body.Close()
	buf, err := ioutil.ReadAll(r.Body)
	if err != nil {
		fmt.Println("Error reading response:", err)
	}
	if r.StatusCode != http.StatusOK {
		fmt.Println("Received error response", r.StatusCode)
		fmt.Println("Robot response:", string(buf))
	} else {
		fmt.Println("Robot response:", string(buf))
	}
	return nil
}

func getRobotIP() string {
	if val, ok := os.LookupEnv("ANKI_ROBOT_HOST"); ok {
		if getBoolQuestion("Detected an ANKI_ROBOT_HOST IP of '" + val + "' - is this the IP you want?") {
			return val
		}
	}
	return getStringPrompt("Please enter the IP of the robot you want to provision")
}
